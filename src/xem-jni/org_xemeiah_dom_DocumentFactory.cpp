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

#include <Xemeiah/auto-inline.hpp>

JNIEXPORT void JNICALL
Java_org_xemeiah_dom_DocumentFactory_initStore (JNIEnv* ev, jobject jFactory, jstring filename)
{
    XemJniInit(ev);

    Info("Init Store (C++) Xemeiah Version '%s'!\n", __XEM_VERSION);
    Info("Sizes : (void*)=%lu, jint=%lu, jlong=%lu\n", sizeof(void*), sizeof(jint), sizeof(jlong));
    jclass factoryClass = ev->GetObjectClass (jFactory);
    jfieldID __storePtrId = ev->GetFieldID (factoryClass, "__storePtr", "J");
    jfieldID __xprocessorPtrId = ev->GetFieldID (factoryClass, "__xprocessorPtr", "J");

    AssertBug ( ev->GetLongField(jFactory,__storePtrId) == 0, "Already initialized !\n" );

    Xem::Store* store = NULL;
    if (filename == NULL)
    {
        store = new Xem::VolatileStore ();
    }
    else
    {
        Xem::PersistentStore* pStore = new Xem::PersistentStore ();

        jboolean isCopy = false;
        const char* cfilename = ev->GetStringUTFChars (filename, &isCopy);

        Info("Opening file '%s'\n", cfilename);
        if (pStore->open (cfilename))
        {
            pStore->dropUncommittedRevisions ();
        }
        else
        {
            Info("Could not open, trying format\n");
            if (!pStore->format (cfilename))
            {
                Info("Could not format !\n");
                return;
            }
            else
            {
                Xem::String mainBranch ("main");
                Xem::BranchFlags branchFlags = 0;
                pStore->getBranchManager ().createBranch (mainBranch, branchFlags);
            }
        }
        ev->ReleaseStringUTFChars (filename, cfilename);

        store = pStore;
    }

    Log("jFactory=%p, Store : %p\n", jFactory, store);
    ev->SetLongField (jFactory, __storePtrId, (jlong) store);

    Xem::XProcessor* xprocessor = new Xem::XProcessor (*store);
    ev->SetLongField (jFactory, __xprocessorPtrId, (long) xprocessor);
}

JNIEXPORT jobject JNICALL
Java_org_xemeiah_dom_DocumentFactory_newStandaloneDocument (JNIEnv *ev, jobject jFactory, jstring branch, jstring flags)
{
    Xem::Store* store = jDocumentFactory2Store (ev, jFactory);

    jboolean isCopy = false;
    const char* cBranch = ev->GetStringUTFChars (branch, &isCopy);
    const char* cFlags = ev->GetStringUTFChars (flags, &isCopy);
    Xem::Document* document = store->getBranchManager ().openDocument (
            cBranch, cFlags, store->getKeyCache ().getBuiltinKeys ().nons.none ());
    ev->ReleaseStringUTFChars (branch, cBranch);
    ev->ReleaseStringUTFChars (flags, cFlags);
    document->incrementRefCount ();

    jobject jDocument = document2JDocument (ev, jFactory, document);

    Log("DocumentFactory : jFactory=%p, new document=%p, jDocument=%p\n", jFactory, document, jDocument);
    return jDocument;
}

JNIEXPORT jobject JNICALL
Java_org_xemeiah_dom_DocumentFactory_newVolatileDocument (JNIEnv *ev, jobject jFactory)
{
    Xem::Store* store = jDocumentFactory2Store (ev, jFactory);
    Xem::Document* document = store->createVolatileDocument ();
    document->incrementRefCount ();
    jobject jDocument = document2JDocument (ev, jFactory, document);
    return jDocument;
}

JNIEXPORT void JNICALL
Java_org_xemeiah_dom_DocumentFactory_releaseDocument (JNIEnv *ev, jobject jFactory, jobject jDocument)
{
    Log("releaseDocument(%p)\n", jDocument);

    Xem::Document* doc = jDocument2Document (ev, jDocument);

    Xem::Store* store = jDocumentFactory2Store (ev, jFactory);

    store->releaseDocument (doc);
    store->housewife ();
    Log("releaseDocument(%p) : done.\n", jDocument);

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

