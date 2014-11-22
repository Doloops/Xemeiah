package org.xemeiah.test;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.NodeList;
import org.w3c.dom.Text;
import org.w3c.dom.xpath.XPathEvaluator;
import org.w3c.dom.xpath.XPathNSResolver;
import org.xemeiah.dom.DocumentFactory;
import org.xemeiah.dom.xpath.XPathExpression;

public class TestXPath
{
    private DocumentFactory documentFactory; 
    
    @Before
    public void setUp()
    {
        documentFactory = new DocumentFactory();
    }
    
    @Test
    public void testParsing()
    {
        XPathEvaluator evaluator = new org.xemeiah.dom.xpath.XPathEvaluator(documentFactory);
        
        XPathNSResolver nsResolver = new XPathNSResolver()
        {
            @Override
            public String lookupNamespaceURI(String arg0)
            {
                if ( arg0.equals("myns1"))
                {
                    return "http://myns1";
                }
                else if ( arg0.equals("myns2"))
                {
                    return "http://myns2";
                }
                // TODO Auto-generated method stub
                return null;
            }
        };
        org.w3c.dom.xpath.XPathExpression expression = evaluator.createExpression("myns1:key1/myns2:key2", nsResolver);

        Document doc = documentFactory.newVolatileDocument();
        Element elt = doc.getDocumentElement();
        
        Element elt1 = doc.createElementNS("http://myns1", "key1");
        elt.appendChild(elt1);
        
        Element elt2 = doc.createElementNS("http://myns2", "key2");
        elt1.appendChild(elt2);
        
        Text text = doc.createTextNode("Hello");
        elt2.appendChild(text);
        
        NodeList nodeList = (NodeList) expression.evaluate(elt, (short)0, null);
        
        Assert.assertEquals(1, nodeList.getLength());
        
        Element eltRes = (Element) nodeList.item(0);
        Assert.assertEquals("key2", eltRes.getLocalName());
        Assert.assertEquals("http://myns2", eltRes.getNamespaceURI());
    }

    @Test
    public void testXPathWithVariables()
    {
        org.xemeiah.dom.xpath.XPathEvaluator evaluator = new org.xemeiah.dom.xpath.XPathEvaluator(documentFactory);
        
        XPathNSResolver nsResolver = new XPathNSResolver()
        {
            @Override
            public String lookupNamespaceURI(String arg0)
            {
                throw new RuntimeException("Shall not be called !");
            }
        };
        XPathExpression expression = evaluator.createExpression("/*[@tag=$arg]/*", nsResolver);

        Document doc = documentFactory.newVolatileDocument();
        Element elt = doc.getDocumentElement();
        
        Element elt1 = doc.createElementNS("http://myns1", "key1");
        elt1.setAttribute("tag", "MyValue");
        elt.appendChild(elt1);
        
        Element elt2 = doc.createElementNS("http://myns2", "key2");
        elt1.appendChild(elt2);
        
        Text text = doc.createTextNode("Hello");
        elt2.appendChild(text);
        
        expression.pushEnv();
        expression.setVariable("arg", "MyValue");
        NodeList nodeList = (NodeList) expression.evaluate(elt, (short)0, null);
        expression.popEnv();
        
        Assert.assertEquals(1, nodeList.getLength());
        
        Element eltRes = (Element) nodeList.item(0);
        Assert.assertEquals("key2", eltRes.getLocalName());
        Assert.assertEquals("http://myns2", eltRes.getNamespaceURI());
        
        expression = evaluator.createExpression("/*/*[local-name()=$arg]", nsResolver);
        
        expression.pushEnv();
        expression.setVariable("arg", "key2");
        nodeList = (NodeList) expression.evaluate(elt, (short)0, null);
        expression.popEnv();
        
        Assert.assertEquals(1, nodeList.getLength());
        
        eltRes = (Element) nodeList.item(0);
        Assert.assertEquals("key2", eltRes.getLocalName());
        Assert.assertEquals("http://myns2", eltRes.getNamespaceURI());
        
        
    }
}
