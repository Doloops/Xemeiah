/*
 * persistence/auto-inline.hpp
 * Inlining capability for persistence module
 *
 *  Created on: 15 déc. 2009
 *      Author: francois
 */

#ifdef __XEM_PERSISTENCE_AUTO_INLINE_HPP
#error __XEM_PERSISTENCE_AUTO_INLINE_HPP Already Defined !
#endif
#define __XEM_PERSISTENCE_AUTO_INLINE_HPP

#ifdef __XEM_USE_INLINE

#ifdef __INLINE
#error __INLINE Already defined !
#else
#define __INLINE inline
#endif

#ifdef __XEM_PERSISTENCE_ABSOLUTEPAGEREF_H
#include "../../../persistence/absolutepageref.hpp"
#endif
#ifdef __XEM_PERSISTENCE_PERSISTENTSTORE_H
#include "../../../persistence/persistentstore.hpp"
#include "../../../persistence/persistentstore-protect.hpp"
#endif
#ifdef __XEM_PERSISTENCE_PERSISTENTDOCUMENT_H
#endif
#ifdef __XEM_PERSISTENCE_PERSISTENTDOCUMENTALLOCATOR_H
#include "../../../persistence/persistentdocumentallocator.hpp"
#endif
#ifdef __XEM_PERSISTENCE_PERSISTENTDOCUMENTALLOCATOR_WRITABLEPAGECACHE
#include "../../../persistence/writablepagecache.hpp"
#endif

#ifdef __XEM_PERSISTENCE_PAGEINFOCACHE_H
#include "../../../persistence/pageinfocache.hpp"
#endif

#undef __INLINE

#else // no  __XEM_USE_INLINE

/**
 * We must enforce template instanciation so include the hpp anyway
 */
#ifdef __XEM_PERSISTENCE_ABSOLUTEPAGEREF_H
#include "../../../persistence/absolutepageref.hpp"
#endif


#endif // __XEM_USE_INLINE
