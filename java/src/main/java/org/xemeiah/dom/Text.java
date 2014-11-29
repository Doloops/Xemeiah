package org.xemeiah.dom;

import org.w3c.dom.DOMException;

public class Text extends org.xemeiah.dom.CharacterData implements org.w3c.dom.Text
{
    protected Text(Document document, long elementPtr)
    {
        super(document, elementPtr);
    }

    @Override
    public short getNodeType()
    {
        return org.w3c.dom.Node.TEXT_NODE;
    }

    @Override
    public String getNodeName()
    {
        return "#text";
    }

    @Override
    public String getNodeValue() throws DOMException
    {
        return getData();
    }

    @Override
    public String getWholeText()
    {
        return getData();
    }

    @Override
    public String getTextContent() throws DOMException
    {
        return getData();
    }

    @Override
    public void setTextContent(String textContent) throws DOMException
    {
        setData(textContent);
    }
    
    @Override
    public native boolean isElementContentWhitespace();

    @Override
    public native org.w3c.dom.Text replaceWholeText(String content) throws DOMException;

    @Override
    public native org.w3c.dom.Text splitText(int offset) throws DOMException;
}
