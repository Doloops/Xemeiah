/*
 * org_xemeiah_dom_Document.cpp
 *
 *  Created on: 15 janv. 2010
 *      Author: francois
 */

#include "include/org_xemeiah_transform_TransformXSL.h"
#include <Xemeiah/kern/document.h>
#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/dom/childiterator.h>
#include <Xemeiah/trace.h>
#include <Xemeiah/log.h>
#include <Xemeiah/xprocessor/xprocessor.h>
#include <Xemeiah/dom/nodeset.h>
#include <Xemeiah/nodeflow/nodeflow-dom.h>

#include "xem-jni-dom.h"
#include "javawriter.h"

#include <Xemeiah/auto-inline.hpp>

JNIEXPORT void JNICALL Java_org_xemeiah_transform_TransformXSL_doTransform
  (JNIEnv *ev, jobject transformXSLObject, jobject xmlElementObject, jobject resultElementObject)
{
  try
  {
    Xem::ElementRef xmlRoot = jElement2ElementRef(ev, xmlElementObject);
    Xem::ElementRef resultRoot = jElement2ElementRef(ev, resultElementObject);

    jclass transformXSLClass = ev->GetObjectClass(transformXSLObject);
    jmethodID getParameterId = ev->GetMethodID(transformXSLClass,"getParameter", "(Ljava/lang/String;)Ljava/lang/Object;");

    jstring xslObjectParameter = ev->NewStringUTF("{http://www.w3.org/1999/XSL/Transform}stylesheet");
    jobject xslElementObject = ev->CallObjectMethod(transformXSLObject,getParameterId,xslObjectParameter);

    Xem::ElementRef xslStylesheet = jElement2ElementRef(ev, xslElementObject);

    Log ( "xslStylesheet=%s\n", xslStylesheet.generateVersatileXPath().c_str() );

    Xem::XProcessor xproc(xmlRoot.getStore());
    // xproc.installModule ( "http://www.xemeiah.org/ns/xemint-pi" );
    xproc.loadLibrary("xsl", true);
    xproc.loadLibrary("exslt", false);


    Xem::NodeSet xmlTreeNodeSet;
    xmlTreeNodeSet.pushBack ( xmlRoot );
    Xem::NodeSet::iterator xmlTreeIterator ( xmlTreeNodeSet, xproc );

    Xem::NodeFlowDom nodeFlow(xproc, resultRoot);
    xproc.setNodeFlow(nodeFlow);

    xproc.process(xslStylesheet);
  }
  catch ( Xem::Exception *e )
  {
    jclass exceptionClass = ev->FindClass("javax/xml/transform/TransformerException");
    ev->ThrowNew(exceptionClass,e->getMessage().c_str());
  }
}
