/*
 * org_xemeiah_dom_Store.cpp
 *
 *  Created on: 15 janv. 2010
 *      Author: francois
 */

#include "include/org_xemeiah_dom_DocumentFactory.h"
#include <Xemeiah/kern/volatilestore.h>
#include <Xemeiah/kern/volatiledocument.h>
#include <Xemeiah/persistence/persistentstore.h>
#include <Xemeiah/xprocessor/xprocessor.h>
#include <Xemeiah/parser/parser.h>
#include <Xemeiah/trace.h>
#include <Xemeiah/log.h>

#include <Xemeiah/version.h>

#include "xem-jni-dom.h"
#include "xem-jni-classes.h"

#include <Xemeiah/auto-inline.hpp>

#if 0
#undef Log
#define Log Info
#endif

JNIEXPORT void JNICALL
Java_org_xemeiah_dom_DocumentFactory_cleanUp (JNIEnv *ev, jobject jFactory)
{
    XEMJNI_PROLOG
    {
        Xem::Store* store = jDocumentFactory2Store(ev, jFactory);
        if (store)
        {
            Xem::PersistentStore* pStore = dynamic_cast<Xem::PersistentStore*>(store);
            if (pStore)
            {
                pStore->close();
            }
            delete (store);
        }
        initDocumentFactory(ev, jFactory, NULL);
    }
    XEMJNI_POSTLOG;
}

JNIEXPORT void JNICALL
Java_org_xemeiah_dom_DocumentFactory_openVolatile (JNIEnv *ev, jobject jFactory)
{
    Xem::Store* store = new Xem::VolatileStore();
    initDocumentFactory(ev, jFactory, store);
}

JNIEXPORT void JNICALL
Java_org_xemeiah_dom_DocumentFactory_format (JNIEnv *ev, jobject jFactory, jstring jFilename)
{
    Xem::String filename = jstring2XemString(ev, jFilename);

    Xem::PersistentStore* persistentStore = new Xem::PersistentStore();
    XEMJNI_PROLOG
    {
        persistentStore->format(filename.c_str());
    }
    XEMJNI_POSTLOG;
    delete (persistentStore);
}

JNIEXPORT void JNICALL
Java_org_xemeiah_dom_DocumentFactory_open (JNIEnv *ev, jobject jFactory, jstring jFilename)
{
    Xem::String filename = jstring2XemString(ev, jFilename);

    Xem::PersistentStore* persistentStore = new Xem::PersistentStore();
    XEMJNI_PROLOG
    {
        persistentStore->open(filename.c_str());
        initDocumentFactory(ev, jFactory, persistentStore);
        persistentStore->dropUncommittedRevisions();
    }
    XEMJNI_POSTLOG_OPS(delete (persistentStore));
}

JNIEXPORT jstring JNICALL
Java_org_xemeiah_dom_DocumentFactory_doCreateBranch (JNIEnv *ev, jobject jFactory, jstring jBranchName,
                                                     jstring jBranchFlags)
{
    XEMJNI_PROLOG
    {
        Xem::Store* store = jDocumentFactory2Store(ev, jFactory);
        Xem::String branchName = jstring2XemString(ev, jBranchName);
        Xem::String branchFlags = jstring2XemString(ev, jBranchFlags);

        Xem::BranchRevId forkedFromBrId =
            { 0, 0 };
        Xem::BranchId newBranchId = store->getBranchManager().createBranch(branchName, forkedFromBrId, branchFlags);
        if (!newBranchId)
        {
            throwException(Xem::RuntimeException, "Could not create branch '%s' from %llx:%llx (flags=%s)\n",
                           branchName.c_str(), _brid(forkedFromBrId), branchFlags.c_str());
        }
        Xem::String newBranchName = store->getBranchManager().getBranchName(newBranchId);
        Log("Created branch '%s' (%llx => %s) (from [%llx:%llx], flags=%s)\n", branchName.c_str(), newBranchId,
                newBranchName.c_str(), _brid(forkedFromBrId), branchFlags.c_str());
        return ev->NewStringUTF(newBranchName.c_str());
    }
    XEMJNI_POSTLOG;
    return NULL;
}

JNIEXPORT void JNICALL
Java_org_xemeiah_dom_DocumentFactory_close (JNIEnv *ev, jobject jFactory)
{
    XEMJNI_PROLOG
    {
#if 1
        throwException(Xem::RuntimeException, "Invalid call to DocumentFactory.close() !");
#else
        Xem::Store* store = jDocumentFactory2Store(ev, jFactory);

        Xem::PersistentStore* pStore = dynamic_cast<Xem::PersistentStore*>(store);
        if (pStore)
        {
            Log("Closing PersistentStore at %p\n", pStore);
            pStore->close();
        }
#endif
    }
    XEMJNI_POSTLOG;
}

JNIEXPORT void JNICALL
Java_org_xemeiah_dom_DocumentFactory_check (JNIEnv *ev, jobject jFactory)
{
    XEMJNI_PROLOG
    {
        Xem::Store* store = jDocumentFactory2Store(ev, jFactory);
        Xem::PersistentStore* pStore = dynamic_cast<Xem::PersistentStore*>(store);
        if (pStore)
        {
            Log("Checking PersistentStore at %p\n", pStore);
            pStore->check(Xem::PersistentStore::Check_AllContents);
        }
    }
    XEMJNI_POSTLOG;
}

JNIEXPORT jobject JNICALL
Java_org_xemeiah_dom_DocumentFactory_newStandaloneDocument (JNIEnv *ev, jobject jFactory, jstring jBranchName,
                                                            jstring jOpenFlags)
{
    XEMJNI_PROLOG
    {
        Xem::Store* store = jDocumentFactory2Store(ev, jFactory);

        Xem::String branchName = jstring2XemString(ev, jBranchName);
        Xem::String openFlags = jstring2XemString(ev, jOpenFlags);

        Xem::KeyId roleId = store->getKeyCache().getBuiltinKeys().nons.none();
        Xem::Document* document = store->getBranchManager().openDocument(branchName, openFlags, roleId);

        document->incrementRefCount();

        Xem::XProcessor* xprocessor = new Xem::XProcessor(*store);
        xprocessor->installModule("http://www.xemeiah.org/ns/xem");

        jobject jDocument = createJDocument(ev, jFactory, document, xprocessor);

        Log("DocumentFactory : jFactory=%p, new document=%p, jDocument=%p\n", jFactory, document, jDocument);
        return jDocument;
    }
    XEMJNI_POSTLOG;
    return NULL;
}

JNIEXPORT jobject JNICALL
Java_org_xemeiah_dom_DocumentFactory_newVolatileDocument (JNIEnv *ev, jobject jFactory)
{
    Xem::Store* store = jDocumentFactory2Store(ev, jFactory);
    Xem::Document* document = store->createVolatileDocument();
    document->incrementRefCount();
    Log("Root element at : %llx\n", document->getRootElement().getElementPtr());

    Xem::XProcessor* xprocessor = new Xem::XProcessor(*store);
    xprocessor->installModule("http://www.xemeiah.org/ns/xem");

    jobject jDocument = createJDocument(ev, jFactory, document, xprocessor);
    return jDocument;
}

JNIEXPORT void JNICALL
Java_org_xemeiah_dom_DocumentFactory_releaseDocument (JNIEnv *ev, jobject jFactory, jobject jDocument)
{
    /**
     * Do not perform any cleanup here, maybe only a housewife could occur
     * BUT WE DO NOT WANT TO CORRUPT JAVA-SIDE OBJECTS !!!
     */
    Log("releaseDocument(%p)\n", jDocument);

    // Xem::Document* document = jDocument2Document(ev, jDocument);
    // document->housewife();
}

JNIEXPORT jboolean JNICALL
Java_org_xemeiah_dom_DocumentFactory_isNamespaceAware (JNIEnv *, jobject)
{
    return true;
}

JNIEXPORT jboolean JNICALL
Java_org_xemeiah_dom_DocumentFactory_isValidating (JNIEnv *, jobject)
{
    return false;
}

JNIEXPORT void JNICALL
Java_org_xemeiah_dom_DocumentFactory_process (JNIEnv *ev, jobject jFactory, jobject jElement, jobject jNodeList)
{
    Xem::ElementRef eltRef = jElement2ElementRef(ev, jElement);
    jobject jDocument = jNode2JDocument(ev, jElement);

    Xem::XProcessor * xprocessor = jDocument2XProcessor(ev, jDocument);
    Xem::NodeSet* nodeSet = jNodeList2NodeSet(ev, jNodeList);

    xprocessor->pushEnv();

    Xem::NodeSet::iterator iter(*nodeSet, *xprocessor);

    xprocessor->process(eltRef);

    xprocessor->popEnv();
}
