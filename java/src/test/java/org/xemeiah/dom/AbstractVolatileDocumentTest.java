package org.xemeiah.dom;

import org.junit.After;
import org.junit.Before;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.w3c.dom.Document;
import org.xemeiah.dom.DocumentFactory;

public abstract class AbstractVolatileDocumentTest
{
    private static final Logger LOGGER = LoggerFactory.getLogger(AbstractVolatileDocumentTest.class);

    private DocumentFactory documentFactory;

    protected Document createDocument()
    {
        return documentFactory.newVolatileDocument();
    }
    
    @Before
    public void init()
    {
        documentFactory = new DocumentFactory();
        documentFactory.openVolatile();
    }

    @After
    public void tearDown()
    {
        LOGGER.info("At tearDown() : calling final System.gc()");
        System.gc();
    }
}
