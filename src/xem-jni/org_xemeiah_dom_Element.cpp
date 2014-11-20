/*
 * org_xemeiah_dom_Document.cpp
 *
 *  Created on: 15 janv. 2010
 *      Author: francois
 */

#include "include/org_xemeiah_dom_Element.h"
#include <Xemeiah/kern/document.h>
#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/dom/nodeset.h>
#include <Xemeiah/dom/childiterator.h>
#include <Xemeiah/trace.h>
#include <Xemeiah/log.h>

#include "xem-jni-dom.h"

#include <Xemeiah/auto-inline.hpp>

#undef Log
#define Log(...) do{}while(0)

JNIEXPORT jstring JNICALL
Java_org_xemeiah_dom_Element_getNodeName (JNIEnv *ev, jobject elementObject)
{
    Xem::ElementRef eltRef = jElement2ElementRef(ev, elementObject);
    // Log ( "eltRef %llx, at %s\n", eltRef.getElementPtr(), eltRef.generateVersatileXPath().c_str() );
    return ev->NewStringUTF(eltRef.getKey().c_str());
}

JNIEXPORT jstring JNICALL
Java_org_xemeiah_dom_Element_getNamespaceURI (JNIEnv *ev, jobject elementObject)
{
    Xem::ElementRef eltRef = jElement2ElementRef(ev, elementObject);
    Xem::KeyCache& keyCache = eltRef.getKeyCache();
    return ev->NewStringUTF(keyCache.getNamespaceURL(eltRef.getNamespaceId()));
}

JNIEXPORT jstring JNICALL
Java_org_xemeiah_dom_Element_getLocalName (JNIEnv *ev, jobject elementObject)
{
    Xem::ElementRef eltRef = jElement2ElementRef(ev, elementObject);
    Xem::KeyCache& keyCache = eltRef.getKeyCache();
    return ev->NewStringUTF(keyCache.getLocalKey(keyCache.getLocalKeyId(eltRef.getKeyId())));
}

JNIEXPORT jobject JNICALL
Java_org_xemeiah_dom_Element_getFirstChild (JNIEnv *ev, jobject elementObject)
{
    Xem::ElementRef eltRef = jElement2ElementRef(ev, elementObject);
    Xem::ElementRef childRef = eltRef.getChild();

    if ( ! childRef )
    {
        return NULL;
    }

    jobject documentObject = jNode2JDocument(ev, elementObject);
    return elementRef2JElement(ev, documentObject, childRef);
}

JNIEXPORT jobject JNICALL
Java_org_xemeiah_dom_Element_getLastChild (JNIEnv *ev, jobject jElement)
{
    Log("Calling getLastChild() on jElement=%p\n", jElement);
    Xem::ElementRef eltRef = jElement2ElementRef(ev, jElement);
    Log("eltRef at %llx\n", eltRef.getElementPtr());

    Xem::ElementRef childRef = eltRef.getLastChild();

    if ( ! childRef )
    {
        return NULL;
    }

    jobject jDocument = jNode2JDocument(ev, jElement);
    jobject jElementChild = elementRef2JElement(ev, jDocument, childRef);

    Log("Called getLastChild() on jElement=%p, jDocument=%p, jElementChild=%p\n", jElement, jDocument, jElementChild);
    return jElementChild;
}

JNIEXPORT jobject JNICALL
Java_org_xemeiah_dom_Element_getChildNodes (JNIEnv *ev, jobject elementObject)
{
    jobject documentObject = jNode2JDocument(ev, elementObject);

    Xem::ElementRef eltRef = jElement2ElementRef(ev, elementObject);

    Xem::NodeSet* nodeSet = new Xem::NodeSet();

    for (Xem::ChildIterator iter(eltRef); iter; iter++)
    {
        nodeSet->pushBack(iter.toElement());
    }
    return nodeSet2JNodeList(ev, documentObject, nodeSet);
}

JNIEXPORT jstring JNICALL
Java_org_xemeiah_dom_Element_getAttributeNS (JNIEnv *ev, jobject elementObject, jstring jNamespace, jstring jName)
{
    Xem::ElementRef eltRef = jElement2ElementRef(ev, elementObject);
    Xem::KeyId keyId = j2KeyId(ev, eltRef.getKeyCache(), jNamespace, jName);
    Xem::AttributeRef attrRef = eltRef.findAttr(keyId);
    if (attrRef)
        return ev->NewStringUTF(attrRef.toString().c_str());
    return NULL;
}

JNIEXPORT jobject JNICALL
Java_org_xemeiah_dom_Element_getAttributeNodeNS (JNIEnv *ev, jobject jElement, jstring jNamespace, jstring jName)
{
    jobject jDocument = jNode2JDocument(ev, jElement);

    Xem::ElementRef eltRef = jElement2ElementRef(ev, jElement);
    Xem::KeyId keyId = j2KeyId(ev, eltRef.getKeyCache(), jNamespace, jName);
    Xem::AttributeRef attrRef = eltRef.findAttr(keyId);
    if (attrRef)
    {
        return attributeRef2JAttribute(ev, jDocument, attrRef);
    }
    return NULL;
}

JNIEXPORT jobject JNICALL
Java_org_xemeiah_dom_Element_getAttributes (JNIEnv *ev, jobject jElement)
{
    jobject jDocument = jNode2JDocument(ev, jElement);
    Xem::ElementRef eltRef = jElement2ElementRef(ev, jElement);

    Xem::NodeSet* nodeSet = new Xem::NodeSet();

    for (Xem::AttributeRef attrRef = eltRef.getFirstAttr(); attrRef; attrRef = attrRef.getNext())
    {
        if (attrRef.getAttributeType() != Xem::AttributeType_NamespaceAlias && !attrRef.isBaseType())
            continue;
        nodeSet->pushBack(attrRef);
    }

    return nodeSet2JNamedNodeMap(ev, jDocument, nodeSet);
}

JNIEXPORT void JNICALL
Java_org_xemeiah_dom_Element_setAttributeNS (JNIEnv *ev, jobject jElement, jstring jNamespaceUri, jstring jKey,
                                             jstring jValue)
{
    Xem::ElementRef element = jElement2ElementRef(ev, jElement);
    Xem::KeyId keyId = j2KeyId(ev, element.getKeyCache(), jNamespaceUri, jKey);

    jboolean isCopy = false;
    const char* value = ev->GetStringUTFChars(jValue, &isCopy);

    element.addAttr(keyId, value);

    ev->ReleaseStringUTFChars(jValue, value);
}

JNIEXPORT jobject JNICALL
Java_org_xemeiah_dom_Element_appendChild (JNIEnv *ev, jobject jElement, jobject jChildNode)
{
    AssertBug(jElement != NULL, "Null jElement !\n");
    AssertBug(jChildNode != NULL, "Null child !\n");
    Xem::ElementRef father = jElement2ElementRef(ev, jElement);
    Xem::ElementRef child = jElement2ElementRef(ev, jChildNode);

    father.appendChild(child);

    return jChildNode;
}

JNIEXPORT void JNICALL Java_org_xemeiah_dom_Element_triggerElementEnd
  (JNIEnv *ev, jobject jElement)
{
    Xem::ElementRef me = jElement2ElementRef(ev, jElement);

    jobject jDocument = jNode2JDocument(ev, jElement);
    jobject jDocumentFactory = jDocument2JDocumentFactory(ev, jDocument);
    Xem::XProcessor* xprocessor = jDocumentFactory2XProcessor(ev, jDocumentFactory);

    Log("Trigger event : %s\n", me.generateVersatileXPath().c_str());

    me.eventElement(*xprocessor, DomEventType_CreateElement);
}
