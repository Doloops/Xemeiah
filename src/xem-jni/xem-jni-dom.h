/*
 * xem-jni.h
 *
 *  Created on: 26 janv. 2010
 *      Author: francois
 */

#ifndef XEMJNI_H_
#define XEMJNI_H_

#include <Xemeiah/kern/document.h>
#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/dom/attributeref.h>
#include <Xemeiah/xpath/xpath.h>
#include <Xemeiah/kern/exception.h>

#include "xem-jni-classes.h"

#include <jni.h>

void
XemJniInit (JNIEnv* ev);

Xem::String
jstring2XemString (JNIEnv* ev, jstring js);

jobject
jDocument2JDocumentFactory (JNIEnv* ev, jobject jDocument);

jobject
jXPathEvaluator2JDocumentFactory (JNIEnv* ev, jobject jXPathEvaluator);

Xem::XProcessor*
jDocument2XProcessor (JNIEnv* ev, jobject jDocument);

jobject
elementRef2JElement (JNIEnv* ev, jobject documentObject, Xem::ElementRef& eltRef);
jobject
attributeRef2JAttribute (JNIEnv* ev, jobject documentObject, Xem::AttributeRef& attrRef);

jobject
jNode2JDocument (JNIEnv* ev, jobject nodeObject);
jobject
jNodeList2JDocument (JNIEnv* ev, jobject nodeObject);

jobject
createJDocument (JNIEnv* ev, jobject jFactory, Xem::Document* document, Xem::XProcessor* xprocessor);

void
cleanupJDocument (JNIEnv *ev, jobject jDocument);

Xem::Store*
jDocumentFactory2Store (JNIEnv* ev, jobject jFactory);

void
initDocumentFactory (JNIEnv* ev, jobject jFactory, Xem::Store* store);

Xem::Document*
jDocument2Document (JNIEnv* ev, jobject documentObject);
Xem::ElementRef
jElement2ElementRef (JNIEnv* ev, jobject jElement);

bool
isJElement (JNIEnv* ev, jobject jElement);

Xem::AttributeRef
j2AttributeRef (JNIEnv* ev, jobject elementObject);

Xem::KeyId
j2KeyId (JNIEnv* ev, Xem::KeyCache& keyCache, jstring jNamespace, jstring jName);

jobject
nodeSet2JNodeList (JNIEnv* ev, jobject documentObject, Xem::NodeSet* result);

jobject
nodeSet2JNamedNodeMap (JNIEnv* ev, jobject documentObject, Xem::NodeSet* result);

Xem::NodeSet*
jNodeList2NodeSet (JNIEnv* ev, jobject jNodeList);

jobject
createJXPathExpression (JNIEnv* ev, Xem::XPathParser* xpathParser, jobject jFactory);

Xem::XPathParser*
jXPathExpression2XPathParser (JNIEnv* ev, jobject jXPathExpression);

jobject
jXPathExpression2JDocumentFactory (JNIEnv* ev, jobject jXPathExpression);

jthrowable
exception2JXPathException (JNIEnv* ev, Xem::Exception* exception);

jthrowable
exception2JDOMException (JNIEnv* ev, const char* message);

jthrowable
exception2JRuntimeException (JNIEnv* ev, Xem::Exception* exception);

jthrowable
exception2JDOMException (JNIEnv* ev, Xem::Exception* exception);

Xem::String
jLookupNamespaceURI (JNIEnv* ev, jobject jNSResolver, Xem::String nsPrefix);

extern XemJNI xemJNI;

static INLINE
XemJNI&
getXemJNI ()
{
    return xemJNI;
}

#define XEMJNI_PROLOG try
#define XEMJNI_CATCH_CLAUSE catch ( Xem::Exception* e )

#define XEMJNI_THROW_RUNTIME ev->Throw(exception2JRuntimeException(ev, e));
#define XEMJNI_POSTLOG XEMJNI_CATCH_CLAUSE { XEMJNI_THROW_RUNTIME } do{} while(0)

#define XEMJNI_POSTLOG_OPS(...) XEMJNI_CATCH_CLAUSE { __VA_ARGS__ ; XEMJNI_THROW_RUNTIME } do{} while(0)
#endif /* XEMJNI_H_ */
