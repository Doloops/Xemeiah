package org.xemeiah.dom.xpath;

import org.w3c.dom.DOMException;
import org.w3c.dom.Node;
import org.w3c.dom.xpath.XPathException;

public class XPathExpression implements org.w3c.dom.xpath.XPathExpression {

	private final long __xpathPtr;
	
	XPathExpression ( long __xpathPtr )
	{
		this.__xpathPtr = __xpathPtr;
	}
	
	public native Object evaluate(Node contextNode, short type, Object result)
			throws XPathException, DOMException;

}
