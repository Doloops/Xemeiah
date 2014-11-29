#ifndef __XEM_PERSISTENCE_PERSISTENTDOCUMENT_H
#define __XEM_PERSISTENCE_PERSISTENTDOCUMENT_H

#include <Xemeiah/kern/journaleddocument.h>
#include <Xemeiah/kern/branchmanager.h>
#include <Xemeiah/persistence/persistentstore.h>
#include <Xemeiah/persistence/persistentdocumentallocator.h>

#include <Xemeiah/xprocessor/xprocessor.h>

// #define __XEM_PERSISTENTDOCUMENT_HAS_WRITABLEPAGECACHE //< Option : writablePageCache

namespace Xem
{

    class EventHandlerDom;

    /**
     * Persistent Documents
     *
     * -- Transactional aspects of Document --
     *
     * Each document represents a specific revision of a branch.
     * At a given time, no writable revision can have more than one document bound to it.
     * So documents can be commited or dropped, with no incidence on other documents.
     *
     * -- Page Indirection Mecanism & Indirection hypotheses --
     *
     * - Indirections are linear : relative pages are indexed linearly.
     * - Each relative page *must* have a corresponding absolute page.
     * - There is no hole in relative page mappings.
     * - revPage->indirection level and firstPage are modified when
     *   revPage->nextRelativePagePtr exceeds the area covered by the
     *   corresponding level.
     *
     */
    class PersistentDocument : public JournaledDocument
    {
        friend class PersistentStore;
        friend class CacheBranchManager;
        friend class PersistentBranchManager;
    protected:
        /**
         * Our instanciation flags
         */
        DocumentOpeningFlags documentOpeningFlags;

        /**
         * Get the allocator as a PersistentDocumentAllocator
         */
        PersistentDocumentAllocator&
        getPersistentDocumentAllocator () const
        {
            return dynamic_cast<PersistentDocumentAllocator&>(getDocumentAllocator());
        }

        /**
         * Bind the document allocator for garbage collection
         */
        void
        bindDocumentAllocator (PersistentDocumentAllocator* allocator)
        {
            Document::bindDocumentAllocator(allocator);
        }

        /**
         * Get my PersistentStore
         */
        PersistentStore&
        getPersistentStore () const
        {
            return getPersistentDocumentAllocator().getPersistentStore();
        }

        /**
         * Protected constructor : Persistent documents can only be created from PersistentStore
         */
        PersistentDocument (Store& store, PersistentDocumentAllocator& persistentDocumentAllocator,
                            DocumentOpeningFlags flags);

        /**
         * Protected destructor : only PersistentStore can delete myself !
         */
        ~PersistentDocument ();

        /**
         * Get the journalHead
         */
        JournalHead&
        getJournalHead ();

        /**
         * Alter the journal head
         */
        void
        alterJournalHead ()
        {
            getPersistentDocumentAllocator().alterRevisionPage();
        }
        void
        protectJournalHead ()
        {
            getPersistentDocumentAllocator().protectRevisionPage();
        }

        /**
         * (debug) determines if the PersistentStore asked us for deletion
         */
        bool acknowledgedPersistentDocumentDeletion;

        /**
         * Scheduled revision removal (at the PersistentDocument destruction)
         */
        bool scheduledRevisionRemoval;

        /**
         * Scheduled PersistentDocument release (de-instanciation, that is), triggered in mayRelease ();
         */
        bool scheduledDocumentRelease;

        /**
         * Document Elements must be indexed or not ?
         * @return must index Elements or not
         */
        bool
        mayIndex ();

        /**
         * DoInitialize
         */
        void
        doInitialize ();

        /**
         * drop all pages
         */
        void
        dropAllPages ();

        /**
         * release document resources
         */
        virtual void
        releaseDocumentResources ();

        virtual bool
        isJournalEnabled ()
        {
            return false;
        }
    public:
        /**
         * Get our DocumentCreationFlags
         */
        DocumentOpeningFlags
        getDocumentOpeningFlags ()
        {
            return documentOpeningFlags;
        }

        /**
         * Get the current branchRevId
         */
        BranchRevId
        getBranchRevId ()
        {
            return getPersistentDocumentAllocator().getBranchRevId();
        }

        /**
         * Get the current branch's flags
         */
        BranchFlags
        getBranchFlags ()
        {
            return getPersistentDocumentAllocator().getBranchFlags();
        }

        /**
         * Get the current branch name
         */
        String
        getBranchName ()
        {
            return getPersistentDocumentAllocator().getBranchName();
        }

        /**
         * Commits the current Document (and the revision behind).
         * The Document must be openned Write, must be locked Write, and may not be volatile.
         * After the commit, the Document remains open Read-only, and is unlocked Write.
         */
        void
        commit (XProcessor& xproc);

        /**
         * This will be deprecated sooner or later
         */
        void
        commit ();

        /**
         * Checks if the currrent Document worths commiting
         */
        bool
        mayCommit ();

        /**
         * Re-opens a previously commited Document.
         * This may fail for obvious reasons, such as a revison has been created
         * inbetween.
         */
        void
        reopen ();

        /**
         * Grant a file write
         * If the file was writable, then this will lockWrite this doc
         * Otherwise, it will try and find a way to make it writable
         */
        void
        grantWrite ();

        /**
         * Forks the current revision to a new branch, keeping all current mappings intact (conservative) ; but the revision *MUST NOT* be writable
         * @param branchName the new branch name
         * @param branchFlags the new branch's flag
         * @return true upon success, false otherwise
         */
        void
        fork (const String& branchName, BranchFlags branchFlags);

        /**
         * Merge current document to the forked branch
         */
        void
        merge (XProcessor& xproc, bool keepBranch);

        /**
         * drops document
         */
        void
        drop ();

        /**
         * Locks the document for writing (automatic on openDocument("write") or reopen()
         */
        virtual void
        lockWrite ();

        /**
         * Unlocks the document for writing
         */
        virtual void
        unlockWrite ();

        /**
         * Checks if a document is writable
         */
        virtual bool
        isLockedWrite ();

        /**
         * Must return true if we need to lock BranchManager to delete this document (optimization hack)
         * Yes, we need to.
         */
        virtual bool
        deletionImpliesLockBranchManager ()
        {
            return true;
        }

        /**
         * Schedules the whole branch for dropping : effective drop will happen in PersistentDocumentMap
         * when no other document exists for this branch.
         */
        void
        scheduleBranchForRemoval ();

        /**
         * Schedules the revision for dropping : effective drop will happen at document deletion
         * so the document contents are useable after (although not modifiable)
         */
        virtual void
        scheduleRevisionForRemoval ();

        /**
         * Returns true if the revision has been scheduled for removal
         */
        virtual bool
        isScheduledRevisionRemoval ();

        /**
         * Schedule release : set scheduledDocumentRelease to true, used in mayRelease ()
         */
        void
        scheduleDocumentRelease ();

#if 0
        /**
         * Releases the current PersistentDocument, ie decrements refcount if any and schedule for deletion
         */
        void release ();

        /**
         * Only if we have been told to release, call release(), do nothing otherwise
         * @return true if we are about to release, false if we dont need to
         */
        bool mayRelease ();
#endif

        /**
         * Check my contents
         */
        virtual void
        checkContents (AllocationStats& stats)
        {
            getPersistentDocumentAllocator().checkContents(stats);
        }

        /**
         * Do some cleanup.
         */
        void
        housewife ();

    };
}

#endif // __XEM_PERSISTENCE_PERSISTENTDOCUMENT_H
