package org.xemeiah.dom;

import org.junit.Test;
import org.w3c.dom.DOMException;
import org.w3c.dom.Document;
import org.w3c.dom.Node;
import org.w3c.dom.Text;

public class TestDomText extends AbstractVolatileDocumentTest
{
    @Test(expected=DOMException.class)
    public void testTextGetFirstChildFails()
    {
        Document document = createDocument();
        Text text = document.createTextNode("Some text");
        text.getFirstChild();
    }
    
    @Test(expected=DOMException.class)
    public void testTextGetLastChildFails()
    {
        Document document = createDocument();
        Text text = document.createTextNode("Some text");
        text.getLastChild();
    }
    
    @Test(expected=DOMException.class)
    public void testTextAppendChildFails()
    {
        Document document = createDocument();
        Text text = document.createTextNode("Some text");
        Node newChild= document.createElement("newChild");
        text.appendChild(newChild);
    }

}
