package org.xemeiah.dom;

import org.w3c.dom.DOMException;
import org.w3c.dom.NamedNodeMap;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;

public abstract class CharacterData extends org.xemeiah.dom.Node implements org.w3c.dom.CharacterData
{
    protected CharacterData(Document document, long elementPtr)
    {
        super(document, elementPtr);
    }

    @Override
    public native void appendData(String arg0) throws DOMException;

    @Override
    public native void deleteData(int arg0, int arg1) throws DOMException;

    @Override
    public native String getData() throws DOMException;

    @Override
    public native int getLength();

    @Override
    public native void insertData(int arg0, String arg1) throws DOMException;

    @Override
    public native void replaceData(int arg0, int arg1, String arg2) throws DOMException;

    @Override
    public native void setData(String arg0) throws DOMException;

    @Override
    public native String substringData(int arg0, int arg1) throws DOMException;

    @Override
    public Node appendChild(Node newChild) throws DOMException
    {
        throw new DOMException(DOMException.INVALID_ACCESS_ERR, "Could not add child to Document, I already have a Root element !");
    }

    @Override
    public NamedNodeMap getAttributes() throws DOMException
    {
        throw new DOMException(DOMException.INVALID_ACCESS_ERR, "Could not add child to Document, I already have a Root element !");
    }

    @Override
    public NodeList getChildNodes() throws DOMException
    {
        throw new DOMException(DOMException.INVALID_ACCESS_ERR, "Could not add child to Document, I already have a Root element !");
    }

    @Override
    public Node getFirstChild() throws DOMException
    {
        throw new DOMException(DOMException.INVALID_ACCESS_ERR, "Could not add child to Document, I already have a Root element !");
    }

    @Override
    public Node getLastChild() throws DOMException
    {
        throw new DOMException(DOMException.INVALID_ACCESS_ERR, "Could not add child to Document, I already have a Root element !");
    }
}
