package org.xemeiah.dom;

import java.io.IOException;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.w3c.dom.DOMImplementation;
import org.xemeiah.dom.parser.XemParser;
import org.xml.sax.EntityResolver;
import org.xml.sax.ErrorHandler;
import org.xml.sax.InputSource;
import org.xml.sax.SAXException;

public class DocumentFactory extends javax.xml.parsers.DocumentBuilder
{
    private static final Logger LOGGER = LoggerFactory.getLogger(DocumentFactory.class);

    static
    {
        System.loadLibrary("xemeiah");
    }

    private long __storePtr = 0;

    private native void cleanUp();

    public native void openVolatile();

    public native void format(String fileName);

    public native void open(String fileName);

    private native void close();
    
    public native void check();

    public native void releaseDocument(Document document);

    public DocumentFactory()
    {
    }

    protected static boolean exceptionsHappenedInFinalizer = false;
    
    public static boolean isExceptionsHappenedInFinalizer()
    {
        return exceptionsHappenedInFinalizer;
    }

    protected void finalize()
    {
        try
        {
            cleanUp();
        }
        catch (RuntimeException e)
        {
            exceptionsHappenedInFinalizer = true;
            LOGGER.error("Could not finalize() !", e);
            throw (e);
        }
    }

    private final XemParser xemParser = new XemParser();

    public final XemParser getXemParser()
    {
        return xemParser;
    }

    private native String doCreateBranch(String branchName, String branchFlags);

    /**
     * Create a new Branch
     * @param branchName
     * @param branchFlags
     * @return the new branch name, can be different upon branchFlags 
     */
    public String createBranch(String branchName, String branchFlags)
    {
        return doCreateBranch(branchName, branchFlags);
    }
    
    public native org.xemeiah.dom.Document newStandaloneDocument(String branchName, String branchFlags);

    public native org.xemeiah.dom.Document newVolatileDocument();

    public native void process(org.w3c.dom.Element processElement, org.w3c.dom.NodeList initialNodeSet);

    public org.w3c.dom.Document newDocument()
    {
        return newVolatileDocument();
    }

    @Override
    public org.w3c.dom.Document parse(InputSource inputSource) throws SAXException, IOException
    {
        org.w3c.dom.Document doc = newDocument();
        xemParser.parse(doc.getDocumentElement(), inputSource);
        return doc;
    }

    @Override
    public org.w3c.dom.Document parse(String uri) throws SAXException, IOException
    {
        org.w3c.dom.Document doc = newDocument();
        xemParser.parse(doc.getDocumentElement(), uri);
        return doc;
    }

    @Override
    public native boolean isNamespaceAware();

    @Override
    public native boolean isValidating();

    @Override
    public DOMImplementation getDOMImplementation()
    {
        throw new RuntimeException("Not implemented : getDOMImplementation()");
    }

    @Override
    public void setEntityResolver(EntityResolver er)
    {
        throw new RuntimeException("Not implemented : setEntityResolver()");
    }

    @Override
    public void setErrorHandler(ErrorHandler eh)
    {
        throw new RuntimeException("Not implemented : setErrorHandler()");
    }
}
