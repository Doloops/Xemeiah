package org.xemeiah.test;

import org.junit.Test;
import org.w3c.dom.DOMException;
import org.w3c.dom.Document;
import org.w3c.dom.Element;

public class TestDomDocument extends AbstractVolatileDocumentTest
{
    @Test(expected=DOMException.class)
    public void testDocumentAppendChildFails()
    {
        Document document = documentFactory.newVolatileDocument();
        
        Element element = document.createElement("MyFakeRoot");
        document.appendChild(element);
    }
}
