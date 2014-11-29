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
}
