package org.xemeiah.dom.parser;

import java.io.IOException;
import java.io.InputStream;

import javax.xml.transform.TransformerException;

import org.xml.sax.InputSource;
import org.xml.sax.SAXException;

public class XemParser
{
    public void parse(org.w3c.dom.Element rootElement, InputSource inputSource) throws SAXException,
            IOException
    {
        javax.xml.transform.sax.SAXSource source = new javax.xml.transform.sax.SAXSource();
        source.setInputSource(inputSource);

        javax.xml.transform.dom.DOMResult result = new javax.xml.transform.dom.DOMResult();
        result.setNode(rootElement);

        org.xemeiah.transform.TransformParser transformParser = new org.xemeiah.transform.TransformParser();

        try
        {
            transformParser.transform(source, result);
        }
        catch (TransformerException e)
        {
            e.printStackTrace();
            throw new SAXException("Could not parse", e);
        }

        if (false && rootElement.getOwnerDocument() instanceof org.xemeiah.dom.Document)
        {
            org.xemeiah.dom.Document xemDoc = (org.xemeiah.dom.Document) rootElement.getOwnerDocument();
            xemDoc.commit();
            xemDoc.reopen();
        }
    }

    public void parse(org.w3c.dom.Element rootElement, String uri) throws SAXException, IOException
    {
        java.io.InputStream inputStream = new java.io.FileInputStream(uri);
        org.xml.sax.InputSource inputSource = new org.xml.sax.InputSource(inputStream);
        parse(rootElement, inputSource);
    }

    public void parse(org.w3c.dom.Element rootElement, InputStream is) throws SAXException, IOException
    {
        InputSource inputSource = new InputSource(is);
        parse(rootElement, inputSource);
    }

}
