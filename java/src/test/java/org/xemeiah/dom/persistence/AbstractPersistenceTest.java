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
        LOGGER.info("Starting documentFactory...");
        documentFactory = new DocumentFactory();
        String filename = "target/xem_" + System.currentTimeMillis() + "_dat.xem";
        documentFactory.format(filename);
        documentFactory.open(filename);
    }

    @After
    public void tearDown()
    {
        documentFactory.check();
        LOGGER.info("At tearDown() : calling System.gc() : 1");
        System.gc();
        try
        {
            Thread.sleep(100);
        }
        catch (InterruptedException e1)
        {
        }
        documentFactory = null;
        LOGGER.info("At tearDown() : calling System.gc() : 2");
        System.gc();
        try
        {
            Thread.sleep(100);
        }
        catch (InterruptedException e1)
        {
        }
        verifyFinalizerExceptions();
        LOGGER.info("At tearDown() : Ok !");
    }

    private void verifyFinalizerExceptions()
    {
        System.gc();
        try
        {
            Thread.sleep(10);
        }
        catch (InterruptedException e)
        {
        }
        if (DocumentFactory.isExceptionsHappenedInFinalizer())
        {
            throw new RuntimeException("Exceptions happened in object Finalizers !");
        }
    }

    public DocumentFactory getDocumentFactory()
    {
        return documentFactory;
    }
}
