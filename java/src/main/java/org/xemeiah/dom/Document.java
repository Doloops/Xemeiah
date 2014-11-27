package org.xemeiah.dom;

import javax.xml.transform.TransformerException;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.w3c.dom.DOMConfiguration;
import org.w3c.dom.DOMException;
import org.w3c.dom.DOMImplementation;
import org.w3c.dom.DocumentFragment;
import org.w3c.dom.DocumentType;
import org.w3c.dom.Element;
import org.w3c.dom.EntityReference;
import org.w3c.dom.Node;
import org.w3c.dom.ProcessingInstruction;
import org.w3c.dom.Text;

public class Document extends org.xemeiah.dom.Element implements org.w3c.dom.Document
{
    private static final Logger LOGGER = LoggerFactory.getLogger(Document.class);

    private final DocumentFactory documentFactory;

    private final long __documentPtr;

    private final long __xprocessorPtr;

    private Document(DocumentFactory documentFactory, long __documentPtr, long __xprocessorPtr)
    {
        super(null, 0);

        this.document = this;
        this.documentFactory = documentFactory;
        this.__documentPtr = __documentPtr;
        this.__xprocessorPtr = __xprocessorPtr;
    }

    private native void cleanUp();

    protected void finalize()
    {
        cleanUp();
    }

    public final DocumentFactory getDocumentFactory()
    {
        return documentFactory;
    }

    public native void commit();

    public native void reopen();

    @Override
    public short getNodeType()
    {
        return org.w3c.dom.Node.DOCUMENT_NODE;
    }

    @Override
    public native String toString();

    @Override
    public native String getBaseURI();

    @Override
    public void serializeTo(java.io.OutputStream out) throws TransformerException
    {
        getDocumentElement().serializeTo(out);
    }

    @Override
    public org.xemeiah.dom.Attr createAttribute(String name) throws DOMException
    {
        return createAttributeNS(null, name);
    }

    @Override
    public native org.xemeiah.dom.Attr createAttributeNS(String namespaceURI, String qualifiedName) throws DOMException;

    @Override
    public native org.xemeiah.dom.CDATASection createCDATASection(String data) throws DOMException;

    @Override
    public native org.xemeiah.dom.Comment createComment(String data);

    @Override
    public native Text createTextNode(String data) throws DOMException;

    @Override
    public DocumentFragment createDocumentFragment()
    {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public Element createElement(String tagName) throws DOMException
    {
        return createElementNS(null, tagName);
    }

    @Override
    public native Element createElementNS(String namespaceURI, String qualifiedName) throws DOMException;

    @Override
    public Node appendChild(Node newChild) throws DOMException
    {
        throw new DOMException(DOMException.INVALID_ACCESS_ERR, "Could not add child to Document, I already have a Root element !");
    }

    /*
     * **************************************************************************
     * ********* Everything beyond this point is just NOT IMPLEMENTED ! ********* 
     * **************************************************************************
     */
    
    @Override
    public Node adoptNode(Node source) throws DOMException
    {
        throw new RuntimeException("NOT IMPLEMENTED !");
    }
    
    
    @Override
    public EntityReference createEntityReference(String name) throws DOMException
    {
        throw new RuntimeException("NOT IMPLEMENTED !");
    }

    @Override
    public ProcessingInstruction createProcessingInstruction(String target, String data) throws DOMException
    {
        throw new RuntimeException("NOT IMPLEMENTED !");
    }

    @Override
    public DocumentType getDoctype()
    {
        throw new RuntimeException("NOT IMPLEMENTED !");
    }

    @Override
    public native org.xemeiah.dom.Element getDocumentElement() throws DOMException;

    @Override
    public String getDocumentURI()
    {
        throw new RuntimeException("NOT IMPLEMENTED !");
    }

    @Override
    public DOMConfiguration getDomConfig()
    {
        throw new RuntimeException("NOT IMPLEMENTED !");
    }

    @Override
    public native Element getElementById(String elementId);

    @Override
    public DOMImplementation getImplementation()
    {
        throw new RuntimeException("NOT IMPLEMENTED !");
    }

    @Override
    public String getInputEncoding()
    {
        throw new RuntimeException("NOT IMPLEMENTED !");
    }

    @Override
    public boolean getStrictErrorChecking()
    {
        throw new RuntimeException("NOT IMPLEMENTED !");
    }

    @Override
    public String getXmlEncoding()
    {
        throw new RuntimeException("NOT IMPLEMENTED !");
    }

    @Override
    public boolean getXmlStandalone()
    {
        throw new RuntimeException("NOT IMPLEMENTED !");
    }

    @Override
    public String getXmlVersion()
    {
        throw new RuntimeException("NOT IMPLEMENTED !");
    }

    @Override
    public Node importNode(Node importedNode, boolean deep) throws DOMException
    {
        throw new RuntimeException("NOT IMPLEMENTED !");
    }

    @Override
    public void normalizeDocument()
    {
        throw new RuntimeException("NOT IMPLEMENTED !");
    }

    @Override
    public Node renameNode(Node n, String namespaceURI, String qualifiedName) throws DOMException
    {
        throw new RuntimeException("NOT IMPLEMENTED !");
    }

    @Override
    public void setDocumentURI(String documentURI)
    {
        LOGGER.debug("Setting documentURI=" + documentURI);
    }

    @Override
    public void setStrictErrorChecking(boolean strictErrorChecking)
    {
        LOGGER.debug("Setting strictErrorChecking=" + strictErrorChecking);
    }

    @Override
    public void setXmlStandalone(boolean xmlStandalone) throws DOMException
    {
        LOGGER.debug("Setting xmlStandalone=" + xmlStandalone);
    }

    @Override
    public void setXmlVersion(String xmlVersion) throws DOMException
    {
        LOGGER.debug("Setting xmlVersion=" + xmlVersion);
    }
}
