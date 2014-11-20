/*
 * xem-jni.cpp
 *
 *  Created on: 26 janv. 2010
 *      Author: francois
 */

#include "xem-jni-dom.h"

#include <Xemeiah/trace.h>
#include <Xemeiah/log.h>
#include <Xemeiah/dom/nodeset.h>

#include <Xemeiah/auto-inline.hpp>

#undef Log
#define Log(...) do{} while(0)

#define __XEM_JNI_USE_CACHE
#define Log_XEMJNI Debug

#ifdef __XEM_JNI_USE_CACHE
#define JCLASS(__jName) \
        jclass __class = NULL; \
        jclass getClass(JNIEnv* __ev) { \
        if ( __class == NULL ) { \
            jclass __local_class = __ev->FindClass(__jName); \
            __class = (jclass) __ev->NewGlobalRef(__local_class); \
            __ev->DeleteLocalRef(__local_class); \
            Log_XEMJNI ("[ev=%p] For class=%s, caching %p\n", __ev, __jName, __class); \
            AssertBug ( __class != NULL, "Could not find class " __jName ); \
        } \
        else if ( 0 )\
        { \
            jclass __reClass = __ev->FindClass(__jName); \
            AssertBug ( !__ev->ExceptionCheck() , "Exception check !"); \
            if ( __class != __reClass ) \
            { Log_XEMJNI ( "[ev=%p] Diverging classes for %s, cached %p, should have %p\n", __ev, __jName, __class, __reClass ); \
                /* __class = __reClass; */ \
            } \
        } \
    Log_XEMJNI ("[ev=%p] Returning class=%s => %p\n", __ev, __jName, __class); \
    return __class; }
#define JFIELD(__cName, __fieldName, __signature) \
      jfieldID __cName##Fcache = NULL; \
      jfieldID __cName(JNIEnv* __ev) { \
      if ( __cName##Fcache == NULL ) \
      { \
        jclass _clz = getClass(__ev); \
        __cName##Fcache = __ev->GetFieldID(_clz, __fieldName, __signature); \
        Log_XEMJNI("[ev=%p] SET clz=%p, __fieldName=%s, __signature=%s, jfieldID=%p\n", __ev, _clz, __fieldName, __signature, __cName##Fcache); \
        } \
    Log_XEMJNI("[ev=%p] __fieldName=%s, __signature=%s, jfieldID=%p\n", __ev, __fieldName, __signature, __cName##Fcache); \
    return __cName##Fcache; \
}
#define JMETHOD(__cName, __mthName, __signature) \
      jmethodID __cName##Mcache = NULL; \
      jmethodID __cName(JNIEnv* __ev) { \
      if ( __cName##Mcache == NULL ) \
      { \
        jclass _clz = getClass(__ev); \
        __cName##Mcache = __ev->GetMethodID(_clz, __mthName, __signature); \
        Log_XEMJNI("[ev=%p] SET class=%p, __mthName=%s, __signature=%s, jmethodID=%p\n", __ev, _clz, __mthName, __signature, __cName##Mcache ); \
      } \
    Log_XEMJNI("[ev=%p] __mthName=%s, __signature=%s, jmethodID=%p\n", __ev, __mthName, __signature, __cName##Mcache ); \
    return __cName##Mcache; \
  }

//        jobject newGlobalRef = __ev->NewGlobalRef((jobject)__cName##Mcache);
//        Log_XEMJNI ("[ev=%p] newGlobalRef=%p\n", __ev, newGlobalRef);

//            jobject newGlobalRef = __ev->NewGlobalRef(__class);
//            AssertBug ( newGlobalRef != NULL, "Null global ref ?\n");

#else
#define JCLASS(__jName) \
    jclass getClass(JNIEnv* __ev) { \
        jclass result = __ev->FindClass(__jName); \
        Log_XEMJNI ("[ev=%p] Returning class=%s => %p\n", __ev, __jName, result); \
        return result;  \
    }
#define JFIELD(__cName, __fieldName, __signature) \
  jfieldID __cName(JNIEnv* __ev) { \
    jclass _clz = getClass(__ev); \
    jfieldID result = __ev->GetFieldID(_clz, __fieldName, __signature); \
    Log_XEMJNI("[ev=%p] class=%p, __fieldName=%s, __signature=%s, jfieldID=%p\n", __ev, _clz, __fieldName, __signature, result); \
    return result; \
  }
#define JMETHOD(__cName, __mthName, __signature) \
  jmethodID __cName(JNIEnv* __ev) { \
    jclass _clz = getClass(__ev); \
    jmethodID result = __ev->GetMethodID(_clz, __mthName, __signature); \
    Log_XEMJNI("[ev=%p] class=%p, __mthName=%s, __signature=%s, jmethodID=%p\n", __ev, _clz, __mthName, __signature, result); \
    return result; \
  }
#endif

class JClass_JavaLangClass
{
public:
    JCLASS("java/lang/Class")
    JMETHOD(getName, "getName", "()Ljava/lang/String;")
};

class JClass_DocumentFactory
{
public:
    JCLASS("org/xemeiah/dom/DocumentFactory")
    JFIELD(__storePtr, "__storePtr", "J")
    JFIELD(__xprocessorPtr, "__xprocessorPtr", "J")
};

class JClass_Document
{
public:
    JCLASS("org/xemeiah/dom/Document")
    JMETHOD(constructor, "<init>",
            "(Lorg/xemeiah/dom/DocumentFactory;J)V")
    JMETHOD(getDocumentFactory, "getDocumentFactory",
            "()Lorg/xemeiah/dom/DocumentFactory;")
    JFIELD(__documentPtr, "__documentPtr", "J")
};

class JClass_Node
{
public:
    JCLASS("org/xemeiah/dom/Node")
    JFIELD(nodePtr, "__nodePtr", "J")
    JFIELD(document, "document", "Lorg/xemeiah/dom/Document;")
};

class JClass_Element
{
public:
    JCLASS("org/xemeiah/dom/Element")
    JMETHOD(constructor, "<init>", "(Lorg/xemeiah/dom/Document;J)V")
};

class JClass_Text
{
public:
    JCLASS("org/xemeiah/dom/Text")
    JMETHOD(constructor, "<init>", "(Lorg/xemeiah/dom/Document;J)V")
};

class JClass_Attribute
{
public:
    JCLASS("org/xemeiah/dom/Attr")
    JMETHOD(constructor, "<init>", "(Lorg/xemeiah/dom/Document;JJ)V")
};

class JClass_XPathEvaluator
{
public:
    JCLASS("org/xemeiah/dom/xpath/XPathEvaluator")
    JMETHOD(getDocumentFactory, "getDocumentFactory",
            "()Lorg/xemeiah/dom/DocumentFactory;")
};

class JClass_XPathNSResolver
{
public:
    JCLASS("org/w3c/dom/xpath/XPathNSResolver")
    JMETHOD(lookupNamespaceURI, "lookupNamespaceURI", "(Ljava/lang/String;)Ljava/lang/String;")
};

class JClass_XPathExpression
{
public:
    JCLASS("org/xemeiah/dom/xpath/XPathExpression")
    JMETHOD(constructor, "<init>", "(J)V")
    JFIELD(__xpathPtr, "__xpathPtr", "J")
};

class JClass_XPathException
{
public:
    JCLASS("org/w3c/dom/xpath/XPathException")
    JMETHOD(constructor, "<init>", "(SLjava/lang/String;)V")
};

class JClass_XPathResult
{
public:
    JCLASS("org/xemeiah/dom/xpath/XPathResult")
    JMETHOD(constructor, "<init>", "(Lorg/xemeiah/dom/Document;IJ)V")
};

class JClass_NamedNodeMap
{
public:
    JCLASS("org/xemeiah/dom/NamedNodeMap")
    JMETHOD(constructor, "<init>", "(Lorg/xemeiah/dom/Document;IJ)V")
};

class JClass_NodeList
{
public:
    JCLASS("org/xemeiah/dom/NodeList")
    JFIELD(document, "document", "Lorg/xemeiah/dom/Document;")
    JFIELD(__nodeListPtr, "__nodeListPtr", "J")
};

class XemJNI
{
public:
    JClass_JavaLangClass javaLangClass;
    JClass_DocumentFactory documentFactory;
    JClass_Document document;
    JClass_Node node;
    JClass_Element element;
    JClass_Text text;
    JClass_Attribute attr;
    JClass_XPathNSResolver xpathNSResolver;
    JClass_XPathEvaluator xpathEvaluator;
    JClass_XPathExpression xpathExpression;
    JClass_XPathException xpathException;
    JClass_XPathResult xpathResult;
    JClass_NodeList nodeList;
    JClass_NamedNodeMap namedNodeMap;
};

static XemJNI xemJNI;

INLINE
XemJNI&
getXemJNI ()
{
    return xemJNI;
}

void
XemJniInit (JNIEnv* ev)
{
    Log("Called XemJniInit with ev=%p\n", ev);

    Log("XemJNI size=%lu\n", sizeof(XemJNI));
#ifdef __XEM_JNI_USE_CACHE
    Log("Init : docc lass=%p\n", getXemJNI().document.__class);
#endif

//    jclass cl1 = ev->FindClass("Lorg/xemeiah/dom/Document;");
//    jclass cl2 = ev->FindClass("Lorg/xemeiah/dom/Document;");
//
//    if (cl1 != cl2)
//    {
//        Warn("Diverging classes ! cl1=%p, cl2=%p\n", cl1, cl2);
//    }
//    jmethodID mth1 = ev->GetMethodID(cl1, "<init>", "(Lorg/xemeiah/dom/DocumentFactory;J)V");
//    jmethodID mth2 = ev->GetMethodID(cl2, "<init>", "(Lorg/xemeiah/dom/DocumentFactory;J)V");
//    if (mth1 != mth2)
//    {
//        Warn("Diverging methodes ! mth1=%p, mth2=%p\n", mth1, mth2);
//    }
}

Xem::String
jstring2XemString (JNIEnv* ev, jstring js)
{
    jboolean isCopy = false;
    const char* utf = ev->GetStringUTFChars(js, &isCopy);
    Xem::String result = Xem::String(utf).copy();
    ev->ReleaseStringUTFChars(js, utf);
    return result;
}

jobject
jDocument2JDocumentFactory (JNIEnv* ev, jobject jDoc)
{
    jobject jDocumentFactory = ev->CallObjectMethod(jDoc, getXemJNI().document.getDocumentFactory(ev));
    return jDocumentFactory;
}

jobject
jXPathEvaluator2JDocumentFactory (JNIEnv* ev, jobject jXPathEvaluator)
{
    jobject jDocumentFactory = ev->CallObjectMethod(jXPathEvaluator, getXemJNI().xpathEvaluator.getDocumentFactory(ev));
    return jDocumentFactory;
}

Xem::XProcessor*
jDocumentFactory2XProcessor (JNIEnv* ev, jobject jDocumentFactory)
{
    Xem::XProcessor* xprocessor = (Xem::XProcessor*) (ev->GetLongField(jDocumentFactory,
                                                                       getXemJNI().documentFactory.__xprocessorPtr(ev)));
    return xprocessor;
}

jobject
elementRef2JElement (JNIEnv* ev, jobject jDocument, Xem::ElementRef& eltRef)
{
    jclass clazz = NULL;
    jmethodID constructor = NULL;
    if (eltRef.isComment())
    {
        // clazz = ev->FindClass("org/xemeiah/dom/Comment");
        Bug("Not implemented !");
    }
    else if (eltRef.isPI())
    {
        // clazz = ev->FindClass("org/xemeiah/dom/ProcessingInstruction");
        Bug("Not implemented !");
    }
    else if (eltRef.isText())
    {
        clazz = getXemJNI().text.getClass(ev);
        constructor = getXemJNI().text.constructor(ev);
    }
    else
    {
        clazz = getXemJNI().element.getClass(ev);
        constructor = getXemJNI().element.constructor(ev);
    }

    AssertBug(clazz != NULL, "Null class !\n");
    AssertBug(constructor != NULL, "Null constructor !\n");

    jlong elementPtr = (jlong) eltRef.getElementPtr();
    jobject jElement = ev->NewObject(clazz, constructor, jDocument, elementPtr);
    Log("New jElement=%p, jDocument=%p (document=%p), elementPtr=%lx, eltRef=%s\n", jElement, jDocument,
        jDocument2Document(ev, jDocument), elementPtr, eltRef.getKey().c_str());
    return jElement;
}

jobject
attributeRef2JAttribute (JNIEnv* ev, jobject documentObject, Xem::AttributeRef& attrRef)
{
    jobject jAttr = ev->NewObject(getXemJNI().attr.getClass(ev), getXemJNI().attr.constructor(ev), documentObject,
                                  attrRef.getElement().getElementPtr(), attrRef.getAttributePtr());

    return jAttr;
}

jobject
document2JDocument (JNIEnv* ev, jobject jFactory, Xem::Document* document)
{
    jobject jDocument = ev->NewObject(getXemJNI().document.getClass(ev), getXemJNI().document.constructor(ev), jFactory,
                                      (jlong) (document));
    return jDocument;
}

void
initDocumentFactory (JNIEnv* ev, jobject jFactory, Xem::Store* store, Xem::XProcessor* xprocessor)
{
    ev->SetLongField(jFactory, getXemJNI().documentFactory.__storePtr(ev), (jlong) store);
    ev->SetLongField(jFactory, getXemJNI().documentFactory.__xprocessorPtr(ev), (jlong) xprocessor);
}

Xem::Store*
jDocumentFactory2Store (JNIEnv* ev, jobject jFactory)
{
    jlong __storePtr = ev->GetLongField(jFactory, getXemJNI().documentFactory.__storePtr(ev));
    Xem::Store* store = (Xem::Store*) (__storePtr);
    return store;
}

Xem::Document*
jDocument2Document (JNIEnv* ev, jobject documentObject)
{
    jlong __documentPtr = ev->GetLongField(documentObject, getXemJNI().document.__documentPtr(ev));
    Xem::Document* doc = (Xem::Document*) __documentPtr;
    AssertBug(doc, "Null document !\n");
    return doc;
}

jobject
jNode2JDocument (JNIEnv* ev, jobject nodeObject)
{
    jobject documentObject = ev->GetObjectField(nodeObject, getXemJNI().node.document(ev));
    return documentObject;
}

jobject
jNodeList2JDocument (JNIEnv* ev, jobject jNodeList)
{
    jobject jDocument = ev->GetObjectField(jNodeList, getXemJNI().nodeList.document(ev));
    return jDocument;
}

Xem::String
getJObjectClassname (JNIEnv* ev, jobject jObject)
{
    jclass clazz = ev->GetObjectClass(jObject);
    jobject jName = ev->CallObjectMethod(clazz, getXemJNI().javaLangClass.getName(ev));
    return jstring2XemString(ev, (jstring) jName);
}

bool
isJElement (JNIEnv* ev, jobject jElement)
{
    return ( ev->IsInstanceOf(jElement, getXemJNI().element.getClass(ev))
            || ev->IsInstanceOf(jElement, getXemJNI().text.getClass(ev)) );
}

Xem::ElementRef
jElement2ElementRef (JNIEnv* ev, jobject jElement)
{
    AssertBug(jElement, "Null object !\n");
    AssertBug(isJElement(ev, jElement), "Incorrect class : %s!\n", getJObjectClassname(ev, jElement).c_str());

    jobject jDocument = jNode2JDocument(ev, jElement);
    AssertBug(jDocument, "Null document object !\n");

    Xem::Document* doc = jDocument2Document(ev, jDocument);

    Log("jElement=%p, jDocument=%p, doc=%p\n", jElement, jDocument, doc);

    jlong elementPtr = ev->GetLongField(jElement, getXemJNI().node.nodePtr(ev));

    AssertBug(elementPtr, "Null ElementPtr !\n");

    Xem::ElementRef eltRef = Xem::ElementRefConstructor(*doc, elementPtr);
    return eltRef;
}

Xem::AttributeRef
j2AttributeRef (JNIEnv* ev, jobject attrObject)
{
    jobject documentObject = jNode2JDocument(ev, attrObject);
    Xem::Document* doc = jDocument2Document(ev, documentObject);

    jclass attrClass = ev->GetObjectClass(attrObject);
    jfieldID nodePtrId = ev->GetFieldID(attrClass, "__nodePtr", "J");
    jfieldID attrPtrId = ev->GetFieldID(attrClass, "__attributePtr", "J");
    jlong nodePtr = ev->GetLongField(attrObject, nodePtrId);
    jlong attrPtr = ev->GetLongField(attrObject, attrPtrId);

    return Xem::AttributeRefConstructor(*doc, nodePtr, attrPtr);
}

Xem::KeyId
j2KeyId (JNIEnv* ev, Xem::KeyCache& keyCache, jstring jNamespace, jstring jName)
{
    jboolean isCopy = false;
    const char* nsUrl = jNamespace ? ev->GetStringUTFChars(jNamespace, &isCopy) : NULL;
    const char* localName = ev->GetStringUTFChars(jName, &isCopy);

    Log("j2KeyId : nsUrl=%s, localName=%s\n", nsUrl, localName);

    Xem::NamespaceId nsId = nsUrl ? keyCache.getNamespaceId(nsUrl) : 0;
    Xem::KeyId keyId = keyCache.getKeyId(nsId, localName, false);

    ev->ReleaseStringUTFChars(jNamespace, nsUrl);
    ev->ReleaseStringUTFChars(jName, localName);
    return keyId;
}

jobject
nodeSet2JNamedNodeMap (JNIEnv* ev, jobject jDocument, Xem::NodeSet* result)
{
    jobject nodeListObject = ev->NewObject(getXemJNI().namedNodeMap.getClass(ev),
                                           getXemJNI().namedNodeMap.constructor(ev), jDocument, result->size(), result);
    return nodeListObject;

}

jobject
nodeSet2JNodeList (JNIEnv* ev, jobject jDocument, Xem::NodeSet* result)
{
    jobject nodeListObject = ev->NewObject(getXemJNI().xpathResult.getClass(ev),
                                           getXemJNI().xpathResult.constructor(ev), jDocument, result->size(), result);
    return nodeListObject;
}

Xem::NodeSet*
jNodeList2NodeSet (JNIEnv* ev, jobject jNodeList)
{
    jlong ptr = ev->GetLongField(jNodeList, getXemJNI().nodeList.__nodeListPtr(ev));
    Xem::NodeSet* nodeSet = (Xem::NodeSet*) ptr;
    Log("[ev=%p], jNodeList=%p => nodeList=%p\n", ev, jNodeList, nodeSet);
    return nodeSet;
}

jobject
xpath2JXPathExpression (JNIEnv* ev, Xem::XPath* xpath)
{
    jobject jXPathExpression = ev->NewObject(getXemJNI().xpathExpression.getClass(ev),
                                             getXemJNI().xpathExpression.constructor(ev), (jlong) xpath);
    Log("[ev=%p], xpath=%p, jXPath=%p\n", ev, xpath, jXPathExpression);
    return jXPathExpression;
}

Xem::XPath*
jXPathExpression2XPath (JNIEnv* ev, jobject jXPathExpression)
{
    jlong ptr = ev->GetLongField(jXPathExpression, getXemJNI().xpathExpression.__xpathPtr(ev));
    Xem::XPath* xpath = (Xem::XPath*) (ptr);
    Log("[ev=%p], xpath=%p, jXPath=%p\n", ev, xpath, jXPathExpression);
    return xpath;
}

jthrowable
exception2JXPathException (JNIEnv* ev, Xem::Exception* exception)
{
    jstring msg = ev->NewStringUTF(exception->getMessage().c_str());
    return (jthrowable) ev->NewObject(getXemJNI().xpathException.getClass(ev),
                                      getXemJNI().xpathException.constructor(ev), (jshort) 0, msg);
}

Xem::String
jLookupNamespaceURI (JNIEnv* ev, jobject jNSResolver, Xem::String nsPrefix)
{
    jstring jNsPrefix = ev->NewStringUTF(nsPrefix.c_str());
    jstring result = (jstring) ev->CallObjectMethod(jNSResolver, getXemJNI().xpathNSResolver.lookupNamespaceURI(ev),
                                                    jNsPrefix);
    if ( result != NULL )
    {
        return jstring2XemString(ev, result);
    }
    Error("Could not resolve %s !\n", nsPrefix.c_str());
    return "";
}
