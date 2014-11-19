/*
 * org_xemeiah_dom_Document.cpp
 *
 *  Created on: 15 janv. 2010
 *      Author: francois
 */

#include "include/org_xemeiah_transform_TransformSerializer.h"
#include <Xemeiah/kern/document.h>
#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/dom/childiterator.h>
#include <Xemeiah/trace.h>
#include <Xemeiah/log.h>

#include "xem-jni-dom.h"
#include "javawriter.h"

#include <Xemeiah/auto-inline.hpp>

JNIEXPORT void JNICALL Java_org_xemeiah_transform_TransformSerializer_doSerialize
  (JNIEnv *ev, jobject transformSerializerObject, jobject elementObject, jobject outputStreamObject)
{
  Xem::ElementRef rootElement = jElement2ElementRef(ev, elementObject);

  Xem::JavaWriter writer(ev, outputStreamObject);

  bool indent = false;
  bool omitXMLDeclaration = false;
  rootElement.toXML(writer,
      Xem::ElementRef::Flag_ChildrenOnly
      | Xem::ElementRef::Flag_SortAttributesAndNamespaces
      | ( indent ? 0 : Xem::ElementRef::Flag_NoIndent )
      | ( omitXMLDeclaration ? 0 : Xem::ElementRef::Flag_XMLHeader )
    ,"UTF-8");

  writer.flushBuffer();
}
