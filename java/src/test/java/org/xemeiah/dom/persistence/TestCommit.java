package org.xemeiah.dom.persistence;

import org.junit.Assert;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.w3c.dom.Element;
import org.w3c.dom.Text;
import org.xemeiah.dom.Document;
import org.xemeiah.it_tests.TestMassiveVolatileDocuments;

public class TestCommit extends AbstractPersistenceTest
{
    private static final Logger LOGGER = LoggerFactory.getLogger(TestMassiveVolatileDocuments.class);

    @Test
    public void testCommit()
    {
        String branchName = "main";
        getDocumentFactory().createBranch(branchName, "");

        {
            Document document = getDocumentFactory().newStandaloneDocument(branchName, "write");
            Element root = document.createElement("Root");
            document.getDocumentElement().appendChild(root);

            Text textNode = document.createTextNode("Some value");
            root.appendChild(textNode);

            document.commit();
        }
        /**
         * Now check
         */
        {
            Document document = getDocumentFactory().newStandaloneDocument(branchName, "read");
            Element root = document.getDocumentElement().getFirstChild();
            Assert.assertEquals("Root", root.getNodeName());

            Text textNode = (Text) root.getFirstChild();
            Assert.assertEquals("Some value", textNode.getTextContent());
        }
        /**
         * Reopen, to change values
         */
        {
            Document document = getDocumentFactory().newStandaloneDocument(branchName, "write");
            Element root = document.getDocumentElement().getFirstChild();
            Assert.assertEquals("Root", root.getNodeName());

            Text textNode = (Text) root.getFirstChild();
            Assert.assertEquals("Some value", textNode.getTextContent());
            textNode.setData("Another value");

            document.commit();
        }
        /**
         * Finally, test that the commit has changed values
         */
        {
            Document document = getDocumentFactory().newStandaloneDocument(branchName, "read");
            Element root = document.getDocumentElement().getFirstChild();
            Assert.assertEquals("Root", root.getNodeName());

            Text textNode = (Text) root.getFirstChild();
            Assert.assertEquals("Another value", textNode.getTextContent());
        }
    }
}
