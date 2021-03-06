#include <Xemeiah/persistence/persistentstore.h>
#include <Xemeiah/persistence/persistentdocument.h>
#include <Xemeiah/persistence/persistentbranchmanager.h>
#include <Xemeiah/trace.h>

#include <sys/timeb.h>
#include <list>
#include <unistd.h>
#include <time.h>

#include <Xemeiah/auto-inline.hpp>
#include <Xemeiah/persistence/auto-inline.hpp>

#define Log_CreateRevision Debug
#define Log_DropRevision Log

#define __XEM_PERS_DEFAULT_ALLOCATION_PROFILES 16

namespace Xem
{
    void
    PersistentBranchManager::inheritRevisionPage (RevisionPage* target, const RevisionPage* source)
    {
#define __copy(_e) target->_e = source->_e;
#if 1 // Normal behavior : inherit nbFreeListHeaders and freeListHeaders
        __copy(documentAllocationHeader.nbAllocationProfiles);
        for (AllocationProfile p = 0; p < source->documentAllocationHeader.nbAllocationProfiles; p++)
        {
            memcpy(&(target->documentAllocationHeader.freeSegmentsLevelHeaders[p]),
                   &(source->documentAllocationHeader.freeSegmentsLevelHeaders[p]), sizeof(FreeSegmentsLevelHeader));

        }
#else // Do not inherit nbFreeListHeaders and freeListHeaders (test only)
        target->nbFreeListHeaders = 16;
        for ( AllocationProfile p = 0; p < target->nbFreeListHeaders; p++ )
        memset ( &(target->freeListHeaders[p]), 0, sizeof ( FreeListHeader ) );

#endif

        Log_CreateRevision ( "Got %x defined Allocation Profiles ! (source had %x)\n",
                target->documentAllocationHeader.nbAllocationProfiles,
                source->documentAllocationHeader.nbAllocationProfiles );

        __copy(journalHead);
        __copy(indirection);
        if (target->indirection.firstPage)
        {
            target->indirection.firstPage |= PageFlags_Stolen;
            Log_CreateRevision ( "I've stolen the first Indirection Page !"
                    " src=%llx, tgt=%llx, bit=%llx\n",
                    source->indirection.firstPage,
                    target->indirection.firstPage,
                    PageFlags_Stolen );
        }
        else
        {
            Bug("No indirection page ???\n");
        }
        __copy(documentAllocationHeader.nextRelativePagePtr);
        __copy(documentHeadPtr);
#undef __copy  
    }

    void
    PersistentBranchManager::formatHeadRevisionPage (RevisionPage* revPage)
    {
        revPage->documentAllocationHeader.nbAllocationProfiles = __XEM_PERS_DEFAULT_ALLOCATION_PROFILES;
        for (AllocationProfile p = 0; p < revPage->documentAllocationHeader.nbAllocationProfiles; p++)
            memset(&(revPage->documentAllocationHeader.freeSegmentsLevelHeaders[p]), 0,
                   sizeof(FreeSegmentsLevelHeader));
        /*
         * indirectionPage is alloced at preliminary state of revision creation,
         * so do not use getFreePage<> ( revPage );
         */

        revPage->indirection.firstPage = NullPage;
        revPage->indirection.level = 1; // 1
        revPage->documentHeadPtr = NullPtr;

        memset(&(revPage->journalHead), 0, sizeof(JournalHead));

        AbsolutePagePtr pageInfoPagePtr = getPersistentStore().getFreePagePtr();
        AbsolutePageRef<PageInfoPage> pageInfoPageRef = getPersistentStore().getAbsolutePage<PageInfoPage>(
                pageInfoPagePtr);

        getPersistentStore().alterPage(pageInfoPageRef.getPage());
        memset(pageInfoPageRef.getPage(), 0, PageSize);
        getPersistentStore().protectPage(pageInfoPageRef.getPage());

        revPage->indirection.firstPage = pageInfoPagePtr | PageType_PageInfo;
        revPage->ownedPages++;
        revPage->ownedTypedPages[PageType_PageInfo]++;
    }

    void
    PersistentBranchManager::formatRevisionPage (AbsolutePagePtr revPagePtr, RevisionPage* revPage, BranchRevId brId)
    {
        memset(revPage, 0, PageSize);
        revPage->branchRevId = brId;
        revPage->revisionAbsolutePagePtr = revPagePtr;
        revPage->lastRevisionPage = NullPage;
        revPage->freePageList = NullPage;
        revPage->ownedPages = 1; // The first ownedPage is myself ( the RevisionPage)

        memset(revPage->ownedTypedPages, 0, sizeof(revPage->ownedTypedPages));
        revPage->ownedTypedPages[PageType_Revision]++;
        revPage->documentAllocationHeader.writable = true;

        revPage->creationTime = time(NULL);
        revPage->commitTime = 0;
    }

    RevisionPagePtr
    PersistentBranchManager::createWritableRevisionJustAfter (BranchRevId brId)
    {
        assertIsLocked();
        assertBranchLockedForWrite(brId.branchId);

        BranchPage* branchPage = getBranchPage(brId.branchId);
        AssertBug(branchPage, "Could not find branch page !\n");

        AssertBug(branchPage->lastRevisionPage, "Invalid null branch ???\n");

        AbsolutePageRef<RevisionPage> lastRevPageRef = getPersistentStore().getAbsolutePage<RevisionPage>(
                branchPage->lastRevisionPage);

        if (lastRevPageRef.getPage()->branchRevId.revisionId == brId.revisionId)
        {
            /**
             * We have to lock the branch, as we will modifiy it !
             */
            RevisionPagePtr revPagePtr = createRevision(branchPage);
            return revPagePtr;
        }

        Warn("Could not createWritableRevisionJustAfter([%llx:%llx]), lastRevPage is [%llx:%llx] \n", _brid(brId),
             _brid(lastRevPageRef.getPage()->branchRevId));
        return NullPage;
    }

    RevisionPagePtr
    PersistentBranchManager::createRevision (BranchPage* branchPage, bool forceNoInherit)
    {
        // assertIsLocked ();
        Log("createRevision(branchPage=%p)\n", branchPage);
        AssertBug(branchPage, "Null branchPage provided !");
        assertBranchLockedForWrite(branchPage->branchId);
        /*
         * We are creating a revision after the last current one. It is necessary that we mark that branch as being locked write
         */
        AssertBug(branchPage->branchId, "Null BranchId ?\n");

        AbsolutePagePtr revPagePtr = getPersistentStore().getFreePagePtr();
        AbsolutePageRef<RevisionPage> revPageRef = getPersistentStore().getAbsolutePage<RevisionPage>(revPagePtr);
        AbsolutePageRef<RevisionPage> lastRevPageRef(getPersistentStore());
        if (branchPage->lastRevisionPage)
        {
            AssertBug(branchPage->lastRevisionPage < getPersistentStore().getSB()->noMansLand,
                      "Corrupted branch %llx:'%s' : lastRevisionPage=%llx is after noMansLand=%llx !.\n",
                      branchPage->branchId, branchPage->name, branchPage->lastRevisionPage,
                      getPersistentStore().getSB()->noMansLand);
            lastRevPageRef = getPersistentStore().getAbsolutePage<RevisionPage>(branchPage->lastRevisionPage);
            if (lastRevPageRef.getPage()->documentAllocationHeader.writable)
            {
                Warn("At branch %llx:'%s' : last revision [%llx:%llx] already writable, could not create a new revision.\n",
                     branchPage->branchId, branchPage->name, _brid(lastRevPageRef.getPage()->branchRevId));
                if (forceNoInherit)
                {
                    /*
                     * \todo Fix Bug() : try to make a revision not writable if we are resetting it (this is just a temporary hack)
                     */
                    Warn("Forcing writable = false for last revision...\n");
                    getPersistentStore().alterPage(lastRevPageRef.getPage());
                    lastRevPageRef.getPage()->documentAllocationHeader.writable = false;
                    getPersistentStore().protectPage(lastRevPageRef.getPage());
                }
                else
                {
                    throwException(Exception, "At branch %llx:'%s' : "
                                   "last revision [%llx:%llx] already writable, could not create a new revision.\n",
                                   branchPage->branchId, branchPage->name,
                                   _brid(lastRevPageRef.getPage()->branchRevId));
                }
            }
            AssertBug(lastRevPageRef.getPage()->documentAllocationHeader.writable == false,
                      "Last Revision *was* writable !\n");
        }

        BranchRevId branchRevId;
        branchRevId.branchId = branchPage->branchId;
        branchRevId.revisionId = lastRevPageRef.getPage() ? lastRevPageRef.getPage()->branchRevId.revisionId + 1 : 1;

        getPersistentStore().alterPage(revPageRef.getPage());

        formatRevisionPage(revPagePtr, revPageRef.getPage(), branchRevId);

        if (lastRevPageRef.getPage() && !forceNoInherit)
        {
            inheritRevisionPage(revPageRef.getPage(), lastRevPageRef.getPage());
        }
        else
        {
            formatHeadRevisionPage(revPageRef.getPage());
        }

        revPageRef.getPage()->lastRevisionPage = branchPage->lastRevisionPage;

        getPersistentStore().protectPage(revPageRef.getPage());

        if (0)
        {
            getPersistentStore().syncPage(revPageRef.getPage());
        }

        getPersistentStore().alterPage(branchPage);
        branchPage->lastRevisionPage = revPagePtr;
        getPersistentStore().protectPage(branchPage);

        Log_CreateRevision ( "Created revision %llx:%llx\n", _brid(revPageRef.getPage()->branchRevId) );
        return revPagePtr;
    }

    AbsolutePagePtr
    PersistentBranchManager::forkRevision (const RevisionPage* sourceRevisionPage, const char* branchName,
                                           BranchFlags branchFlags)
    {
        assertBranchUnlockedForWrite(sourceRevisionPage->branchRevId.branchId);

        AssertBug(sourceRevisionPage, "Null source revision !!!\n");
        AssertBug(!sourceRevisionPage->documentAllocationHeader.writable, "Source revision is still writable !\n");

        BranchId forkedBranchId = createBranch(branchName, sourceRevisionPage->branchRevId, branchFlags);
        if (forkedBranchId == 0)
        {
            Error("Could not fork revision [%llx:%llx], branchName='%s', branchFlags=%llx : could not createBranch()\n",
                  _brid(sourceRevisionPage->branchRevId), branchName, branchFlags);
            return NullPage;
        }

        // lockBranchManager();

        /*
         * Lock that branch for writing
         */
        lockBranchForWrite(forkedBranchId);

        BranchPage* forkedBranchPage = getBranchPage(forkedBranchId);

        AssertBug(forkedBranchPage, "Could not get forked branch for forkedBranchId=%llx\n", forkedBranchId);

        AbsolutePagePtr revPagePtr = getPersistentStore().getFreePagePtr();
        AbsolutePageRef<RevisionPage> revPageRef = getPersistentStore().getAbsolutePage<RevisionPage>(revPagePtr);

        BranchRevId branchRevId;
        branchRevId.branchId = forkedBranchPage->branchId;
        branchRevId.revisionId = 1;

        getPersistentStore().alterPage(revPageRef.getPage());

        formatRevisionPage(revPagePtr, revPageRef.getPage(), branchRevId);
        inheritRevisionPage(revPageRef.getPage(), sourceRevisionPage);

        revPageRef.getPage()->journalHead.firstJournalItem = NullPtr;

        Info("[JOURNAL] Journal reset for forked brId=[%llx:%llx]\n", _brid(branchRevId));

#if 0
        /*
         * We do not wish to share reserved ElementIds with the forked branch, so make them both zero
         */
        revPageRef.getPage()->documentHead.firstReservedElementId = 0;
        revPageRef.getPage()->documentHead.lastReservedElementId = 0;
#endif

        getPersistentStore().protectPage(revPageRef.getPage());

        getPersistentStore().alterPage(forkedBranchPage);
        forkedBranchPage->lastRevisionPage = revPagePtr;
        getPersistentStore().protectPage(forkedBranchPage);

        return revPagePtr;
    }

    void
    PersistentBranchManager::dropRevisions (BranchPage* branchPage, RevisionId fromRev, RevisionId toRev)
    {
        Warn("dropRevisions() is not completely implemented !!\n");
        Warn("dropRevisions(branch=%llx '%s', fromRev=%llu, toRev=%llu)\n", branchPage->branchId, branchPage->name,
             fromRev, toRev);

        AssertBug(branchPage, "Null branchPage provided.\n");

        if (!fromRev && !toRev)
        {
            // Specific case : will blindly delete all revisions
            Log_DropRevision ( "Dropping every revision for branch %llx\n", branchPage->branchId);
            AbsolutePagePtr revPagePtr = branchPage->lastRevisionPage, nextRevPagePtr = NullPage;

            while (revPagePtr)
            {
                AbsolutePageRef<RevisionPage> revPageRef = getPersistentStore().getAbsolutePage<RevisionPage>(
                        revPagePtr);

                nextRevPagePtr = revPageRef.getPage()->lastRevisionPage;

                Log_DropRevision ( "[DROPREVISIONS] dropping revision [%llx:%llx], page=%llx, next=%llx\n",
                        _brid ( revPageRef.getPage()->branchRevId ), revPagePtr, nextRevPagePtr );

                PersistentDocument* doc = instanciateTemporaryPersistentDocument(revPagePtr);

                doc->dropAllPages();

                delete (doc);

                revPagePtr = nextRevPagePtr;
            }

            getPersistentStore().alterPage(branchPage);
            branchPage->lastRevisionPage = NullPage;
            getPersistentStore().protectPage(branchPage);
        }
        else
        {
            /**
             * [from, to] must be converted to
             * (a)->(b)->(from)..->(to)->(c) => (a)->(b)->(c)
             *
             * firstRevPagePtr=(b) and lastRevPagePtr=(c)
             *
             */
            AbsolutePagePtr firstRevPagePtr = NullPage, lastRevPagePtr = NullPage;

            AbsolutePagePtr revPagePtr = branchPage->lastRevisionPage;

            while (revPagePtr)
            {
                Log_DropRevision("At revPagePtr=%llx, firstRevPagePtr=%llx\n", revPagePtr, firstRevPagePtr);
                AbsolutePageRef<RevisionPage> revPageRef = getPersistentStore().getAbsolutePage<
                RevisionPage> (revPagePtr);
                BranchRevId branchRevId = revPageRef.getPage()->branchRevId;
                AbsolutePagePtr nextRevPagePtr = revPageRef.getPage()->lastRevisionPage;

                revPageRef = AbsolutePageRef<RevisionPage>(getPersistentStore());

                AssertBug ( branchRevId.branchId == branchPage->branchId, "Invalid branchId !\n" );
                if (toRev && branchRevId.revisionId > toRev)
                {
                    lastRevPagePtr = revPagePtr;
                }
                else if (fromRev && branchRevId.revisionId < fromRev)
                {
                    firstRevPagePtr = revPagePtr;
                    break;
                }
                revPagePtr = nextRevPagePtr;
            }
            Log_DropRevision ( "Deleting : branch lastRev=%llx, keep lastRev=%llx and firstRev=%llx\n",
                    branchPage->lastRevisionPage, lastRevPagePtr, firstRevPagePtr );

            if (lastRevPagePtr)
            {
                AbsolutePageRef<RevisionPage> revPageRef = getPersistentStore().getAbsolutePage<RevisionPage>(
                        lastRevPagePtr);
                getPersistentStore().alterPage(revPageRef.getPage());
                revPageRef.getPage()->lastRevisionPage = firstRevPagePtr;
                getPersistentStore().protectPage(revPageRef.getPage());
            }
            else
            {
                getPersistentStore().alterPage(branchPage);
                branchPage->lastRevisionPage = firstRevPagePtr;
                getPersistentStore().protectPage(branchPage);
            }
            Log_DropRevision( "Final branchpage->lastRevisionPage=%llx\n", branchPage->lastRevisionPage);
        }
    }

#if 1
    bool
    PersistentStore::dropUncommittedRevisions ()
    {
        SuperBlock* sb = getSB();
        for (AbsolutePageRef<BranchPage> branchPageRef = getAbsolutePage<BranchPage>(sb->lastBranch);
                branchPageRef.getPage();
                branchPageRef = getAbsolutePage<BranchPage>(branchPageRef.getPage()->lastBranch))
        {
            Log_DropRevision ( "At branch=%p, sb->lastBranch=%llx, branch->lastRevPage=%llx\n",
                    branchPageRef.getPage(), sb->lastBranch,
                    branchPageRef.getPage()->lastRevisionPage );
            if ( branchPageRef.getPage()->lastRevisionPage == NullPage )
            {
                Log_DropRevision ( "Empty branch %llx\n", branchPageRef.getPage()->branchId );
                continue;
            }
            AbsolutePagePtr lastRevisionPagePtr = branchPageRef.getPage()->lastRevisionPage;
            AbsolutePageRef<RevisionPage> revPageRef = getAbsolutePage<RevisionPage> ( lastRevisionPagePtr );
            if ( revPageRef.getPage() && revPageRef.getPage()->documentAllocationHeader.writable )
            {
                Warn ( "Branch %llx, Revision %llx %llx:%llx is marked writable !\n",
                        branchPageRef.getPage()->branchId,
                        branchPageRef.getPage()->lastRevisionPage, _brid ( revPageRef.getPage()->branchRevId ) );
                Warn ( "Dropping this revision, last revision at %llx\n",
                        revPageRef.getPage()->lastRevisionPage );
#if 0
                PersistentDocument* persistentDocument = getPersistentBranchManager().instanciateTemporaryPersistentDocument ( branchPage->lastRevisionPage);
                persistentDocument->drop ();
                delete ( persistentDocument );
                revPage = NULL;
#else
                BranchRevId brId = revPageRef.getPage()->branchRevId;
                getPersistentBranchManager().dropRevisions(branchPageRef.getPage(), brId.revisionId, brId.revisionId);
#endif
            }
        }
        return true;
    }
#endif

}
;

