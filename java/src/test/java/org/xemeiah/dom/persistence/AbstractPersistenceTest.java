package org.xemeiah.dom.persistence;

import org.junit.After;
import org.junit.Before;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xemeiah.dom.DocumentFactory;

public abstract class AbstractPersistenceTest
{
    private static final Logger LOGGER = LoggerFactory.getLogger(AbstractPersistenceTest.class);

    private DocumentFactory documentFactory;
    
    @Before
    public void init()
    {
        documentFactory = new DocumentFactory();
        String filename = "target/xem_" + System.currentTimeMillis() + "_dat.xem";
        documentFactory.format(filename);
        documentFactory.open(filename);
    }

    @After
    public void tearDown()
    {
        documentFactory.close();
        LOGGER.info("At tearDown() : calling final System.gc()");
        System.gc();
    }
    
    public DocumentFactory getDocumentFactory()
    {
        return documentFactory;
    }
}
