#include <Xemeiah/persistence/cachebranchmanager.h>
#include <Xemeiah/persistence/persistentdocument.h>
#include <Xemeiah/kern/document.h>

#include <Xemeiah/auto-inline.hpp>
#include <Xemeiah/persistence/auto-inline.hpp>

#define Log_CBM Debug
#define Log_CBM_GarbageCollector Debug

namespace Xem
{
    CacheBranchManager::CacheBranchManager () :
            branchManagerMutex("Branch Manager Mutex")
    {
        branchManagerMutex.setLogLevel(1);
    }

    CacheBranchManager::~CacheBranchManager ()
    {
        Warn("Deleting CacheBranchManager !\n");
        for (BranchMap::iterator iter = branchMap.begin(); iter != branchMap.end(); iter++)
        {
            if (!iter->second)
                continue;
            BranchInfo* branchInfo = iter->second;
            iter->second = NULL;
            delete (branchInfo);
        }
        Warn("Deleting CacheBranchManager OK !\n");
    }

    bool
    CacheBranchManager::isLocked ()
    {
        return branchManagerMutex.isLocked();
    }

    void
    CacheBranchManager::assertIsLocked ()
    {
        branchManagerMutex.assertLocked();
    }

    void
    CacheBranchManager::assertIsUnlocked ()
    {
        branchManagerMutex.assertUnlocked();
    }

    void
    CacheBranchManager::lockBranchManager ()
    {
        branchManagerMutex.lock();
    }

    void
    CacheBranchManager::unlockBranchManager ()
    {
        branchManagerMutex.unlock();
    }

    void
    CacheBranchManager::assertBranchLockedForWrite (BranchId branchId)
    {
        AssertBug(isBranchLockedForWrite(branchId), "Not locked for write !\n");
    }

    void
    CacheBranchManager::assertBranchUnlockedForWrite (BranchId branchId)
    {
        AssertBug(!isBranchLockedForWrite(branchId), "Locked for write !\n");
    }

    bool
    CacheBranchManager::isBranchLockedForWrite (BranchId branchId)
    {
        if (!branchId)
        {
            Warn("Asking if branchId '0' locked for write ???\n");
            return false;
        }
        BranchInfo* branchInfo = branchMap[branchId];
        AssertBug(branchInfo, "Invalid branchId : %llx\n", branchId);
        return branchInfo->isLockedWrite();
    }

    void
    CacheBranchManager::lockBranchForWrite (BranchId branchId)
    {
        BranchInfo* branchInfo = branchMap[branchId];
        AssertBug(branchInfo, "Invalid branchId.\n");
        branchInfo->lockWrite();
    }

    void
    CacheBranchManager::unlockBranchForWrite (BranchId branchId)
    {
        BranchInfo* branchInfo = branchMap[branchId];
        AssertBug(branchInfo, "Invalid branchId.\n");
        branchInfo->unlockWrite();
    }

    CacheBranchManager::BranchInfo*
    CacheBranchManager::referenceBranch (BranchId branchId, const BranchRevId& forkedFrom)
    {
        if (branchMap.find(branchId) != branchMap.end() && branchMap[branchId])
        {
            Warn("Branch '%llx' already referenced !\n", branchId);
            return branchMap[branchId];
        }
        BranchInfo* branchInfo = new BranchInfo(*this, branchId, forkedFrom);
        branchMap[branchId] = branchInfo;
        return branchInfo;
    }

    void
    CacheBranchManager::markBranchDependency (BranchInfo* branchInfo)
    {
        BranchId sourceBranchId = branchInfo->getForkedFrom().branchId;
        BranchInfo* sourceBranchInfo = branchMap[sourceBranchId];

        if (!sourceBranchInfo)
        {
            Error("Branch %llx : could not get branch=%llx for forkedFrom=%llx:%llx, we may delete this branch.\n",
                  branchInfo->getBranchId(), sourceBranchId, _brid(branchInfo->getForkedFrom() ));
            return;
        }
        Log_CBM ( "Marking branch dependency %llx -> %llx\n", branchInfo->getBranchId(), sourceBranchId );
        sourceBranchInfo->getDependantBranchInfos().push_back(branchInfo);
        branchInfo->setDependingBranchInfo(sourceBranchInfo);
    }

    void
    CacheBranchManager::checkBranchRemoval (BranchInfo* branchInfo)
    {
        AssertBug(branchInfo, "NULL branchInfo !\n");

        if ((branchInfo->isScheduledForRemoval() /* || branchInfo->getBranchPage()->lastRevisionPage == NullPage */)
                && branchInfo->isFreeFromDocuments())
        {
            Log_CBM ( "At doc deref : Branch %llx to delete, scheduled=%s, has revisions=%s.\n",
                    branchInfo->getBranchId(),
                    branchInfo->isScheduledForRemoval() ? "yes" : "no",
                    "??" /* branchInfo->getBranchPage()->lastRevisionPage == NullPage ? "yes" : "no" */);

            deleteBranch(branchInfo);
        }
    }

    Document*
    CacheBranchManager::getDocumentFromCache (BranchId branchId, RevisionId revisionId, DocumentOpeningFlags flags,
                                              RoleId roleId)
    {
        BranchInfo* branchInfo = branchMap[branchId];
        if (!branchInfo)
            return NULL;
        return getDocumentFromCache(branchInfo, revisionId, flags, roleId);
    }

    Document*
    CacheBranchManager::getDocumentFromCache (BranchInfo* branchInfo, RevisionId revisionId, DocumentOpeningFlags flags,
                                              RoleId roleId)
    {
        Log_CBM ( "Getting brId=[%llx:%llx], flags=%x '%s', roleId='%x'\n", branchInfo->getBranchId(), revisionId,
                flags, getDocumentOpeningFlagsName(flags), roleId );

        return branchInfo->getReusableDocument(revisionId, roleId, flags);
    }

    void
    CacheBranchManager::releaseDocument (Document* doc)
    {
        assertIsLocked();

        bool mayReuse = false;

        Log_CBM ( "Releasing document %p, [%llx:%llx], flags=%x, refCount=%llu\n", doc,
                _brid(doc->getBranchRevId()), doc->getDocumentOpeningFlags(), doc->getRefCount() );

        BranchRevId docBrId = doc->getBranchRevId();
        if (docBrId.branchId == 0)
        {
            return;
        }

        AssertBug(! doc->isLockedWrite(), "While releasing document %p [%llx:%llx] : document is locked write !\n", doc,
                  _brid (doc->getBranchRevId()));

        BranchId branchId = doc->getBranchRevId().branchId;
        BranchInfo* branchInfo = branchMap[branchId];
        AssertBug(branchInfo, "Could not get branchInfo for branch %llx\n", branchId);

        if (doc->isScheduledRevisionRemoval())
        {
            doc->drop();
        }
        else if (mayReuse)
        {
            DocumentOpeningFlags flags = doc->getDocumentOpeningFlags();
            if (flags == DocumentOpeningFlags_ExplicitRead || flags == DocumentOpeningFlags_Read
                    || flags == DocumentOpeningFlags_FollowBranch || flags == DocumentOpeningFlags_AsRevision)
            {
                Log_CBM ( "[REUSE] Document %p [%llx:%llx] : setting to reusability.\n", doc, _brid(doc->getBranchRevId()) );
                if (branchInfo->setReusableDocument(doc))
                {
                    return;
                }
                Log_CBM ( "[REUSE] Document %p [%llx:%llx] : *NOT* set reusable.\n", doc, _brid(doc->getBranchRevId()) );
            }
        }
        branchInfo->dereferenceDocument(doc);

        Log_CBM ( "Document %p [%llx:%llx], write=%d, deleting document.\n", doc, _brid(doc->getBranchRevId()), doc->isWritable() );

        checkBranchRemoval(branchInfo);
    }

    void
    CacheBranchManager::updateDocumentReference (Document* doc, BranchRevId oldBrId, BranchRevId newBrId)
    {
        Log_CBM("[UPDATEDOCREF] doc=%p (%llx:%llx) oldBrId=[%llx:%llx] => newBrId=[%llx:%llx]\n", doc,
             _brid(doc->getBranchRevId()), _brid(oldBrId), _brid(newBrId));
        lockBranchManager();
        BranchInfo* oldBranchInfo = branchMap[oldBrId.branchId];
        AssertBug(oldBranchInfo, "Null branchInfo for branchId=%llx\n", oldBrId.branchId);
        BranchInfo* newBranchInfo = branchMap[newBrId.branchId];

        if (newBranchInfo == NULL)
        {
            Warn("Inexistant branchInfo for newBranchId=%llx, faking it...\n", newBrId.branchId);
            BranchRevId nullBrId =
                { 0, 0 };
            newBranchInfo = referenceBranch(newBrId.branchId, nullBrId);
        }
        AssertBug(newBranchInfo, "Null branchInfo for branchId=%llx\n", newBrId.branchId);
        oldBranchInfo->dereferenceDocument(doc);
        checkBranchRemoval(oldBranchInfo);
        newBranchInfo->referenceDocument(doc);
        unlockBranchManager();
    }

    void
    CacheBranchManager::garbageCollectorThread ()
    {
        setStarted();
        int loopInterval = 1;
        int garbageInterval = 60;
        int nbIntervals = 0;

        while (true)
        {
            sleep(loopInterval);

            if (isStopping())
            {
                Info("CacheBranchManager : stopping garbageCollectorThread !\n");
                return;
            }

            if (nbIntervals != garbageInterval)
            {
                nbIntervals++;
                continue;
            }
            nbIntervals = 0;

            Log_CBM_GarbageCollector ( "***************** GARBAGE COLLECTOR THREAD ***************\n" );
            getStore().housewife();

            lockBranchManager();

            for (BranchMap::iterator iter = branchMap.begin(); iter != branchMap.end(); iter++)
            {
                if (!iter->second)
                    continue;
                BranchInfo& branchInfo = *(iter->second);
                branchInfo.cleanupReusableDocument();
            }

            unlockBranchManager();
        }
    }

    void
    CacheBranchManager::start ()
    {
        Info("Starting service for CacheBranchManager !\n");
        startThread(boost::bind(&CacheBranchManager::garbageCollectorThread, this));
    }

    void
    CacheBranchManager::stop ()
    {
        Info("Stopping service for CacheBranchManager !\n");

    }

    void
    CacheBranchManager::checkIsFreeFromDocuments ()
    {
        for (BranchMap::iterator iter = branchMap.begin(); iter != branchMap.end(); iter++)
        {
            if (!iter->second)
                continue;
            BranchInfo& branchInfo = *(iter->second);

            branchInfo.checkIsFreeFromDocuments();
            if (!branchInfo.isFreeFromDocuments())
            {
                Warn("CacheBranchManager : still documents opened for branch '%s' (%llx)\n", branchInfo.getName(),
                     branchInfo.getBranchId());

            }
        }
    }

}
