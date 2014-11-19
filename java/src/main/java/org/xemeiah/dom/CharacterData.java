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

	
	public native void appendData(String arg0) throws DOMException;

	public native void deleteData(int arg0, int arg1) throws DOMException;

	public native String getData() throws DOMException;

	public native int getLength();

	public native void insertData(int arg0, String arg1) throws DOMException;

	public native void replaceData(int arg0, int arg1, String arg2) throws DOMException;

	public native void setData(String arg0) throws DOMException;

	public native String substringData(int arg0, int arg1) throws DOMException;
	
	
	public Node appendChild(Node newChild) throws DOMException { return null;	}

	public NamedNodeMap getAttributes() { return null; }

	public NodeList getChildNodes() { return null; }

	public Node getFirstChild() { return null; }

	public Node getLastChild() { return null;	}

}
