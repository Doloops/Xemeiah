#include <Xemeiah/persistence/persistentbranchmanager.h>
#include <Xemeiah/persistence/persistentstore.h>
#include <Xemeiah/persistence/persistentdocument.h>

#include <Xemeiah/auto-inline.hpp>
#include <Xemeiah/persistence/auto-inline.hpp>

namespace Xem
{
  PersistentBranchManager::PersistentBranchInfo::PersistentBranchInfo ( PersistentBranchManager& _persistentBranchManager, 
      BranchPagePtr _branchPagePtr, BranchPage* _branchPage )
  : BranchInfo ( _persistentBranchManager, _branchPage->branchId, _branchPage->forkedFrom ), branchPageRef(_persistentBranchManager.getPersistentStore(), _branchPagePtr)
  {
  }
  
  const char* PersistentBranchManager::PersistentBranchInfo::getName()
  {
    return branchPageRef.getPage()->name;
  }

  RevisionId PersistentBranchManager::PersistentBranchInfo::getLastRevisionId ()
  {
    // BranchPage* branchPage = getPersistentBranchManager().getPersistentStore().getAbsolutePage()
    AbsolutePageRef<RevisionPage> revPageRef = getPersistentBranchManager().getPersistentStore().getAbsolutePage<RevisionPage> (branchPageRef.getPage()->lastRevisionPage);
    return revPageRef.getPage()->branchRevId.revisionId;
  }
};
