/*
 * org_xemeiah_dom_Document.cpp
 *
 *  Created on: 15 janv. 2010
 *      Author: francois
 */

#include "include/org_xemeiah_transform_TransformParser.h"
#include <Xemeiah/kern/document.h>
#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/dom/childiterator.h>
#include <Xemeiah/xprocessor/xprocessor.h>
#include <Xemeiah/parser/saxhandler-dom.h>
#include <Xemeiah/parser/parser.h>
#include <Xemeiah/trace.h>
#include <Xemeiah/log.h>

#include "xem-jni-dom.h"
#include "javareader.h"

#include <Xemeiah/auto-inline.hpp>

JNIEXPORT void JNICALL Java_org_xemeiah_transform_TransformParser_doParse
  (JNIEnv *ev, jobject parserObject, jobject inputStreamObject, jobject elementObject)
{
  Xem::ElementRef root = jElement2ElementRef(ev, elementObject);
  Xem::XProcessor xproc(root.getStore());
  xproc.loadLibrary("xsl",true);
  // xproc.registerEvents(root.getDocument());
  Xem::SAXHandlerDom saxHandler(xproc,root);
  Xem::JavaReader reader(ev, inputStreamObject);
  Xem::Parser parser(reader, saxHandler);

  try
  {
    parser.parse();
  }
  catch ( Xem::Exception * e )
  {
    jclass exceptionClass = ev->FindClass("javax/xml/transform/TransformerException");
    ev->ThrowNew(exceptionClass,e->getMessage().c_str());
  }
}
