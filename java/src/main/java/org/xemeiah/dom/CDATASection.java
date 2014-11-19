package org.xemeiah.dom;

public class CDATASection extends org.xemeiah.dom.Text implements org.w3c.dom.CDATASection
{
	protected CDATASection(Document document, long elementPtr) {
		super(document, elementPtr);
	}

	public short getNodeType() { return org.w3c.dom.Node.CDATA_SECTION_NODE; }
	
	public String getNodeName() {
		return "#cdata-section";
	}

}
