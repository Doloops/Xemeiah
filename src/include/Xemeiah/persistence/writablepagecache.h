/*
 * writablepagecache.h
 *
 *  Created on: 9 déc. 2009
 *      Author: francois
 */

#ifndef __XEM_PERSISTENCE_PERSISTENTDOCUMENTALLOCATOR_WRITABLEPAGECACHE
#define __XEM_PERSISTENCE_PERSISTENTDOCUMENTALLOCATOR_WRITABLEPAGECACHE

#include <Xemeiah/kern/format/core_types.h>

#include <map>

// #define __XEM_PERSISTENCE_PERSISTENTDOCUMENTALLOCATOR_WRITABLEPAGECACHE__USE_STDMAP

namespace Xem
{
  /**
   * Cache telling if each page is writable or must be COWed
   */
  class WritablePageCache
  {
  protected:
#ifdef __XEM_PERSISTENCE_PERSISTENTDOCUMENTALLOCATOR_WRITABLEPAGECACHE__USE_STDMAP
    typedef std::map<RelativePagePtr,bool> Map;
    Map map;
#else
    char* map;
    RelativePagePtr alloced;
#endif

  public:
    /**
     * Simple constructor
     */
    INLINE WritablePageCache ();

    /**
     * Simple destructor
     */
    INLINE ~WritablePageCache ();

    /**
     * Clear cache
     */
    INLINE void clearCache ();

    /**
     * Set a page as writable
     */
    INLINE void setWritable ( RelativePagePtr relPagePtr );

    /**
     * Check if a page is writable
     */
    INLINE bool isWritable ( RelativePagePtr relPagePtr );
  };

};

#endif /* __XEM_PERSISTENCE_PERSISTENTDOCUMENTALLOCATOR_WRITABLEPAGECACHE */
