package org.xemeiah.dom;

import org.w3c.dom.DOMException;
import org.w3c.dom.Document;
import org.w3c.dom.UserDataHandler;

public abstract class Node implements org.w3c.dom.Node
{
    protected org.xemeiah.dom.Document document;

    protected long __nodePtr;

    protected Node(org.xemeiah.dom.Document document, long __nodePtr)
    {
        this.document = document;
        this.__nodePtr = __nodePtr;
    }

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
        return document.getBaseURI();
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
        // TODO Auto-generated method stub
        return null;
    }

    public Document getOwnerDocument()
    {
        return document;
    }

    @Override
    public org.w3c.dom.Node getParentNode()
    {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public String getPrefix()
    {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public org.w3c.dom.Node getPreviousSibling()
    {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public String getTextContent() throws DOMException
    {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public Object getUserData(String key)
    {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public boolean hasAttributes()
    {
        // TODO Auto-generated method stub
        return false;
    }

    @Override
    public boolean hasChildNodes()
    {
        // TODO Auto-generated method stub
        return false;
    }

    @Override
    public org.w3c.dom.Node insertBefore(org.w3c.dom.Node newChild, org.w3c.dom.Node refChild) throws DOMException
    {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public boolean isDefaultNamespace(String namespaceURI)
    {
        // TODO Auto-generated method stub
        return false;
    }

    @Override
    public boolean isEqualNode(org.w3c.dom.Node arg)
    {
        // TODO Auto-generated method stub
        return false;
    }

    @Override
    public boolean isSameNode(org.w3c.dom.Node other)
    {
        // TODO Auto-generated method stub
        return false;
    }

    @Override
    public boolean isSupported(String feature, String version)
    {
        // TODO Auto-generated method stub
        return false;
    }

    @Override
    public String lookupNamespaceURI(String prefix)
    {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public String lookupPrefix(String namespaceURI)
    {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public void normalize()
    {
        // TODO Auto-generated method stub

    }

    @Override
    public org.w3c.dom.Node removeChild(org.w3c.dom.Node oldChild) throws DOMException
    {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public org.w3c.dom.Node replaceChild(org.w3c.dom.Node newChild, org.w3c.dom.Node oldChild) throws DOMException
    {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public void setNodeValue(String nodeValue) throws DOMException
    {
        // TODO Auto-generated method stub

    }

    @Override
    public void setPrefix(String prefix) throws DOMException
    {
        throw new IllegalArgumentException("Called setPrefix() on a non-element class " + this.getClass().getName());
    }

    @Override
    public void setTextContent(String textContent) throws DOMException
    {
        // TODO Auto-generated method stub

    }

    @Override
    public Object setUserData(String key, Object data, UserDataHandler handler)
    {
        // TODO Auto-generated method stub
        return null;
    }

}
