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

JNIEXPORT void JNICALL Java_org_xemeiah_dom_DocumentFactory_openVolatile
  (JNIEnv *ev, jobject jFactory)
{
    Xem::Store* store = new Xem::VolatileStore();
    initDocumentFactory(ev, jFactory, store);
}

JNIEXPORT void JNICALL
Java_org_xemeiah_dom_DocumentFactory_format (JNIEnv *ev, jobject jFactory, jstring jFilename)
{
    Xem::PersistentStore* persistentStore = new Xem::PersistentStore();

    jboolean isCopy = false;
    const char* cfilename = ev->GetStringUTFChars(jFilename, &isCopy);

    if (!persistentStore->format(cfilename))
    {
        Error("Could not format !\n");
        ev->ThrowNew(getXemJNI().javaLangRuntimeException.getClass(ev), "Could not format file !");
    }
    ev->ReleaseStringUTFChars(jFilename, cfilename);
    delete (persistentStore);
}

JNIEXPORT void JNICALL
Java_org_xemeiah_dom_DocumentFactory_open (JNIEnv *ev, jobject jFactory, jstring jFilename)
{
    jboolean isCopy = false;
    const char* cfilename = ev->GetStringUTFChars(jFilename, &isCopy);

    Xem::PersistentStore* persistentStore = new Xem::PersistentStore();

    if (!persistentStore->open(cfilename))
    {
        Error("Could not open file %s\n", cfilename);
        ev->ReleaseStringUTFChars(jFilename, cfilename);
        ev->ThrowNew(getXemJNI().javaLangRuntimeException.getClass(ev), "Could not open file !");
        delete (persistentStore);
        return;
    }
    ev->ReleaseStringUTFChars(jFilename, cfilename);
    persistentStore->dropUncommittedRevisions();

//    Xem::XProcessor* xprocessor = new Xem::XProcessor(*persistentStore);
//    xprocessor->installModule("http://www.xemeiah.org/ns/xem");

    initDocumentFactory(ev, jFactory, persistentStore);
}

JNIEXPORT void JNICALL
Java_org_xemeiah_dom_DocumentFactory_createBranch (JNIEnv *ev, jobject jFactory, jstring jBranchName,
                                                   jstring jBranchFlags)
{
    Xem::Store* store = jDocumentFactory2Store(ev, jFactory);

    Xem::String branchName = jstring2XemString(ev, jBranchName);
    Xem::BranchFlags branchFlags = 0;
    store->getBranchManager().createBranch(branchName, branchFlags);
}

JNIEXPORT void JNICALL
Java_org_xemeiah_dom_DocumentFactory_close (JNIEnv *ev, jobject jFactory)
{
    Xem::Store* store = jDocumentFactory2Store(ev, jFactory);

    Xem::PersistentStore* pStore = dynamic_cast<Xem::PersistentStore*>(store);
    if (pStore)
    {
        Log("Closing PersistentStore at %p\n", pStore);
        pStore->close();
    }
    delete (store);

    initDocumentFactory(ev, jFactory, NULL);
}

JNIEXPORT jobject JNICALL
Java_org_xemeiah_dom_DocumentFactory_newStandaloneDocument (JNIEnv *ev, jobject jFactory, jstring jBranchName,
                                                            jstring jBranchFlags)
{
    Xem::Store* store = jDocumentFactory2Store(ev, jFactory);

    jboolean isCopy = false;
    const char* cBranch = ev->GetStringUTFChars(jBranchName, &isCopy);
    const char* cFlags = ev->GetStringUTFChars(jBranchFlags, &isCopy);

    Xem::Document* document = NULL;
    try
    {
        Xem::KeyId roleId = store->getKeyCache().getBuiltinKeys().nons.none();
        document = store->getBranchManager().openDocument(cBranch, cFlags, roleId);
    }
    catch (Xem::Exception* e)
    {
        ev->ReleaseStringUTFChars(jBranchName, cBranch);
        ev->ReleaseStringUTFChars(jBranchFlags, cFlags);
        ev->ThrowNew(getXemJNI().javaLangRuntimeException.getClass(ev), "Could not open document !");
        return NULL;
    }
    ev->ReleaseStringUTFChars(jBranchName, cBranch);
    ev->ReleaseStringUTFChars(jBranchFlags, cFlags);
    document->incrementRefCount();

    Xem::XProcessor* xprocessor = new Xem::XProcessor(*store);
    xprocessor->installModule("http://www.xemeiah.org/ns/xem");

    jobject jDocument = createJDocument(ev, jFactory, document, xprocessor);

    Log("DocumentFactory : jFactory=%p, new document=%p, jDocument=%p\n", jFactory, document, jDocument);
    return jDocument;
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

    Xem::Document* document = jDocument2Document(ev, jDocument);
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
