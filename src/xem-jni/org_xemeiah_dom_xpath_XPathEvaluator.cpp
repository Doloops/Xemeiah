/*
 * org_xemeiah_dom_Store.cpp
 *
 *  Created on: 15 janv. 2010
 *      Author: francois
 */

#include "include/org_xemeiah_dom_xpath_XPathEvaluator.h"
#include <Xemeiah/trace.h>
#include <Xemeiah/log.h>
#include <Xemeiah/xpath/xpathparser.h>

#include "xem-jni-dom.h"

#include <Xemeiah/auto-inline.hpp>

JNIEXPORT jobject JNICALL Java_org_xemeiah_dom_xpath_XPathEvaluator_createExpression
  (JNIEnv *ev, jobject xpathEvaluatorObject, jstring jExpression, jobject resolverObject)
{
  Xem::ElementRef resolver = jElement2ElementRef(ev, resolverObject);
  Xem::String expression = jstring2XemString(ev, jExpression);

  Xem::XPathParser* xPathParser = new Xem::XPathParser(resolver, expression);

  jobject jDocument = jNode2JDocument(ev, resolverObject);
  jobject jFactory = jDocument2JDocumentFactory(ev, jDocument);
  Xem::XProcessor* xprocessor = jDocumentFactory2XProcessor (ev, jFactory);


  Xem::XPath* xpath = new Xem::XPath(*xprocessor, xPathParser, true);

  return xpath2JXPathExpression(ev, xpath);

//  int sz = (int) xpathParser.getParsedSize();
//  Xem::XPathSegment* segment = xpathParser.getPackedParsed();
//
//  void* copiedSegment = malloc(sz);
//  memcpy ( copiedSegment, segment, sz);

//  int blks = (sz / sizeof(jlong)) + 1;
//  Log ( "Taking sz=%d, blks=%d\n", sz, blks );
//
//  jlongArray packedArray = ev->NewLongArray(blks);
//  jboolean isCopy = false;
//  jlong* packed = ev->GetLongArrayElements(packedArray,&isCopy);
//  memcpy ( packed, seg, sz );
//  ev->ReleaseLongArrayElements(packedArray,  packed, 0); // packedArray, 0, blks, packed);
//
//  Log ( "Copied seg=%p => packed=%p, packedArray=%p\n", seg, packed, packedArray );
//
//  jclass xpathExpressionClass = ev->FindClass("org/xemeiah/dom/xpath/XPathExpression");
//  jmethodID xpathExpressionConstructorId = ev->GetMethodID(xpathExpressionClass,"<init>","([J)V");
//
//  return ev->NewObject(xpathExpressionClass,xpathExpressionConstructorId, packedArray);

}
