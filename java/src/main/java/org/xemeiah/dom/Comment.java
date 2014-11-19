package org.xemeiah.dom;

public class Comment extends org.xemeiah.dom.CharacterData implements org.w3c.dom.Comment {

	protected Comment(Document document, long elementPtr) 
	{
		super(document, elementPtr);
	}

	public short getNodeType() { return org.w3c.dom.Node.COMMENT_NODE; }

	public String getNodeName() { return "#comment"; }
}
