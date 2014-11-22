/*
 * org_xemeiah_dom_Store.cpp
 *
 *  Created on: 15 janv. 2010
 *      Author: francois
 */

#include "include/org_xemeiah_dom_xpath_XPathEvaluator.h"
#include <Xemeiah/trace.h>
#include <Xemeiah/log.h>
#include <Xemeiah/kern/namespacealias.h>
#include <Xemeiah/xpath/xpathparser.h>

#include "xem-jni-dom.h"

#include <Xemeiah/auto-inline.hpp>

namespace Xem
{
    class JNamespaceAlias : public NamespaceAlias
    {
    private:
        JNIEnv* ev;
        jobject resolver;
    public:
        JNamespaceAlias (KeyCache& keyCache, JNIEnv* ev, jobject resolver) : NamespaceAlias(keyCache)
        {
            this->ev = ev;
            this->resolver = resolver;
        }

        NamespaceId getNamespaceIdFromPrefix ( LocalKeyId prefixId )
        {
            Xem::String prefix = keyCache.getLocalKey(prefixId);

            Xem::String result = jLookupNamespaceURI(ev, resolver, prefix);

            Log("prefix = %s => result = %s\n", prefix.c_str(), result.c_str());
            return keyCache.getNamespaceId(result.c_str());
        }
    };
}

JNIEXPORT jobject JNICALL
Java_org_xemeiah_dom_xpath_XPathEvaluator_createExpression (JNIEnv *ev, jobject jXPathEvaluator,
                                                            jstring jExpression, jobject resolverObject)
{
    Xem::String expression = jstring2XemString(ev, jExpression);

    Log("createExpression(%s)\n", expression.c_str());
    try
    {
        Xem::XPathParser* xPathParser;
        jobject jFactory;


        if (isJElement(ev, resolverObject))
        {
            Xem::ElementRef resolver = jElement2ElementRef(ev, resolverObject);
            xPathParser = new Xem::XPathParser(resolver, expression);

            jobject jDocument = jNode2JDocument(ev, resolverObject);
            jFactory =  jDocument2JDocumentFactory(ev, jDocument);
        }
        else
        {
            jFactory = jXPathEvaluator2JDocumentFactory(ev, jXPathEvaluator);

            AssertBug ( jFactory != NULL, "Null documentFactory provided !");

            Xem::Store* store = jDocumentFactory2Store(ev, jFactory);

            Xem::KeyCache& keyCache = store->getKeyCache();
            Xem::JNamespaceAlias nsAlias(keyCache, ev, resolverObject);
            xPathParser = new Xem::XPathParser(keyCache, nsAlias, expression);
        }

        Xem::XProcessor* xprocessor = jDocumentFactory2XProcessor(ev, jFactory);

        Xem::XPath* xpath = new Xem::XPath(*xprocessor, xPathParser, true);

        return xpath2JXPathExpression(ev, xpath, jFactory);
    }
    catch (Xem::XPathException * e)
    {
        Error("Catched Xem::XPathException : %s\n", e->getMessage().c_str());
        jthrowable jException = exception2JXPathException(ev, e);
        ev->Throw(jException);
    }
    catch (Xem::Exception * e)
    {
        Error("Catched Xem::Exception : %s\n", e->getMessage().c_str());
        jthrowable jException = exception2JXPathException(ev, e);
        ev->Throw(jException);
        return NULL;
    }
    catch (void * e)
    {
        Bug("Catched : %p\n", e);
    }
    return NULL;
}
