package org.xemeiah.dom;

import org.w3c.dom.DOMException;
import org.w3c.dom.Document;
import org.w3c.dom.UserDataHandler;

public abstract class Node implements org.w3c.dom.Node
{
    protected org.xemeiah.dom.Document document;

    protected final org.xemeiah.dom.Document getDocument()
    {
        return document;
    }

    protected final long __nodePtr;

    protected Node(org.xemeiah.dom.Document document, long __nodePtr)
    {
        this.document = document;
        this.__nodePtr = __nodePtr;
    }

    @Override
    public Document getOwnerDocument()
    {
        return getDocument();
    }

    @Override
    public native org.w3c.dom.Node getParentNode();

    @Override
    public native String getPrefix();

    @Override
    public org.w3c.dom.Node cloneNode(boolean deep)
    {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public short compareDocumentPosition(org.w3c.dom.Node other) throws DOMException
    {
        // TODO Auto-generated method stub
        return 0;
    }

    public String getBaseURI()
    {
        return getDocument().getBaseURI();
    }

    @Override
    public Object getFeature(String feature, String version)
    {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public String getLocalName()
    {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public String getNamespaceURI()
    {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public native org.w3c.dom.Node getNextSibling();

    @Override
    public short getNodeType()
    {
        // TODO Auto-generated method stub
        return 0;
    }

    @Override
    public String getNodeValue() throws DOMException
    {
        throw new RuntimeException("NOT IMPLEMENTED !");
    }

    @Override
    public org.w3c.dom.Node getPreviousSibling()
    {
        throw new RuntimeException("NOT IMPLEMENTED !");
    }

    @Override
    public String getTextContent() throws DOMException
    {
        throw new RuntimeException("NOT IMPLEMENTED !");
    }

    @Override
    public Object getUserData(String key)
    {
        throw new RuntimeException("NOT IMPLEMENTED !");
    }

    @Override
    public boolean hasAttributes()
    {
        throw new RuntimeException("NOT IMPLEMENTED !");
    }

    @Override
    public boolean hasChildNodes()
    {
        throw new RuntimeException("NOT IMPLEMENTED !");
    }

    @Override
    public org.w3c.dom.Node insertBefore(org.w3c.dom.Node newChild, org.w3c.dom.Node refChild) throws DOMException
    {
        throw new RuntimeException("NOT IMPLEMENTED !");
    }

    @Override
    public boolean isDefaultNamespace(String namespaceURI)
    {
        throw new RuntimeException("NOT IMPLEMENTED !");
    }

    @Override
    public boolean isEqualNode(org.w3c.dom.Node arg)
    {
        throw new RuntimeException("NOT IMPLEMENTED !");
    }

    @Override
    public boolean isSameNode(org.w3c.dom.Node other)
    {
        throw new RuntimeException("NOT IMPLEMENTED !");
    }

    @Override
    public boolean isSupported(String feature, String version)
    {
        throw new RuntimeException("NOT IMPLEMENTED !");
    }

    @Override
    public String lookupNamespaceURI(String prefix)
    {
        throw new RuntimeException("NOT IMPLEMENTED !");
    }

    @Override
    public String lookupPrefix(String namespaceURI)
    {
        throw new RuntimeException("NOT IMPLEMENTED !");
    }

    @Override
    public void normalize()
    {
        throw new RuntimeException("NOT IMPLEMENTED !");
    }

    @Override
    public native org.w3c.dom.Node removeChild(org.w3c.dom.Node oldChild) throws DOMException;

    @Override
    public org.w3c.dom.Node replaceChild(org.w3c.dom.Node newChild, org.w3c.dom.Node oldChild) throws DOMException
    {
        throw new RuntimeException("NOT IMPLEMENTED !");
    }

    @Override
    public void setNodeValue(String nodeValue) throws DOMException
    {
        throw new RuntimeException("NOT IMPLEMENTED !");
    }

    @Override
    public void setPrefix(String prefix) throws DOMException
    {
        throw new IllegalArgumentException("Called setPrefix() on a non-element class " + this.getClass().getName());
    }

    @Override
    public void setTextContent(String textContent) throws DOMException
    {
        throw new RuntimeException("NOT IMPLEMENTED !");
    }

    @Override
    public Object setUserData(String key, Object data, UserDataHandler handler)
    {
        throw new RuntimeException("NOT IMPLEMENTED !");
    }
}
