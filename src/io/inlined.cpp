/*
 * inline.cpp
 *
 *  Created on: 12 janv. 2010
 *      Author: francois
 */

#ifdef __XEM_PROVIDE_INLINE

#include <Xemeiah/trace.h>
#include <Xemeiah/kern/exception.h>

#include <Xemeiah/io/bufferedreader.h>
#include <Xemeiah/io/bufferedwriter.h>


#ifdef __XEM_USE_INLINE
#define __INLINE inline
// Include here the hpp used for this module

#undef __INLINE
#endif

#define __INLINE
#include "bufferedreader.hpp"
#undef __INLINE

#endif // __XEM_PROVIDE_INLINE

