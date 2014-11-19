package org.xemeiah.dom;

import org.w3c.dom.Attr;
import org.w3c.dom.CDATASection;
import org.w3c.dom.Comment;
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

public class ChrootDocument extends org.xemeiah.dom.Element implements org.w3c.dom.Document
{
    protected ChrootDocument(org.xemeiah.dom.Element chrootElement)
    {
        super((org.xemeiah.dom.Document) chrootElement.getOwnerDocument(), chrootElement.__nodePtr);
    }

    @Override
    public Node adoptNode(Node arg0) throws DOMException
    {
        return getOwnerDocument().adoptNode(arg0);
    }

    @Override
    public Attr createAttribute(String arg0) throws DOMException
    {
        return getOwnerDocument().createAttribute(arg0);
    }

    @Override
    public Attr createAttributeNS(String arg0, String arg1) throws DOMException
    {
        return getOwnerDocument().createAttributeNS(arg0, arg1);
    }

    @Override
    public CDATASection createCDATASection(String arg0) throws DOMException
    {
        return getOwnerDocument().createCDATASection(arg0);
    }

    @Override
    public Comment createComment(String arg0)
    {
        return getOwnerDocument().createComment(arg0);
    }

    @Override
    public DocumentFragment createDocumentFragment()
    {
        return getOwnerDocument().createDocumentFragment();
    }

    @Override
    public Element createElement(String arg0) throws DOMException
    {
        return getOwnerDocument().createElement(arg0);
    }

    @Override
    public Element createElementNS(String arg0, String arg1) throws DOMException
    {
        return getOwnerDocument().createElementNS(arg0, arg1);
    }

    @Override
    public EntityReference createEntityReference(String arg0) throws DOMException
    {
        return getOwnerDocument().createEntityReference(arg0);
    }

    @Override
    public ProcessingInstruction createProcessingInstruction(String arg0, String arg1) throws DOMException
    {
        return getOwnerDocument().createProcessingInstruction(arg0, arg1);
    }

    @Override
    public Text createTextNode(String arg0)
    {
        return getOwnerDocument().createTextNode(arg0);
    }

    @Override
    public DocumentType getDoctype()
    {
        return getOwnerDocument().getDoctype();
    }

    @Override
    public Element getDocumentElement()
    {
        return this;
    }

    @Override
    public String getDocumentURI()
    {
        return getOwnerDocument().getDocumentURI();
    }

    @Override
    public DOMConfiguration getDomConfig()
    {
        return getOwnerDocument().getDomConfig();
    }

    @Override
    public Element getElementById(String arg0)
    {
        return getOwnerDocument().getElementById(arg0);
    }

    @Override
    public DOMImplementation getImplementation()
    {
        return getOwnerDocument().getImplementation();
    }

    @Override
    public String getInputEncoding()
    {
        return getOwnerDocument().getInputEncoding();
    }

    @Override
    public boolean getStrictErrorChecking()
    {
        return getOwnerDocument().getStrictErrorChecking();
    }

    @Override
    public String getXmlEncoding()
    {
        return getOwnerDocument().getXmlEncoding();
    }

    @Override
    public boolean getXmlStandalone()
    {
        return getOwnerDocument().getXmlStandalone();
    }

    @Override
    public String getXmlVersion()
    {
        return getOwnerDocument().getXmlVersion();
    }

    @Override
    public Node importNode(Node arg0, boolean arg1) throws DOMException
    {
        return getOwnerDocument().importNode(arg0, arg1);
    }

    @Override
    public void normalizeDocument()
    {
        getOwnerDocument().normalizeDocument();        
    }

    @Override
    public Node renameNode(Node arg0, String arg1, String arg2) throws DOMException
    {
        return getOwnerDocument().renameNode(arg0, arg1, arg2);
    }

    @Override
    public void setDocumentURI(String arg0)
    {
        getOwnerDocument().setDocumentURI(arg0);        
    }

    @Override
    public void setStrictErrorChecking(boolean arg0)
    {
        getOwnerDocument().setStrictErrorChecking(arg0);
    }

    @Override
    public void setXmlStandalone(boolean arg0) throws DOMException
    {
        getOwnerDocument().setXmlStandalone(arg0);
    }

    @Override
    public void setXmlVersion(String arg0) throws DOMException
    {
        getOwnerDocument().setXmlVersion(arg0);        
    }
}
