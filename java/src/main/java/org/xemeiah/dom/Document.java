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

    private Document(DocumentFactory documentFactory, long __documentPtr)
    {
        super(null, 0);
        
        // LOGGER.info("New " + Document.class.getName() + " with documentFactory=" + documentFactory + ", __documentPtr=0x" + Long.toHexString(__documentPtr));
        
        this.document = this;
        this.documentFactory = documentFactory;
        this.__documentPtr = __documentPtr;
    }

    private final long __documentPtr;

    private final DocumentFactory documentFactory;

    public final DocumentFactory getDocumentFactory()
    {
        return documentFactory;
    }

    private long getDocumentPtr()
    {
        return this.__documentPtr;
    }

    public native void commit();

    public native void reopen();

    public short getNodeType()
    {
        return org.w3c.dom.Node.DOCUMENT_NODE;
    }

    public native String toString();

    public native String getBaseURI();

    // public native void serializeTo ( OutputStream out );
    public void serializeTo(java.io.OutputStream out) throws TransformerException
    {
        getDocumentElement().serializeTo(out);
    }

    @Override
    public Node adoptNode(Node source) throws DOMException
    {
        // TODO Auto-generated method stub
        return null;
    }

    public org.xemeiah.dom.Attr createAttribute(String name) throws DOMException
    {
        return createAttributeNS(null, name);
    }

    public native org.xemeiah.dom.Attr createAttributeNS(String namespaceURI, String qualifiedName) throws DOMException;

    public native org.xemeiah.dom.CDATASection createCDATASection(String data) throws DOMException;

    public native org.xemeiah.dom.Comment createComment(String data);

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
    public EntityReference createEntityReference(String name) throws DOMException
    {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public ProcessingInstruction createProcessingInstruction(String target, String data) throws DOMException
    {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public native Text createTextNode(String data);

    @Override
    public DocumentType getDoctype()
    {
        throw new RuntimeException ("NOT IMPLEMENTED !");
    }

    @Override
    public native org.xemeiah.dom.Element getDocumentElement();

    @Override
    public String getDocumentURI()
    {
        throw new RuntimeException ("NOT IMPLEMENTED !");
    }

    @Override
    public DOMConfiguration getDomConfig()
    {
        throw new RuntimeException ("NOT IMPLEMENTED !");
    }

    @Override
    public native Element getElementById(String elementId);

    @Override
    public DOMImplementation getImplementation()
    {
        throw new RuntimeException ("NOT IMPLEMENTED !");
    }

    @Override
    public String getInputEncoding()
    {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public boolean getStrictErrorChecking()
    {
        // TODO Auto-generated method stub
        return false;
    }

    @Override
    public String getXmlEncoding()
    {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public boolean getXmlStandalone()
    {
        // TODO Auto-generated method stub
        return false;
    }

    @Override
    public String getXmlVersion()
    {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public Node importNode(Node importedNode, boolean deep) throws DOMException
    {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public void normalizeDocument()
    {
        // TODO Auto-generated method stub

    }

    @Override
    public Node renameNode(Node n, String namespaceURI, String qualifiedName) throws DOMException
    {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public void setDocumentURI(String documentURI)
    {
        // TODO Auto-generated method stub

    }

    @Override
    public void setStrictErrorChecking(boolean strictErrorChecking)
    {
        // TODO Auto-generated method stub

    }

    @Override
    public void setXmlStandalone(boolean xmlStandalone) throws DOMException
    {
        // TODO Auto-generated method stub

    }

    @Override
    public void setXmlVersion(String xmlVersion) throws DOMException
    {
        // TODO Auto-generated method stub

    }
}
