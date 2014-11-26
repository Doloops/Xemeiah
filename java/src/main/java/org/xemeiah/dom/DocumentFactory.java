package org.xemeiah.dom;

import java.io.IOException;

import org.w3c.dom.DOMImplementation;
import org.xemeiah.dom.parser.XemParser;
import org.xml.sax.EntityResolver;
import org.xml.sax.ErrorHandler;
import org.xml.sax.InputSource;
import org.xml.sax.SAXException;

public class DocumentFactory extends javax.xml.parsers.DocumentBuilder
{
    private final XemParser xemParser = new XemParser();

    private long __storePtr = 0;

    public native void openVolatile();
    
    public native void format(String fileName);
    
    public native void open(String fileName);
    
    public native void close();
    
    public native void releaseDocument(Document document);

    static
    {
        System.loadLibrary("xemeiah");
    }

    public DocumentFactory()
    {
    }

    public final XemParser getXemParser()
    {
        return xemParser;
    }
    
    public native void createBranch(String branchName, String branchFlags);
    
    public native org.xemeiah.dom.Document newStandaloneDocument(String branchName, String branchFlags);

    public native org.xemeiah.dom.Document newVolatileDocument();
    
    
    public native void process (org.w3c.dom.Element processElement, org.w3c.dom.NodeList initialNodeSet);
    
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
