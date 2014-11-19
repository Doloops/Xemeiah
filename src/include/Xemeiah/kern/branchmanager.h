#ifndef __XEM_KERN_BRANCHMANAGER_H
#define __XEM_KERN_BRANCHMANAGER_H

#include <Xemeiah/kern/format/core_types.h>
#include <Xemeiah/kern/format/document_head.h>
#include <Xemeiah/dom/string.h>
#include <Xemeiah/kern/service.h>

namespace Xem
{
  class XProcessor;
  class Document;

  /**
   * General API to handle branches
   */
  class BranchManager : public Service
  {
  public:
  
  
    virtual ~BranchManager() {}
  
    /**
     * Creates a new branch with no dependencies
     * @param branchName the name of the branch to create, must not exist except if BranchFlags_MayIndexNameIfDuplicate flag is set
     * @param branchFlags the flags of the branch to create
     * @return the BranchId of the new branch, 0 or an exception on problems
     */
    virtual BranchId createBranch ( const String& branchName, BranchFlags branchFlags )
    { 
      BranchRevId noFork = {0, 0};
      return createBranch ( branchName, noFork, branchFlags ); 
    }

    /**
     * Creates a new branch with no dependencies
     * @param branchName the name of the branch to create, must not exist except if BranchFlags_MayIndexNameIfDuplicate flag is set
     * @param branchFlags the flags of the branch to create
     * @return the BranchId of the new branch, 0 or an exception on problems
     */
    virtual BranchId createBranch ( const String& branchName, const String& branchFlags )
    { 
      BranchRevId noFork = {0, 0};
      return createBranch ( branchName, noFork, branchFlags ); 
    }

    /**
     * Create a branch with dependency information
     * @param branchName the name of the branch to create, must not exist except if BranchFlags_MayIndexNameIfDuplicate flag is set
     * @param forkedFrom the BranchRevId of the revision we forked from
     * @param branchFlags the flags of the branch to create
     * @return the BranchId of the new branch, 0 or an exception on problems
     */
    virtual BranchId createBranch ( const String& branchName, BranchRevId forkedFrom, BranchFlags branchFlags ) = 0; 

    /**
     * Create a branch with dependency information
     * @param branchName the name of the branch to create, must not exist except if BranchFlags_MayIndexNameIfDuplicate flag is set
     * @param forkedFrom the BranchRevId of the revision we forked from
     * @param branchFlags the flags of the branch to create
     * @return the BranchId of the new branch, 0 or an exception on problems
     */
    virtual BranchId createBranch ( const String& branchName, BranchRevId forkedFrom, const String& branchFlags );

    /**
     * Rename an existing branch, or throws exceptions upon failures (invalid name, duplicate) (locks branchLock)
     * @param branchId the branchId to rename
     * @param branchName the new name to assign
     */ 
    virtual void renameBranch ( BranchId branchId, const String& branchName ) = 0;

    /**
     * Schedule a branch for removal ; the branch removal will happen when all documents on this branch will be deinstanciated
     * @param branchId the branchId to schedule for removal
     */
    virtual void scheduleBranchForRemoval ( BranchId branchId ) = 0;

    /**
     * Get a BranchId from a branch name (locks branchLock)
     * @param branchName the branch name to lookup (fully defined, including indexation suffix for BranchFlags_MayIndexNameIfDuplicate)
     * @return the BranchId of the found branch, 0 if none was found
     */
    virtual BranchId getBranchId ( const String& branchName ) = 0;
  
    /**
     * Get the branch name from a branch Id (locks branchLock)
     * @param branchId the branch Id to lookup (must not be zero)
     * @return the branch name of the found branch, NULL if none was found
     */
    virtual String getBranchName ( BranchId branchId ) = 0;

    /**
     * Get the revision from which the given branch has been forked
     * @param branchId the branchId to lookup
     * @return the BranchRevId of the revision we forked from
     */
    virtual BranchRevId getForkedFrom ( BranchId branchId ) = 0;

    /**
     * Get the last (read) revisionId for the given branch
     */
    virtual RevisionId lookupRevision ( BranchId branchId ) = 0;

    /**
     * Opens a document with string-based values
     * @param branchName the String-based branchRevId, can be a fully-defined branch name, or a branchId:revisionId format.
     * @param flagsStr the String-based flags for document openning
     * @param role the role associated with the document
     * @return the openned document, will throw an Exception on error (can assume document will never be NULL) 
     */
    virtual Document* openDocument ( const String& branchName, const String& flagsStr, RoleId roleId );
     
    /**
     * Opens a document from a given branchId, with a given RevisionId (locks branchLock)
     * @param branchId the branchId from which the document must be created (must exist)
     * @param revId the revisionId, or 0 to take the last one
     * @param flag Document_Read or Document_Write, Document_Write implies revId=0 or the last revisionId of the branch
     * @return the instanciated document, or NULL if an error occured
     */    
    virtual Document* openDocument ( BranchId branchId, RevisionId revisionId, DocumentOpeningFlags flags );

    /**
     * Opens a document from a given branchId, with a given RevisionId (locks branchLock)
     * @param branchId the branchId from which the document must be created (must exist)
     * @param revId the revisionId, or 0 to take the last one
     * @param flag Document_Read or Document_Write, Document_Write implies revId=0 or the last revisionId of the branch
     * @param roleId the role to set to the document
     * @return the instanciated document, or NULL if an error occured
     */    
    virtual Document* openDocument ( BranchId branchId, RevisionId revisionId, DocumentOpeningFlags flags, RoleId roleId ) = 0;

    /**
     * Generate a (volatile) document containing all branches and revisions information (locks branchLock)
     * no reference is kept in PersistentStore, beware of garbage collection
     * @return a Document containing all we need about branches, in the xemp namespace format
     */
    virtual Document* generateBranchesTree ( XProcessor& xproc ) = 0;

    /**
     * Default document releasing scheme
     */
    virtual void releaseDocument ( Document* doc ) = 0;

    /**
     * Requalify mapping of a document
     */
    virtual void updateDocumentReference ( Document* doc, BranchRevId oldBrId, BranchRevId newBrId ) = 0;

    /**
     * Parses a string formed by BranchId:RevisionId (example : '1:1' format)
     * @param branchRevStr the string version of the branch rev
     * @return the parsed BranchRevId, or a 0:0 branch if parse failed.
     */
    static BranchRevId parseBranchRevId ( const char* branchRevStr );
    
    /**
     * Parses a string-format flag
     */
    static DocumentOpeningFlags parseDocumentOpeningFlags ( const String& flagsStr );
    
    /**
     *
     */
    static const char* getDocumentOpeningFlagsName ( DocumentOpeningFlags flags );

    /**
     * Lock branch manager
     */
    virtual void lockBranchManager () {}

    /**
     * Unlock branch manager
     */
    virtual void unlockBranchManager () {}
  };

};

#endif // __XEM_KERN_BRANCHMANAGER_H

