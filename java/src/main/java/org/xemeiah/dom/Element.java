package org.xemeiah.dom;

import java.io.OutputStream;

import javax.xml.transform.TransformerException;

import org.w3c.dom.Attr;
import org.w3c.dom.DOMException;
import org.w3c.dom.NamedNodeMap;
import org.w3c.dom.Node;
import org.w3c.dom.TypeInfo;
import org.w3c.dom.xpath.XPathNSResolver;

public class Element extends org.xemeiah.dom.Node implements org.w3c.dom.Element, XPathNSResolver
{
    protected Element(org.xemeiah.dom.Document document, long __elementPtr)
    {
        super(document, __elementPtr);
    }

    public short getNodeType()
    {
        return org.w3c.dom.Node.ELEMENT_NODE;
    }

    public native String getNodeName();

    public native String getNamespaceURI();

    public native String getLocalName();

    public native org.xemeiah.dom.Element getFirstChild();

    public native org.xemeiah.dom.Element getLastChild();

    public native org.xemeiah.dom.NodeList getChildNodes();

    public String getAttribute(String name)
    {
        return getAttributeNS(null, name);
    }

    public native String getAttributeNS(String namespaceURI, String localName) throws DOMException;

    public org.xemeiah.dom.Attr getAttributeNode(String name)
    {
        return getAttributeNodeNS(null, name);
    }

    public native org.xemeiah.dom.Attr getAttributeNodeNS(String namespaceURI, String localName) throws DOMException;

    public org.xemeiah.dom.NodeList getElementsByTagName(String name)
    {
        return getElementsByTagNameNS(null, name);
    }

    public native org.xemeiah.dom.NodeList getElementsByTagNameNS(String namespaceURI, String localName)
            throws DOMException;

    @Override
    public TypeInfo getSchemaTypeInfo()
    {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public String getTagName()
    {
        // TODO Auto-generated method stub
        return null;
    }

    public boolean hasAttribute(String name)
    {
        return hasAttributeNS(null, name);
    }

    public boolean hasAttributeNS(String namespaceURI, String localName) throws DOMException
    {
        Attr attr = getAttributeNodeNS(namespaceURI, localName);
        return (attr != null);
    }

    public void removeAttribute(String name) throws DOMException
    {
        removeAttributeNS(null, name);
    }

    public void removeAttributeNS(String namespaceURI, String localName) throws DOMException
    {
        org.xemeiah.dom.Attr attr = getAttributeNodeNS(namespaceURI, localName);
        if (attr != null)
        {
            removeAttributeNode(attr);
        }
    }

    public native Attr removeAttributeNode(Attr oldAttr) throws DOMException;

    public void setAttribute(String name, String value) throws DOMException
    {
        setAttributeNS(null, name, value);
    }

/*
    public void setAttributeNS(String namespaceURI, String qualifiedName, String value) throws DOMException
    {
        org.xemeiah.dom.Attr attr = getAttributeNodeNS(namespaceURI, qualifiedName);
        if (attr == null)
        {
            attr = document.createAttributeNS(namespaceURI, qualifiedName);
            setAttributeNodeNS(attr);
        }
        attr.setValue(value);
    }
*/
    public native void setAttributeNS(String namespaceURI, String qualifiedName, String value) throws DOMException;
    
    public Attr setAttributeNode(Attr newAttr) throws DOMException
    {
        return setAttributeNodeNS(newAttr);
    }

    public native Attr setAttributeNodeNS(Attr newAttr) throws DOMException;

    public void setIdAttribute(String name, boolean isId) throws DOMException
    {
        setIdAttributeNS(null, name, isId);
    }

    public void setIdAttributeNS(String namespaceURI, String localName, boolean isId) throws DOMException
    {
        org.xemeiah.dom.Attr attr = getAttributeNodeNS(namespaceURI, localName);
        setIdAttributeNode(attr, isId);
    }

    public native void setIdAttributeNode(Attr idAttr, boolean isId) throws DOMException;

    public native Node appendChild(Node newChild) throws DOMException;

    public native NamedNodeMap getAttributes();

    public native String lookupNamespaceURI(String prefix);

    public void serializeTo(OutputStream outputStream) throws TransformerException
    {
        javax.xml.transform.dom.DOMSource source = new javax.xml.transform.dom.DOMSource();
        source.setNode(this);

        javax.xml.transform.stream.StreamResult result = new javax.xml.transform.stream.StreamResult();
        result.setOutputStream(outputStream);

        org.xemeiah.transform.TransformSerializer serializer = new org.xemeiah.transform.TransformSerializer();

        serializer.transform(source, result);
    }

    @Override
    public void setPrefix(String prefix) throws DOMException
    {
        if (getNamespaceURI() != null)
        {
            setAttributeNS("http://www.w3.org/2000/xmlns/", prefix, getNamespaceURI());
        }
    }
}
