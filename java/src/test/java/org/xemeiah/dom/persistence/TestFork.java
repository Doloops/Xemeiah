package org.xemeiah.dom.persistence;

import org.junit.Assert;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.w3c.dom.Element;
import org.w3c.dom.Text;
import org.xemeiah.dom.Document;

public class TestFork extends AbstractPersistenceTest
{
    private static final Logger LOGGER = LoggerFactory.getLogger(TestFork.class);

    @Test
    public void testSimpleFork()
    {
        String branchName = "main";
        String forkName = "forked";
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
         * Create a fork, to change values
         */
        // getDocumentFactory().createBranch(forkName, "", branchName);
        {
            Document document = getDocumentFactory().newStandaloneDocument(branchName, "read");
            document.createForkBranch(forkName, "");

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
            Document document = getDocumentFactory().newStandaloneDocument(forkName, "read");
            Element root = document.getDocumentElement().getFirstChild();
            Assert.assertEquals("Root", root.getNodeName());

            Text textNode = (Text) root.getFirstChild();
            Assert.assertEquals("Another value", textNode.getTextContent());
        }

        /**
         * But main branch did not change at all
         */
        {
            Document document = getDocumentFactory().newStandaloneDocument(branchName, "read");
            Element root = document.getDocumentElement().getFirstChild();
            Assert.assertEquals("Root", root.getNodeName());

            Text textNode = (Text) root.getFirstChild();
            Assert.assertEquals("Some value", textNode.getTextContent());
        }
    }

    @Test
    public void testThreeLevels()
    {
        String branchName = "main";
        String forkName = "forked";
        String fork2Name = "second forked";
        getDocumentFactory().createBranch(branchName, "");

        {
            Document document = getDocumentFactory().newStandaloneDocument(branchName, "write");

            Element root = document.createElement("Root");
            document.getDocumentElement().appendChild(root);

            Text text = document.createTextNode("First");
            root.appendChild(text);

            document.commit();
        }
        
        LOGGER.info("forkName=" + forkName);

        Document forkedDocument = getDocumentFactory().newStandaloneDocument(branchName, "read");
        forkedDocument.createForkBranch(forkName, "");

        LOGGER.info("forkedDocument=" + forkedDocument);

        {
            Element root = forkedDocument.getDocumentElement().getFirstChild();
            Assert.assertEquals("First", root.getFirstChild().getTextContent());

            Text text = (Text) root.getFirstChild();
            text.setTextContent("Second");

            forkedDocument.commit();
        }


        Document forked2Document = getDocumentFactory().newStandaloneDocument(forkName, "read");
        forked2Document.createForkBranch(fork2Name, "");

        Element root = forked2Document.getDocumentElement().getFirstChild();
        Assert.assertEquals("Second", root.getFirstChild().getTextContent());

        Text text = (Text) root.getFirstChild();
        text.setTextContent("Third");

        forked2Document.commit();

        Document finalDocument = getDocumentFactory().newStandaloneDocument(fork2Name, "read");

        root = finalDocument.getDocumentElement().getFirstChild();
        Assert.assertEquals("Third", root.getFirstChild().getTextContent());

        Document initialDocument = getDocumentFactory().newStandaloneDocument(branchName, "read");

        root = initialDocument.getDocumentElement().getFirstChild();
        Assert.assertEquals("First", root.getFirstChild().getTextContent());
    }
}
