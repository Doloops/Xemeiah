/*
 * org_xemeiah_dom_Document.cpp
 *
 *  Created on: 15 janv. 2010
 *      Author: francois
 */

#include "include/org_xemeiah_dom_NodeList.h"

#include <Xemeiah/dom/nodeset.h>

#include <Xemeiah/trace.h>
#include <Xemeiah/log.h>

#include "xem-jni-dom.h"
#include <Xemeiah/auto-inline.hpp>

#undef Log
#define Log(...) do{}while(0)


JNIEXPORT void JNICALL Java_org_xemeiah_dom_NodeList_cleanUp
  (JNIEnv *ev, jobject jNodeList)
{
    Xem::NodeSet* nodeSet = jNodeList2NodeSet(ev, jNodeList);
    Log("Delete nodeSet=%p\n", nodeSet);
    delete(nodeSet);
}

JNIEXPORT jobject JNICALL
Java_org_xemeiah_dom_NodeList_getItem (JNIEnv *ev, jobject jNodeList, jint jindex)
{
    AssertBug(jNodeList, "Null jNodeList provided !\n");
    jobject jDocument = jNodeList2JDocument(ev, jNodeList);
    Xem::NodeSet* nodeSet = jNodeList2NodeSet(ev, jNodeList);
    AssertBug(nodeSet, "Null jNodeList provided !\n");

    int index = 0;
    for (Xem::NodeSet::iterator iter(*nodeSet); iter; iter++)
    {
        if ( index == jindex )
        {
            if (iter->isElement() )
            {
                Xem::ElementRef eltRef = iter->toElement();
                return elementRef2JElement(ev, jDocument, eltRef);
            }
            else if ( iter->isAttribute() )
            {
                Xem::AttributeRef attrRef = iter->toAttribute();
                return attributeRef2JAttribute(ev, jDocument, attrRef);
            }
            else
            {
                Bug("Not implemented !");
            }
        }
        index++;
    }
    return NULL;
}
