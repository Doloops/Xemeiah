#include <Xemeiah/persistence/persistentdocumentallocator.h>
#include <Xemeiah/persistence/persistentbranchmanager.h>
#include <Xemeiah/trace.h>

#include <Xemeiah/auto-inline.hpp>
#include <Xemeiah/persistence/auto-inline.hpp>

namespace Xem
{
  void PersistentDocumentAllocator::reopen ( BranchFlags branchFlags )
  {
    getPersistentStore().getPersistentBranchManager().assertIsUnlocked();
    getPersistentStore().getPersistentBranchManager().assertBranchUnlockedForWrite(getBranchRevId().branchId);

    AssertBug ( revisionPageRef.getPage(), "Null RevsionPage !\n" );
    AssertBug ( getDocumentAllocationHeader().writable == false, "Previous RevisionPage is writable ! commit first !\n" );

    if ( branchFlags & BranchFlags_MustForkBeforeUpdate )
      {
        String newBranchLabel; stringPrintf ( newBranchLabel, "Forked from %s", getBranchName().c_str() );
        BranchFlags inheritedFlags = (branchFlags & BranchFlags_AutoMerge);
        try
          {
            fork ( newBranchLabel, BranchFlags_MayIndexNameIfDuplicate | BranchFlags_AutoForked | inheritedFlags );
          }
        catch ( Exception* e )
          {
            detailException ( e, "Could not fork revision in reopen.\n" );
          }
        return;
      }

    getPersistentStore().getPersistentBranchManager().lockBranchManager();
    getPersistentStore().getPersistentBranchManager().lockBranchForWrite(getBranchRevId().branchId);
    AbsolutePagePtr newRevisionPagePtr = 
        getPersistentStore().getPersistentBranchManager().createWritableRevisionJustAfter ( getBranchRevId() );
    
    getPersistentStore().getPersistentBranchManager().unlockBranchManager();

    if ( newRevisionPagePtr == NullPage )
      {
        getPersistentStore().getPersistentBranchManager().unlockBranchForWrite(getBranchRevId().branchId);
        if ( branchFlags & BranchFlags_CanForkAtReopen )
          {
            String newBranchLabel; 
            stringPrintf ( newBranchLabel, "Forked from %s", getBranchName().c_str() );
            try
              {
                fork ( newBranchLabel, BranchFlags_MayIndexNameIfDuplicate | BranchFlags_AutoForked | BranchFlags_AutoMerge );
              }
            catch ( Exception* e )
              {
                detailException ( e, "Could not fork revision at reopen().\n" );
              }
            return;
          }
        else
          {
            throwException ( Exception,
                "Could not reopen(), because we already had a revision after and we did not have the can-fork-at-reopen flag.\n" );
          }
      }

    AssertBug ( newRevisionPagePtr, "Invalid NULL newRevisionPagePtr.\n" );

    mapMutex.lock();
    AbsolutePageRef<RevisionPage> newRevPageRef = getRevisionPage ( newRevisionPagePtr );
    AssertBug ( newRevPageRef.getPage(), "Could not create revision.\n" );
    AssertBug ( newRevPageRef.getPage()->branchRevId.branchId == revisionPageRef.getPage()->branchRevId.branchId,
        "New revision brid differs : mine=[%llx:%llx], new=[%llx:%llx]\n",
        _brid(revisionPageRef.getPage()->branchRevId),
        _brid(newRevPageRef.getPage()->branchRevId) );
    if ( newRevPageRef.getPage()->branchRevId.revisionId != revisionPageRef.getPage()->branchRevId.revisionId + 1 )
      {
        NotImplemented ( "reopen() : we MUST clear all and remap !\n" );
      }

    Info ( "Reopened doc=%p : rev [%llx:%llx]->[%llx:%llx]\n",
        this, _brid(revisionPageRef.getPage()->branchRevId), _brid(newRevPageRef.getPage()->branchRevId) );

    /*
     * Ok, now switch revision pages 
     */
    // revisionPageRef = AbsolutePageRef<RevisionPage>(getPersistentStore(), newRevisionPagePtr);
    revisionPageRef = newRevPageRef;

    mapMutex.unlock();

    /**
     * Then we flush all caches
     */
    flushInMemCaches ();


    /**
     * Reference documentHead and freeSegmentsHeader for Document class
     */
    // documentHead = &(revisionPage->documentHead);
    // freeSegmentsHeader = &(revisionPage->freeSegmentsHeader);
    // setJournalHead ( &(revisionPage->journalHead) );
    documentAllocationHeader = &(revisionPageRef.getPage()->documentAllocationHeader);
  }

};

