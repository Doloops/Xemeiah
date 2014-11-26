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

#if 0
#undef Log
#define Log Info
#endif

JNIEXPORT void JNICALL
Java_org_xemeiah_transform_TransformParser_doParse (JNIEnv *ev, jobject jTransformParser, jobject jInputStream,
                                                    jobject jElement)
{
    Log("doParse : jTransformParser=%p\n", jTransformParser);

    AssertBug(jInputStream != NULL, "Null jInputstream !");
    AssertBug(jElement != NULL, "Null jElement !");

    jobject jDocument = jNode2JDocument(ev, jElement);

    Xem::XProcessor* xprocessor = jDocument2XProcessor(ev, jDocument);

    Xem::ElementRef root = jElement2ElementRef(ev, jElement);

    Xem::SAXHandlerDom saxHandler(*xprocessor, root);
    Xem::JavaReader reader(ev, jInputStream);
    Xem::Parser parser(reader, saxHandler);

    Log("doParse : jTransformParser=%p => inited.\n", jTransformParser);

    try
    {
        parser.parse();
    }
    catch (Xem::Exception * e)
    {
        jclass exceptionClass = ev->FindClass("javax/xml/transform/TransformerException");
        ev->ThrowNew(exceptionClass, e->getMessage().c_str());
    }
    Log("doParse : jTransformParser=%p => finished parse.\n", jTransformParser);
}
