package org.xemeiah.dom;

import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.w3c.dom.Document;
import org.w3c.dom.Element;

public class TestVolatileDocument extends AbstractVolatileDocumentTest
{
    private static final Logger LOGGER = LoggerFactory.getLogger(TestVolatileDocument.class);

    @Test
    public void testCreateReleaseFinalize()
    {
        if ( true )
        {
            createSimpleVolatileDocument();
        }
        LOGGER.info("Calling gc()");
        System.gc();
        LOGGER.info("Called gc()");
    }

    private void createSimpleVolatileDocument()
    {
        Document document = createDocument();
        Element newChild = document.createElement("Toto");
        document.getDocumentElement().appendChild(newChild);
        
        releaseDocument(document);
    }
}
