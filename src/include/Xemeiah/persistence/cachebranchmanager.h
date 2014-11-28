#ifndef __XEM_PERSISTENCE_CACHEBRANCHMANAGER_H
#define __XEM_PERSISTENCE_CACHEBRANCHMANAGER_H

#include <Xemeiah/kern/branchmanager.h>
#include <Xemeiah/kern/mutex.h>

#include <map>

#include <pthread.h>

// #define __XEM_PERSISTENCE_CACHEBRANCHMANAGER_REUSABLE_DOCUMENTS_LIST //< Option : set a list of reusable documents (whereas singleton)

namespace Xem
{
  /**
   * CacheBranchManager handles cached information about branches in the persistence store
   * It also handles locking scenarios with multiple access to the same branch
   * Locking dependency is as follows :
   * branchInfo->lockWrite() implies CacheBranchManager locked
   * branchInfo->unlockWrite() implies CacheBranchManager unlocked
   */
  class CacheBranchManager : public BranchManager
  {
  protected:
    /**
     * The branchManagerMutex, hold by any public functions
     */
    Mutex branchManagerMutex;

    /**
     * Thread lock
     */
    void lockBranchManager ();
    
    /**
     * Thread unlock
     */
    void unlockBranchManager ();

    /**
     * check that the branchManager is locked
     */
    bool isLocked ();

    /**
     * check that the branchManager is locked
     */
    void assertIsLocked ();

    /**
     * check that the branchManager is locked
     */
    void assertIsUnlocked ();

    /**
     * The in-mem BranchInfo holds references to the Documents
     */
    class BranchInfo
    {
    public:
      /**
       * Type : List of branchInfos which forked from this branch
       */
      typedef std::list<BranchInfo*> DependantBranchInfos;

    protected:
      /**
       * Reference to the CacheBranchManager which instanciated us
       */
      CacheBranchManager& cacheBranchManager;

      /**
       * The branchLock Mutex, hold by any public functions
       */
      Mutex branchWriteMutex;

      /**
       * Reference accessor to the CacheBranchManager
       */
      CacheBranchManager& getCacheBranchManager() const { return cacheBranchManager; }
      /**
       * Our very own BranchId
       */
      BranchId branchId;
  
      /**
       * Forked From
       */
      BranchRevId forkedFrom;
      
      /**
       * Reference to the Branch which we depend on
       */
      BranchInfo* dependingBranchInfo;
     
      /**
       * List of branchInfos which forked from this branch
       */
      DependantBranchInfos dependantBranchInfos;

      /**
       * Is the revision scheduled for removal
       */
      bool scheduledForRemoval;
      
      /**
       * List of documents which are instanciated on this branch
       */
      typedef std::list<Document*> InstanciatedDocuments;

      /**
       * List of documents instance
       */
      InstanciatedDocuments instanciatedDocuments;
         

#ifdef __XEM_PERSISTENCE_CACHEBRANCHMANAGER_REUSABLE_DOCUMENTS_LIST
      /**
       * Type : Documents we can reuse for a given Revision
       */      
      typedef std::map<RevisionId,Document*> ReusableDocuments;

      /**
       * Documents we can reuse for a given Revision
       */
      ReusableDocuments reusableDocuments;
#else
      /**
       * Set the revisionId for the reusable document
       */
      // RevisionId reusableDocumentRevisionId;

      /**
       * Our current reusable document
       */
      Document* reusableDocument;
#endif // __XEM_PERSISTENCE_CACHEBRANCHMANAGER_REUSABLE_DOCUMENTS_LIST      
    public:
      /**
       * Main constructor
       */
      BranchInfo ( CacheBranchManager& _cacheBranchManager, BranchId branchId, const BranchRevId& forkedFrom );
      
      /**
       * Destructor
       */
      virtual ~BranchInfo ();
    
      /**
       * Fetch name
       */
      virtual const char* getName ();

      /**
       * Fetch branchId
       */
      virtual BranchId getBranchId ();
      
      /**
       * Fetch the BranchRevId we forked from
       */
      virtual BranchRevId getForkedFrom ();
      
      /**
       * Reference a document in this branch
       */
      void referenceDocument ( Document* doc );
      
      /**
       * Dereference a document in this branch
       */
      void dereferenceDocument ( Document* doc );
      
      /**
       * Return number of instanciated documents in the given branch
       */
      __ui64 getInstanciatedDocumentsNumber ();
      
      /**
       * Returns the first instanciated document
       */
      Document* getInstanciatedDocument(RevisionId,DocumentOpeningFlags);

      /**
       * Check if the branch has instanciated documents
       * @return true if branch and its forks are free from any document
       */
      bool isFreeFromDocuments ();
      
      /**
       * Check whether we can safely consider this branch as free from any instantiated documents
       *
       * @throw Exception upon error
       */
      void checkIsFreeFromDocuments();

      /**
       * Set the scheduled for removal bit
       */
      void setScheduledForRemoval ();
      
      /**
       * Get the scheduled for removal bit
       */
      bool isScheduledForRemoval () const { return scheduledForRemoval; }
      
      /**
       * Get the list of dependant branchInfos
       */
      DependantBranchInfos& getDependantBranchInfos() { return dependantBranchInfos; }
      
      /**
       * Get the depending branchInfo
       */
      BranchInfo* getDependingBranchInfo() { return dependingBranchInfo; }

      /**
       * Set the depending branchInfo
       */      
      void setDependingBranchInfo ( BranchInfo* _dependingBranchInfo ) { dependingBranchInfo = _dependingBranchInfo; }
      
      /**
       * Get a reusable document from a given revisionId
       * @param revisionId the revision to lookup
       * @param roleId the document in cache shall have the same role 
       * @param flags the document in cache shall have the same openning flags
       */
      Document* getReusableDocument ( RevisionId revisionId, RoleId roleId, DocumentOpeningFlags flags );
      
      /**
       * Set a document reusable for a given revisionId
       * @param doc the document to try to set reusable
       * @return true if the document was set reusable, false otherwise (and the doc shall be deleted)
       */
      bool setReusableDocument ( Document* doc );
      
      /**
       * Cleanup a reusable document
       */
      void cleanupReusableDocument ();

      /**
       * LockWrite this branch
       */
      void lockWrite ();
      
      /**
       * UnlockWrite this branch
       */
      void unlockWrite ();
      
      /**
       * Check that this branch is locked write (by myself, obviously)
       */
      bool isLockedWrite ();
      
      /**
       * check if there is an writable instanciated document
       */
      bool hasInstanciatedWritableDocument ();

      /**
       * Get last Revision for this branch
       */
      virtual RevisionId getLastRevisionId () { Bug ( "." ); return 0; }
    };
  
    /**
     * The effective branchId to BranchInfo mapping
     */
    typedef std::map<BranchId, BranchInfo*> BranchMap;

    /**
     * The branch map
     */
    BranchMap branchMap;
  
    /**
     * Default Constructor
     */
    CacheBranchManager ();

    /**
     * Add a BranchInfo
     */
    virtual BranchInfo* referenceBranch ( BranchId branchId, const BranchRevId& forkedFrom );    

    /**
     * Mark branch dependency, ie the fact that a branch has been forked from another revision
     */
    virtual void markBranchDependency ( BranchInfo* branchInfo );
    
    /**
     * Check whether the branch is to be deleted or not ; this may clear the branchInfo if the branch has to be deleted
     */
    virtual void checkBranchRemoval ( BranchInfo* branchInfo );

    /** 
     * Release a document, deleting or recycling it for future uses
     */
    virtual void releaseDocument ( Document* doc );

#if 0
    /**
     * Deletes a given document
     */
    virtual void deleteDocument ( Document* doc );
#endif
    
    /**
     * 
     */
    virtual void deleteBranch ( BranchInfo* branchInfo ) = 0;
    
    /**
     * Try to use the document cache to fetch documents
     */
    virtual Document* getDocumentFromCache ( BranchInfo* branchInfo, RevisionId revisionId, DocumentOpeningFlags flags, RoleId roleId );

    /**
     * Try to use the document cache to fetch documents
     */
    virtual Document* getDocumentFromCache ( BranchId branchId, RevisionId revisionId, DocumentOpeningFlags flags, RoleId roleId );

    /**
     * Cache Branch Manager garbage collector thread
     */
    void garbageCollectorThread ();
  public:
    virtual ~CacheBranchManager ();

    void checkIsFreeFromDocuments ();

    virtual Store& getStore() = 0;
    virtual Store& getStore() const = 0;

    virtual void updateDocumentReference ( Document* doc, BranchRevId oldBrId, BranchRevId newBrId );    
    
    virtual void lockBranchForWrite ( BranchId branchId );

    virtual void assertBranchLockedForWrite ( BranchId branchId );
    
    virtual void assertBranchUnlockedForWrite ( BranchId branchId );

    virtual bool isBranchLockedForWrite ( BranchId branchId );

    virtual void unlockBranchForWrite ( BranchId branchId );

    /**
     * Service : start implementation
     */
    virtual void start ();

    /**
     * Service : stop implementation
     */
    virtual void stop ();
  };


};

#endif // __XEM_PERSISTENCE_CACHEBRANCHMANGER_H
