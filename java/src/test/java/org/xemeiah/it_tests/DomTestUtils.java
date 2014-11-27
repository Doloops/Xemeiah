package org.xemeiah.it_tests;

import org.w3c.dom.Document;
import org.w3c.dom.Element;

public class DomTestUtils
{

    public static void fillDocumentWithNodes(Document document)
    {
        Element node = document.createElement("myElement");
        document.getDocumentElement().appendChild(node);

        for (int nbNode = 0; nbNode < 100; nbNode++)
        {
            Element subNode = document.createElement("child_" + nbNode);
            node.appendChild(subNode);
        }
    }
}
