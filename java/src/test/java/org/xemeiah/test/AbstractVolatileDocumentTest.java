package org.xemeiah.test;

import org.junit.After;
import org.junit.Before;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xemeiah.dom.DocumentFactory;

public abstract class AbstractVolatileDocumentTest
{
    private static final Logger LOGGER = LoggerFactory.getLogger(AbstractVolatileDocumentTest.class);

    protected DocumentFactory documentFactory;

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
