/*
 * org_xemeiah_dom_Document.cpp
 *
 *  Created on: 15 janv. 2010
 *      Author: francois
 */

#include "include/org_xemeiah_dom_Element.h"
#include <Xemeiah/kern/document.h>
#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/dom/childiterator.h>
#include <Xemeiah/trace.h>
#include <Xemeiah/log.h>

#include "xem-jni-dom.h"

#include <Xemeiah/auto-inline.hpp>

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
    Xem::Integer childrenNumber = eltRef.getNumberOfChildren();

    jlongArray nodesArray = ev->NewLongArray(childrenNumber);

    jboolean isCopy = false;
    jlong* nodes = ev->GetLongArrayElements(nodesArray, &isCopy);

    int index = 0;
    for (Xem::ChildIterator iter(eltRef); iter; iter++)
    {
        nodes[index++] = iter.getElementPtr();
    }
    ev->SetLongArrayRegion(nodesArray, 0, childrenNumber, nodes);

    jclass nodeListClass = ev->FindClass("org/xemeiah/dom/NodeList");
    jmethodID nodeListConstructorId = ev->GetMethodID(nodeListClass, "<init>", "(Lorg/xemeiah/dom/Document;[J[J)V");

    jlongArray attrsArray = NULL;

    jobject nodeListObject = ev->NewObject(nodeListClass, nodeListConstructorId, documentObject, nodesArray,
                                           attrsArray);

    return nodeListObject;
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
        return attributeRef2J(ev, jDocument, attrRef);
    }
    return NULL;
}

JNIEXPORT jobject JNICALL
Java_org_xemeiah_dom_Element_getAttributes (JNIEnv *ev, jobject elementObject)
{
    jobject documentObject = jNode2JDocument(ev, elementObject);
    Xem::ElementRef eltRef = jElement2ElementRef(ev, elementObject);

    Xem::Integer attrsNumber = 0;
    for (Xem::AttributeRef attrRef = eltRef.getFirstAttr(); attrRef; attrRef = attrRef.getNext())
    {
        // Log ( "At %s\n", attrRef.generateVersatileXPath().c_str() );
        if (attrRef.getAttributeType() != Xem::AttributeType_NamespaceAlias && !attrRef.isBaseType())
            continue;
        attrsNumber++;
    }

    jlongArray eltsArray = ev->NewLongArray(attrsNumber);
    jlongArray attrsArray = ev->NewLongArray(attrsNumber);

    jboolean isCopy = false;
    jlong* elts = ev->GetLongArrayElements(eltsArray, &isCopy);
    jlong* attrs = ev->GetLongArrayElements(attrsArray, &isCopy);

    int index = 0;
    for (Xem::AttributeRef attrRef = eltRef.getFirstAttr(); attrRef; attrRef = attrRef.getNext())
    {
        if (attrRef.getAttributeType() != Xem::AttributeType_NamespaceAlias && !attrRef.isBaseType())
            continue;
        elts[index] = eltRef.getElementPtr();
        attrs[index] = attrRef.getAttributePtr();
        index++;
    }
    ev->SetLongArrayRegion(eltsArray, 0, attrsNumber, elts);
    ev->SetLongArrayRegion(attrsArray, 0, attrsNumber, attrs);

    jclass namedNodeMapClass = ev->FindClass("org/xemeiah/dom/NamedNodeMap");
    jmethodID namedNodeMapConstructorId = ev->GetMethodID(namedNodeMapClass, "<init>",
                                                          "(Lorg/xemeiah/dom/Document;[J[J)V");

    jobject namedNodeMapObject = ev->NewObject(namedNodeMapClass, namedNodeMapConstructorId, documentObject, eltsArray,
                                               attrsArray);

    return namedNodeMapObject;
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
    Xem::ElementRef father = jElement2ElementRef(ev, jElement);
    Xem::ElementRef child = jElement2ElementRef(ev, jChildNode);

    father.appendChild(child);

    return jChildNode;
}
