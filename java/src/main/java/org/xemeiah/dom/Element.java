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
    
    public native void triggerElementEnd();

    protected Element(org.xemeiah.dom.Document document, long __elementPtr)
    {
        super(document, __elementPtr);
    }

    @Override
    public short getNodeType()
    {
        return org.w3c.dom.Node.ELEMENT_NODE;
    }

    @Override
    public native String getNodeName();

    @Override
    public native String getNamespaceURI();

    @Override
    public native String getLocalName();

    @Override
    public native org.xemeiah.dom.Element getFirstChild();

    @Override
    public native org.xemeiah.dom.Element getLastChild();

    @Override
    public native org.xemeiah.dom.NodeList getChildNodes();

    @Override
    public String getAttribute(String name)
    {
        return getAttributeNS(null, name);
    }

    @Override
    public native String getAttributeNS(String namespaceURI, String localName) throws DOMException;

    @Override
    public org.xemeiah.dom.Attr getAttributeNode(String name)
    {
        return getAttributeNodeNS(null, name);
    }

    @Override
    public native org.xemeiah.dom.Attr getAttributeNodeNS(String namespaceURI, String localName) throws DOMException;

    @Override
    public org.xemeiah.dom.NodeList getElementsByTagName(String name)
    {
        return getElementsByTagNameNS(null, name);
    }

    @Override
    public native org.xemeiah.dom.NodeList getElementsByTagNameNS(String namespaceURI, String localName)
            throws DOMException;

    @Override
    public boolean hasAttribute(String name)
    {
        return hasAttributeNS(null, name);
    }

    @Override
    public boolean hasAttributeNS(String namespaceURI, String localName) throws DOMException
    {
        Attr attr = getAttributeNodeNS(namespaceURI, localName);
        return (attr != null);
    }

    @Override
    public void removeAttribute(String name) throws DOMException
    {
        removeAttributeNS(null, name);
    }

    @Override
    public void removeAttributeNS(String namespaceURI, String localName) throws DOMException
    {
        org.xemeiah.dom.Attr attr = getAttributeNodeNS(namespaceURI, localName);
        if (attr != null)
        {
            removeAttributeNode(attr);
        }
    }

    @Override
    public native Attr removeAttributeNode(Attr oldAttr) throws DOMException;

    @Override
    public void setAttribute(String name, String value) throws DOMException
    {
        setAttributeNS(null, name, value);
    }

    @Override
    public native void setAttributeNS(String namespaceURI, String qualifiedName, String value) throws DOMException;
    
    @Override
    public Attr setAttributeNode(Attr newAttr) throws DOMException
    {
        return setAttributeNodeNS(newAttr);
    }

    @Override
    public native Attr setAttributeNodeNS(Attr newAttr) throws DOMException;

    @Override
    public void setIdAttribute(String name, boolean isId) throws DOMException
    {
        setIdAttributeNS(null, name, isId);
    }

    @Override
    public void setIdAttributeNS(String namespaceURI, String localName, boolean isId) throws DOMException
    {
        org.xemeiah.dom.Attr attr = getAttributeNodeNS(namespaceURI, localName);
        setIdAttributeNode(attr, isId);
    }

    @Override
    public native void setIdAttributeNode(Attr idAttr, boolean isId) throws DOMException;

    @Override
    public native Node appendChild(Node newChild) throws DOMException;

    @Override
    public native NamedNodeMap getAttributes();

    @Override
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
    
    
    @Override
    public TypeInfo getSchemaTypeInfo()
    {
        // TODO Auto-generated method stub
        throw new RuntimeException("NOT IMPLEMENTED !!!");
    }

    @Override
    public String getTagName()
    {
        // TODO Auto-generated method stub
        throw new RuntimeException("NOT IMPLEMENTED !!!");
    }
}
