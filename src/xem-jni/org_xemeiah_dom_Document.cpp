/*
 * org_xemeiah_dom_Document.cpp
 *
 *  Created on: 15 janv. 2010
 *      Author: francois
 */

#include "include/org_xemeiah_dom_Document.h"
#include <Xemeiah/kern/document.h>
#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/trace.h>
#include <Xemeiah/log.h>
#include "xem-jni-dom.h"
#include "xem-jni-classes.h"

#include <Xemeiah/auto-inline.hpp>

JNIEXPORT void JNICALL
Java_org_xemeiah_dom_Document_cleanUp (JNIEnv *ev, jobject jDocument)
{
    Log("At jDocument=%p, cleanUp\n", jDocument);
    cleanupJDocument(ev, jDocument);
}

JNIEXPORT void JNICALL
Java_org_xemeiah_dom_Document_commit (JNIEnv *ev, jobject jDocument)
{
    Xem::Document* doc = jDocument2Document(ev, jDocument);
    Xem::XProcessor* xprocessor = jDocument2XProcessor(ev, jDocument);

    Log("jDocument at %p, document at %p, xprocessor at %p\n", jDocument, doc, xprocessor);

    Log("Document : %s : commit()\n", doc->getDocumentTag().c_str());
    Xem::TransactionalDocument* transactionalDoc = dynamic_cast<Xem::TransactionalDocument*>(doc);
    if (transactionalDoc != NULL)
    {
        transactionalDoc->commit(*xprocessor);
    }
    else
    {
        Log("Document is not transactional !");
    }
}

JNIEXPORT void JNICALL
Java_org_xemeiah_dom_Document_reopen (JNIEnv *ev, jobject jDocument)
{
    Xem::Document* doc = jDocument2Document(ev, jDocument);
    Xem::XProcessor* xprocessor = jDocument2XProcessor(ev, jDocument);

    Log("Document : %s : commit()\n", doc->getDocumentTag().c_str());

    Xem::TransactionalDocument* transactionalDoc = dynamic_cast<Xem::TransactionalDocument*>(doc);
    if (transactionalDoc != NULL)
    {
        transactionalDoc->reopen(*xprocessor);
    }
    else
    {
        Log("Document is not transactional !");
    }
}

JNIEXPORT jstring JNICALL
Java_org_xemeiah_dom_Document_toString (JNIEnv *ev, jobject documentObject)
{
    Xem::Document* doc = jDocument2Document(ev, documentObject);

    Log("Document : %s\n", doc->getDocumentTag().c_str());

    Xem::String str = "Xem::Document : ";
    str += doc->getDocumentURI();
    return ev->NewStringUTF(str.c_str());
}

JNIEXPORT jstring JNICALL
Java_org_xemeiah_dom_Document_getBaseURI (JNIEnv *ev, jobject jDocument)
{
    Xem::Document* doc = jDocument2Document(ev, jDocument);
    return ev->NewStringUTF(doc->getDocumentURI().c_str());
}

JNIEXPORT jobject JNICALL
Java_org_xemeiah_dom_Document_getDocumentElement (JNIEnv * ev, jobject jDocument)
{
    Xem::Document* doc = jDocument2Document(ev, jDocument);
    Xem::ElementRef rootElement = doc->getRootElement();
    return elementRef2JElement(ev, jDocument, rootElement);
}

JNIEXPORT jobject JNICALL
Java_org_xemeiah_dom_Document_createElementNS (JNIEnv *ev, jobject jDocument, jstring jNamespaceUri, jstring jKey)
{
    Xem::Document* doc = jDocument2Document(ev, jDocument);
    jboolean isCopy = false;

    Xem::NamespaceId nsId = 0;
    if (jNamespaceUri != NULL)
    {
        const char* ns = ev->GetStringUTFChars(jNamespaceUri, &isCopy);
        nsId = doc->getKeyCache().getNamespaceId(ns);
        ev->ReleaseStringUTFChars(jNamespaceUri, ns);
    }
    const char* key = ev->GetStringUTFChars(jKey, &isCopy);
    Xem::KeyId keyId = doc->getKeyCache().getKeyId(nsId, key, true);

    Xem::ElementRef nullRef = Xem::ElementRef(*doc);
    Xem::ElementRef newElement = doc->createElement(nullRef, keyId);

    Log("Created element at %llx\n", newElement.getElementPtr());

    jobject jElement = elementRef2JElement(ev, jDocument, newElement);

    ev->ReleaseStringUTFChars(jKey, key);

    return jElement;
}

JNIEXPORT jobject JNICALL
Java_org_xemeiah_dom_Document_createTextNode (JNIEnv *ev, jobject jDocument, jstring jText)
{
    Xem::Document* doc = jDocument2Document(ev, jDocument);
    Xem::ElementRef nullRef = Xem::ElementRef(*doc);

    Xem::String xemText = jstring2XemString(ev, jText);
    try
    {
        Xem::ElementRef newElement = doc->createTextNode(nullRef, xemText);
        return elementRef2JElement(ev, jDocument, newElement);
    }
    catch (Xem::Exception* e)
    {
        ev->Throw(exception2JDOMException(ev, e));
    }
    return NULL;
}

JNIEXPORT jobject JNICALL
Java_org_xemeiah_dom_Document_createAttributeNS (JNIEnv *ev, jobject jDocument, jstring jNamespaceUri, jstring jKey)
{
#if 0
    Xem::Document* doc = jDocument2Document (ev, jDocument);
    jboolean isCopy = false;
    const char* ns = ev->GetStringUTFChars(jNamespaceUri, &isCopy);
    Xem::NamespaceId nsId = doc->getKeyCache().getNamespaceId(ns);

    const char* key = ev->GetStringUTFChars(jKey, &isCopy);
    Xem::KeyId keyId = doc->getKeyCache().getKeyId(nsId, key, true);

    Xem::ElementRef nullRef = Xem::ElementRef(*doc);

#endif
    ev->Throw(exception2JDOMException(ev, "Not implemented : Document:createAttributeNS()"));
    return NULL;
}
