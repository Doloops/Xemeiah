package org.xemeiah.dom.persistence;

import org.junit.Assert;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.w3c.dom.Element;
import org.w3c.dom.Text;
import org.xemeiah.dom.Document;

public class TestMerge extends AbstractPersistenceTest
{
    private static final Logger LOGGER = LoggerFactory.getLogger(TestMerge.class);

    @Test
    public void testSimpleMerge()
    {
        String branchName = "main";
        getDocumentFactory().createBranch(branchName, "");

        Document document = getDocumentFactory().newStandaloneDocument(branchName, "write");
        {
            Element root = document.createElement("Root");
            document.getDocumentElement().appendChild(root);

            Text textNode = document.createTextNode("Initial");
            root.appendChild(textNode);

            document.commit();
        }
        String forkedBranch = "forked";
        document.createForkBranch(forkedBranch, "");
        {
            Element root = document.getDocumentElement().getFirstChild();
            Assert.assertEquals("Root", root.getNodeName());

            Text textNode = (Text) root.getFirstChild();
            Assert.assertEquals("Initial", textNode.getTextContent());
            
            textNode.setTextContent("Value From Forked");
        }
        document.commit();
        document.merge();
        
        document = getDocumentFactory().newStandaloneDocument(branchName, "read");
        {
            Element root = document.getDocumentElement().getFirstChild();
            Assert.assertEquals("Root", root.getNodeName());

            Text textNode = (Text) root.getFirstChild();
            Assert.assertEquals("Value From Forked", textNode.getTextContent());
        }
        
        LOGGER.info("Test Merge Ok !");
    }

    @Test
    public void testOneBranchMerge()
    {
        String branchName = "main";
        getDocumentFactory().createBranch(branchName, "");

        {
            Document document = getDocumentFactory().newStandaloneDocument(branchName, "write");
            Element root = document.createElement("Root");
            document.getDocumentElement().appendChild(root);

            Element elt0 = document.createElement("elt");
            root.appendChild(elt0);
            Text textNode = document.createTextNode("Initial");
            elt0.appendChild(textNode);

            document.commit();
        }
        String forkedBranch = "forked";
        Document document1 = getDocumentFactory().newStandaloneDocument(branchName, "read");
        {
            document1.createForkBranch(forkedBranch, "");
            Element root = document1.getDocumentElement().getFirstChild();
            Assert.assertEquals("Root", root.getNodeName());

            Element elt1 = document1.createElement("elt");
            root.appendChild(elt1);
            
            Text textNode = document1.createTextNode("Branch 1");
            elt1.appendChild(textNode);
            
            document1.commit();
        }
        document1.merge();
        
        Document finalDocument = getDocumentFactory().newStandaloneDocument(branchName, "read");
        {
            Element root = finalDocument.getDocumentElement().getFirstChild();
            Assert.assertEquals("Root", root.getNodeName());
            Assert.assertEquals(2, root.getChildNodes().getLength());
            
            Assert.assertEquals("elt", root.getChildNodes().item(0).getNodeName());
            Assert.assertEquals("elt", root.getChildNodes().item(1).getNodeName());

            Assert.assertEquals("#text", root.getChildNodes().item(0).getFirstChild().getNodeName());
            
            // Assert.assertEquals("Initial", root.getChildNodes().item(0).getFirstChild().getTextContent());
            // Assert.assertEquals("Branch 1", root.getChildNodes().item(1).getFirstChild().getTextContent());
        }
    }

    @Test
    public void testTwoBranchesMerge()
    {
        String branchName = "main";
        getDocumentFactory().createBranch(branchName, "");

        {
            Document document = getDocumentFactory().newStandaloneDocument(branchName, "write");
            Element root = document.createElement("Root");
            document.getDocumentElement().appendChild(root);

            Element elt0 = document.createElement("elt");
            root.appendChild(elt0);
            Text textNode = document.createTextNode("Initial");
            elt0.appendChild(textNode);

            document.commit();
        }
        String forkedBranch = "forked";
        Document document1 = getDocumentFactory().newStandaloneDocument(branchName, "read");
        {
            document1.createForkBranch(forkedBranch, "");
            Element root = document1.getDocumentElement().getFirstChild();
            Assert.assertEquals("Root", root.getNodeName());

            Element elt1 = document1.createElement("elt");
            root.appendChild(elt1);
            
            Text textNode = document1.createTextNode("Branch 1");
            elt1.appendChild(textNode);
            
            document1.commit();
        }
        String forkedBranch2 = "forked 2";
        Document document2 = getDocumentFactory().newStandaloneDocument(branchName, "read");
        {
            document2.createForkBranch(forkedBranch2, "");
            Element root = document2.getDocumentElement().getFirstChild();
            Assert.assertEquals("Root", root.getNodeName());

            Element elt1 = document2.createElement("elt");
            root.appendChild(elt1);
            
            Text textNode = document2.createTextNode("Branch 2");
            elt1.appendChild(textNode);
            
            document2.commit();
        }
        
        document1.merge();
        document2.merge();
        
        Document document3 = getDocumentFactory().newStandaloneDocument(branchName, "read");
        {
            Element root = document3.getDocumentElement().getFirstChild();
            Assert.assertEquals("Root", root.getNodeName());
            Assert.assertEquals(3, root.getChildNodes().getLength());
            
            Assert.assertEquals("Initial", root.getChildNodes().item(0).getFirstChild().getTextContent());
            Assert.assertEquals("Branch 1", root.getChildNodes().item(1).getFirstChild().getTextContent());
            Assert.assertEquals("Branch 2", root.getChildNodes().item(2).getFirstChild().getTextContent());
        }
    }
}
