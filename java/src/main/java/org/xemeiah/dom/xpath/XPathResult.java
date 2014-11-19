package org.xemeiah.dom.xpath;

import org.w3c.dom.DOMException;
import org.w3c.dom.Node;
import org.w3c.dom.xpath.XPathException;
import org.xemeiah.dom.Document;

public class XPathResult extends org.xemeiah.dom.NodeList implements org.w3c.dom.xpath.XPathResult {

	protected XPathResult(Document document,  int length, long __nodeListPtr) {
		super(document, length, __nodeListPtr);
	}

	public int getSnapshotLength() throws XPathException {
		return getLength();
	}

	public Node snapshotItem(int index) throws XPathException {
		return item(index);
	}


	@Override
	public boolean getBooleanValue() throws XPathException {
		// TODO Auto-generated method stub
		return false;
	}

	@Override
	public boolean getInvalidIteratorState() {
		// TODO Auto-generated method stub
		return false;
	}

	@Override
	public double getNumberValue() throws XPathException {
		// TODO Auto-generated method stub
		return 0;
	}

	@Override
	public short getResultType() {
		// TODO Auto-generated method stub
		return 0;
	}

	@Override
	public Node getSingleNodeValue() throws XPathException {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public String getStringValue() throws XPathException {
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public Node iterateNext() throws XPathException, DOMException {
		// TODO Auto-generated method stub
		return null;
	}

}
