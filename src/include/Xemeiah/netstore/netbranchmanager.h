#ifndef __XEM_NETSTORE_NETBRANCHMANAGER_H
#define __XEM_NETSTORE_NETBRANCHMANAGER_H

#include <Xemeiah/kern/branchmanager.h>
#include <Xemeiah/persistence/cachebranchmanager.h>

namespace Xem
{
  class NetStore;
  
  class NetBranchManager : public CacheBranchManager
  {
  protected:
    NetStore& netStore;

  public:
    
    NetBranchManager ( NetStore& _netStore );
  
    ~NetBranchManager();
    
    NetStore& getNetStore() { return netStore; }
  
    virtual BranchId getBranchId ( const String& branchName );
  
    virtual RevisionId lookupRevision ( BranchId branchId );
    
    virtual void renameBranch ( BranchId branchId, const String& branchName ) ;

    virtual BranchId createBranch ( const String& branchName, BranchFlags branchFlags ) ;      

    virtual BranchId createBranch ( const String& branchName, BranchRevId forkedFrom, BranchFlags branchFlags ) ; 

    virtual void deleteBranch ( BranchInfo* branchInfo );

    virtual Document* generateBranchesTree () ;
    virtual String getBranchName ( BranchId branchId ) ;

    // virtual Document* openDocument ( BranchId branchId, RevisionId revisionId, DocumentOpeningFlags flags ) ;

    virtual Document* openDocument ( BranchId branchId, RevisionId revisionId, DocumentOpeningFlags flags, const String& role ) ;

    virtual void scheduleBranchForRemoval ( BranchId branchId ) ;

    virtual BranchRevId getForkedFrom ( BranchId branchId );  
  };
};

#endif // __XEM_NETSTORE_NETBRANCHMANAGER_H

