/*
 * cachebranchmanager-branchinfo.cpp
 *
 *  Created on: 7 nov. 2009
 *      Author: francois
 */

#include <Xemeiah/persistence/cachebranchmanager.h>
#include <Xemeiah/persistence/persistentdocument.h>
#include <Xemeiah/kern/document.h>

#include <Xemeiah/auto-inline.hpp>
#include <Xemeiah/persistence/auto-inline.hpp>

#define Log_CBM Debug

namespace Xem
{
    CacheBranchManager::BranchInfo::BranchInfo (CacheBranchManager& _cacheBranchManager, BranchId _branchId,
                                                const BranchRevId& _forkedFrom) :
            cacheBranchManager(_cacheBranchManager), branchWriteMutex("BranchInfo", _branchId)
    {
        branchWriteMutex.setLogLevel(1);

        branchId = _branchId;
        forkedFrom = _forkedFrom;
        scheduledForRemoval = false;

        dependingBranchInfo = NULL;

#ifdef __XEM_PERSISTENCE_CACHEBRANCHMANAGER_REUSABLE_DOCUMENTS_LIST
        // Nothing to do here
#else
        reusableDocument = NULL;
#endif // __XEM_PERSISTENCE_CACHEBRANCHMANAGER_REUSABLE_DOCUMENTS_LIST
    }

    CacheBranchManager::BranchInfo::~BranchInfo ()
    {
        if (instanciatedDocuments.size())
        {
            Warn("Pre-clean : Branch %llx still has %llu documents instanciated :\n", getBranchId(),
                 (__ui64) instanciatedDocuments.size());
            for (InstanciatedDocuments::iterator iter = instanciatedDocuments.begin();
                    iter != instanciatedDocuments.end(); iter++)
            {
                Document* doc = *iter;
                Warn("\t%p %llx:%llx role='%s', flags='%s', refCount=%llx\n", doc, _brid(doc->getBranchRevId()),
                     doc->getRole().c_str(), getDocumentOpeningFlagsName(doc->getDocumentOpeningFlags()),
                     doc->getRefCount());
            }
        }

        if (reusableDocument)
        {
            Warn("Pre-clean : Branch %llx still has a reusable document !\n", getBranchId());
            Document* doc = reusableDocument;
            Warn("\t%p %llx:%llx role='%s', flags='%s', refCount=%llx\n", doc, _brid(doc->getBranchRevId()),
                 doc->getRole().c_str(), getDocumentOpeningFlagsName(doc->getDocumentOpeningFlags()),
                 doc->getRefCount());
        }
        cleanupReusableDocument();

        Warn("Post-clean : Branch %llx\n", getBranchId());
        if (reusableDocument)
        {
            Warn("Post-clean : Still have a reusable document at %p !\n", reusableDocument);
        }

        if (instanciatedDocuments.size())
        {
            Warn("Post-clean : Branch %llx still has %llu documents instanciated :\n", getBranchId(),
                 (__ui64) instanciatedDocuments.size());
            for (InstanciatedDocuments::iterator iter = instanciatedDocuments.begin();
                    iter != instanciatedDocuments.end(); iter++)
            {
                Warn("\t%p\n", *iter);
            }
        }

    }

    BranchId
    CacheBranchManager::BranchInfo::getBranchId ()
    {
        return branchId;
    }

    BranchRevId
    CacheBranchManager::BranchInfo::getForkedFrom ()
    {
        return forkedFrom;
    }

    const char*
    CacheBranchManager::BranchInfo::getName ()
    {
        NotImplemented("getName()\n");
        return NULL;
    }

    void
    CacheBranchManager::BranchInfo::lockWrite ()
    {
        if (cacheBranchManager.isLocked())
        {
            Log("Branch %llx : locking with cacheBranchManager locked !\n", getBranchId());
        }
        branchWriteMutex.lock();
    }

    void
    CacheBranchManager::BranchInfo::unlockWrite ()
    {
        branchWriteMutex.unlock();
    }

    bool
    CacheBranchManager::BranchInfo::isLockedWrite ()
    {
        return branchWriteMutex.isLocked();
    }

    void
    CacheBranchManager::BranchInfo::referenceDocument (Document* doc)
    {
#if PARANOID
        for ( InstanciatedDocuments::iterator iter = instanciatedDocuments.begin();
                iter != instanciatedDocuments.end(); iter++ )
        {
            AssertBug ( doc != *iter, "Document %p ('%s', %llx:%llx) already referenced for this branch (%p) !\n",
                    doc, doc->getRole().c_str(), _brid(doc->getBranchRevId()), *iter );
        }
#endif
        instanciatedDocuments.push_back(doc);
    }

    void
    CacheBranchManager::BranchInfo::dereferenceDocument (Document* doc)
    {
        for (InstanciatedDocuments::iterator iter = instanciatedDocuments.begin(); iter != instanciatedDocuments.end();
                iter++)
        {
            if (doc == *iter)
            {
                instanciatedDocuments.erase(iter);
                return;
            }
        }

        Bug("Document %p [%llx:%llx] not referenced for this branch (%llx) !\n", doc, _brid(doc->getBranchRevId()),
            getBranchId());
    }

    __ui64
    CacheBranchManager::BranchInfo::getInstanciatedDocumentsNumber ()
    {
        __ui64 nb = instanciatedDocuments.size();
        for (DependantBranchInfos::iterator iter = dependantBranchInfos.begin(); iter != dependantBranchInfos.end();
                iter++)
        {
            nb += (*iter)->getInstanciatedDocumentsNumber();
        }
        return nb;
    }

    Document*
    CacheBranchManager::BranchInfo::getInstanciatedDocument (RevisionId revisionId, DocumentOpeningFlags flags)
    {
        for (std::list<Document*>::iterator iter = instanciatedDocuments.begin(); iter != instanciatedDocuments.end();
                iter++)
        {
            Document* doc = *iter;
            if (doc->getBranchRevId().revisionId == revisionId && doc->getDocumentOpeningFlags() == flags)
            {
                return doc;
            }
        }
        return NULL;
    }

    bool
    CacheBranchManager::BranchInfo::isFreeFromDocuments ()
    {
        if (!instanciatedDocuments.empty())
        {
            return false;
        }

        for (DependantBranchInfos::iterator iter = dependantBranchInfos.begin(); iter != dependantBranchInfos.end();
                iter++)
        {
            if (!(*iter)->isFreeFromDocuments())
            {
                return false;
            }
        }
        return true;
    }

    void
    CacheBranchManager::BranchInfo::checkIsFreeFromDocuments ()
    {
        cleanupReusableDocument();

        if (instanciatedDocuments.empty())
        {
            return;
        }

        Exception* e = new PersistenceException();
        for (std::list<Document*>::iterator iter = instanciatedDocuments.begin(); iter != instanciatedDocuments.end();
                iter++)
        {
            Document* doc = (*iter);
            detailException(e, "Document [%llx:%llx] still opened for branch %s\n", _brid(doc->getBranchRevId()),
                            getName());
        }
        throw(e);
    }

    bool
    CacheBranchManager::BranchInfo::hasInstanciatedWritableDocument ()
    {
        for (std::list<Document*>::iterator iter = instanciatedDocuments.begin(); iter != instanciatedDocuments.end();
                iter++)
        {
            Document* doc = (*iter);
            if (doc->isWritable())
            {
                Warn("Yes, we have an instanciated document [%llx:%llx]\n", _brid(doc->getBranchRevId()));
                return true;
            }
        }
        return false;
    }

    void
    CacheBranchManager::BranchInfo::setScheduledForRemoval ()
    {
        scheduledForRemoval = true;
    }

#ifdef __XEM_PERSISTENCE_CACHEBRANCHMANAGER_REUSABLE_DOCUMENTS_LIST
    Document* CacheBranchManager::BranchInfo::getReusableDocument ( RevisionId revisionId )
    {
        ReusableDocuments::iterator iter = reusableDocuments.find ( revisionId );
        if ( iter == reusableDocuments.end() )
        return NULL;
        return iter->second;
    }

    void CacheBranchManager::BranchInfo::setReusableDocument ( RevisionId revisionId, Document* doc )
    {
        Document* previous = getReusableDocument ( revisionId );
        if ( ! previous )
        {
            reusableDocuments[revisionId] = doc;
            return;
        }
        if ( doc == NULL )
        {
            // This is a reset.
            reusableDocuments[revisionId] = doc;
            return;
        }
        if ( previous != doc )
        {
            Warn ( "Already have a previous document !!!\n" );
            reusableDocuments[revisionId] = doc;
        }
    }

    void CacheBranchManager::BranchInfo::deleteReusableDocuments ()
    {
        NotImplemented ( "Yet.\n" );
#if 0
        /**
         * Flush our reusable document stack
         */
        for ( BranchInfo::ReusableDocuments::iterator iter = reusableDocuments.begin();
                iter != reusableDocuments.end(); iter++ )
        {
            PersistentDocument* doc = dynamic_cast<PersistentDocument*> ( iter->second );
            AssertBug ( doc, "Invalid doc type.\n" );

            doc->acknowledgedPersistentDocumentDeletion = true;
            delete ( doc );
        }
#endif
        reusableDocuments.clear ();
    }
#else
    Document*
    CacheBranchManager::BranchInfo::getReusableDocument (RevisionId revisionId, RoleId roleId,
                                                         DocumentOpeningFlags flags)
    {
        getCacheBranchManager().assertIsLocked();

        Log_CBM ( "[REUSE] curr=[R%llu->%p], asking R%llu\n",
                reusableDocument ? reusableDocument->getBranchRevId().revisionId : 0,
                reusableDocument, revisionId );
        if (reusableDocument) // revisionId == reusableDocumentRevisionId )
        {
            if (reusableDocument->getBranchRevId().revisionId != revisionId)
            {
                Log_CBM ( "[REUSE] Diverging revisionId : have=%llx, asked=%llx\n",
                        reusableDocument->getBranchRevId().revisionId, revisionId );
                return NULL;
            }
            if (reusableDocument->getRoleId() != roleId)
            {
                Log_CBM ( "[REUSE] Diverging roles : have=%s (%x), asked=%x\n",
                        reusableDocument->getRole().c_str(), reusableDocument->getRoleId(), roleId );
                return NULL;
            }
            if (reusableDocument->getDocumentOpeningFlags() != flags)
            {
                Log_CBM ( "[REUSE] Diverging Openning Flags : have=%s, asked=%s\n",
                        getDocumentOpeningFlagsName(reusableDocument->getDocumentOpeningFlags()),
                        getDocumentOpeningFlagsName(flags) );
                return NULL;
            }
            /*
             * We found it, reset all
             */
            Document* doc = reusableDocument;

            AssertBug(doc->getRefCount(), "Document as a zero refCount !\n");
            if (doc->getDocumentOpeningFlags() == DocumentOpeningFlags_ExplicitRead)
            {

            }
            else
            {
                reusableDocument = NULL;
                doc->decrementRefCount();
                // doc->decrementRefCountLockLess();
            }
            return doc;
        }
        return NULL;
    }

    void
    CacheBranchManager::BranchInfo::cleanupReusableDocument ()
    {
        if (!reusableDocument)
        {
            return;
        }

        Log_CBM ( "Branch %llx has reusableDocument [%llx:%llx] refCount=%llx, role=%s, flags=%s, alloced=%llu MBytes\n",
                getBranchId(), _brid(reusableDocument->getBranchRevId()),
                reusableDocument->getRefCount(),
                reusableDocument->getRole().c_str(),
                getDocumentOpeningFlagsName(reusableDocument->getDocumentOpeningFlags()),
                ((reusableDocument->getNumberOfAreasMapped() * reusableDocument->getAreaSize()) >> 20)
        );

        PersistentDocument* pDoc = dynamic_cast<PersistentDocument*>(reusableDocument);

        AssertBug(pDoc, "Reusable document not a persistent document ?\n");

        pDoc->lockRefCount();

        if (pDoc->getRefCount() == 1)
        {
            reusableDocument = NULL;
            dereferenceDocument(pDoc);
            pDoc->acknowledgedPersistentDocumentDeletion = true;
            pDoc->decrementRefCountLockLess();
            pDoc->unlockRefCount();
            delete (pDoc);
            Log_CBM ( "[REUSE] Effectively deleted document %p\n", pDoc );
        }
        else
        {
            Log_CBM ( "[REUSE] RefCount of [%llx:%llx] jumped to %llx while locking\n",
                    _brid(pDoc->getBranchRevId()), pDoc->getRefCount() );
            pDoc->unlockRefCount();
        }
    }

    bool
    CacheBranchManager::BranchInfo::setReusableDocument (Document* doc)
    {
        getCacheBranchManager().assertIsLocked();
        doc->assertRefCountLocked();

        AssertBug(doc, "NULL document provided !\n");
        AssertBug(doc->getBranchRevId().branchId == getBranchId(), "Diverging branchIds !\n");

        Log_CBM ( "[REUSE] curr=[R%llx->%p], setting [R%llx->%p]\n",
                reusableDocument ? reusableDocument->getBranchRevId().revisionId : 0, reusableDocument,
                doc->getBranchRevId().revisionId, doc );

        if (reusableDocument)
        {
            AssertBug(getBranchId() == reusableDocument->getBranchRevId().branchId, "Diverging branch Ids !\n");

            if (doc->getBranchRevId().revisionId < reusableDocument->getBranchRevId().revisionId)
            {
                Warn("[REUSE] Provided me with an older reusable (%p : %llx) than what I got (%p : %llx).\n", doc,
                     doc->getBranchRevId().revisionId, reusableDocument, reusableDocument->getBranchRevId().revisionId);
                return false;
            }
            else if (reusableDocument == doc)
            {
                Log_CBM ( "[REUSE] Already recorded document %p [%llx:%llx] as reusable\n",
                        reusableDocument, _brid(reusableDocument->getBranchRevId()) );
                return true;
            }
            else if (0 && doc->getBranchRevId().revisionId == reusableDocument->getBranchRevId().revisionId)
            {
                Log_CBM ( "[REUSE] Already have a reusable ! Previous is %p [%llx:%llx] (refCount=%llu), same revision than doc=%p [%llx:%llx]\n",
                        reusableDocument, _brid(reusableDocument->getBranchRevId()), reusableDocument->getRefCount(),
                        doc, _brid(doc->getBranchRevId()) );
                return false;
            }
            else // if ( revisionId > reusableDocument->getBranchRevId().revisionId )
            {
                Log_CBM ( "[REUSE] Refresh reusable ! Previous is %p [%llx:%llx] (refCount=%llu), superseeded by doc=%p [%llx:%llx]\n",
                        reusableDocument, _brid(reusableDocument->getBranchRevId()), reusableDocument->getRefCount(),
                        doc, _brid(doc->getBranchRevId()) );

                Log_CBM ( "[REUSE] I Will destroy that document ! [%llx:%llx], reuse=%p (refCount=%llu)\n",
                        _brid(reusableDocument->getBranchRevId()), reusableDocument, reusableDocument->getRefCount() );
                if (reusableDocument->getRefCount() == 1)
                {
                    /**
                     * We have to brute-force kill this document.
                     */
#if 1
                    Log_CBM ( "[REUSE] Brute-force kill old document %p [%llx:%llx]\n", reusableDocument,
                            _brid(reusableDocument->getBranchRevId()) );
                    cleanupReusableDocument();
#endif
                }
            }
        }
        // doc->housewife();
        doc->incrementRefCountLockLess();
        reusableDocument = doc;
        return true;
    }

#endif

}
;
