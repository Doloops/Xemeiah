#ifndef __XEM_KERN_VOLATILESTORE_H
#define __XEM_KERN_VOLATILESTORE_H

#include <Xemeiah/kern/store.h>
#include <Xemeiah/kern/branchmanager.h>

namespace Xem
{

  /**
   * Volatile Store : all in-memory !
   */
  class VolatileStore : public Store
  {
  protected:
    /**
     * This is a fake one
     */
    class VolatileBranchManager : public BranchManager
    {
    protected:

    public:
      VolatileBranchManager() {}
      ~VolatileBranchManager() {}

      virtual BranchId createBranch ( const String& branchName, BranchRevId forkedFrom, BranchFlags branchFlags )
      { return 0; }
      virtual void renameBranch ( BranchId branchId, const String& branchName ) {}
      virtual void scheduleBranchForRemoval ( BranchId branchId ) {}
      virtual BranchId getBranchId ( const String& branchName ) { return 0; }
      virtual BranchRevId getForkedFrom ( BranchId branchId ) { BranchRevId brid = {0, 0}; return brid; }
      virtual String getBranchName ( BranchId branchId ) { return ""; }
      virtual RevisionId lookupRevision ( BranchId branchId ) { return 0; }
      virtual Document* openDocument ( BranchId branchId, RevisionId revisionId, DocumentOpeningFlags flags, RoleId roleId ) { return NULL; }
      virtual Document* generateBranchesTree ( XProcessor& xproc ) { return NULL; }
      virtual void releaseDocument ( Document* doc ) {}
      virtual void updateDocumentReference ( Document* doc, BranchRevId oldBrId, BranchRevId newBrId ) {}

      virtual void start () {}
      virtual void stop () {}
    };

    /**
     * Holds a reference to my lie
     */
    VolatileBranchManager volatileBranchManager;
  public:
    /**
     * Single Constructor
     */
    VolatileStore ();
    
    /**
     * Single Destructor
     */
    ~VolatileStore ();
    
    /**
     * Access to Branch Manager (not implemented here)
     */  
    virtual BranchManager& getBranchManager();

    /**
     * Key persistence : add a key in the store.
     * @param keyName the local part of the key
     * @return a new LocalKeyId corresponding to the key
     */
    virtual LocalKeyId addKeyInStore ( const char* keyName );

    /**
     * Namespace persistence : add a namespace to the store.
     * @param namespaceURL the URL of the namespace
     * @return a new NamespaceId for it.
     */
    virtual NamespaceId addNamespaceInStore ( const char* namespaceURL );

    /**
     * Element Indexing
     * Free elementIds are reserved per-revision to reduce access to SuperBlock
     */ 
    virtual bool reserveElementIds ( ElementId& nextId, ElementId& lastId );

  };
};


#endif //  __XEM_KERN_VOLATILESTORE_H
