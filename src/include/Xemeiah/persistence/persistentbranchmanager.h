#ifndef __XEM_PERSISTENCE_PERSISTENTBRANCHMANAGER_H
#define __XEM_PERSISTENCE_PERSISTENTBRANCHMANAGER_H

#include <Xemeiah/kern/branchmanager.h>

#include <Xemeiah/persistence/cachebranchmanager.h>
#include <Xemeiah/kern/hashbucket.h>
#include <Xemeiah/persistence/persistentstore.h>
#include <Xemeiah/persistence/persistentdocument.h>
#include <list>
#include <map>

namespace Xem
{
  /**
   * PersistentBranchManager is responsible for handling Branch operations, with in-mem cache of branch names, mappings, locking, ...
   * All public functions lock the branchLock
   */
  class PersistentBranchManager : public CacheBranchManager
  { 
    friend class PersistentDocumentAllocator;
    friend class PersistentDocument;
    friend class PersistentStore;
  protected:
    /**
     * Reference to the PersistentStore which instanciated us
     */
    PersistentStore& persistentStore;

    /**
     * The fast branch lookup mechanism uses kern HashBucket to associate branch names to branchIds
     */
    typedef HashBucketTemplate<BranchId> BranchLookup;

    /**
     * The branch name lookup
     */
    BranchLookup branchLookup;
    
    /**
     * Load all branch metadata from store file
     */
    void buildBranchInfos ();

    /**
     * Per-branch information
     */
    class PersistentBranchInfo : public BranchInfo
    {
    protected:
      /**
       * The BranchPage ptr
       */
      BranchPagePtr branchPagePtr;
      
      /**
       * The branchPage
       */
      BranchPage* branchPage;
    public:
      PersistentBranchInfo ( PersistentBranchManager& _persistentBranchManager, BranchPagePtr _branchPagePtr, BranchPage* _branchPage );
    
      BranchPage* getBranchPage () { return branchPage; }
      BranchPagePtr getBranchPagePtr () { return branchPagePtr; }

      virtual const char* getName();

      virtual RevisionId getLastRevisionId ();

      PersistentBranchManager& getPersistentBranchManager()
      { return dynamic_cast<PersistentBranchManager&> ( cacheBranchManager ); }
    };
    
    /**
     * Reference a branch into the BranchInfo stuff
     */
    virtual BranchInfo* referenceBranch ( BranchId branchId, const BranchRevId& forkedFrom );
    
    /**
     * Reference a branch into the BranchInfo stuff
     * @param branchPage the branchPage to reference
     */
    virtual BranchInfo* referenceBranch ( BranchPagePtr branchPagePtr, BranchPage* branchPage );
       
    /**
     * Lookup a revision for a given branch
     */
    RevisionPagePtr lookupRevision ( BranchPage* branchPage, RevisionId& revisionId );

    /**
     * Delete a branch represented by the provided BranchInfo, which will also be deleted by the operation
     */
    void deleteBranch ( BranchInfo* branchInfo );

    /**
     * Get the branchPage
     * @param branchId the branchId to lookup
     * @return the corresponding BranchPage
     */
    BranchPage* getBranchPage ( BranchId branchId );

    /**
     * Check if a branch has to be removed
     */
    virtual void checkBranchRemoval ( BranchInfo* branchInfo );

    /**
     * Format a newly created RevisionPage for default variables ;
     * One shall call formatHeadRevisionPage() or inheritRevisionPage() after this format.
     */
    void formatRevisionPage ( AbsolutePagePtr revPagePtr, RevisionPage* revPage, BranchRevId brId );

    /**
     * Format a RevisionPage with standard values (must be the first revision of a given branch)
     */
    void formatHeadRevisionPage ( RevisionPage* revPage );

    /**
     * Inherit RevisionPage values from an already existing revision
     * @param target the target revision page to copy to (must be writable)
     * @param source the source revision to inherit from
     */
    void inheritRevisionPage ( RevisionPage* target, const RevisionPage* source );
    
    /**
     * Create a new (writable) revision at the end of a branch, asserting that no writable revision exists for this branch
     * @param branchPage the branch page to create revision for
     * @param forceNoInherit do not inherit previous revision data
     * @return the pointer the new revision page
     */
    RevisionPagePtr createRevision ( BranchPage* branchPage, bool forceNoInherit=false );
    
    /**
     * Create a new (writable) revision at the end of the branch if possible
     * @param brId the branchRevId to create a new revision after
     * @return the pointer to the new revision page, or NullPagePtr otherwise
     */
    RevisionPagePtr createWritableRevisionJustAfter ( BranchRevId brId );
    
    /**
     * Revision forker
     */
    RevisionPagePtr forkRevision ( const RevisionPage* sourceRevision, const char* branchName, BranchFlags branchFlags  );

    /**
     * Drop certain revisions for a given branch
     * Drop certain revisions, provide fromRev=0 and toRev=0 for all revisions (but not the branch, of course).
     */
    void dropRevisions ( BranchPage* branchPage, RevisionId fromRev, RevisionId toRev );

    /**
     * Document instanciator for a given (existing) Revision, this will NOT add it in the BranchManager cache
     * @param revisionPagePtr the effective absolute page pointer of the revision
     * @param flag the instanciation flags
     * @return the instanciated document (may not fail)
     */
    PersistentDocument* instanciatePersistentDocument ( RevisionPagePtr revisionPagePtr, DocumentOpeningFlags flags,
        PersistentDocumentAllocator* allocator = NULL );

    /**
     * Document instanciator with initial params setting
     */
    PersistentDocument* instanciatePersistentDocument ( BranchInfo& branchInfo, 
          RevisionPagePtr revisionPagePtr, DocumentOpeningFlags flags, 
          RoleId roleId, PersistentDocumentAllocator* allocator = NULL);

    /**
     * Document instanciation, refer to BranchManager for further details 
     */
    RevisionPagePtr lookupRevision ( PersistentBranchInfo& branchInfo, RevisionId& revisionId, DocumentOpeningFlags flags );

  public:
    /**
     * Branch Manager Constructor
     */
    PersistentBranchManager ( PersistentStore& _persistentStore );
    
    /**
     * Branch Manager Destructor
     */
    ~PersistentBranchManager ();

    /**
     * Const Reference to our hosting PersistentStore
     */
    PersistentStore& getPersistentStore() const { return persistentStore; }

    /**
     * Reference to our hosting PersistentStore
     */
    PersistentStore& getPersistentStore() { return persistentStore; }

    /**
     * Reference to our hosting Store
     */
    Store& getStore() { return getPersistentStore(); }

    /**
     * const reference to our hosting Store
     */
    Store& getStore() const { return getPersistentStore(); }

    /**
     * Branch Creation, locks BranchManager
     */
    BranchId createBranch ( const String& branchName, BranchRevId forkedFrom, BranchFlags branchFlags );

    /**
     * Branch Creation with BranchManager mutex hold
     */
    BranchId doCreateBranch ( const String& branchName, BranchRevId forkedFrom, BranchFlags branchFlags );

    /**
     * Branch renaming
     */
    void renameBranch ( BranchId branchId, const String& branchName );

    /**
     * Branch deletion scheduling
     */
    void scheduleBranchForRemoval ( BranchId branchId );

    /*
     * Branch Lookup
     */

    BranchId getBranchId ( const String& branchName );
    String getBranchName ( BranchId branchId );
    BranchRevId getForkedFrom ( BranchId branchId );
    RevisionId lookupRevision ( BranchId branchId );
    
    /*
     * Branch options
     */
    BranchFlags getBranchFlags ( BranchId branchId )
    { return getBranchPage(branchId)->branchFlags; }
    
    
    /**
     * Document instanciation, refer to BranchManager for further details 
     */
    Document* openDocument ( BranchId branchId, RevisionId revisionId, DocumentOpeningFlags flags, RoleId roleId );

    /**
     * Get the Branches Tree
     */
    Document* generateBranchesTree ( XProcessor& xproc );

    /**
     * Document temporary instanciator, not included in BranchManager document reference, and forced read (check purposes)
     * @param revisionPagePtr the effective absolute page pointer of the revision
     * @return the instanciated document (may not fail)
     */
    PersistentDocument* instanciateTemporaryPersistentDocument ( RevisionPagePtr revisionPagePtr );

  };
};

#endif // __XEM_PERSISTENCE_PERSISTENTBRANCHMANAGER_H
