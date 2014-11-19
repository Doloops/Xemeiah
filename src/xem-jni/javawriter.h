/*
 * jwriter.h
 *
 *  Created on: 27 janv. 2010
 *      Author: francois
 */

#ifndef JWRITER_H_
#define JWRITER_H_

#include <Xemeiah/io/bufferedwriter.h>
#include <jni.h>

namespace Xem
{
  class JavaWriter : public BufferedWriter
  {
  protected:
    JNIEnv* env;
    jobject outputStreamObject;
  public:
    JavaWriter(JNIEnv* env_, jobject outputStreamObject_)
    : BufferedWriter(4096), env(env_), outputStreamObject(outputStreamObject_)
    {

    }

    ~JavaWriter()
    {

    }

    void flushBuffer ()
    {
      jclass outputStreamClass = env->GetObjectClass(outputStreamObject);
      jmethodID writeMethodId = env->GetMethodID(outputStreamClass,"write","([B)V");
      AssertBug ( writeMethodId, "Could not fetch writeMethodId ?\n" );
      jbyteArray byteArray = env->NewByteArray(cacheIdx);
      jboolean isCopy = false;
      jbyte* bytes = env->GetByteArrayElements(byteArray,&isCopy);

      Log ( "Flushing %d out of %d\n", cacheIdx, cacheSize );
      memcpy ( bytes, cache, cacheIdx );
      env->SetByteArrayRegion(byteArray,0,cacheIdx,bytes);

      env->CallVoidMethod(outputStreamObject,writeMethodId, byteArray);

      env->ReleaseByteArrayElements(byteArray,bytes,0);

      cacheIdx = 0;
    }
  };
};

#endif /* JWRITER_H_ */
