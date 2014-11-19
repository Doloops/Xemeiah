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

JNIEXPORT void JNICALL Java_org_xemeiah_dom_xpath_XPathExpression_cleanUp
  (JNIEnv *ev, jobject jXPathExpression)
{
    AssertBug(jXPathExpression, "Null XPathExpression object provided !");
    Xem::XPath* xpath = jXPathExpression2XPath(ev, jXPathExpression);
    AssertBug(xpath, "Null xpath provided !");

    Log("Delete xpath=%p\n", xpath);
    delete(xpath);
}

JNIEXPORT jobject JNICALL
Java_org_xemeiah_dom_xpath_XPathExpression_evaluate (JNIEnv *ev, jobject jXPathExpression, jobject jContextNode,
                                                     jshort type, jobject resultObject)
{
    AssertBug(jXPathExpression, "Null XPathExpression object provided !");
    Xem::XPath* xpath = jXPathExpression2XPath(ev, jXPathExpression);
    AssertBug(xpath, "Null xpath provided !");
    jobject jDocument = jNode2JDocument(ev, jContextNode);

    Log("jContextNode=%p, jDocument=%p, doc=%p\n", jContextNode, jDocument, jDocument2Document(ev, jDocument));

    Xem::ElementRef contextNode = jElement2ElementRef(ev, jContextNode);

    Xem::NodeSet* result = new Xem::NodeSet();
    xpath->eval(*result, contextNode);

    Log("jContextNode=%p, jDocument=%p, doc=%p, contextNode->getDocument()=%p\n", jContextNode, jDocument,
        jDocument2Document(ev, jDocument), &(contextNode.getDocument()));
    return nodeSet2JNodeList(ev, jDocument, result);
}
