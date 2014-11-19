/*
 * org_xemeiah_dom_Document.cpp
 *
 *  Created on: 15 janv. 2010
 *      Author: francois
 */

#include "include/org_xemeiah_dom_NamedNodeMap.h"
#include <Xemeiah/trace.h>
#include <Xemeiah/log.h>

#include "xem-jni-dom.h"
#include <Xemeiah/auto-inline.hpp>

JNIEXPORT jobject JNICALL Java_org_xemeiah_dom_NamedNodeMap_getNamedItemNS
  (JNIEnv *ev, jobject namedNodeMapObject, jstring jNamespace, jstring jName)
{
    NotImplemented ( "." );
    return NULL;
#if 0
  jobject documentObject = jNodeList2JDocument(ev, namedNodeMapObject);
  Xem::Document* doc = jDocument2Document(ev, documentObject);
  jclass namedNodeMapClass = ev->GetObjectClass(namedNodeMapObject);

  Xem::KeyId keyId = j2KeyId(ev, doc->getKeyCache(),jNamespace, jName);

  jfieldID eltPtrListId = ev->GetFieldID(namedNodeMapClass,"eltPtrList", "[J");
  jfieldID attrPtrListId = ev->GetFieldID(namedNodeMapClass,"attrPtrList", "[J");

  jobject eltPtrListObject = ev->GetObjectField(namedNodeMapObject, eltPtrListId);
  jobject attrPtrListObject = ev->GetObjectField(namedNodeMapObject, attrPtrListId);

  jsize sz = ev->GetArrayLength((jlongArray)eltPtrListObject);

  jboolean isCopy = false;
  jlong* eltPtrs = ev->GetLongArrayElements((jlongArray)eltPtrListObject, &isCopy);
  jlong* attrPtrs = ev->GetLongArrayElements((jlongArray)attrPtrListObject, &isCopy);

  for ( jsize i = 0 ; i < sz ; i++ )
    {
      // Log ( "Elt %llx, attr %llx\n", eltPtrs[i], attrPtrs[i] );
      if ( attrPtrs && attrPtrs[i] )
        {
          Xem::AttributeRef attrRef = Xem::AttributeRefConstructor(*doc, eltPtrs[i], attrPtrs[i]);
          if ( attrRef.getKeyId() == keyId )
            {
              ev->ReleaseLongArrayElements((jlongArray)eltPtrListObject, eltPtrs, 0);
              ev->ReleaseLongArrayElements((jlongArray)attrPtrListObject, attrPtrs, 0);
              return attributeRef2JAttribute(ev, documentObject, attrRef);
            }
        }
      else
        {
          NotImplemented ( "No attribute pointer here !\n" );
        }
    }
  ev->ReleaseLongArrayElements((jlongArray)eltPtrListObject, eltPtrs, 0);
  ev->ReleaseLongArrayElements((jlongArray)attrPtrListObject, attrPtrs, 0);
  return NULL;
#endif
}

JNIEXPORT jobject JNICALL Java_org_xemeiah_dom_NamedNodeMap_removeNamedItemNS
  (JNIEnv *, jobject, jstring, jstring)
{
  NotImplemented ( "." );
  return NULL;
}

JNIEXPORT jobject JNICALL Java_org_xemeiah_dom_NamedNodeMap_setNamedItemNS
  (JNIEnv *, jobject, jobject)
{
  NotImplemented ( "." );
  return NULL;
}

