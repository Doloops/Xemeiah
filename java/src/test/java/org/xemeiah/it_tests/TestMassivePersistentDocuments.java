package org.xemeiah.it_tests;

import java.util.List;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xemeiah.dom.Document;
import org.xemeiah.dom.DocumentFactory;

import com.google.common.collect.Lists;

public class TestMassivePersistentDocuments
{
    private static final Logger LOGGER = LoggerFactory.getLogger(TestMassivePersistentDocuments.class);

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
        LOGGER.info("At tearDown() : calling final System.gc()");
        System.gc();
    }
    
    @Test
    public void testCreateDocuments()
    {
        List<Document> createdDocs = Lists.newArrayList();
        for ( int i = 0 ; i < 1000 ; i++ )
        {
            String branchName = "Branch_" + i;
            documentFactory.createBranch(branchName, "");
            
            Document document = documentFactory.newStandaloneDocument(branchName, "write");
            
            DomTestUtils.fillDocumentWithNodes(document);
            
            document.commit();
            
            documentFactory.releaseDocument(document);
            
            createdDocs.add(document);
        }
        createdDocs.clear();
        
        System.gc();
    }

}
