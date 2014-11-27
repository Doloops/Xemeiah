#include <Xemeiah/persistence/persistentdocumentallocator.h>
#include <Xemeiah/persistence/persistentbranchmanager.h>
#include <Xemeiah/trace.h>

#include <Xemeiah/auto-inline.hpp>
#include <Xemeiah/persistence/auto-inline.hpp>

#include <Xemeiah/persistence/pageinfoiterator.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#define Log_PDA Debug
#define Log_PDA_Housewife Info
#define Log_APW Debug // Logging for authrorizePageWrite

namespace Xem
{
    PersistentDocumentAllocator::PersistentDocumentAllocator (PersistentStore& store, AbsolutePagePtr revisionPagePtr) :
            DocumentAllocator(store), persistentStore(store), revisionPageRef(store, revisionPagePtr), mapMutex("Map Mutex", this), allocMutex("Alloc Mutex",
                                                                                                      this)
    {
#ifdef __XEM_PERSISTENTDOCUMENT_HAS_PAGEINFOPAGECACHE
        pageInfoPageCache = NULL;
        pageInfoPageCacheSz = 0;
#endif // __XEM_PERSISTENTDOCUMENT_HAS_PAGEINFOPAGECACHE

        doInitialize();
    }

    PersistentDocumentAllocator::~PersistentDocumentAllocator ()
    {
        Log_PDA ( "At PersistentDocumentAllocator destructor !\n" );
        if (revisionPageRef.getPage())
        {
            Log_PDA ( "Deleting PersistentDocumentAllocator brId=[%llx:%llx]\n", _brid(revisionPageRef.getPage()->branchRevId) );
        }
        else
        {
            Warn("Deleting PersistentDocumentAllocator with no revisionPage mapping.\n");
        }
        flushInMemCaches();
        unmapAllAreas();
    }

    void
    PersistentDocumentAllocator::flushInMemCaches ()
    {
#ifdef __XEM_PERSISTENTDOCUMENTALLOCATOR_HAS_PAGEINFOPAGETABLE
        Log_PDA ( "PIPT CACHE Flush pageInfoPageTable\n" );
        pageInfoPageTable.clear();
#endif // __XEM_PERSISTENTDOCUMENTALLOCATOR_HAS_PAGEINFOPAGETABLE
        releaseAllAbsolutePages();
#ifdef __XEM_PERSISTENTDOCUMENTALLOCATOR_HAS_WRITABLEPAGECACHE
        writablePageCache.clearCache();
#endif // __XEM_PERSISTENTDOCUMENTALLOCATOR_HAS_WRITABLEPAGECACHE
#ifdef __XEM_PERSISTENTDOCUMENTALLOCATOR_HAS_PAGEINFOPAGEPTRTABLE
        pageInfoPtrTable.clear();
#endif // __XEM_PERSISTENTDOCUMENTALLOCATOR_HAS_PAGEINFOPAGEPTRTABLE
#ifdef __XEM_PERSISTENTDOCUMENT_HAS_PAGEINFOPAGECACHE
        flushPageInfoPageCache ();
#endif // __XEM_PERSISTENTDOCUMENT_HAS_PAGEINFOPAGECACHE
    }

#ifdef __XEM_PERSISTENTDOCUMENT_HAS_PAGEINFOPAGECACHE
    void PersistentDocumentAllocator::flushPageInfoPageCache ()
    {
        Log_PDA ( "Flushing writable page cache.\n" );
        pageInfoPageCacheSz = 0;
        if ( pageInfoPageCache)
        free ( pageInfoPageCache );
        pageInfoPageCache = NULL;
    }
#endif // __XEM_PERSISTENTDOCUMENT_HAS_PAGEINFOPAGECACHE

    void
    PersistentDocumentAllocator::releaseAllAbsolutePages ()
    {
#if 0
        Lock lock(mapMutex);
        Log_PDA_Housewife ( "releaseAllAbsolutePages : releasing %lu pages.\n", (unsigned long) absolutePages.size() );
        for (AbsolutePages::iterator iter = absolutePages.begin(); iter != absolutePages.end(); iter++)
        {
            AssertBug(iter->first && iter->second, "Null Absolute page %llx -> %p provided !\n", iter->first,
                      iter->second);
            if (iter->second == revisionPageRef.getPage())
            {
                Log_PDA ( "Skipping %llx => %p as it is the revisionPage !\n", iter->first, iter->second );
                continue;
            }
            Log_PDA ( "releaseAllAbsolute : at %llx => %p (revision at %p)\n",
                    iter->first, iter->second, revisionPage );
            getPersistentStore().releasePage(iter->first);
        }
        absolutePages.clear();
#endif
    }

    void
    PersistentDocumentAllocator::unmapAllAreas ()
    {
        if (areas)
        {
            __ui64 areasUnmapped = 0;
            for (__ui64 idx = 0; idx < areasAlloced; idx++)
            {
                void* area = areas[idx];
                if (!area)
                    continue;
                areasUnmapped++;
                Log_PDA ( "[PERSDOC-UNMAP] idx=0x%llx, area=%p\n", idx, area );

                getPersistentStore().unmapArea(area, AreaSize);
            }
            free(areas);
            areas = NULL;

            Log_PDA ( "areasAlloced=%llu, areasMapped=%llu, areasUnmapped=%llu\n",
                    areasAlloced, areasMapped, areasUnmapped );
            areasAlloced = 0;
            areasMapped = 0;
            revisionPageRef = AbsolutePageRef<RevisionPage>(getPersistentStore(), (RevisionPagePtr) NullPage);
            documentAllocationHeader = NULL;
        }
    }

    void
    PersistentDocumentAllocator::authorizePageWrite (RelativePagePtr relPagePtr)
    {
        /*
         * Grant that a segment page is writable
         * It is assumed that the page corresponds to an already mapped Area
         * because Document::getSegment<Write>() is called after Document::getSegment<Read> ()
         */
        Log_APW ( "authorizePageWrite relPagePtr=0x%llx\n", relPagePtr );
#ifdef __XEM_PERSISTENTDOCUMENTALLOCATOR_HAS_WRITABLEPAGECACHE
        if (writablePageCache.isWritable(relPagePtr))
            return;
#endif // __XEM_PERSISTENTDOCUMENTALLOCATOR_HAS_WRITABLEPAGECACHE

#if PARANOID
        getPersistentStore().getPersistentBranchManager().assertBranchLockedForWrite ( getBranchRevId().branchId );
#endif

        mapMutex.lock();

        __ui64 index;
        AbsolutePageRef<PageInfoPage> pageInfoPageRef = doGetPageInfoPage(relPagePtr, index, true);
        PageInfo& pageInfo = getPageInfo(pageInfoPageRef, index);
        BranchRevId& segPageBranchRevId = pageInfo.branchRevId;

        AssertBug(segPageBranchRevId.branchId && segPageBranchRevId.revisionId,
                  "Null branchRevId=%llx:%llx for page %llx, from revision %llx:%llx\n", _brid(segPageBranchRevId),
                  relPagePtr, _brid(revisionPageRef.getPage()->branchRevId));

        if ( bridcmp(segPageBranchRevId, revisionPageRef.getPage()->branchRevId) != 0)
        {
            /**
             * BranchRevId differs between this page and the RevisionPage, so we must copy this page.
             */
            PageFlags pageFlags = pageInfo.absolutePagePtr & PageFlags_Mask;

            SegmentPage* segPage = getRelativePage(relPagePtr);

            Log_APW ( "Steal segmentPage (rev brid=%llx:%llx, page brid=%llx:%llx) : rel=0x%llx, pageFlags=0x%llx, pageType=0x%llx, segPage=%p\n",
                    _brid(revisionPageRef.getPage()->branchRevId),
                    _brid(segPageBranchRevId),
                    relPagePtr, pageFlags, __getPageType(pageFlags), segPage );

            AbsolutePagePtr newAbsPagePtr = stealSegmentPage(segPage, __getPageType(pageFlags));
            AssertBug(__getPageType(newAbsPagePtr) == PageType_Segment, "Invalid type !\n");

            getPersistentStore().alterPage(pageInfoPageRef.getPage());
            pageInfo.absolutePagePtr = newAbsPagePtr | pageFlags;
            pageInfo.branchRevId = revisionPageRef.getPage()->branchRevId;
            getPersistentStore().protectPage(pageInfoPageRef.getPage());
            // setPageInfo(relPagePtr, pageInfo);

            __ui64 areaIdx = relPagePtr >> InAreaBits;
            AssertBug(areaIdx < areasAlloced && areas[areaIdx], "Area not allocated !\n");
            void* area = areas[areaIdx];
            mapRelativePages(area, relPagePtr, newAbsPagePtr & PagePtr_Mask, 1);
        }
#ifdef __XEM_PERSISTENTDOCUMENTALLOCATOR_HAS_WRITABLEPAGECACHE
        writablePageCache.setWritable(relPagePtr);
#endif // __XEM_PERSISTENTDOCUMENTALLOCATOR_HAS_WRITABLEPAGECACHE
        mapMutex.unlock();

    }

    void
    PersistentDocumentAllocator::checkPageIsAlterable (void* page)
    {
        /*
         * Stupid brute-force algorithm to check that a page is really alterable
         * This is only for testing purposes, and should not be used in a production context
         *
         * This checks that a call to alterSegment() is always preceded by a call to getSegment<Write> ()
         *
         * The idea is to reverse-lookup the in-page mem pointer to find the RelativePagePtr back
         * and check in PageInfo that the BranchRevId really corresponds to this document.
         *
         * It is assumed that the page corresponds to a mapped area.
         *
         * We are playing with pointers, and 32bit processors seem buggy on void*->ui64 comparison. Use __hptr instead
         */
        typedef unsigned long int __hptr;
        __hptr pageVMPtr = (__hptr) page;

        for (__ui64 areaIdx = 0; areaIdx < areasAlloced; areaIdx++)
        {
            __hptr areaVMPtr = (__hptr) areas[areaIdx];
            if ( areaVMPtr <= pageVMPtr && pageVMPtr < areaVMPtr + AreaSize )
            {
                __hptr offset = pageVMPtr - areaVMPtr;
                if ( areaIdx == 0 && offset == 0 )
                {
                    // This is the revision page, nothing to do.
                    return;
                }
                RelativePagePtr relPagePtr = ( areaIdx << InAreaBits ) + offset;

                __ui64 index;
                AbsolutePageRef<PageInfoPage> pageInfoPageRef = doGetPageInfoPage(relPagePtr, index, false);
                PageInfo& pageInfo = getPageInfo(pageInfoPageRef, index);

                Log_PDA ( "pageInfo : rel=%llx, abs=%llx, brid=%llx:%llx - myBrid=%llx:%llx\n",
                        relPagePtr, pageInfo.absolutePagePtr, _brid(pageInfo.branchRevId),
                        _brid(getBranchRevId() ) );
                if ( bridcmp(pageInfo.branchRevId, getBranchRevId()) )
                {
                    Bug ( "Page is not alterable ! at rel=%llx, abs=%llx, page brid=%llx:%llx, rev brid=%llx:%llx\n",
                            relPagePtr, pageInfo.absolutePagePtr,
                            _brid(pageInfo.branchRevId), _brid(getBranchRevId()) );
                }
                return;
            }

        }
        Bug("Could not find any area matching page=%p\n", page);
    }

    AllocationProfile
    PersistentDocumentAllocator::getAllocationProfile (SegmentPtr segmentPtr)
    {
        RelativePagePtr relPagePtr = (segmentPtr & PagePtr_Mask);
        Log_PDA ( "Getting allocation profile for seg=%llx, page=%llx\n", segmentPtr, relPagePtr );
        Lock lock(mapMutex);

        __ui64 index;
        AbsolutePageRef<PageInfoPage> pageInfoPageRef = doGetPageInfoPage(relPagePtr, index, false);
        PageInfo& pageInfo = getPageInfo(pageInfoPageRef, index);
        return pageInfo.allocationProfile;
    }

    __ui32
    PersistentDocumentAllocator::getFirstFreeSegmentOffset (RelativePagePtr relPagePtr)
    {
        Lock lock(mapMutex);
        __ui64 index;
        AbsolutePageRef<PageInfoPage> pageInfoPageRef = doGetPageInfoPage(relPagePtr, index, false);
        PageInfo& pageInfo = getPageInfo(pageInfoPageRef, index);

        Log_PDA ( "getFirstFreeSegmentOffset : page=%llx -> offset=%x (ptr=%llx)\n",
                relPagePtr, pageInfo.firstFreeSegmentInPage, relPagePtr + pageInfo.firstFreeSegmentInPage );
        return pageInfo.firstFreeSegmentInPage;
    }

    void
    PersistentDocumentAllocator::setFirstFreeSegmentOffset (RelativePagePtr relPagePtr, __ui32 offset)
    {
        Lock lock(mapMutex);

        __ui64 index;
        AbsolutePageRef<PageInfoPage> pageInfoPageRef = doGetPageInfoPage(relPagePtr, index, true);
        PageInfo& pageInfo = getPageInfo(pageInfoPageRef, index);

        // PageInfo pageInfo = getPageInfo(relPagePtr, true);
        alterPageInfo(pageInfo);
        pageInfo.firstFreeSegmentInPage = offset;
        // setPageInfo(relPagePtr, pageInfo);
        protectPageInfo(pageInfo);

        Log_PDA ( "setFirstFreeSegmentOffset : page=%llx -> offset=%x (ptr=%llx)\n",
                relPagePtr, offset, relPagePtr + offset );
        // syncPage ( pageInfoPage );
    }

    AbsolutePagePtr
    PersistentDocumentAllocator::__getFreePagePtr (PageType pageType)
    {
        if (pageType != PageType_Segment)
        {
            Bug("Invalid pageType : 0x%llx\n", pageType);
        }
        Log_PDA( "Getting freePage for revisionPage %llx:%llx, ownRef=%llx, pageType=%llx\n",
                _brid(revisionPageRef.getPage()->branchRevId),
                revisionPageRef.getPage()->freePageList, pageType );

        if (!revisionPageRef.getPage()->freePageList)
        {
            AbsolutePagePtr freePageListPtr = getPersistentStore().getFreePageList();
            alterRevisionPage();
            revisionPageRef.getPage()->freePageList = freePageListPtr;
            protectRevisionPage();

#if PARANOID
            AbsolutePageRef<PageList> ownRef_ = getPageList ( revisionPageRef.getPage()->freePageList );
            AssertBug ( ownRef_.getPage()->number == PageList::maxNumber, "Non full page !\n" );
#endif

#ifdef __XEM_COUNT_FREEPAGES
            getPersistentStore().decrementFreePagesCount(PageList::maxNumber + 1);
#endif
        }
        AbsolutePageRef<PageList> ownRef = getPageList(revisionPageRef.getPage()->freePageList);

        AbsolutePagePtr newFreePagePtr;
        if (ownRef.getPage()->number)
        {
            getPersistentStore().alterPage(ownRef.getPage());
            ownRef.getPage()->number--;
            getPersistentStore().protectPage(ownRef.getPage());
            newFreePagePtr = ownRef.getPage()->pages[ownRef.getPage()->number];
        }
        else
        {
#if 0 // Force Sync while getting a new freePageList
            __ui64 nbPages;
            forAllRevisionOwnedPages ( revisionPage, &Xem::Store::syncRevOwnedPage,
                    (void*) PageType_Indirection, nbPages );
#endif
            newFreePagePtr = revisionPageRef.getPage()->freePageList;
            alterRevisionPage();
            revisionPageRef.getPage()->freePageList = NullPage;
            protectRevisionPage();
        }
        alterRevisionPage();
        getRevisionPage()->ownedPages++;
        getRevisionPage()->ownedTypedPages[pageType]++;
        protectRevisionPage();

        Log_PDA ( "New Segmented Page=%llx for revision %llx:%llx, ownedPages=%llu\n",
                newFreePagePtr,
                _brid(getBranchRevId()),
                getRevisionPage()->ownedPages );
        return newFreePagePtr;
    }

    RelativePagePtr
    PersistentDocumentAllocator::getFreeRelativePages (__ui64 askedNumber, __ui64& allocedNumber, AllocationProfile allocProfile )
    {
        allocMutex.assertLocked();

        /*
         * Allocate a minimum of pages as a bunch, to increase page affinity
         */
        __ui64 minimumNumber = 4;
        if ( getNextRelativePagePtr() > ((__ui64)1) << 24 )
        minimumNumber = AreaSize / PageSize;
        allocedNumber = askedNumber;
        if ( allocedNumber < minimumNumber )
        {
            allocedNumber = minimumNumber;
        }

        PageType pageType = PageType_Segment;

        Log_PDA ( "getFreeSegmentPages() : allocedNumber=%llu (0x%llx)\n", allocedNumber, allocedNumber );
        AssertBug ( allocedNumber*PageSize <= AreaSize, "Segment is larger than Area Size !\n" );

        /*
         * The first page used for this allocation
         */
        RelativePagePtr firstUsedPagePtr = getNextRelativePagePtr();

        /*
         * The effective relative pagePtr to provide for this allocation
         */
        RelativePagePtr firstProvidedPagePtr = firstUsedPagePtr;

        /*
         * The last relative page provided for this allocation
         */
        RelativePagePtr lastProvidedPagePtr = firstProvidedPagePtr + (PageSize*(allocedNumber-1));

        /*
         * First, check that we are not crossing Area bounds.
         * If we are, then we must align to the next Area start.
         * \todo : We must compute this border with the SegmentSizeMax, not the AreaSize !
         */
        __ui64 firstAreaIdx = firstUsedPagePtr >> InAreaBits;
        __ui64 lastAreaIdx = lastProvidedPagePtr >> InAreaBits;

        if ( firstAreaIdx != lastAreaIdx )
        {
            /*
             * Align the first provided page with the next area
             */
            firstProvidedPagePtr = lastAreaIdx << InAreaBits;

            /*
             * Recompute last page used
             */
            lastProvidedPagePtr = firstProvidedPagePtr + (PageSize*(allocedNumber-1));

            Warn ( "/!\\ Aligning to areas !!! firstUsed=0x%llx, firstProvided=0x%llx, last=0x%llx\n",
            firstUsedPagePtr, firstProvidedPagePtr, lastProvidedPagePtr );
        }

        /*
         * The next relative page free, to set in RevisionPage
         */
        RelativePagePtr nextRelativePagePtr = lastProvidedPagePtr + PageSize;

        /*
         * First, update the DocumentHead with this new upper limit of relative page
         */
        alterDocumentAllocationHeader ();
        getDocumentAllocationHeader().nextRelativePagePtr = nextRelativePagePtr;
        protectDocumentAllocationHeader ();

        Log_PDA ( "Allocating %llu pages : get relative pages from 0x%llx to 0x%llx\n",
        allocedNumber, firstProvidedPagePtr, lastProvidedPagePtr );

        mapMutex.lock ();
        /*
         * Then, loop on the pageInfoPage to initialize the new relative-to-absolute mapping
         */
        for ( PageInfoIterator iter ( *this, firstUsedPagePtr, true ); iter.first() < nextRelativePagePtr; iter++ )
        {
            AbsolutePagePtr absPtr = __getFreePagePtr ( pageType );
            Log_PDA ( "New SegmentPage absolute=%llx, relative=%llx\n", absPtr, iter.first() );

            PageInfo& pageInfo = iter.second();

            /*
             * Initialize the PageInfo
             */
            alterPageInfo(pageInfo);
            pageInfo.branchRevId = revisionPageRef.getPage()->branchRevId;
            pageInfo.absolutePagePtr = absPtr | pageType;
            pageInfo.allocationProfile = allocProfile;
            pageInfo.firstFreeSegmentInPage = PageSize * 2;
            protectPageInfo(pageInfo);
            // setPageInfo(iter.first(), pageInfo);

            /*
             * If the area containing this page is allocated, we have to remap it to the
             * new absolute page. Otherwise, this will be done the next time we mapArea() it.
             */
            __ui64 areaIdx = iter.first() >> InAreaBits;
            if ( areaIdx < areasAlloced && areas[areaIdx] )
            {
                mapRelativePages ( areas[areaIdx], iter.first(), absPtr, 1 );
            }
        }
        mapMutex.unlock ();

        Log_PDA ( "OK, Allocated %llu pages : get relative pages from 0x%llx to 0x%llx\n",
        allocedNumber, firstProvidedPagePtr, nextRelativePagePtr );

        /*
         * If we have over-allocated in case of an Area-boundary cross, we have to
         * mark the gap as a free segment
         */
        if ( firstProvidedPagePtr != firstUsedPagePtr )
        {
            markSegmentAsFree ( firstUsedPagePtr, firstProvidedPagePtr - firstUsedPagePtr, allocProfile );
        }

        /*
         * If the first provided page is part of an Area which is not mapped
         * especially when the area is greater than the current areasAlloced limit,
         * we *have* to map it because Document::__getSegment_Read() will not check that
         * the area is upper limits.
         * TODO be clear about that
         */
        __ui64 areaIdx = firstProvidedPagePtr >> InAreaBits;
        if ( areaIdx >= areasAlloced || areas[areaIdx] == NULL )
        {
            mapArea ( areaIdx );
        }
        return firstProvidedPagePtr;
    }

    BranchFlags
    PersistentDocumentAllocator::getBranchFlags ()
    {
        return getPersistentStore().getPersistentBranchManager().getBranchFlags(getBranchRevId().branchId);
    }

    String
    PersistentDocumentAllocator::getBranchName ()
    {
        String branchName = getPersistentStore().getBranchManager().getBranchName(getBranchRevId().branchId);
        AssertBug(branchName.c_str() && branchName.c_str()[0], "Invalid NULL branch name for branchId %llx\n",
                  getBranchRevId().branchId);
        return branchName;
    }

    void
    PersistentDocumentAllocator::housewife ()
    {
        if (refCount > 1)
        {
            Log_PDA ( "[HOUSEWIFE] : refCount=%llu : exiting.\n", refCount );
            return;
        }
        static const __ui64 minAreasMapped = (64ULL * 1024ULL * 1024ULL) >> InAreaBits;
        static const __ui64 maxAreasMapped = (128ULL * 1024ULL * 1024ULL) >> InAreaBits;

        if (areasMapped < maxAreasMapped)
        {
            return;
        }

        Log_PDA_Housewife ( "[HOUSEWIFE %llx:%llx] : %llu MBytes mapped, " // "%llu KBytes for absolute pages"
                "(refCount=%llx, areasAlloced = '%llu', areasMapped = '%llu' -> %llu%% mapped)\n",
                _brid(getBranchRevId()),
                ((areasMapped << InAreaBits)>>20),
                // ((((unsigned long long)absolutePages.size()) << InPageBits)>>10),
                refCount, areasAlloced, areasMapped, (100* areasMapped)/areasAlloced );

        flushInMemCaches();

        __ui64 originalAreasMapped = areasMapped;
        (void) originalAreasMapped;

        // for (__ui64 idx = 1; idx < areasAlloced; idx++)
        for (__ui64 idx = areasAlloced - 1; idx > 0; idx--)
        {
            void* area = areas[idx];
            if (!area)
            {
                continue;
            }
            Log_PDA ( "[HOUSEWIFE] : unalloc '%p' (idx=%llu)\n", area, idx );

            getPersistentStore().unmapArea(area, AreaSize);

            areas[idx] = NULL;
            areasMapped--;
            if (areasMapped <= minAreasMapped)
            {
                break;
            }
        }
        Log_PDA_Housewife ( "[HOUSEWIFE %llx:%llx] : areasAlloced = '%llu', areasMapped reduced from %llu (%llu%%) to %llu (%llu%%) !\n",
                _brid(getBranchRevId()),
                areasAlloced,
                originalAreasMapped, (100* originalAreasMapped)/areasAlloced,
                areasMapped, (100* areasMapped)/areasAlloced );
    }

}
