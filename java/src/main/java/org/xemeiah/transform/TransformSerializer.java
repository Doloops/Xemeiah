package org.xemeiah.transform;

import java.io.OutputStream;
import java.util.Properties;

import javax.xml.transform.ErrorListener;
import javax.xml.transform.Result;
import javax.xml.transform.Source;
import javax.xml.transform.TransformerException;
import javax.xml.transform.URIResolver;

public class TransformSerializer extends javax.xml.transform.Transformer {

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
	public String getOutputProperty(String name) throws IllegalArgumentException {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public Object getParameter(String name) {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public URIResolver getURIResolver() {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public void setErrorListener(ErrorListener listener)
			throws IllegalArgumentException {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void setOutputProperties(Properties oformat) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void setOutputProperty(String name, String value)
			throws IllegalArgumentException {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void setParameter(String name, Object value) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void setURIResolver(URIResolver resolver) {
		// TODO Auto-generated method stub
		
	}

	public native void doSerialize(org.xemeiah.dom.Element source, OutputStream os );

	public void transform(javax.xml.transform.dom.DOMSource source, javax.xml.transform.stream.StreamResult result)
		throws TransformerException 
	{
		doSerialize((org.xemeiah.dom.Element) source.getNode(), result.getOutputStream() );
	}
	
	public void transform(Source source, Result result)
			throws TransformerException 
	{
		throw new TransformerException("Serializer : Class not handled : source : " + source.getClass().getCanonicalName()
			+ "result : " + result.getClass().getCanonicalName() );
	}
}
