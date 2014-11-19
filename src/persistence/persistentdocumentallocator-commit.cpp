#include <Xemeiah/persistence/persistentdocument.h>
#include <Xemeiah/persistence/persistentbranchmanager.h>

#include <Xemeiah/trace.h>
#include <Xemeiah/log-time.h>

#include <Xemeiah/auto-inline.hpp>
#include <Xemeiah/persistence/auto-inline.hpp>

#include <sys/mman.h>

#define Log_PDACommit Debug
#define Info_PDACommit Info
#define Log_SyncPage Debug

#define __XEM_PERSISTENTDOCUMENT_COMMIT_OWNED_PAGES


namespace Xem
{
  /**
   * \todo reimplement this. We cannot use Store::syncPage(), because Store will check that page is in its own boundaries. It is not, because we have remap_file_pages () !
   */
  void
  __forceSyncPage(void* _page)
  {
    void* page = (void*) ((__ui64 ) _page & PagePtr_Mask);
    Log_SyncPage ( "syncPage _page=%p, page=%p\n", _page, page );
    if (msync(page, PageSize, MS_ASYNC) == -1)
      {
        Warn ( "Could not sync page %p\n", page );
      }
  }

  void
  PersistentDocumentAllocator::syncPage(void* _page)
  {
#if 0
    __forceSyncPage ( _page );
#endif
  }

  bool
  PersistentDocumentAllocator::syncOwnedIndirectionPage(
      RelativePagePtr relPagePtr, AbsolutePagePtr absPagePtr,
      PageType pageType, bool isStolen, void* arg)
  {
    switch (pageType)
      {
    case PageType_Indirection:
      {
        IndirectionPage* page = getIndirectionPage(absPagePtr);
        Log_SyncPage ( "Syncing Indirection page abs=%llx, page=%p\n", absPagePtr, page );
        syncPage(page);
        releasePage(absPagePtr);
        break;
      }
    case PageType_PageInfo:
      {
        PageInfoPage* page = getPageInfoPage(absPagePtr);
        Log_SyncPage ( "Syncing PageInfo page abs=%llx, page=%p\n", absPagePtr, page );
        syncPage(page);
#ifndef __XEM_PERSISTENTDOCUMENTALLOCATOR_HAS_PAGEINFOPAGETABLE
        releasePage(absPagePtr);
#endif // __XEM_PERSISTENTDOCUMENTALLOCATOR_HAS_PAGEINFOPAGETABLE
        break;
      }
    default:
      {
        Warn ( "Not Implemented : default sync !\n" );
      }
      }
    return true;
  }

  bool
  PersistentDocumentAllocator::syncOwnedSegmentPage(RelativePagePtr relPagePtr,
      PageInfo& pageInfo, PageType pageType, bool isStolen, void* arg)
  {
    switch (pageType)
      {
      case PageType_Segment:
        {
          SegmentPage* segPage = getRelativePage(relPagePtr);
          Log_SyncPage ( "Syncing Segment page rel=%llx, stolen=%d, segPage=%p\n", relPagePtr, isStolen, segPage );
          syncPage(segPage);
          break;
        }
      default:
        {
          Warn ( "Not Implemented : default sync !\n" );
        }
      }
    return true;
  }

  bool
  PersistentDocumentAllocator::flushFreePageList()
  {
    /*
     * Flush the freePageList.
     */
    if (revisionPage->freePageList)
      {
        getPersistentStore().freePageList(revisionPage->freePageList);
      }
    else
      {
        Log_PDACommit ( "Empty freePageList\n" );
      }
    return true;
  }

  void
  PersistentDocumentAllocator::commit()
  {
    AssertBug ( revisionPage, "Null RevsionPage !\n" );

    Log_PDACommit ( "Committing %llx:%llx\n", _brid(revisionPage->branchRevId) );

    if (revisionPage->ownedTypedPages[PageType_Segment] == 0)
      {
        Warn ( "Committing %llx:%llx is worthless : no segment page altered !\n",
            _brid(revisionPage->branchRevId) );
      }

    if (!isWritable())
      {
        Log_PDACommit ( "Nothing to commit(), aborting.\n" );
        return;
      }

    AssertBug ( getPersistentStore().getPersistentBranchManager().isBranchLockedForWrite ( getBranchRevId().branchId ),
        "Branch not locked for write !\n" );

#if 1
    /*
     * Very ugly hack : we have to clear reserved element ids
     */
    DocumentHead* documentHead = getSegment<DocumentHead, Read> ( revisionPage->documentHeadPtr, sizeof(DocumentHead) );
    alter ( documentHead );
    documentHead->firstReservedElementId = 0;
    documentHead->lastReservedElementId = 0;
    protect ( documentHead );

#endif

    struct timeb presync;
    ftime(&presync);

#ifdef __XEM_PERSISTENTDOCUMENT_COMMIT_OWNED_PAGES
    __ui64 nbPages = 0;
    forAllIndirectionPages(
        &PersistentDocumentAllocator::syncOwnedIndirectionPage,
        &PersistentDocumentAllocator::syncOwnedSegmentPage, NULL, true, true);
    nbPages++;

#if 0
    if ( nbPages != revisionPage->ownedPages )
      {
        Log_PDACommit ( "Synced %llu pages, but Revision %llx:%llx has %llu owned pages.\n",
            nbPages, _brid(revisionPage->branchRevId),
            revisionPage->ownedPages );
        Store::AllocationStats stats;
        getPersistentStore().checkRevision ( revisionPage, stats );
        Bug ( "Synced %llu pages, but Revision %llx:%llx has %llu owned pages.\n",
            nbPages,
            _brid(revisionPage->branchRevId),
            revisionPage->ownedPages );

      }
#endif

#endif
    /**
     * Flush the free page list
     */
    flushFreePageList();

    /**
     * Flush in-mem caches
     */
    flushInMemCaches();

    alterRevisionPage();
    /**
     * Delete the freePageList header.
     */
    revisionPage->freePageList = NullPage;

    /**
     * Mark the revision as non writable (committed)
     */
    revisionPage->commitTime = time(NULL);
    protectRevisionPage();

    alterDocumentAllocationHeader();
    getDocumentAllocationHeader().writable = false;
    protectDocumentAllocationHeader();

    __forceSyncPage(revisionPage);

    struct timeb postsync;
    ftime(&postsync);
    __ui64 syncms = ((__ui64 ) postsync.time * 1000 + postsync.millitm)
        - ((__ui64 ) presync.time * 1000 + presync.millitm);
    Info_PDACommit ( "Committed %llx:%llx in %llu ms\n",
        _brid(revisionPage->branchRevId), syncms );

    getPersistentStore().getPersistentBranchManager().unlockBranchForWrite(
        getBranchRevId().branchId);
  }

  bool PersistentDocumentAllocator::mayCommit()
  {
    AssertBug ( revisionPage, "Null revisionPage !\n" );
    if ( revisionPage->ownedTypedPages[PageType_Segment] == 0 )
      {
        return false;
      }
    return true;
  }

}
;

