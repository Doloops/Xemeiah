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

JNIEXPORT jobject JNICALL
Java_org_xemeiah_dom_NodeList_getItem (JNIEnv *ev, jobject jNodeList, jint jindex)
{
    AssertBug(jNodeList, "Null jNodeList provided !\n");
    jobject jDocument = jNodeList2JDocument(ev, jNodeList);

    Xem::Document* doc = jDocument2Document(ev, jDocument);
    Log("jDocument=%p, doc=%p, jNodeList=%p, %i\n", jDocument, doc, jNodeList, jindex);

    Xem::NodeSet* nodeSet = jNodeList2NodeSet(ev, jNodeList);
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
            else
            {
                Bug("Not implemented !");
            }
        }
        index++;
//        if (iter->isElement())
//        {
//            elts[index] = iter->toElement().getElementPtr();
//            attrs[index] = 0;
//            index++;
//        }
//        else if (iter->isElement())
//        {
//            Xem::AttributeRef attrRef = iter->toAttribute();
//            elts[index] = attrRef.getElement().getElementPtr();
//            attrs[index] = attrRef.getAttributePtr();
//            index++;
//        }
//        else
//        {
//            Bug(".");
//        }
    }

//  Log("jDocument=%p, doc=%p, eltPtr=%lx, attrPtr=%lx\n", jDocument, doc, eltPtr, attrPtr);
//
//  if ( attrPtr )
//    {
//      Xem::AttributeRef attrRef = Xem::AttributeRefConstructor(*doc, (Xem::ElementPtr) eltPtr, (Xem::AttributePtr) attrPtr );
//      return attributeRef2J ( ev, jDocument, attrRef );
//    }
//  Xem::ElementRef eltRef = Xem::ElementRefConstructor(*doc, (Xem::ElementPtr) eltPtr );
    // return elementRef2JElement(ev, jDocument, eltRef);
    return NULL;
}
