#include <Xemeiah/persistence/persistentbranchmanager.h>
#include <Xemeiah/persistence/persistentdocumentallocator.h>

#include <Xemeiah/auto-inline.hpp>
#include <Xemeiah/persistence/auto-inline.hpp>

#define Log_PDAFork Log

namespace Xem
{
  void PersistentDocumentAllocator::fork ( String& branchName, BranchFlags branchFlags )
  {
    getPersistentStore().getPersistentBranchManager().assertIsUnlocked();

    AssertBug ( ! isWritable(), "Document [%llx:%llx] is set writable !!!\n", _brid(revisionPage->branchRevId) );
    AssertBug ( revisionPage, "No revision page !\n" );

    BranchRevId initialBranchRevId = revisionPage->branchRevId;

    Info ( "Forking revision [%llx:%llx] to branchName=%s, branchFlags=%llx\n",
      _brid(revisionPage->branchRevId), branchName.c_str(), branchFlags );

    /**
     * Flush in-mem caches
     */
    flushInMemCaches ();
    
    /*
     * Create a new revision based on the current one
     */  
    AbsolutePagePtr newRevisionPagePtr = 
        getPersistentStore().getPersistentBranchManager().forkRevision ( revisionPage, branchName.c_str(), branchFlags );
    
    if ( newRevisionPagePtr == NullPage )
      {
        throwException ( Exception, "Could not fork revision [%llx:%llx] to branchName=%s, branchFlags=%llx\n",
          _brid(revisionPage->branchRevId), branchName.c_str(), branchFlags );
      }
    
    /*
     * Update the revisionPage.
     */
    mapMutex.lock();
    revisionPage = getRevisionPage ( newRevisionPagePtr );
    mapMutex.unlock();

    AssertBug ( revisionPage, "Could not get revisionPage !\n" );

    /**
     * Reference documentHead and freeSegmentsHeader for Document class
     */
    documentAllocationHeader = &(revisionPage->documentAllocationHeader);

    Log_PDAFork ( "Forked revision from [%llx:%llx] to [%llx:%llx]\n", _brid(initialBranchRevId), _brid(revisionPage->branchRevId) );
    
  }

};
