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
    Xem::XPathParser* xpath = jXPathExpression2XPathParser(ev, jXPathExpression);
    AssertBug(xpath, "Null xpath provided !");

    Log("Delete xpath=%p\n", xpath);
    delete (xpath);
}

void
setVariable (JNIEnv *ev, Xem::XProcessor& xprocessor, Xem::String variableName, jobject jValue)
{
//    Xem::XPath* xpath = jXPathExpression2XPathParser(ev, jXPathExpression);
//    jobject jDocumentFactory = jXPathExpression2JDocumentFactory(ev, jXPathExpression);
//    Xem::Store* store = jDocumentFactory2Store(ev, jDocumentFactory);
//
//    Xem::String cName = jstring2XemString(ev, jName);
    Xem::KeyId keyId = xprocessor.getKeyCache().getKeyId(0, variableName.c_str(), true);

    Xem::NodeSet* nodeSet = xprocessor.setVariable(keyId);

    try
    {
        if (ev->IsInstanceOf(jValue, getXemJNI().javaLangString.getClass(ev)))
        {
            jstring jValueString = (jstring) jValue;
            Xem::String cValue = jstring2XemString(ev, jValueString);

            Log("Set '%s' = '%s'\n", variableName.c_str(), cValue.c_str());
            nodeSet->setSingleton(cValue);
        }
        else if (ev->IsInstanceOf(jValue, getXemJNI().document.getClass(ev)))
        {
            NotImplemented("Variable '%s' resolves to a Document !\n", variableName.c_str());
        }
        else if (ev->IsInstanceOf(jValue, getXemJNI().element.getClass(ev)))
        {
            Xem::ElementRef elt = jElement2ElementRef(ev, jValue);
            nodeSet->pushBack(elt);
        }
        else
        {
            NotImplemented("Invalid class for variable '%s'\n", variableName.c_str());
        }
    }
    catch (Xem::Exception* e)
    {
        //  ev->Throw(exception2JDOMException(ev, e));
        Error("While setting variable '%s', catched exception %s\n", variableName.c_str(), e->getMessage().c_str());
        throw(e);
    }
}

void
setVariables (JNIEnv *ev, Xem::XProcessor& xprocessor, jobject jVariableMap)
{
    AssertBug( ev->IsInstanceOf(jVariableMap, getXemJNI().javaUtilMap.getClass(ev)), "Invalid class for jVariableMap !");
    jobject jEntrySet = ev->CallObjectMethod(jVariableMap, getXemJNI().javaUtilMap.entrySet(ev));
    jobject jIterator = ev->CallObjectMethod(jEntrySet, getXemJNI().javaUtilSet.iterator(ev));

    while ( ev->CallBooleanMethod(jIterator, getXemJNI().javaUtilIterator.hasNext(ev)))
    {
        jobject jEntry = ev->CallObjectMethod(jIterator, getXemJNI().javaUtilIterator.next(ev));
        AssertBug( ev->IsInstanceOf(jEntry, getXemJNI().javaUtilMapEntry.getClass(ev)), "Invalid class for jEntry !");

        jstring jKey = (jstring) ev->CallObjectMethod(jEntry, getXemJNI().javaUtilMapEntry.getKey(ev));
        AssertBug ( ev->IsInstanceOf(jKey, getXemJNI().javaLangString.getClass(ev)), "Invalid class for jKey !");
        jobject jValue = ev->CallObjectMethod(jEntry, getXemJNI().javaUtilMapEntry.getValue(ev));

        Xem::String variableName = jstring2XemString(ev, jKey);
        setVariable(ev, xprocessor, variableName, jValue);
    }
}

//JNIEXPORT jobject JNICALL
//Java_org_xemeiah_dom_xpath_XPathExpression_evaluate (JNIEnv *ev, jobject jXPathExpression, jobject jContextNode,
//                                                     jshort type, jobject resultObject)
JNIEXPORT jobject JNICALL
Java_org_xemeiah_dom_xpath_XPathExpression_doEvaluateExpression (JNIEnv *ev, jobject jXPathExpression,
                                                                 jobject jContextNode, jobject jVariableMap)
{
    AssertBug(jXPathExpression, "Null XPathExpression object provided !");
    Xem::XPathParser* xpathParser = jXPathExpression2XPathParser(ev, jXPathExpression);
    AssertBug(xpathParser, "Null XPathParser provided !");
    jobject jDocument = jNode2JDocument(ev, jContextNode);

    Log("jContextNode=%p, jDocument=%p, doc=%p\n", jContextNode, jDocument, jDocument2Document(ev, jDocument));

    Xem::ElementRef contextNode = jElement2ElementRef(ev, jContextNode);
    Xem::XProcessor* xprocessor = jDocument2XProcessor(ev, jDocument);

    Xem::XPath xpath(*xprocessor, *xpathParser);

    Xem::NodeSet* result = new Xem::NodeSet();

    jobject jNodeSet = NULL;
    try
    {
        xprocessor->pushEnv();

        if (jVariableMap != NULL)
        {
            setVariables(ev, *xprocessor, jVariableMap);
        }
        xpath.eval(*result, contextNode);

        Log("jContextNode=%p, jDocument=%p, doc=%p, contextNode->getDocument()=%p\n", jContextNode, jDocument,
            jDocument2Document(ev, jDocument), &(contextNode.getDocument()));
        jNodeSet = nodeSet2JNodeList(ev, jDocument, result);
    }
    catch (Xem::Exception *e)
    {
        xprocessor->popEnv();
        delete (result);
        ev->Throw(exception2JXPathException(ev, e));
        return NULL;
    }
    xprocessor->popEnv();
    return jNodeSet;
}

//JNIEXPORT void JNICALL
//Java_org_xemeiah_dom_xpath_XPathExpression_pushEnv (JNIEnv *ev, jobject jXPathExpression)
//{
//    Xem::XPath* xpath = jXPathExpression2XPathParser(ev, jXPathExpression);
//    xpath->getXProcessor().pushEnv();
//}
//
//JNIEXPORT void JNICALL
//Java_org_xemeiah_dom_xpath_XPathExpression_popEnv (JNIEnv *ev, jobject jXPathExpression)
//{
//    Xem::XPath* xpath = jXPathExpression2XPathParser(ev, jXPathExpression);
//    xpath->getXProcessor().popEnv();
//}
//
