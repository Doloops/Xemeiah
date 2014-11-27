#include <Xemeiah/persistence/persistentbranchmanager.h>
#include <Xemeiah/persistence/persistentstore.h>
#include <Xemeiah/persistence/persistentdocumentallocator.h>

#include <Xemeiah/trace.h>
#include <Xemeiah/log-time.h>

#include <Xemeiah/auto-inline.hpp>
#include <Xemeiah/persistence/auto-inline.hpp>

// #define __XEM_PERSISTENCE_QUICKMERGE_EXTRA_CHECK

#define Log_PDAQMerge Debug // That is very verbose

namespace Xem
{
  void PersistentDocumentAllocator::quickMerge ( PersistentDocumentAllocator& targetAllocator )
  {
    AssertBug ( getBranchRevId().revisionId == 1, "QuickMerge : not handled !\n" );
    
    struct timeb premerge;
    ftime ( &premerge );
    
    /*
     * First, copy the FreeSegmentsHeader
     */
    AssertBug ( targetAllocator.documentAllocationHeader->nbAllocationProfiles == 
        documentAllocationHeader->nbAllocationProfiles,
        "Diverging allocation profiles !\n" );

    for ( AllocationProfile allocProfile = 0 ; allocProfile < targetAllocator.documentAllocationHeader->nbAllocationProfiles ; allocProfile ++ )
      {
        FreeSegmentsLevelHeader* sourceFreeSegs = getFreeSegmentsLevelHeader ( allocProfile );
        FreeSegmentsLevelHeader* targetFreeSegs = targetAllocator.getFreeSegmentsLevelHeader ( allocProfile );        
        targetAllocator.alterFreeSegmentsLevelHeader(0);
        memcpy ( targetFreeSegs, sourceFreeSegs, sizeof(FreeSegmentsLevelHeader) );
        targetAllocator.protectFreeSegmentsLevelHeader(0);                    
      }

    /**
     * Also : alter the revision's journalEnd and nextRelativePagePtr
     */
    targetAllocator.alterDocumentAllocationHeader ();
    Info ( "[MERGE] : setting allocedJournal : tgt=0x%llx, setting to 0x%llx\n", 
        targetAllocator.getRevisionPage()->journalHead.lastAllocedJournalItem, getRevisionPage()->journalHead.lastAllocedJournalItem );
    targetAllocator.getRevisionPage()->journalHead.lastAllocedJournalItem = getRevisionPage()->journalHead.lastAllocedJournalItem;
    
    Info ( "[MERGE] : setting nextRelativePagePtr : tgt=0x%llx, setting to 0x%llx\n", 
        targetAllocator.getNextRelativePagePtr(), getNextRelativePagePtr() );
    targetAllocator.getDocumentAllocationHeader().nextRelativePagePtr = getNextRelativePagePtr();
    targetAllocator.protectDocumentAllocationHeader ();    
    
    /**
     * Next : steal all our contents.
     */
#if 0 // Only for owned
    forAllIndirectionPages ( &PersistentDocumentAllocator::quickMergeIndirectionPage, &PersistentDocumentAllocator::quickMergeSegmentPage, 
          (void*)&targetDocument, true, true );
#else // Extra care : for all pages, we will check BranchRevId manually
#if 0
    targetAllocator.alterRevisionPage ();
    targetAllocator.getRevisionPage()->indirection = getRevisionPage()->indirection;
    targetAllocator.protectRevisionPage ();    
    alterRevisionPage ();
    getRevisionPage()->indirection.level = 0;
    getRevisionPage()->indirection.firstPage = NullPage;
    protectRevisionPage();
#else
    forAllIndirectionPages ( &PersistentDocumentAllocator::quickMergeIndirectionPage, &PersistentDocumentAllocator::quickMergeSegmentPage, 
          (void*)&targetAllocator, false, false );
#endif
#endif

    /*
     * We shall not have any segment pages owned. Check that
     */
#if 0
    AssertBug ( getDocumentHead()->ownedTypedPages[PageType_Segment] == 0,
      "Error ! brId=[%llx:%llx] Still 0x%llx segment pages owned !\n", _brid(getBranchRevId()), 
      getDocumentHead()->ownedTypedPages[PageType_Segment] );
#endif
      
    struct timeb postmerge;
    ftime ( &postmerge );
     
     
    __ui64 mergems = ( (__ui64)postmerge.time * 1000 + postmerge.millitm )
      - ( (__ui64)premerge.time * 1000 + premerge.millitm );
    Info ( "Merged %llx:%llx in %llu ms\n", _brid(getBranchRevId()), mergems );
     
#if 0

    // __ui64 freeSegmentsSize = sizeof(AllocationProfile) + sizeof(FreeSegmentsLevelHeader) * freeSegmentsHeader->nbAllocationProfiles;

//    getPersistentStore().alterPage ( targetFreeSegmentsHeader & PagePtr_Mask );
    targetAllocator.alterFreeSegmentsLevelHeader(0);
    memcpy ( targetFreeSegmentsHeader, freeSegmentsHeader, freeSegmentsSize );
    targetAllocator.protectFreeSegmentsLevelHeader(0);    
//    getPersistentStore().protectPage ( targetFreeSegmentsHeader & PagePtr_Mask  );
#endif

  }

  bool PersistentDocumentAllocator::quickMergeIndirectionPage ( RelativePagePtr relPagePtr, AbsolutePagePtr absPagePtr, 
        PageType pageType, bool isStolen, void* arg )
  {
    Log_PDAQMerge ( "Nothing to do for quickMerge at rel=0x%llx, abs=0x%llx, pageType=0x%llx\n", relPagePtr, absPagePtr, pageType );
    return true;
  }
  
  bool PersistentDocumentAllocator::quickMergeSegmentPage ( RelativePagePtr relPagePtr, PageInfo& pageInfo, PageType pageType, bool isStolen, void* arg )
  {
 
    // PersistentDocument& targetDocument = *((PersistentDocument*)arg);
    PersistentDocumentAllocator& targetDocumentAllocator = *((PersistentDocumentAllocator*) ( arg ));
    
    BranchRevId& myBranchRevId = revisionPageRef.getPage()->branchRevId;
    // Info ( "quickMerge : at rel=0x%llx, abs=0x%llx, pageType=0x%llx\n", relPagePtr, absPagePtr, pageType );
    if ( bridcmp(pageInfo.branchRevId, myBranchRevId ) ) return true;
    AssertBug ( !isStolen, "Page supposed to be my rev, but was said stolen !\n" );

    PageInfo myPageInfo = pageInfo;

    Log_PDAQMerge ( "quickMerge : rel=0x%llx, abs=0x%llx, brId=%llx:%llx, alloc=%x, firstFree=%x\n",
        relPagePtr, myPageInfo.absolutePagePtr, _brid(myPageInfo.branchRevId), 
        myPageInfo.allocationProfile, myPageInfo.firstFreeSegmentInPage );

    PageInfo tgtPageInfo = pageInfo;
    
#if 0
    if ( 1 )
      {
        PageInfoPage* pip; __ui64 idx;
        targetAllocator.getPageInfoPage ( relPagePtr, pip, idx, true );
      }
    if ( targetAllocator.getPageInfo ( relPagePtr, tgtPageInfo ) )
      {
        Log_PDAQMerge ( "\t(tgt) rel=0x%llx, abs=0x%llx, brId=%llx:%llx, alloc=%x, firstFree=%x\n",
            relPagePtr, tgtPageInfo.absolutePagePtr, _brid(tgtPageInfo.branchRevId), 
            tgtPageInfo.allocationProfile, tgtPageInfo.firstFreeSegmentInPage );
      }
#endif

    myPageInfo.branchRevId = targetDocumentAllocator.getBranchRevId();

    if ( __isStolen ( myPageInfo.absolutePagePtr ) )
      {
        Warn ( "\tStrange stolen flag !!!\n" );
      }

    alterRevisionPage ();
    getRevisionPage()->ownedPages --;
    getRevisionPage()->ownedTypedPages[PageType_Segment] --;    
    protectRevisionPage ();

    Bug ("NOT IMPLEMENTED !!!");
#if 0
    targetDocumentAllocator.mapMutex.lock();
    targetDocumentAllocator.setPageInfo ( relPagePtr, myPageInfo );
    targetDocumentAllocator.mapMutex.unlock();

    setPageInfo ( relPagePtr, myPageInfo );
#endif
    return true;

  }

};

