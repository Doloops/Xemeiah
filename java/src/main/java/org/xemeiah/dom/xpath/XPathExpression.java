package org.xemeiah.dom.xpath;

import java.util.Map;

import org.w3c.dom.DOMException;
import org.w3c.dom.Node;
import org.w3c.dom.xpath.XPathException;
import org.xemeiah.dom.DocumentFactory;

public class XPathExpression implements org.w3c.dom.xpath.XPathExpression
{

    private final long __xpathPtr;

    private final DocumentFactory documentFactory;

    private XPathExpression(long __xpathPtr, DocumentFactory documentFactory)
    {
        this.__xpathPtr = __xpathPtr;
        this.documentFactory = documentFactory;
    }

    private native void cleanUp();

    protected void finalize()
    {
        cleanUp();
    }

    private native Object doEvaluateExpression(Node contextNode, Map<String, Object> objectMap) throws XPathException, DOMException;

    @Override
    public Object evaluate(Node contextNode, short type, Object result) throws XPathException, DOMException
    {
        return doEvaluateExpression(contextNode, null);
    }

    public Object evaluate(Node contextNode, Map<String, Object> objectMap) throws XPathException, DOMException
    {
        return doEvaluateExpression(contextNode, objectMap);
    }
}
