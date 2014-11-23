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
#include "xem-jni-classes.h"

#include <Xemeiah/auto-inline.hpp>

JNIEXPORT void JNICALL
Java_org_xemeiah_dom_xpath_XPathExpression_cleanUp (JNIEnv *ev, jobject jXPathExpression)
{
    AssertBug(jXPathExpression, "Null XPathExpression object provided !");
    Xem::XPath* xpath = jXPathExpression2XPath(ev, jXPathExpression);
    AssertBug(xpath, "Null xpath provided !");

    Log("Delete xpath=%p\n", xpath);
    delete (xpath);
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
    try
    {
        xpath->eval(*result, contextNode);

        Log("jContextNode=%p, jDocument=%p, doc=%p, contextNode->getDocument()=%p\n", jContextNode, jDocument,
            jDocument2Document(ev, jDocument), &(contextNode.getDocument()));
        return nodeSet2JNodeList(ev, jDocument, result);
    }
    catch (Xem::Exception *e)
    {
        delete(result);
        ev->Throw(exception2JXPathException(ev, e));
    }
}

JNIEXPORT void JNICALL
Java_org_xemeiah_dom_xpath_XPathExpression_pushEnv (JNIEnv *ev, jobject jXPathExpression)
{
    Xem::XPath* xpath = jXPathExpression2XPath(ev, jXPathExpression);
    xpath->getXProcessor().pushEnv();
}

JNIEXPORT void JNICALL
Java_org_xemeiah_dom_xpath_XPathExpression_popEnv (JNIEnv *ev, jobject jXPathExpression)
{
    Xem::XPath* xpath = jXPathExpression2XPath(ev, jXPathExpression);
    xpath->getXProcessor().popEnv();
}

JNIEXPORT void JNICALL
Java_org_xemeiah_dom_xpath_XPathExpression_setVariable (JNIEnv *ev, jobject jXPathExpression, jstring jName,
                                                        jobject jValue)
{
    Xem::XPath* xpath = jXPathExpression2XPath(ev, jXPathExpression);
    jobject jDocumentFactory = jXPathExpression2JDocumentFactory(ev, jXPathExpression);
    Xem::Store* store = jDocumentFactory2Store(ev, jDocumentFactory);

    jboolean isCopy = false;

    Xem::String cName = jstring2XemString(ev, jName);
    Xem::KeyId keyId = store->getKeyCache().getKeyId(0, cName.c_str(), true);

    Xem::NodeSet* nodeSet = xpath->getXProcessor().setVariable(keyId);

    if (ev->IsInstanceOf(jValue, getXemJNI().javaLangString.getClass(ev)))
    {
        jstring jValueString = (jstring) jValue;
        Xem::String cValue = jstring2XemString(ev, jValueString);

        Log("Set '%s' = '%s'\n", cName.c_str(), cValue.c_str());
        nodeSet->setSingleton(cValue);
    }
    else
    {
        NotImplemented("Invalid class for variable '%s'\n", cName.c_str());
    }
}
