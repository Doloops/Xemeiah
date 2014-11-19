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
	
	protected Attr ( org.xemeiah.dom.Document document, long __elementPtr, long __attributePtr )
	{
		super ( document, __elementPtr );
		this.__attributePtr = __attributePtr;
	}

	public short getNodeType() { return org.w3c.dom.Node.ATTRIBUTE_NODE; }

	public Element getOwnerElement() {
		return new org.xemeiah.dom.Element (document, __nodePtr);
	}

	public String getNodeName() { return getName();	}
	public String getNodeValue() { return getValue();	}

	public native String getName();
	public native String getValue();
	public native String getNamespaceURI();

	public native void setValue(String value) throws DOMException;

	@Override
	public TypeInfo getSchemaTypeInfo() {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public boolean getSpecified() {
		// TODO Auto-generated method stub
		return false;
	}

	public native boolean isId();

	public Node appendChild(Node newChild) throws DOMException { return null;	}

	public NamedNodeMap getAttributes() { return null; }

	public NodeList getChildNodes() { return null; }

	public Node getFirstChild() { return null; }

	public Node getLastChild() { return null;	}
}
