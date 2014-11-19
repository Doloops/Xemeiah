/*
 * org_xemeiah_dom_Store.cpp
 *
 *  Created on: 15 janv. 2010
 *      Author: francois
 */

#include "include/org_xemeiah_dom_xpath_XPathExpression.h"
#include <Xemeiah/trace.h>
#include <Xemeiah/log.h>
#include <Xemeiah/xpath/xpath.h>
#include <Xemeiah/xprocessor/xprocessor.h>

#include "xem-jni-dom.h"

#include <Xemeiah/auto-inline.hpp>

JNIEXPORT jobject JNICALL
Java_org_xemeiah_dom_xpath_XPathExpression_evaluate (JNIEnv *ev, jobject jXPathExpression, jobject jContextNode,
                                                     jshort type, jobject resultObject)
{
    jobject jDocument = jNode2JDocument(ev, jContextNode);

    Log("jContextNode=%p, jDocument=%p, doc=%p\n", jContextNode, jDocument, jDocument2Document(ev, jDocument));

    Xem::ElementRef contextNode = jElement2ElementRef(ev, jContextNode);
    //   Xem::XProcessor xproc(contextNode.getDocument().getStore());

    jobject jFactory = jDocument2JDocumentFactory(ev, jDocument);
    // Xem::XProcessor* xprocessor = jDocumentFactory2XProcessor(ev, jFactory);

    Xem::XPath* xpath = jXPathExpression2XPath(ev, jXPathExpression);

    Xem::NodeSet* result = new Xem::NodeSet();
    xpath->eval(*result, contextNode);

    Log("jContextNode=%p, jDocument=%p, doc=%p, contextNode->getDocument()=%p\n", jContextNode, jDocument,
        jDocument2Document(ev, jDocument), &(contextNode.getDocument()));
    return nodeSet2JNodeList(ev, jDocument, result);

//    jobject parsedBytesObject = jXPathExpression2ParsedBytes(ev, jXPathExpression);
//    jlongArray parsedBytesArray = (jlongArray) (parsedBytesObject);
//
//    Log("evaluate : parsedBytesArray at %p, length=%i\n", parsedBytesArray, ev->GetArrayLength(parsedBytesArray));
//
//    jboolean isCopy = false;
//    jlong* parsedBytes = ev->GetLongArrayElements(parsedBytesArray, &isCopy);
//
//    Log("evaluate : parsedBytes at %p\n", parsedBytes);
//    Xem::XPathSegment* xpathSegment = (Xem::XPathSegment*) (parsedBytes);
//    Xem::XPath xpath(xproc, xpathSegment);
//
//    Xem::NodeSet* result = new Xem::NodeSet();
//    xpath.eval(*result, contextNode);
//
//    ev->ReleaseLongArrayElements((jlongArray) parsedBytesObject, parsedBytes, JNI_ABORT);
//
//    Log("evaluated : result=%ld\n", (long ) result->size());
//    Log("jContextNode=%p, jDocument=%p, doc=%p, contextNode->getDocument()=%p\n", jContextNode, jDocument, jDocument2Document(ev, jDocument), &(contextNode.getDocument()));
//    return nodeSet2JNodeList(ev, jDocument, result);
}
