#ifndef __XEMEIAH_TRACE_H
#define __XEMEIAH_TRACE_H

#include <Xemeiah/log.h>

#ifdef __XEM_USE_INLINE
  #if 1
    #define __FORCE_INLINE __attribute__((always_inline))
  #else
    #define __FORCE_INLINE
  #endif
#else
  #define __FORCE_INLINE
#endif

#define DEPRECATED __attribute__((deprecated))

#ifdef __XEM_USE_INLINE
  #define INLINE inline
#else
  #define INLINE
#endif

#ifndef __XEM_USE_INLINE
#ifndef __INLINE
#define __INLINE
#endif
#endif

#endif // __XEMEIAH_TRACE_H
