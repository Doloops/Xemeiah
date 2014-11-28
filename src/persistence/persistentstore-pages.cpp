#include <Xemeiah/persistence/persistentstore.h>
#include <Xemeiah/trace.h>

#include <Xemeiah/auto-inline.hpp>
#include <Xemeiah/persistence/auto-inline.hpp>

#include <sys/mman.h>
#include <errno.h>
#include <string.h>

#include <atomic>

#if 1
#define Log_MapPage_Area Info
#define Log_MapPage(...) do{} while(0)
#else
#define Log_MapPage_Area(...) Info("[MAP_AREA]" __VA_ARGS__)
#define Log_MapPage(...) Info("[MAP_PAGE]" __VA_ARGS__)
#endif
#define Log_ChunkMap Log
#define Log_MapPage_Debug Debug
#define Log_GetFreePage Debug

namespace Xem
{
    static __ui64 totalCurrentlyMapped = 0, totalMapped = 0;

    void*
    PersistentStore::mapArea (__ui64 offset, __ui64 length)
    {
        /*
         * \todo study the advantages to use MAP_NORESERVE here
         */
        void* ptr = mmap(NULL, length,
#ifdef XEM_MEM_PROTECT_SYS
                         PROT_READ
#else
                         PROT_READ | PROT_WRITE
                         // ( readOnly ? ( PROT_READ ) : ( PROT_READ | PROT_WRITE ) )
#endif
                         ,
                         MAP_SHARED | MAP_POPULATE | MAP_NONBLOCK, fd, offset);
        if (ptr == MAP_FAILED)
        {
            Error("Could not mmap ! Error %d:%s\n", errno, strerror(errno));
            Bug(".");
            return NULL;
        }
        if (((__ui64 ) ptr) % PageSize)
        {
            Log_MapPage("MMap pointer is not aligned to PageSize. That may not hurt much...\n");
            Bug(".");
            return NULL;
        }
        totalMapped++;
        totalCurrentlyMapped++;
        if (totalMapped % 10 == 0)
        {
            Log_MapPage("mapArea : offset=%llx, length=%llx, ptr=%p\n", offset, length, ptr);

            Log_MapPage_Area("MapStatus : totalMapped=%llu, totalCurrentlyMapped=%llu\n",
                    totalMapped, totalCurrentlyMapped);
        }

        if (madvise(ptr, length, MADV_RANDOM) == -1)
        {
            Warn("Could not madvise() : error=%d:%s\n", errno, strerror(errno));
        }
        return ptr;
    }

    void
    PersistentStore::unmapArea (void* area, __ui64 length)
    {
        int result = munmap(area, length);
        if (result == -1)
        {
            Bug("Could not munmap area=%p, size=0x%llx, err=%d:%s\n", area, length, errno, strerror(errno));
        }
        totalCurrentlyMapped--;
        if (totalCurrentlyMapped % 10 == 0)
        {
            Log_MapPage_Area("MapStatus : totalMapped=%llu, totalCurrentlyMapped=%llu\n", totalMapped, totalCurrentlyMapped);
        }
    }

    void*
    PersistentStore::mapPage (AbsolutePagePtr absPagePtr)
    {
        AbsolutePagePtr chunkIdx = absPagePtr >> ChunkInfo::PageChunk_Index_Bits;
        AbsolutePagePtr chunkPtr = absPagePtr & ChunkInfo::PageChunk_ChunkMask;
        void* chunk = NULL;

        Lock lock(chunkMapMutex);
        ChunkMap::iterator iter = chunkMap.find(chunkIdx);

        if (iter == chunkMap.end())
        {
            chunk = mapArea(chunkPtr, ChunkInfo::PageChunk_Size);
            chunkMap[chunkIdx].page = chunk;
            chunkMap[chunkIdx].refCount = 1;
            Log_ChunkMap ( "[CHUNK] Mapped chunk chunkIdx=%llx, chunkPtr=%llx for absPagePtr=%llx. Nb chunks=%lu\n",
                    chunkIdx, chunkPtr, absPagePtr, (unsigned long) chunkMap.size() );
            iter = chunkMap.find(chunkIdx);
            AssertBug(iter != chunkMap.end(), "Fail !\n");

            if (chunkMap.size() % 10 == 0)
            {
                __ui64 chunkMapSize = chunkMap.size();
                __ui64 mappedChunkSize = (chunkMapSize << ChunkInfo::PageChunk_Index_Bits) >> 20;
                Info("[CHUNK] Now chunkMap.size()=%llu, (total size=%llu Mb)\n", chunkMapSize, mappedChunkSize);
            }
        }
        else
        {
            chunk = iter->second.page;
            iter->second.refCount++;

        }
        Log_MapPage("CHUNK REF : pagePtr=0x%llx, (chunkIdx=0x%llx, chunkPtr=0x%llx), page=%p, refCount=0x%llx\n",
                    absPagePtr, chunkIdx, chunkPtr, iter->second.page, iter->second.refCount);

        if (iter->second.refCount > 60)
        {
            Log_MapPage("Very large number of refCounts for this page !\n");
        }

        return (void*) (((__ui64 ) chunk) + (absPagePtr & ChunkInfo::PageChunk_Mask));
    }

    void*
    PersistentStore::__getAbsolutePage (AbsolutePagePtr pagePtr)
    {
        if (!pagePtr)
        {
            return NULL;
        }
        Log_MapPage_Debug("Get page for pagePtr at index %llx\n", pagePtr);
#if 1 // PARANOID
        AssertBug(pagePtr % PageSize == 0, "Page not aligned to page size !\n");
        AssertBug(pagePtr < fileLength, "Page is after file length !\n");
#endif
        return mapPage(pagePtr);
    }

    void
    PersistentStore::__releasePage (AbsolutePagePtr absPagePtr)
    {
        AbsolutePagePtr chunkIdx = absPagePtr >> ChunkInfo::PageChunk_Index_Bits;

        Lock lock(chunkMapMutex);

        ChunkMap::iterator iter = chunkMap.find(chunkIdx);
        if (iter == chunkMap.end())
        {
            Warn("Could not release page : pagePtr=%llx, chunkIdx=0x%llx does not belong to page map !\n", absPagePtr,
                 chunkIdx);

            Warn("Dump : \n");
            for (ChunkMap::iterator dmp = chunkMap.begin(); dmp != chunkMap.end(); dmp++)
            {
                Warn("\t%llx => %p (refCount=%llx)\n", dmp->first, dmp->second.page, dmp->second.refCount);
            }

            Bug(".");
            return;
        }

        ChunkInfo& chunkInfo = iter->second;

        AssertBug(chunkInfo.refCount, "Null chunkInfo refCount !\n");
        chunkInfo.refCount--;

        Log_MapPage("CHUNK UNREF : pagePtr=0x%llx, chunkIdx=0x%llx, page=%p, refCount=0x%llx\n", absPagePtr, chunkIdx,
                    chunkInfo.page, chunkInfo.refCount);

        if (chunkInfo.refCount)
        {
            return;
        }
        static const int chunkCacheSize = 2048;
        if (chunkMap.size() < chunkCacheSize)
        {
            return;
        }

        void* page = chunkInfo.page;
        chunkMap.erase(iter);

        unmapArea(page, ChunkInfo::PageChunk_Size);
#if 0
        if (munmap(page, ChunkInfo::PageChunk_Size) == -1)
        {
            Bug("Could not unmap page %llx : %p. Error %d:%s\n", absPagePtr, page, errno, strerror(errno));
        }
#endif

        Log_ChunkMap ( "[CHUNK] Released chunk %llx for absPagePtr=%llx => %p. Nb chunks=%lu\n",
                chunkIdx, absPagePtr, page, (unsigned long) chunkMap.size() );
    }

    AbsolutePagePtr
    PersistentStore::getFreePagePtr ()
    {
        Lock lock(freePageHeaderMutex);

        AbsolutePageRef<FreePageHeader> fpHeaderRef = getFreePageHeader();
        AssertBug(fpHeaderRef.getPage(), "No freePageHeader !\n");
        if (!fpHeaderRef.getPage()->ownFreePageList)
        {
            AbsolutePagePtr pageListPtr = getFreePageList(false);
            //	PageList* pageList = getAbsolutePage<PageList> ( pageListPtr );
            alterPage(fpHeaderRef.getPage());
            fpHeaderRef.getPage()->ownFreePageList = pageListPtr; // getAbsolutePagePtr ( pageList );
            protectPage(fpHeaderRef.getPage());
            Log_GetFreePage( "New own freePageList=%llx\n", fpHeaderRef.getPage()->ownFreePageList );
        }
        AssertBug(fpHeaderRef.getPage()->ownFreePageList, "No ownFreePageList !\n");
        AbsolutePageRef<PageList> ownRef = getAbsolutePage<PageList>(fpHeaderRef.getPage()->ownFreePageList);
        AbsolutePagePtr newFreePagePtr;
        AssertBug(ownRef.getPage()->number <= PageList::maxNumber, "Own number out of range !\n");
        if (ownRef.getPage()->number)
        {
            alterPage(ownRef.getPage());
            ownRef.getPage()->number--;
            protectPage(ownRef.getPage());
            newFreePagePtr = ownRef.getPage()->pages[ownRef.getPage()->number];
        }
        else
        {
            newFreePagePtr = fpHeaderRef.getPage()->ownFreePageList;
            alterPage(fpHeaderRef.getPage());
            fpHeaderRef.getPage()->ownFreePageList = NullPage;
            protectPage(fpHeaderRef.getPage());
        }
        Log_GetFreePage( "Got freePage %llx from own=%p / %llx\n",
                newFreePagePtr, ownRef.getPage(), fpHeaderRef.getPage()->ownFreePageList );
#ifdef __XEM_COUNT_FREEPAGES
        lockSB();
        AssertBug(getSB()->nbFreePages, "No free pages !\n");
        getSB()->nbFreePages--;
        unlockSB();
#endif
        return newFreePagePtr;
    }

    bool
    PersistentStore::freePage (AbsolutePagePtr pagePtr)
    {
        return freePages(&pagePtr, 1);
    }

    bool
    PersistentStore::freePages (AbsolutePagePtr* pageList, __ui64 number)
    {
        AssertBug(pageList, "Null pageList provided !\n");
        AssertBug(number, "Zero pages provided !\n");

#ifdef __XEM_COUNT_FREEPAGES
        /*
         * Acount the number of free pages
         * (pageLists are counted off of free pages).
         */
        lockSB();
        getSB()->nbFreePages += number;
        unlockSB();
#endif

        Lock lock(freePageHeaderMutex);
        AbsolutePageRef<FreePageHeader> freePageHeaderRef = getFreePageHeader();
        AbsolutePageRef<PageList> currentFreedPageList = getAbsolutePage<PageList>(
                freePageHeaderRef.getPage()->currentFreedPageList);
        if (!currentFreedPageList.getPage())
        {
            // Make this page the new currentFreedPageList !
            currentFreedPageList = getAbsolutePage<PageList>(pageList[0]);
            alterPage(currentFreedPageList.getPage());
            currentFreedPageList.getPage()->number = number - 1;
            currentFreedPageList.getPage()->nextPage = NullPage;
            memcpy(currentFreedPageList.getPage()->pages, &(pageList[1]), (number - 1) * sizeof(AbsolutePagePtr));
            protectPage(currentFreedPageList.getPage());
            alterPage(freePageHeaderRef.getPage());
            freePageHeaderRef.getPage()->currentFreedPageList = pageList[0];
            protectPage(freePageHeaderRef.getPage());
            return true;
        }
        AssertBug(currentFreedPageList.getPage()->number < PageList::maxNumber, "Out of bounds !\n");
        /*
         * Ok, now we know we have a currentFreedPageList
         * We first must compute the number of pages we can include
         * in the existing currentFreedPageList
         */
        __ui64 toCopy = number;
        if (currentFreedPageList.getPage()->number + number > PageList::maxNumber)
            toCopy = PageList::maxNumber - currentFreedPageList.getPage()->number;

        alterPage(currentFreedPageList.getPage());
        memcpy(&(currentFreedPageList.getPage()->pages[currentFreedPageList.getPage()->number]), pageList,
               toCopy * sizeof(AbsolutePagePtr));
        currentFreedPageList.getPage()->number += toCopy;
        protectPage(currentFreedPageList.getPage());

        if (currentFreedPageList.getPage()->number == PageList::maxNumber)
        {
            /*
             * If it is full, push our currentFreedPageList to firstFreePageList.
             */
            putFreePageListPtr(freePageHeaderRef.getPage()->currentFreedPageList); // currentFreedPageList );
            alterPage(freePageHeaderRef.getPage());
            freePageHeaderRef.getPage()->currentFreedPageList = NullPage;
            protectPage(freePageHeaderRef.getPage());

            if (toCopy < number)
            {
                /*
                 * If we could not put all our pages in our currentFreedPageList
                 * then we must put our remaining pages in a brand new
                 * currentFreedPageList.
                 */

                currentFreedPageList = getAbsolutePage<PageList>(pageList[toCopy]);
                alterPage(currentFreedPageList.getPage());
                currentFreedPageList.getPage()->number = number - toCopy - 1;
                currentFreedPageList.getPage()->nextPage = NullPage;
                memcpy(currentFreedPageList.getPage()->pages, &(pageList[toCopy + 1]),
                       (number - toCopy - 1) * sizeof(AbsolutePagePtr));
                protectPage(currentFreedPageList.getPage());
                alterPage(freePageHeaderRef.getPage());
                freePageHeaderRef.getPage()->currentFreedPageList = pageList[toCopy];
                protectPage(freePageHeaderRef.getPage());
            }
        }
        return true;
    }

    bool
    PersistentStore::freePageList (AbsolutePagePtr freePageListPtr)
    /*
     * \todo (PARANOID) : When freeing a PageList, we shall take care that the page is not already in the free list.
     */
    {
        {
            AbsolutePageRef<PageList> freePageList = getAbsolutePage<PageList>(freePageListPtr);
            freePages(freePageList.getPage()->pages, freePageList.getPage()->number);
        }
        freePage(freePageListPtr);
        // releasePage(freePageListPtr);
        return true;
    }

    /*
     * Page List
     */

    AbsolutePagePtr
    PersistentStore::getFreePageList (bool mustLock)
    {
        Log_GetFreePage( "getFreePageList mustLock=%s\n", mustLock ? "yes" : "no" );

        if (mustLock)
        {
            freePageHeaderMutex.lock();
        }

        AbsolutePageRef<FreePageHeader> fpHeaderRef = getFreePageHeader();
        if (!fpHeaderRef.getPage()->firstFreePageList)
        {
            /*
             * We dont have a pageList at hand, so we must allocate a new one
             * after the current noMansLand of the Store.
             */
            lockSB();
            __ui64 minFileLength = getSB()->noMansLand + PageSize * (PageList::maxNumber + 2);
            if (!extendFile(minFileLength))
            {
                Fatal("Could not extend file to %lu bytes\n", (unsigned long ) minFileLength);
            }
            AssertBug(
                    getSB()->noMansLand + PageSize * ( PageList::maxNumber + 1) < (__ui64) fileLength,
                    "Not enough space to alloc a freePageList. " "noMansLand at 0x%llx, fileLength is 0x%lx, minFileLength is 0x%lx\n",
                    getSB()->noMansLand, (unsigned long) fileLength, (unsigned long) minFileLength);
            AbsolutePagePtr newPageListPtr = getSB()->noMansLand;
            Log_GetFreePage( "Actual noMansLand %llx\n", getSB()->noMansLand );
            getSB()->noMansLand += PageSize * (PageList::maxNumber + 1);
            Log_GetFreePage( "Pushed noMansLand to %llx\n", getSB()->noMansLand );
#ifdef __XEM_COUNT_FREEPAGES
            getSB()->nbFreePages += (PageList::maxNumber + 1);
#endif
            unlockSB();
            AbsolutePageRef<PageList> freePageList = getAbsolutePage<PageList>(newPageListPtr);
#ifdef XEM_MADVISE
            madvise ( freePageList.getPage(), PageSize, MADV_WILLNEED );
#endif
            /*
             * Build the freePageList contents, with the first index at end.
             * The freePageList is used backwards (last index first).
             */
            alterPage(freePageList.getPage());
            freePageList.getPage()->nextPage = NullPage;
            for (__ui64 index = 0; index < PageList::maxNumber; index++)
            {
                freePageList.getPage()->pages[index] = newPageListPtr + (PageSize * ((PageList::maxNumber - index)));
            }
            freePageList.getPage()->number = PageList::maxNumber;
            protectPage(freePageList.getPage());
#if PARANOID
            AssertBug ( freePageList.getPage()->pages[PageList::maxNumber-1] < getSB()->noMansLand,
                    "Gone too far.\n" );
#endif
            Log_GetFreePage( "New freePageList from noMansLand at %llx\n", newPageListPtr );
            if (mustLock)
                freePageHeaderMutex.unlock();
#ifdef XEM_MADVISE
            madvise ( (void*)((__ui64)freePageList+PageSize), PageSize * PageList::maxNumber, MADV_WILLNEED );
#endif
            return newPageListPtr;
        }
        /*
         * Ok, we have a freePageList at hand.
         * Put the next one at head of the SuperBlock list.
         */
        AbsolutePagePtr freePageListPtr = fpHeaderRef.getPage()->firstFreePageList;
        AbsolutePageRef<PageList> freePageList = getAbsolutePage<PageList>(freePageListPtr);
        alterPage(fpHeaderRef.getPage());
        fpHeaderRef.getPage()->firstFreePageList = freePageList.getPage()->nextPage;
        protectPage(fpHeaderRef.getPage());
        Log_GetFreePage( "Found freePageList %llx\n", freePageListPtr );

#ifdef XEM_MADVISE
        /*
         * Try to advise the OS that we will need these pages
         * Tests are not really conclusive about this...
         */
        for ( __ui64 index = PageList::maxNumber - 1;
                index > PageList::maxNumber / 2; index-- )
        madvise ( getAbsolutePage<void> ( freePageList->pages[index] ),
                PageSize, MADV_WILLNEED );

#endif
        if (mustLock)
        {
            freePageHeaderMutex.unlock();
        }
        return freePageListPtr;
    }

    bool
    PersistentStore::putFreePageListPtr (AbsolutePagePtr freePageListPtr)
    {
        AbsolutePageRef<PageList> freePageList = getAbsolutePage<PageList>(freePageListPtr);
        Log_GetFreePage ( "Full currentFreePageList, putting it at disposal, next=%llx.\n",
                getFreePageHeader().getPage()->firstFreePageList );
        AbsolutePageRef<FreePageHeader> fpHeaderRef = getFreePageHeader();
        alterPage(freePageList.getPage());
        freePageList.getPage()->nextPage = fpHeaderRef.getPage()->firstFreePageList;
        protectPage(freePageList.getPage());
        alterPage(fpHeaderRef.getPage());
        fpHeaderRef.getPage()->firstFreePageList = freePageListPtr;
        protectPage(fpHeaderRef.getPage());

        if (0)
        {
            AllocationStats stats;
            checkFreePageHeader(stats);
        }

        return true;
    }

#ifdef __XEM_COUNT_FREEPAGES
    void
    PersistentStore::decrementFreePagesCount (__ui64 count)
    {
        lockSB();
        AssertBug(getSB()->nbFreePages >= count,
                  "Superblock has not that much free pages ! count=%llx, getSB()->nbFreePages=%llx\n", count,
                  getSB()->nbFreePages);
        getSB()->nbFreePages -= count;
        unlockSB();
    }
#endif
}
