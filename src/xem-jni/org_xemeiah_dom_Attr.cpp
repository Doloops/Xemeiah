/*
 * org_xemeiah_dom_Document.cpp
 *
 *  Created on: 15 janv. 2010
 *      Author: francois
 */

#include "include/org_xemeiah_dom_Attr.h"
#include <Xemeiah/kern/document.h>
#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/dom/childiterator.h>
#include <Xemeiah/trace.h>
#include <Xemeiah/log.h>

#include "xem-jni-dom.h"

#include <Xemeiah/auto-inline.hpp>

JNIEXPORT jstring JNICALL Java_org_xemeiah_dom_Attr_getName
  (JNIEnv *ev, jobject attrObject)
{
  Xem::AttributeRef attrRef = j2AttributeRef(ev, attrObject);
  return ev->NewStringUTF(attrRef.getKey().c_str());
}

JNIEXPORT jstring JNICALL Java_org_xemeiah_dom_Attr_getValue
  (JNIEnv *ev, jobject attrObject)
{
  Xem::AttributeRef attrRef = j2AttributeRef(ev, attrObject);
  return ev->NewStringUTF(attrRef.toString().c_str());
}

JNIEXPORT jstring JNICALL Java_org_xemeiah_dom_Attr_getNamespaceURI
  (JNIEnv *ev, jobject attrObject)
{
  Xem::AttributeRef attrRef = j2AttributeRef(ev, attrObject);
  return ev->NewStringUTF(attrRef.getKeyCache().getNamespaceURL(attrRef.getNamespaceId()));
}

JNIEXPORT void JNICALL Java_org_xemeiah_dom_Attr_setValue
  (JNIEnv *, jobject, jstring)
{
  NotImplemented ( "." );
}

JNIEXPORT jboolean JNICALL Java_org_xemeiah_dom_Attr_isId
  (JNIEnv *, jobject)
{
  static const jboolean jfalse = false;
  return jfalse;
}
