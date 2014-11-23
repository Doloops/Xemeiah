package org.xemeiah.test;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.UUID;

import org.junit.Assert;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import org.w3c.dom.Text;
import org.w3c.dom.xpath.XPathEvaluator;
import org.w3c.dom.xpath.XPathExpression;
import org.w3c.dom.xpath.XPathNSResolver;
import org.xemeiah.dom.DocumentFactory;
import org.xemeiah.dom.xpath.XPathResult;
import org.xml.sax.SAXException;

public class TestLoadDocument
{
    private static final Logger LOGGER = LoggerFactory.getLogger(TestLoadDocument.class);

    public static void main(String arg[]) throws SAXException, IOException
    {
        new TestLoadDocument().test();
    }
    
    @Test
    public void test() throws SAXException, IOException
    {
        new File("test.dat").delete();

        long start = System.currentTimeMillis();

        DocumentFactory documentFactory = new DocumentFactory();
        
        documentFactory.format("test.dat");
        documentFactory.open("test.dat");
        documentFactory.createBranch("main", "");

        XPathEvaluator xpathEvaluator = new org.xemeiah.dom.xpath.XPathEvaluator(documentFactory);

        org.xemeiah.dom.Document rootDocument = documentFactory.newStandaloneDocument("main", "write");
        LOGGER.info("rootDocument => " + rootDocument);

        int nbSamples = 100 * 1000;

        int chosen = (int) (Math.random() * (double) nbSamples);

        String chosenId = null;

        LOGGER.info("createElementNS()");
        Element subRoot = rootDocument.createElementNS("http://www.xemeiah.org/ns/xem", "collection");
        LOGGER.info("setPrefix() => subRoot=" + subRoot);
        subRoot.setPrefix("xem");
        
        LOGGER.info("documentRootElement()");
        org.xemeiah.dom.Element documentRootElement = rootDocument.getDocumentElement();
        LOGGER.info("documentRootElement() => " + documentRootElement);
        documentRootElement.appendChild(subRoot);
        
        LOGGER.info("appendChild() ok");

        XPathExpression xpathExpression = null;
        String xpathExpr = "./doc:Id/text()";

        for (int nbdoc = 0; nbdoc < nbSamples; nbdoc++)
        {

            InputStream is = new FileInputStream("src/test/resources/doc2.xml");
            documentFactory.getXemParser().parse(subRoot, is);
            is.close();

            UUID uuid = UUID.randomUUID();

            if (false)
            {
                Element documentElement = (Element) subRoot.getLastChild();
                Assert.assertEquals(documentElement.getLocalName(), "Document");

                Node child = documentElement.getFirstChild();
                while (child != null)
                {
                    LOGGER.info("child : " + child.getNodeName());
                    child = child.getNextSibling();
                }
            }
            
            if ( false )
            {
                Element documentElement = (Element) subRoot.getLastChild();
                xpathExpression = xpathEvaluator.createExpression(xpathExpr, (XPathNSResolver) documentElement);
            }

            if (true)
            {
                // LOGGER.debug("pre-getLastChild");
                Element documentElement = (Element) subRoot.getLastChild();
                // LOGGER.debug("post-getLastChild");

                if (xpathExpression == null)
                {
                    xpathExpression = xpathEvaluator.createExpression(xpathExpr, (XPathNSResolver) documentElement);
                    // xpathExpression = XPathFactory.newInstance().newXPath().compile(xpathExpr);
                }

                // LOGGER.debug("pre-evaluate");
                Object xpathResult = xpathExpression.evaluate(documentElement, (short) 0, null);
                Assert.assertEquals(xpathResult.getClass(), XPathResult.class);
                org.w3c.dom.NodeList result = (NodeList) xpathResult;
                // LOGGER.debug("post-evaluate");

                if (result.getLength() != 1)
                {
                    throw new RuntimeException("Too much results !");
                }

                Node resultNode = result.item(0);
                if (true)
                {
                    // LOGGER.debug("pre-cast text");
                    Text text = (Text) resultNode;
                    // LOGGER.debug("post-cast text : value=" + text.getData());

                    text.setData(uuid.toString());
                    // LOGGER.debug("After modif => " + text.getData());
                }

                if (false)
                {
                    result = (NodeList) xpathResult;
                    Text text = (Text) resultNode;
                    LOGGER.info("Text data : " + text.getData());
                }
            }
            if (nbdoc == chosen)
            {
                chosenId = uuid.toString();
                LOGGER.info("Chosen id : " + chosenId);
            }

            if (nbdoc % 10000 == 0)
            {
                LOGGER.info("#" + nbdoc + " : " + uuid.toString());
            }
        }
        rootDocument.commit();
        long cmt = System.currentTimeMillis();

        // rootDocument.reopen();
        {
            // String xpathCount = "./*/doc:Document/doc:Id/text()"; //
            // */doc:Document/doc:Id/text()";
            String xpathCount = "./*/doc:Document[common:Tags/common:Tag[@name='Weather']/common:Value='Sunny']";
            org.w3c.dom.NodeList result = (NodeList) xpathEvaluator.evaluate(xpathCount,
                    documentRootElement, (XPathNSResolver) documentRootElement
                            .getFirstChild().getFirstChild(), (short) 0, null);
            // dumpResults(result);
            LOGGER.info("result=" + result.getLength());

            if (result.getLength() != nbSamples)
            {
                throw new RuntimeException("Wrong number of results " + result.getLength() + ", expected " + nbSamples
                        + "!");
            }
        }
        long postCount = System.currentTimeMillis();

        {
            String xpathCount = "./*/doc:Document[doc:Id/text() = '" + chosenId + "']"; // */doc:Document/doc:Id/text()";

            LOGGER.info("Xpath query=" + xpathCount);
            org.w3c.dom.NodeList result = (NodeList) xpathEvaluator.evaluate(xpathCount,
                    documentRootElement, (XPathNSResolver) documentRootElement
                            .getFirstChild().getFirstChild(), (short) 0, null);

            LOGGER.info("result=" + result.getLength());
            dumpResults(result);

            if (result.getLength() != 1)
            {
                throw new RuntimeException("Wrong number of results " + result.getLength() + ", expected " + 1 + "!");
            }
        }

        long fin = System.currentTimeMillis();
        LOGGER.info("Took : " + (fin - start) + "ms" + ", cmt=" + (cmt - start) + "ms, query=" + (postCount - cmt)
                + "ms, byId=" + (fin - postCount) + "ms");
    }

    private void dumpResults(org.w3c.dom.NodeList result)
    {
        LOGGER.info("Result : " + result.getLength());
        for (int i = 0; i < result.getLength(); i++)
        {
            Node node = result.item(i);
            if (node instanceof Text)
            {
                Text text = (Text) result.item(i);
                LOGGER.info("#" + i + " : " + node.getNodeName() + " : " + text.getData());
            }
            else if (node instanceof Element)
            {
                LOGGER.info("#" + i + " : Element " + node.getNodeName());
            }
        }
    }

}
