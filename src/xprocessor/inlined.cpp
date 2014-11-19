#ifdef __XEM_PROVIDE_INLINE

#include <Xemeiah/xprocessor/xprocessor.h>

#ifdef __XEM_USE_INLINE
#define __INLINE inline
#include "../core/store.hpp"
#include "../core/protect.hpp"
#include "../core/keys.hpp"
#include "../core/keycache.hpp"
#include "../core/context.hpp"
#include "../core/context-alloc.hpp"
#include "../core/context-seg.hpp"
#include "../core/env.hpp"
#include "../dom/elementref.hpp"
#include "../dom/attributeref.hpp"
#undef __INLINE
#endif

#define __INLINE

#include "xprocessor.hpp"
#include "env.hpp"

#endif

