#include <Xemeiah/persistence/persistentdocumentallocator.h>
#include <Xemeiah/trace.h>
#include <Xemeiah/log-time.h>

#include <Xemeiah/auto-inline.hpp>
#include <Xemeiah/persistence/auto-inline.hpp>

#include <sys/mman.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define Log_PDAMap Debug

namespace Xem
{
    void
    PersistentDocumentAllocator::doInitialize (AbsolutePagePtr revisionPagePtr)
    {
        Log_PDAMap ( "Initial area alloc with revisionpage at %llx\n", revisionPagePtr );

        /*
         * Get the revisionPage.
         */
        revisionPage = getPersistentStore().getAbsolutePage<RevisionPage>(revisionPagePtr);

        AssertBug(revisionPage, "Could not get revisionPage !\n");

        /**
         * Reference documentHead and freeSegmentsHeader for Document class
         */
        documentAllocationHeader = &(revisionPage->documentAllocationHeader);

        Log_PDAMap ( "revisionPagePtr=%llx, revisionPage=%p, documentAllocationHeader=%p\n",
                (unsigned long long) revisionPagePtr, revisionPage, documentAllocationHeader );

        /**
         * Ok, now we have to allocate some areas.
         * This way, DocumentAllocator::__getSegment_Read() does not have to check that areas is NULL or not
         */
#if 1
        areasAlloced = 32;

        Log_PDAMap ( "Initial alloc : alloced %llu areas\n", areasAlloced );
        areas = (void**) realloc(areas, sizeof(void*) * (areasAlloced));
        for (__ui64 areaIdx = 0; areaIdx < areasAlloced; areaIdx++)
            areas[areaIdx] = NULL;
#endif
    }

    void
    PersistentDocumentAllocator::allocateAreaMap (__ui64 areaIdx)
    {
        mapMutex.assertLocked();
        if (areasAlloced <= areaIdx)
        {
            __ui64 toAlloc = areaIdx + 32;
            Log("Reallocating Map for [%llx:%llx] : areas=%p, areaIdx=%llx, alloced %llx => %llx\n",
                _brid(getBranchRevId()), areas, areaIdx, areasAlloced, toAlloc);
#if 1
            void** newAreas = (void**) malloc(sizeof(void*) * (toAlloc));
            if (!newAreas)
            {
                Fatal("Could not allocate areas array up to %llukB\n", toAlloc * sizeof(void*));
            }
            void** oldAreas = areas;
            if (oldAreas)
                memcpy(newAreas, oldAreas, sizeof(void*) * areasAlloced);
            areas = newAreas;

            /*
             * This may be dangerous, we can be using oldAreas in another thread
             * So take a grace period (reschedule), but this is not a good idea anyway...
             */
            usleep(1);

            if (oldAreas)
                free(oldAreas);

#else
            areas = (void**) realloc ( areas, sizeof(void*) * ( toAlloc ) );
#endif
            for (__ui64 i = areasAlloced; i < toAlloc; i++)
            {
                areas[i] = NULL;
            }
            areasAlloced = toAlloc;
        }
    }

    void*
    PersistentDocumentAllocator::mapAreaFromFile (__ui64 areaIdx, AbsolutePagePtr offset)
    {
        AssertBug(areas[areaIdx] == NULL, "Area already allocated !\n");
        void* area = getPersistentStore().mapArea(offset, AreaSize);
        areasMapped++;
        return area;
    }

    void
    PersistentDocumentAllocator::mapArea (__ui64 areaIdx)
    {
        lockMutex_Map();
        if ((areaIdx < areasAlloced) && (areas[areaIdx]))
        {
            Warn("Doc %p : area %llx already mapped. Lost the race ?\n", this, areaIdx);
            unlockMutex_Map();
            return;
        }
        doMapArea(areaIdx);
        unlockMutex_Map();
    }

    SegmentPage*
    PersistentDocumentAllocator::getRelativePage (RelativePagePtr relPagePtr)
    {
        mapMutex.assertLocked();

#if PARANOID
        AssertBug ( relPagePtr % PageSize == 0, "RelativePagePtr not aligned to PageSize !\n" );

        AssertBug ( relPagePtr < getDocumentAllocationHeader().nextRelativePagePtr,
                "Relative page %llx out of bounds : "
                "indirection.nextRelativePagePtr = %llx\n",
                relPagePtr, getDocumentAllocationHeader().nextRelativePagePtr );

#endif
        __ui64 areaIdx = relPagePtr >> InAreaBits;
        __ui64 pageIdx = relPagePtr & AreaPageMask;
        Log_PDAMap ( "Get rel=%llx, areaIdx = %llx, pageIdx = %llx\n", relPagePtr, areaIdx, pageIdx );
        if (areasAlloced <= areaIdx || areas[areaIdx] == NULL)
        {
            Log_PDAMap ( "Area not alloced ! Must map Area=%llx\n", areaIdx );
            doMapArea(areaIdx);
#if PARANOID
            AssertBug ( areas[areaIdx], "Mapping of Area=%llx failed !\n", areaIdx );
#endif
        }
        return (SegmentPage*) ((__ui64) areas[areaIdx] + pageIdx );
    }

    void
    PersistentDocumentAllocator::doMapArea (__ui64 areaIdx)
    {
        mapMutex.assertLocked();
        allocateAreaMap(areaIdx);

        AssertBug(areas[areaIdx] == NULL, "Area already allocated ! areaIdx=%llx\n", areaIdx);
        void* area = mapAreaFromFile(areaIdx, areaIdx << InAreaBits);

        /**
         * Here we must map all known pages of the corresponding Area.
         */
        __ui64 nbPages = 0, nbMaps = 0;
        __ui64 contiguousPages = 0;
        RelativePagePtr lastRelPagePtr = 0;
        AbsolutePagePtr lastAbsPagePtr = 0;
        AbsolutePagePtr firstAbsPagePtr = 0;

        /**
         * The first Page to map.
         */
        RelativePagePtr beginRelPagePtr = areaIdx << InAreaBits;

        /**
         * The last page to map.
         */
        RelativePagePtr endRelPagePtr = (areaIdx << InAreaBits) + AreaSize - PageSize;
        if (endRelPagePtr + PageSize >= getNextRelativePagePtr())
        {
            endRelPagePtr = getNextRelativePagePtr() - PageSize;
        }

        Log_PDAMap ( "Mapping areaIdx=%llx, from %llx to %llx\n",
                areaIdx, beginRelPagePtr, endRelPagePtr );

        /**
         * Now we loop on the PageInfo to fetch the relative-to-absolute page mapping
         * And we try to group contiguous ones to minimize calls to remap_file_pages() done in mapRelativePages()
         */
        for (PageInfoIterator iter(*this, beginRelPagePtr); iter.first() <= endRelPagePtr; iter++)
        {
            RelativePagePtr relPagePtr = iter.first();
            PageInfo& pageInfo = iter.second();

            AbsolutePagePtr absPagePtr;
            PageFlags pageFlags;
            absPagePtr = pageInfo.absolutePagePtr & PagePtr_Mask;
            pageFlags = pageInfo.absolutePagePtr & PageFlags_Mask;

            Log_PDAMap ( "Page '%llx' -> abs '%llx'\n", relPagePtr, absPagePtr );
            AssertBug(absPagePtr,
                      "Null absolute page ptr provided for relPagePtr=0x%llx, nextRelativePagePtr=0x%llx !\n",
                      relPagePtr, getNextRelativePagePtr());
            if (contiguousPages == 0)
            {
                lastAbsPagePtr = firstAbsPagePtr = absPagePtr;
                lastRelPagePtr = relPagePtr;
                contiguousPages = 1;
            }
            else if (lastAbsPagePtr + PageSize == absPagePtr)
            {
                lastAbsPagePtr = absPagePtr;
                contiguousPages++;
            }
            else if (contiguousPages)
            {
                mapRelativePages(area, lastRelPagePtr, firstAbsPagePtr, contiguousPages);
                nbMaps++;
                lastAbsPagePtr = firstAbsPagePtr = absPagePtr;
                lastRelPagePtr = relPagePtr;
                contiguousPages = 1;
            }
            if (relPagePtr == endRelPagePtr || (relPagePtr + PageSize) % AreaSize == 0)
            {
                mapRelativePages(area, lastRelPagePtr, firstAbsPagePtr, contiguousPages);
                nbMaps++;
                lastAbsPagePtr = firstAbsPagePtr = NullPage;
                lastRelPagePtr = 0;
                contiguousPages = 0;
            }
            nbPages++;
        }
        areas[areaIdx] = area;
        static __ui64 nbAreasMapped = 0;
        nbAreasMapped++;

        Log_PDAMap ( "Mapped area=%llx (at %p) [%llx:%llx], %llu pages, %llu maps, totalMaps=%llu\n",
                areaIdx, area, beginRelPagePtr, endRelPagePtr,
                nbPages, nbMaps,
                nbAreasMapped );
    }

    void*
    PersistentDocumentAllocator::getArea (__ui64 areaIdx)
    {
        if (areaIdx >= areasAlloced)
        {
            Error("Invalid areaIdx=%llx > areasAlloced=%llx\n", areaIdx, areasAlloced);
            return NULL;
        }
        if (!areas[areaIdx])
        {
            mapArea(areaIdx);
        }
        return areas[areaIdx];
    }

    void
    PersistentDocumentAllocator::mapRelativePages (void* area, RelativePagePtr relPagePtr, AbsolutePagePtr absPagePtr,
                                                   __ui64 count)
    {
        mapMutex.assertLocked();
#if PARANOID
        AssertBug ( relPagePtr % PageSize == 0, "RelativePagePtr not aligned to PageSize !\n" );
        AssertBug ( absPagePtr % PageSize == 0, "AbsolutePagePtr not aligned to PageSize !\n" );
#endif
        Log_PDAMap ( "mapRelativePages : rel=%llx, abs=%llx, count=%llx\n", relPagePtr, absPagePtr, count );
        __ui64 areaIdx = relPagePtr >> InAreaBits;
        __ui64 pageIdx = relPagePtr & AreaPageMask;

#if PARANOID
        if ( areaIdx >= areasAlloced )
        {
            Bug ( "Area not alloced ! areaIdx=%llx, areasAlloced=%llx, area=%p\n", areaIdx, areasAlloced,
                    areaIdx < areasAlloced ? areas[areaIdx] : NULL );
        }
#endif

        Log_PDAMap ( "areaIdx = %llx, pageIdx = %llx, endIdx=%llx, AreaSize=%llx\n",
                areaIdx, pageIdx, pageIdx + ( count* PageSize),
                AreaSize );

#if PARANOID
        AssertBug ( pageIdx + ( count * PageSize ) <= AreaSize,
                "Count is too large : pageIdx=%llx, count=%llx, AreaSize=%llx\n",
                pageIdx, count, AreaSize );
#endif

        void * page = (void*) ((__ui64) area + pageIdx );

        /*
         * Get the page-offset of the first absolute page
         */
        ssize_t pgoff = absPagePtr >> InPageBits;

        Log_PDAMap ( "Area=%p, Page=%p, PageSize=%llx, absPagePtr=%llx, pgoff=%lx\n",
                areas[areaIdx], page, PageSize, absPagePtr, (unsigned long) pgoff );

        int res = remap_file_pages(page, PageSize * count, 0, pgoff, 0 | ( MAP_SHARED | MAP_POPULATE | MAP_NONBLOCK));
        if (res == -1)
        {
            Bug("Could not remap file page : errno=%d:%s. Arguments : page=%p, size=%llx, pgoff=%lx\n", errno,
                strerror ( errno ), page, (long long) PageSize*count, (long ) pgoff);

        }
#ifdef XEM_MEM_PROTECT_SYS
        mprotect ( page, PageSize*count, PROT_READ );
#endif
    }

    void
    PersistentDocumentAllocator::doMapAllPages ()
    {
        NTime begin = getntime();
        for (__ui64 areaIdx = 1; areaIdx < areasAlloced; areaIdx++)
            if (areas[areaIdx] == NULL)
            {
                Warn("Mapping area '%llx'\n", areaIdx);
                mapArea(areaIdx);

            }

        NTime end = getntime();
        WarnTime("doMapAllPages took : ", begin, end)
    }

}
