#include "include/org_xemeiah_dom_Node.h"
#include <Xemeiah/kern/document.h>
#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/dom/childiterator.h>
#include <Xemeiah/trace.h>
#include <Xemeiah/log.h>

#include "xem-jni-dom.h"

#include <Xemeiah/auto-inline.hpp>

JNIEXPORT jobject JNICALL
Java_org_xemeiah_dom_Node_getNextSibling (JNIEnv *ev, jobject jNode)
{
    Xem::ElementRef eltRef = jElement2ElementRef(ev, jNode);
    jobject documentObject = jNode2JDocument(ev, jNode);

    Xem::ElementRef nextSibling = eltRef.getYounger();
    if (nextSibling)
    {
        return elementRef2JElement(ev, documentObject, nextSibling);
    }
    return NULL;
}

JNIEXPORT jobject JNICALL
Java_org_xemeiah_dom_Node_getParentNode (JNIEnv *ev, jobject jNode)
{
    Xem::ElementRef eltRef = jElement2ElementRef(ev, jNode);
    jobject documentObject = jNode2JDocument(ev, jNode);

    Xem::ElementRef fatherRef = eltRef.getFather();
    if (fatherRef)
    {
        return elementRef2JElement(ev, documentObject, fatherRef);
    }
    return NULL;
}

JNIEXPORT jstring JNICALL Java_org_xemeiah_dom_Node_getPrefix
  (JNIEnv *, jobject)
{
    return NULL;
}

JNIEXPORT jobject JNICALL Java_org_xemeiah_dom_Node_removeChild
  (JNIEnv *ev, jobject jParentElement, jobject jChildElement)
{
    Log ("removeChild ev=%p, jParentElement=%p, jChildElement=%p\n", ev, jParentElement, jChildElement);

    Xem::ElementRef parentElement = jElement2ElementRef(ev, jParentElement);
    Xem::ElementRef childElement = jElement2ElementRef(ev, jChildElement);

    if ( childElement.getFather() != parentElement )
    {
        ev->Throw(exception2JDOMException(ev, "Element is not the child provided father node !"));
        return NULL;
    }

    jobject jDocument = jNode2JDocument(ev, jChildElement);
    Xem::XProcessor* xprocessor = jDocument2XProcessor(ev, jDocument);

    childElement.deleteElement(*xprocessor);
    return jChildElement;
}
