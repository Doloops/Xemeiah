package org.xemeiah.dom.xpath;

import org.w3c.dom.DOMException;
import org.w3c.dom.Node;
import org.w3c.dom.xpath.XPathException;
import org.w3c.dom.xpath.XPathNSResolver;
import org.xemeiah.dom.DocumentFactory;

public class XPathEvaluator implements org.w3c.dom.xpath.XPathEvaluator
{
	public native org.xemeiah.dom.xpath.XPathExpression createExpression(String expression, XPathNSResolver resolver)
			throws XPathException, DOMException;

	private final DocumentFactory documentFactory;
	
	@Deprecated
	public XPathEvaluator()
	{
	    this(null);
	}
	
	public XPathEvaluator(DocumentFactory documentFactory)
	{
	    this.documentFactory = documentFactory;
	}
	
	public XPathNSResolver createNSResolver(org.w3c.dom.Node node)
	{
		if ( node instanceof org.xemeiah.dom.Element )
		{
			return (org.xemeiah.dom.Element) node;
		}
		return null;
	}

	public Object evaluate(String expression, Node contextNode, XPathNSResolver resolver,
			short type, Object result) throws XPathException, DOMException
	{
		org.xemeiah.dom.xpath.XPathExpression expressionObject = createExpression(expression, resolver);
		return expressionObject.evaluate(contextNode, type, result);
	}

}
