#include <Xemeiah/persistence/persistentbranchmanager.h>
#include <Xemeiah/persistence/persistentstore.h>
#include <Xemeiah/persistence/persistentdocument.h>

#include <Xemeiah/auto-inline.hpp>
#include <Xemeiah/persistence/auto-inline.hpp>

namespace Xem
{
  PersistentBranchManager::PersistentBranchInfo::PersistentBranchInfo ( PersistentBranchManager& _persistentBranchManager, 
      BranchPagePtr _branchPagePtr, BranchPage* _branchPage )
  : BranchInfo ( _persistentBranchManager, _branchPage->branchId, _branchPage->forkedFrom )
  {
    branchPage = _branchPage;
    branchPagePtr = _branchPagePtr;
  
  }
  
  const char* PersistentBranchManager::PersistentBranchInfo::getName()
  {
    return branchPage->name;
  }

  RevisionId PersistentBranchManager::PersistentBranchInfo::getLastRevisionId ()
  {
    // BranchPage* branchPage = getPersistentBranchManager().getPersistentStore().getAbsolutePage()
    RevisionPage* revPage = getPersistentBranchManager().getPersistentStore().getAbsolutePage<RevisionPage> (branchPage->lastRevisionPage);
    return revPage->branchRevId.revisionId;
  }
};

