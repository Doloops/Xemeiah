#ifndef __XEM_JNI_CLASSES_H
#define __XEM_JNI_CLASSES_H

#include <Xemeiah/trace.h>
#include <jni.h>

#include "xem-jni-classes-ll.h"

class JClass_JavaLangClass
{
public:
    JCLASS("java/lang/Class")
    ;JMETHOD(getName, "getName", "()Ljava/lang/String;")
    ;
};

class JClass_JavaLangString
{
public:
    JCLASS("java/lang/String")
    ;
};

class JClass_JavaLangRuntimeException
{
public:
    JCLASS("java/lang/RuntimeException")
    ;
};

class JClass_JavaIoInputStream
{
public:
    JCLASS("java/io/InputStream")
    ;JMETHOD(read, "read", "([B)I")
    ;
    ;
};

class JClass_DocumentFactory
{
public:
    JCLASS("org/xemeiah/dom/DocumentFactory")
    ;JFIELD(__storePtr, "__storePtr", "J")
    ;JFIELD(__xprocessorPtr, "__xprocessorPtr", "J")
    ;
};

class JClass_Document
{
public:
    JCLASS("org/xemeiah/dom/Document")
    ;JMETHOD(constructor, "<init>",
            "(Lorg/xemeiah/dom/DocumentFactory;J)V")
    ;JMETHOD(getDocumentFactory, "getDocumentFactory",
            "()Lorg/xemeiah/dom/DocumentFactory;")
    ;JFIELD(__documentPtr, "__documentPtr", "J")
    ;
};

class JClass_Node
{
public:
    JCLASS("org/xemeiah/dom/Node")
    ;JFIELD(nodePtr, "__nodePtr", "J")
    ;JFIELD(document, "document", "Lorg/xemeiah/dom/Document;")
    ;
};

class JClass_Element
{
public:
    JCLASS("org/xemeiah/dom/Element")
    ;JMETHOD(constructor, "<init>", "(Lorg/xemeiah/dom/Document;J)V")
    ;
};

class JClass_Text
{
public:
    JCLASS("org/xemeiah/dom/Text")
    ;JMETHOD(constructor, "<init>", "(Lorg/xemeiah/dom/Document;J)V")
    ;
};

class JClass_Attribute
{
public:
    JCLASS("org/xemeiah/dom/Attr")
    ;JMETHOD(constructor, "<init>", "(Lorg/xemeiah/dom/Document;JJ)V")
    ;JFIELD(__nodePtr,"__nodePtr", "J")
    ;JFIELD(__attributePtr, "__attributePtr", "J")
    ;
};

class JClass_XPathEvaluator
{
public:
    JCLASS("org/xemeiah/dom/xpath/XPathEvaluator")
    ;JFIELD(documentFactory, "documentFactory",
            "Lorg/xemeiah/dom/DocumentFactory;")
    ;
};

class JClass_XPathNSResolver
{
public:
    JCLASS("org/w3c/dom/xpath/XPathNSResolver")
    ;JMETHOD(lookupNamespaceURI, "lookupNamespaceURI", "(Ljava/lang/String;)Ljava/lang/String;")
    ;
};

class JClass_XPathExpression
{
public:
    JCLASS("org/xemeiah/dom/xpath/XPathExpression")
    ;JMETHOD(constructor, "<init>", "(JLorg/xemeiah/dom/DocumentFactory;)V")
    ;JFIELD(__xpathPtr, "__xpathPtr", "J")
    ;JFIELD(documentFactory, "documentFactory",
            "Lorg/xemeiah/dom/DocumentFactory;")
    ;
};

class JClass_XPathException
{
public:
    JCLASS("org/w3c/dom/xpath/XPathException")
    ;JMETHOD(constructor, "<init>", "(SLjava/lang/String;)V")
    ;
};

class JClass_DOMException
{
public:
    JCLASS("org/w3c/dom/DOMException")
    ;JMETHOD(constructor, "<init>", "(SLjava/lang/String;)V")
    ;
};

class JClass_XPathResult
{
public:
    JCLASS("org/xemeiah/dom/xpath/XPathResult")
    ;JMETHOD(constructor, "<init>", "(Lorg/xemeiah/dom/Document;IJ)V")
    ;
};

class JClass_NamedNodeMap
{
public:
    JCLASS("org/xemeiah/dom/NamedNodeMap")
    ;JMETHOD(constructor, "<init>", "(Lorg/xemeiah/dom/Document;IJ)V")
    ;
};

class JClass_NodeList
{
public:
    JCLASS("org/xemeiah/dom/NodeList")
    ;JFIELD(document, "document", "Lorg/xemeiah/dom/Document;")
    ;JFIELD(__nodeListPtr, "__nodeListPtr", "J")
    ;
};

class XemJNI
{
public:
    JClass_JavaLangClass javaLangClass;
    JClass_JavaLangRuntimeException javaLangRuntimeException;
    JClass_JavaLangString javaLangString;
    JClass_JavaIoInputStream javaIoInputStream;
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
    JClass_DOMException domException;
    JClass_XPathResult xpathResult;
    JClass_NodeList nodeList;
    JClass_NamedNodeMap namedNodeMap;
};

#endif // __XEM_JNI_CLASSES_H
