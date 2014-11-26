package org.xemeiah.test;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.xemeiah.dom.DocumentFactory;

public class TestVolatileDocument
{
    private static final Logger LOGGER = LoggerFactory.getLogger(TestVolatileDocument.class);

    private DocumentFactory documentFactory;

    @Before
    public void init()
    {
        documentFactory = new DocumentFactory();
        documentFactory.openVolatile();
    }

    @After
    public void tearDown()
    {
        System.gc();
    }

    @Test
    public void testMassiveVolatileDocumentCreation()
    {
        for (int nbTest = 0; nbTest < 1000 * 1000; nbTest++)
        {
            LOGGER.info("At test=#" + nbTest);
            Document document = documentFactory.newVolatileDocument();

            Element node = document.createElement("myElement");
            document.getDocumentElement().appendChild(node);

            for (int nbNode = 0; nbNode < 1000; nbNode++)
            {
                Element subNode = document.createElement("child_" + nbNode);
                node.appendChild(subNode);
            }

            if (nbTest % 100 == 0)
            {
                LOGGER.info("Call System.gc()");
                System.gc();
            }
        }
    }
}
