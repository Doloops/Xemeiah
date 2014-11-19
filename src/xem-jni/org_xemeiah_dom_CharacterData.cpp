/*
 * org_xemeiah_dom_Document.cpp
 *
 *  Created on: 15 janv. 2010
 *      Author: francois
 */

#include "include/org_xemeiah_dom_CharacterData.h"
#include <Xemeiah/kern/document.h>
#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/trace.h>
#include <Xemeiah/log.h>

#include "xem-jni-dom.h"

#include <Xemeiah/auto-inline.hpp>

JNIEXPORT void JNICALL Java_org_xemeiah_dom_CharacterData_appendData
  (JNIEnv *, jobject, jstring)
{
  NotImplemented(".");
}


JNIEXPORT void JNICALL Java_org_xemeiah_dom_CharacterData_deleteData
  (JNIEnv *, jobject, jint, jint)
{
  NotImplemented(".");
}

JNIEXPORT jstring JNICALL Java_org_xemeiah_dom_CharacterData_getData
  (JNIEnv *ev, jobject elementObject)
{
  Xem::ElementRef eltRef = jElement2ElementRef(ev, elementObject);
  return ev->NewStringUTF(eltRef.getText().c_str());
}


JNIEXPORT void JNICALL Java_org_xemeiah_dom_CharacterData_setData
  (JNIEnv *ev, jobject elementObject, jstring text)
{
	Xem::ElementRef eltRef = jElement2ElementRef(ev, elementObject);
	jboolean isCopy = false;
	const char* ctext = ev->GetStringUTFChars(text, &isCopy);
	eltRef.setText(ctext);
	ev->ReleaseStringUTFChars(text, ctext);
}

JNIEXPORT jint JNICALL Java_org_xemeiah_dom_CharacterData_getLength
  (JNIEnv *ev, jobject elementObject)
{
  Xem::ElementRef eltRef = jElement2ElementRef(ev, elementObject);
  return (jint) eltRef.getText().size();
}

JNIEXPORT void JNICALL Java_org_xemeiah_dom_CharacterData_insertData
  (JNIEnv *, jobject, jint, jstring)
{
  NotImplemented(".");
}

JNIEXPORT void JNICALL Java_org_xemeiah_dom_CharacterData_replaceData
  (JNIEnv *, jobject, jint, jint, jstring)
{
  NotImplemented(".");
}

JNIEXPORT jstring JNICALL Java_org_xemeiah_dom_CharacterData_substringData
  (JNIEnv *, jobject, jint, jint)
{
  NotImplemented(".");
  return NULL;
}
