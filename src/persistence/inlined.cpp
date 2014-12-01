#ifdef __XEM_PROVIDE_INLINE

#include <Xemeiah/persistence/persistentstore.h>
#include <Xemeiah/persistence/persistentdocument.h>

#ifdef __XEM_USE_INLINE
#define __INLINE inline
#include "../dom/elementref.hpp"
#include "../dom/attributeref.hpp"
#endif

#ifndef __XEM_USE_INLINE
#define __XEM_USE_INLINE
#endif
#undef __INLINE
#define __INLINE

#include "absolutepageref.hpp"
#include "persistentstore.hpp"
#include "persistentstore-protect.hpp"
#include "persistentdocumentallocator.hpp"
#include "writablepagecache.hpp"
#include "pageinfocache.hpp"

#endif
