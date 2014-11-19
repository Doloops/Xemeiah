package org.xemeiah.transform;

import java.io.InputStream;
import java.util.Properties;

import javax.xml.transform.ErrorListener;
import javax.xml.transform.Result;
import javax.xml.transform.Source;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerException;
import javax.xml.transform.URIResolver;

import org.xemeiah.dom.Element;

public class TransformParser extends Transformer {

	@Override
	public void clearParameters() {
		// TODO Auto-generated method stub

	}

	@Override
	public ErrorListener getErrorListener() {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public Properties getOutputProperties() {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public String getOutputProperty(String arg0) throws IllegalArgumentException {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public Object getParameter(String arg0) {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public URIResolver getURIResolver() {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public void setErrorListener(ErrorListener arg0)
			throws IllegalArgumentException {
		// TODO Auto-generated method stub

	}

	@Override
	public void setOutputProperties(Properties arg0) {
		// TODO Auto-generated method stub

	}

	@Override
	public void setOutputProperty(String arg0, String arg1)
			throws IllegalArgumentException {
		// TODO Auto-generated method stub

	}

	@Override
	public void setParameter(String arg0, Object arg1) {
		// TODO Auto-generated method stub

	}

	@Override
	public void setURIResolver(URIResolver arg0) {
		// TODO Auto-generated method stub

	}

	protected native void doParse (InputStream inputStream, Element root) throws TransformerException;

	public void transform(javax.xml.transform.sax.SAXSource source, javax.xml.transform.dom.DOMResult result) throws TransformerException
	{
		doParse(source.getInputSource().getByteStream(), (org.xemeiah.dom.Element) result.getNode() );
	}
	
	public void transform(Source source, Result result) throws TransformerException 
	{
		throw new TransformerException("Parser : Class not handled : source : " + source.getClass().getCanonicalName()
				+ "result : " + result.getClass().getCanonicalName() );
	}

}
