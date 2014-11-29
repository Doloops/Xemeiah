package org.xemeiah.it_tests;

import java.util.List;

import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xemeiah.dom.Document;
import org.xemeiah.dom.persistence.AbstractPersistenceTest;

import com.google.common.collect.Lists;

public class TestMassivePersistentDocuments extends AbstractPersistenceTest
{
    private static final Logger LOGGER = LoggerFactory.getLogger(TestMassivePersistentDocuments.class);

    @Test
    public void testCreateDocumentsInMultipleBranches()
    {
        List<Document> createdDocs = Lists.newArrayList();
        for ( int i = 0 ; i < 100 ; i++ )
        {
            String branchName = "Branch_" + i;
            getDocumentFactory().createBranch(branchName, "");
            
            Document document = getDocumentFactory().newStandaloneDocument(branchName, "write");
            
            DomTestUtils.fillDocumentWithNodes(document);
            
            document.commit();
            
            getDocumentFactory().releaseDocument(document);
            
            createdDocs.add(document);
        }
        createdDocs.clear();
        
        System.gc();
    }

    @Test
    public void testCreateDocumentsInSameBranch()
    {
        String branchName = "Main";
        getDocumentFactory().createBranch(branchName, "");
        
        for ( int i = 0 ; i < 100 ; i++ )
        {
            Document document = getDocumentFactory().newStandaloneDocument(branchName, "write");
            
            DomTestUtils.fillDocumentWithNodes(document);
            
            document.commit();
            
            getDocumentFactory().releaseDocument(document);
        }
    }

}
