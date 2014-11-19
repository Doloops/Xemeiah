package org.xemeiah.dom;

public class NodeList implements org.w3c.dom.NodeList
{
    private org.xemeiah.dom.Document document;

    /**
	 * 
	 */
    private int length;
    
    private long __nodeListPtr;
    
    protected NodeList(org.xemeiah.dom.Document document, int length, long __nodeListPtr)
    {
        this.document = document;
        this.length = length;
        this.__nodeListPtr = __nodeListPtr;
    }

    private native void cleanUp();

    protected void finalize()
    {
        cleanUp();
    }
    
    private native org.xemeiah.dom.Node getItem(int index);

    public int getLength()
    {
        return length;
    }

    public org.xemeiah.dom.Node item(int index)
    {
        return getItem(index);
    }
}
