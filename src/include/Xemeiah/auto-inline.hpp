#ifdef __XEM_AUTO_INLINE_HPP
#error __XEM_AUTO_INLINE_HPP Already Defined !
#endif
#define __XEM_AUTO_INLINE_HPP

#ifdef __XEM_USE_INLINE

#ifdef __INLINE
#error __INLINE Already defined !
#else
#define __INLINE inline
#endif

#ifdef __XEM_KERN_UTF8_H
#include "../../kern/utf8.hpp"
#endif

#ifdef __XEM_KERN_DOCUMENT_H
#include "../../kern/document.hpp"
#include "../../kern/document-protect.hpp"
#endif
#ifdef __XEM_KERN_DOCUMENTALLOCATOR_H
#include "../../kern/documentallocator-protect.hpp"
#include "../../kern/documentallocator.hpp"
#include "../../kern/documentallocator-segment.hpp"
#include "../../kern/documentallocator-fixedsize.hpp"
#endif
#ifdef __XEM_KERN_QNAMEMAP_H
#include "../../kern/qnamemap.hpp"
#endif
#ifdef __XEM_KERN_KEYCACHE_H
#include "../../kern/keycache.hpp"
#endif
#ifdef __XEM_KERN_MUTEX_H
#include "../../kern/mutex.hpp"
#endif
#ifdef __XEM_DOM_NODEREF_H
#include "../../dom/noderef.hpp"
#endif
#ifdef __XEM_DOM_ELEMENT_H
#include "../../dom/elementref.hpp"
#endif

#ifdef __XEM_DOM_ATTRIBUTE_H
#include "../../dom/attributeref.hpp"
#endif
#ifdef __XEM_DOM_SKMAPREF_H
#include "../../dom/skmapref.hpp"
#include "../../dom/skmultimapref.hpp"
#endif
#ifdef __XEM_DOM_DOMEVENTMASK_H
#include "../../dom/domeventmask.hpp"
#endif

#ifdef __XEM_DOM_NODESET_H
#include "../../dom/nodeset.hpp"
#endif

#ifdef __XEM_XPATH_H
#include "../../xpath/xpath.hpp"
#include "../../xpath/xpath-eval.hpp"
#include "../../xpath/xpath-eval-nodetest.hpp"
#endif

#ifdef __XEM_XPROCESSOR_ENV_H
#include "../../xprocessor/env.hpp"
#endif
#ifdef __XEM_XPROCESSOR_H
#include "../../xprocessor/xprocessor.hpp"
#endif

#ifdef __XEM_PARSER_BYTESTREAMREADER_H
#include "../../parser/bytestreamreader.hpp"
#endif

#ifdef __XEM_IO_BUFFEREDREADER_H
#include "../../io/bufferedreader.hpp"
#endif
#undef __INLINE
#endif // __XEM_USE_INLINE
