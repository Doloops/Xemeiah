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

#if 0
#undef Log
#define Log(...) do{}while(0)
#endif

JNIEXPORT jstring JNICALL
Java_org_xemeiah_dom_Element_getNodeName (JNIEnv *ev, jobject elementObject)
{
    Log("getNodeName ev=%p, elementObject=%p\n", ev, elementObject);
    Xem::ElementRef eltRef = jElement2ElementRef(ev, elementObject);
    return ev->NewStringUTF(eltRef.getKey().c_str());
}

JNIEXPORT jstring JNICALL
Java_org_xemeiah_dom_Element_getNamespaceURI (JNIEnv *ev, jobject elementObject)
{
    Log("getNamespaceURI  ev=%p, elementObject=%p\n", ev, elementObject);
    Xem::ElementRef eltRef = jElement2ElementRef(ev, elementObject);
    Xem::KeyCache& keyCache = eltRef.getKeyCache();
    return ev->NewStringUTF(keyCache.getNamespaceURL(eltRef.getNamespaceId()));
}

JNIEXPORT jstring JNICALL
Java_org_xemeiah_dom_Element_getLocalName (JNIEnv *ev, jobject elementObject)
{
    Log("getLocalName  ev=%p, elementObject=%p\n", ev, elementObject);
    Xem::ElementRef eltRef = jElement2ElementRef(ev, elementObject);
    Xem::KeyCache& keyCache = eltRef.getKeyCache();
    return ev->NewStringUTF(keyCache.getLocalKey(keyCache.getLocalKeyId(eltRef.getKeyId())));
}

JNIEXPORT jobject JNICALL
Java_org_xemeiah_dom_Element_getFirstChild (JNIEnv *ev, jobject jElement)
{
    Log("getFirstChild ev=%p, elementObject=%p\n", ev, jElement);

    Xem::ElementRef eltRef = jElement2ElementRef(ev, jElement);
    Xem::ElementRef childRef = eltRef.getChild();

    if ( ! childRef )
    {
        return NULL;
    }

    jobject documentObject = jNode2JDocument(ev, jElement);
    return elementRef2JElement(ev, documentObject, childRef);
}

JNIEXPORT jobject JNICALL
Java_org_xemeiah_dom_Element_getLastChild (JNIEnv *ev, jobject jElement)
{
    Log("getLastChild ev=%p, elementObject=%p\n", ev, jElement);
    Xem::ElementRef eltRef = jElement2ElementRef(ev, jElement);

    Xem::ElementRef childRef = eltRef.getLastChild();

    if ( ! childRef )
    {
        return NULL;
    }

    jobject jDocument = jNode2JDocument(ev, jElement);
    return elementRef2JElement(ev, jDocument, childRef);
}

JNIEXPORT jobject JNICALL
Java_org_xemeiah_dom_Element_getChildNodes (JNIEnv *ev, jobject jElement)
{
    Log("getChildNodes ev=%p, jElement=%p\n", ev, jElement);

    jobject documentObject = jNode2JDocument(ev, jElement);

    Xem::ElementRef eltRef = jElement2ElementRef(ev, jElement);

    Xem::NodeSet* nodeSet = new Xem::NodeSet();

    for (Xem::ChildIterator iter(eltRef); iter; iter++)
    {
        nodeSet->pushBack(iter.toElement());
    }
    return nodeSet2JNodeList(ev, documentObject, nodeSet);
}

JNIEXPORT jstring JNICALL
Java_org_xemeiah_dom_Element_getAttributeNS (JNIEnv *ev, jobject jElement, jstring jNamespace, jstring jName)
{
    Log("getAttributeNS ev=%p, jElement=%p\n", ev, jElement);

    Xem::ElementRef eltRef = jElement2ElementRef(ev, jElement);
    Xem::KeyId keyId = j2KeyId(ev, eltRef.getKeyCache(), jNamespace, jName);
    Xem::AttributeRef attrRef = eltRef.findAttr(keyId);
    if (attrRef)
        return ev->NewStringUTF(attrRef.toString().c_str());
    return NULL;
}

JNIEXPORT jobject JNICALL
Java_org_xemeiah_dom_Element_getAttributeNodeNS (JNIEnv *ev, jobject jElement, jstring jNamespace, jstring jName)
{
    Log("getAttributeNodeNS ev=%p, jElement=%p\n", ev, jElement);

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
    Log("getAttributes ev=%p, jElement=%p\n", ev, jElement);

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
    Log("setAttributeNS ev=%p, jElement=%p\n", ev, jElement);

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
    Log("appendChild ev=%p, jElement=%p\n", ev, jElement);

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
    Log("tiggerElementEnd ev=%p, jElement=%p\n", ev, jElement);

    Xem::ElementRef me = jElement2ElementRef(ev, jElement);

    jobject jDocument = jNode2JDocument(ev, jElement);
    Xem::XProcessor* xprocessor = jDocument2XProcessor(ev, jDocument);

    Log("Trigger event : %s\n", me.generateVersatileXPath().c_str());

    me.eventElement(*xprocessor, DomEventType_CreateElement);
}
