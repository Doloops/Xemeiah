#include "include/org_xemeiah_dom_Node.h"
#include <Xemeiah/kern/document.h>
#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/dom/childiterator.h>
#include <Xemeiah/trace.h>
#include <Xemeiah/log.h>

#include "xem-jni-dom.h"

#include <Xemeiah/auto-inline.hpp>

JNIEXPORT jobject JNICALL Java_org_xemeiah_dom_Node_getNextSibling
  (JNIEnv *ev, jobject jNode)
{
    Xem::ElementRef eltRef  = jElement2ElementRef(ev, jNode);
    jobject documentObject = jNode2JDocument(ev, jNode);

    Xem::ElementRef nextSibling = eltRef.getYounger();
    if ( nextSibling )
    {
        return elementRef2JElement(ev, documentObject, nextSibling);
    }
    return NULL;
}
