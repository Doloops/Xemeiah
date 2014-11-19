package org.xemeiah.dom;

import org.w3c.dom.DOMException;

public class Text extends org.xemeiah.dom.CharacterData implements org.w3c.dom.Text
{
	protected Text(Document document, long elementPtr) 
	{
		super(document, elementPtr);
	}

	public short getNodeType() { return org.w3c.dom.Node.TEXT_NODE; }
	public String getNodeName() { return "#text";	}
	
	@Override
    public String getNodeValue() throws DOMException
    {
	    return getData();
    }
	
	public String getWholeText() { return getData(); }

	public native boolean isElementContentWhitespace();

	public native org.w3c.dom.Text replaceWholeText(String content) throws DOMException;

	public native org.w3c.dom.Text splitText(int offset) throws DOMException;
}
