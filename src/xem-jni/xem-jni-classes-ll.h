#ifndef __XEM_JNI_CLASSES_LL_H
#define __XEM_JNI_CLASSES_LL_H

#include <Xemeiah/trace.h>
#include <jni.h>

#define __XEM_JNI_USE_CACHE

#define Log_XEMJNI Debug

#ifdef __XEM_JNI_USE_CACHE

#if 1

class _JClass
{
private:
    jclass cachedClass;
    const char* className;
public:
    _JClass (const char* className)
    {
        this->cachedClass = NULL;
        this->className = className;
    }

    jclass
    operator() (JNIEnv* ev)
    {
        if (cachedClass == NULL)
        {
            jclass __local_class = ev->FindClass(className);
            cachedClass = (jclass) ev->NewGlobalRef(__local_class);
            ev->DeleteLocalRef(__local_class);
            Log_XEMJNI ("[ev=%p] For class=%s, caching %p\n", ev, className, cachedClass);
            AssertBug(cachedClass != NULL, "Could not find class %s\n", className);
        }
        return cachedClass;
    }
};

class _JMethod
{
private:
    jmethodID cachedMethodId;
    const char* methodName;
    const char* signature;
    _JClass& getClass;
public:
    _JMethod (_JClass& __getClass, const char* methodName, const char* signature) :
            getClass(__getClass)
    {
        this->methodName = methodName;
        this->signature = signature;
        this->cachedMethodId = NULL;
    }

    INLINE
    jmethodID
    operator() (JNIEnv* ev)
    {
        if (cachedMethodId == NULL)
        {
            jclass objectClass = getClass(ev);
            cachedMethodId = ev->GetMethodID(objectClass, methodName, signature);
            Log_XEMJNI("[ev=%p] SET class=%p, mthName=%s, __signature=%s, jmethodID=%p\n", ev, objectClass, methodName, signature, cachedMethodId );
        }
        Log_XEMJNI("[ev=%p] GET __mthName=%s, __signature=%s, jmethodID=%p\n", ev, methodName, signature, cachedMethodId );
        return cachedMethodId;
    }
};

class _JField
{
private:
    jfieldID cachedFieldId;
    const char* fieldName;
    const char* signature;
    _JClass& _getClass;
public:
    _JField (_JClass& __getClass, const char* fieldName, const char* signature) :
            _getClass(__getClass)
    {
        this->fieldName = fieldName;
        this->signature = signature;
        this->cachedFieldId = NULL;
    }

    jfieldID
    operator() (JNIEnv* ev)
    {
        if (cachedFieldId == NULL)
        {
            jclass _clz = _getClass(ev);
            cachedFieldId = ev->GetFieldID(_clz, fieldName, signature);
            Log_XEMJNI("[ev=%p] SET class=%p, fieldName=%s, signature=%s, jfieldID=%p\n", ev, _clz, fieldName, signature, cachedFieldId );
        }
        Log_XEMJNI("[ev=%p] GET fieldName=%s, signature=%s, jfieldID=%p\n", ev, fieldName, signature, cachedFieldId );
        return cachedFieldId;
    }
};

#define JCLASS(__jName) \
        class __JClass : public _JClass { public: __JClass() : _JClass(__jName) {} }; \
        __JClass getClass;

#define JMETHOD(__cName, __mthName, __signature) \
        class __JMethod##__cName : public _JMethod  { \
            protected: __JClass __getClass; \
            public: __JMethod##__cName() : _JMethod(__getClass, __mthName, __signature) {} }; \
        __JMethod##__cName __cName

#define JFIELD(__cName, __fieldName, __signature) \
        class __JField##__cName : public _JField  { \
            protected: __JClass __getClass; \
            public: __JField##__cName() : _JField(__getClass, __fieldName, __signature) {} }; \
        __JField##__cName __cName

#else
#define JCLASS(__jName) \
        jclass getClass(JNIEnv* __ev) { \
        if ( __class == NULL ) { \
            jclass __local_class = __ev->FindClass(__jName); \
            __class = (jclass) __ev->NewGlobalRef(__local_class); \
            __ev->DeleteLocalRef(__local_class); \
            Log_XEMJNI ("[ev=%p] For class=%s, caching %p\n", __ev, __jName, __class); \
            AssertBug ( __class != NULL, "Could not find class " __jName ); \
        } \
        else if ( 0 )\
        { \
            jclass __reClass = __ev->FindClass(__jName); \
            AssertBug ( !__ev->ExceptionCheck() , "Exception check !"); \
            if ( __class != __reClass ) \
            { Log_XEMJNI ( "[ev=%p] Diverging classes for %s, cached %p, should have %p\n", __ev, __jName, __class, __reClass ); \
                /* __class = __reClass; */ \
            } \
        } \
    Log_XEMJNI ("[ev=%p] Returning class=%s => %p\n", __ev, __jName, __class); \
    return __class; } \
        jclass __class = NULL
#define JFIELD(__cName, __fieldName, __signature) \
      jfieldID __cName(JNIEnv* __ev) { \
      if ( __cName##Fcache == NULL ) \
      { \
        jclass _clz = getClass(__ev); \
        __cName##Fcache = __ev->GetFieldID(_clz, __fieldName, __signature); \
        Log_XEMJNI("[ev=%p] SET clz=%p, __fieldName=%s, __signature=%s, jfieldID=%p\n", __ev, _clz, __fieldName, __signature, __cName##Fcache); \
        } \
    Log_XEMJNI("[ev=%p] __fieldName=%s, __signature=%s, jfieldID=%p\n", __ev, __fieldName, __signature, __cName##Fcache); \
    return __cName##Fcache; } \
    jfieldID __cName##Fcache = NULL

#define JMETHOD(__cName, __mthName, __signature) \
      jmethodID __cName(JNIEnv* __ev) { \
      if ( __cName##Mcache == NULL ) \
      { \
        jclass _clz = getClass(__ev); \
        __cName##Mcache = __ev->GetMethodID(_clz, __mthName, __signature); \
        Log_XEMJNI("[ev=%p] SET class=%p, __mthName=%s, __signature=%s, jmethodID=%p\n", __ev, _clz, __mthName, __signature, __cName##Mcache ); \
      } \
    Log_XEMJNI("[ev=%p] __mthName=%s, __signature=%s, jmethodID=%p\n", __ev, __mthName, __signature, __cName##Mcache ); \
    return __cName##Mcache; } \
    jmethodID __cName##Mcache = NULL

#endif

#else
#define JCLASS(__jName) \
    jclass getClass(JNIEnv* __ev) { \
        jclass result = __ev->FindClass(__jName); \
        Log_XEMJNI ("[ev=%p] Returning class=%s => %p\n", __ev, __jName, result); \
        return result;  \
    }
#define JFIELD(__cName, __fieldName, __signature) \
  jfieldID __cName(JNIEnv* __ev) { \
    jclass _clz = getClass(__ev); \
    jfieldID result = __ev->GetFieldID(_clz, __fieldName, __signature); \
    Log_XEMJNI("[ev=%p] class=%p, __fieldName=%s, __signature=%s, jfieldID=%p\n", __ev, _clz, __fieldName, __signature, result); \
    return result; \
  }
#define JMETHOD(__cName, __mthName, __signature) \
  jmethodID __cName(JNIEnv* __ev) { \
    jclass _clz = getClass(__ev); \
    jmethodID result = __ev->GetMethodID(_clz, __mthName, __signature); \
    Log_XEMJNI("[ev=%p] class=%p, __mthName=%s, __signature=%s, jmethodID=%p\n", __ev, _clz, __mthName, __signature, result); \
    return result; \
  }
#endif

#endif // __XEM_JNI_CLASSES_LL_H
