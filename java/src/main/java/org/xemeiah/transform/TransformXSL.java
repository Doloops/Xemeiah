package org.xemeiah.transform;

import java.util.Properties;

import javax.xml.transform.ErrorListener;
import javax.xml.transform.Result;
import javax.xml.transform.Source;
import javax.xml.transform.TransformerException;
import javax.xml.transform.URIResolver;

import org.w3c.dom.Node;

public class TransformXSL extends javax.xml.transform.Transformer {

	protected java.util.Map<String, Object> parameters = new java.util.HashMap<String, Object>();
	
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

	public Object getParameter(String name) {
		return parameters.get(name);
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

	public void setParameter(String name, Object value) {
		parameters.put(name, value);
	}

	@Override
	public void setURIResolver(URIResolver resolver) {
		// TODO Auto-generated method stub
		
	}
	
	public void setXSLRoot ( org.w3c.dom.Node node ) throws TransformerException 
	{
		if ( node.getNamespaceURI().equals("{http://www.w3.org/1999/XSL/Transform}")
				&& node.getLocalName().equals("stylesheet") )
		{
			
		}
		else
		{
			org.w3c.dom.NodeList children = node.getChildNodes();
			node = null;
			for ( int i = 0 ; i < children.getLength() ; i++ )
			{
				org.w3c.dom.Node child = children.item(i);
				System.out.println ( "At {" + child.getNamespaceURI() + "}" + child.getLocalName() );
				if ( child.getNamespaceURI().equals("http://www.w3.org/1999/XSL/Transform")
						&& child.getLocalName().equals("stylesheet") )
				{
					node = child;
					break;
				}
			}
			if ( node == null )
				throw new TransformerException ( "Invalid node ! not an XSL Stylesheet" );
		}
		setParameter("{http://www.w3.org/1999/XSL/Transform}stylesheet", node );
	}

	@Override
	public void transform(Source xmlSource, Result outputTarget)
			throws TransformerException {
		throw new TransformerException("Not handled : source=" + xmlSource.getClass().getCanonicalName() +
				", outputTarget=" + outputTarget.getClass().getCanonicalName() );
	}

	protected native void doTransform(org.xemeiah.dom.Element source, org.xemeiah.dom.Element target);
	
	public void transform(javax.xml.transform.dom.DOMSource source, javax.xml.transform.dom.DOMResult target)
	throws TransformerException {
		doTransform((org.xemeiah.dom.Element)source.getNode(), (org.xemeiah.dom.Element)target.getNode());
	}

}
