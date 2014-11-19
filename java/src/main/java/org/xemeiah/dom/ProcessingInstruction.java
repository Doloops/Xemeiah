package org.xemeiah.dom;

import org.w3c.dom.DOMException;
import org.w3c.dom.NamedNodeMap;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;

public class ProcessingInstruction extends org.xemeiah.dom.Node implements org.w3c.dom.ProcessingInstruction
{

	protected ProcessingInstruction(Document document, long nodePtr) 
	{
		super(document, nodePtr);
	}

	public short getNodeType() { return org.w3c.dom.Node.PROCESSING_INSTRUCTION_NODE; }

	public String getNodeName() {
		return getTarget();
	}

	public native String getData();

	public native String getTarget();

	public native void setData(String arg0) throws DOMException;

	
	public Node appendChild(Node newChild) throws DOMException { return null;	}

	public NamedNodeMap getAttributes() { return null; }

	public NodeList getChildNodes() { return null; }

	public Node getFirstChild() { return null; }

	public Node getLastChild() { return null;	}
}
