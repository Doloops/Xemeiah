package org.xemeiah.dom;

import org.w3c.dom.DOMException;
import org.w3c.dom.Element;
import org.w3c.dom.NamedNodeMap;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import org.w3c.dom.TypeInfo;

public class Attr extends org.xemeiah.dom.Node implements org.w3c.dom.Attr
{
    protected long __attributePtr;

    protected Attr(org.xemeiah.dom.Document document, long __elementPtr, long __attributePtr)
    {
        super(document, __elementPtr);
        this.__attributePtr = __attributePtr;
    }

    @Override
    public short getNodeType()
    {
        return org.w3c.dom.Node.ATTRIBUTE_NODE;
    }

    @Override
    public Element getOwnerElement()
    {
        return new org.xemeiah.dom.Element(getDocument(), __nodePtr);
    }

    @Override
    public String getNodeName()
    {
        return getName();
    }

    @Override
    public String getNodeValue()
    {
        return getValue();
    }

    @Override
    public native String getName();

    @Override
    public native String getValue();

    @Override
    public native String getNamespaceURI();

    @Override
    public native void setValue(String value) throws DOMException;

    @Override
    public native boolean isId();

    @Override
    public TypeInfo getSchemaTypeInfo()
    {
        // TODO Auto-generated method stub
        throw new RuntimeException("NOT IMPLEMENTED !!!");
    }

    @Override
    public boolean getSpecified()
    {
        // TODO Auto-generated method stub
        throw new RuntimeException("NOT IMPLEMENTED !!!");
    }


    @Override
    public Node appendChild(Node newChild) throws DOMException
    {
        throw new DOMException(DOMException.INVALID_ACCESS_ERR, "Could not add child to Document, I already have a Root element !");
    }

    @Override
    public NamedNodeMap getAttributes()
    {
        throw new DOMException(DOMException.INVALID_ACCESS_ERR, "Could not add child to Document, I already have a Root element !");
    }

    @Override
    public NodeList getChildNodes()
    {
        throw new DOMException(DOMException.INVALID_ACCESS_ERR, "Could not add child to Document, I already have a Root element !");
    }

    @Override
    public Node getFirstChild()
    {
        throw new DOMException(DOMException.INVALID_ACCESS_ERR, "Could not add child to Document, I already have a Root element !");
    }

    @Override
    public Node getLastChild()
    {
        throw new DOMException(DOMException.INVALID_ACCESS_ERR, "Could not add child to Document, I already have a Root element !");
    }
}
