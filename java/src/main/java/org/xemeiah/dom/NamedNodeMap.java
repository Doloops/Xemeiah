package org.xemeiah.dom;

import org.w3c.dom.DOMException;
import org.w3c.dom.Node;

public class NamedNodeMap extends org.xemeiah.dom.NodeList implements org.w3c.dom.NamedNodeMap
{

    protected NamedNodeMap(Document document, int length, long __nodeListPtr)
    {
        super(document, length, __nodeListPtr);
    }

    @Override
    public Node getNamedItem(String name)
    {
        return getNamedItemNS(null, name);
    }

    @Override
    public native Node getNamedItemNS(String namespaceURI, String localName) throws DOMException;

    @Override
    public Node removeNamedItem(String name) throws DOMException
    {
        return getNamedItemNS(null, name);
    }

    @Override
    public native Node removeNamedItemNS(String namespaceURI, String localName) throws DOMException;

    @Override
    public Node setNamedItem(Node arg) throws DOMException
    {
        return setNamedItemNS(arg);
    }

    @Override
    public native Node setNamedItemNS(Node arg) throws DOMException;
}
