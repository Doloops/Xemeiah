/*
 * writablepagecache.hpp
 *
 *  Created on: 9 déc. 2009
 *      Author: francois
 */

#include <Xemeiah/persistence/persistentdocumentallocator.h>

#define Log_WPCacheHPP Debug

namespace Xem
{
  __INLINE WritablePageCache::WritablePageCache ()
  {
#ifdef __XEM_PERSISTENCE_PERSISTENTDOCUMENTALLOCATOR_WRITABLEPAGECACHE__USE_STDMAP

#else
    map = NULL;
    alloced = 0;
#endif // __XEM_PERSISTENCE_PERSISTENTDOCUMENTALLOCATOR_WRITABLEPAGECACHE__USE_STDMAP


  }

  __INLINE WritablePageCache::~WritablePageCache ()
  {
    clearCache();
  }

#ifdef __XEM_PERSISTENCE_PERSISTENTDOCUMENTALLOCATOR_WRITABLEPAGECACHE__USE_STDMAP

  __INLINE void WritablePageCache::clearCache ()
  {
    map.clear();
  }

  __INLINE void WritablePageCache::setWritable ( RelativePagePtr relPagePtr )
  {
    Log_WPCacheHPP ( "Set writable %llx\n", relPagePtr );
    map[relPagePtr] = true;
  }

  __INLINE bool WritablePageCache::isWritable ( RelativePagePtr relPagePtr )
  {
    Map::iterator iter = map.find(relPagePtr);
    if ( iter == map.end() )
      {
        Log_WPCacheHPP ( "Is writable %llx : no iter!\n", relPagePtr );
        return false;
      }
    Log_WPCacheHPP ( "Is writable %llx : %d\n", relPagePtr, iter->second );
    return iter->second;
  }
#else // __XEM_PERSISTENCE_PERSISTENTDOCUMENTALLOCATOR_WRITABLEPAGECACHE__USE_STDMAP

  __INLINE void WritablePageCache::clearCache ()
  {
    if ( map )
      free ( map );
    map = NULL;
    alloced = 0;
  }

  inline __ui64 __pagePtrToIdx ( RelativePagePtr relPagePtr )
  {
    return relPagePtr >> InPageBits;
  }

  __INLINE void WritablePageCache::setWritable ( RelativePagePtr relPagePtr )
  {
    __ui64 idx = __pagePtrToIdx(relPagePtr);
    __ui64 idxchar = idx / 8;
    while ( idxchar >= alloced )
      {
        __ui64 newAlloced = alloced ? alloced * 2 : 256;
        map = (char*) realloc ( map, newAlloced * sizeof(char*) );
        AssertBug ( map, "Could not realloc up to newAlloced=%llu bytes\n", newAlloced );
        for ( __ui64 i = alloced ; i < newAlloced ; i++ )
          map[i] = 0;
        alloced = newAlloced;
      }
    Log_WPCacheHPP ( "Set writable %llx, idx=%llx, idxchar=%llx, off=%llx\n",
        relPagePtr, idx, idxchar, idx % 8 );
    map[idxchar] |= (1 << (idx%8));
  }

  __INLINE bool WritablePageCache::isWritable ( RelativePagePtr relPagePtr )
  {
    __ui64 idx = __pagePtrToIdx(relPagePtr);
    __ui64 idxchar = idx / 8;
    if ( idxchar >= alloced )
      {
        Log_WPCacheHPP ( "Is writable %llx : oobounds (idxchar=%llx, alloced=%llx)\n", relPagePtr, idxchar, alloced );
        return false;
      }
    Log_WPCacheHPP ( "Is writable %llx, idx=%llx, idxchar=%llx, off=%llx, map=%x, wr=%x\n",
        relPagePtr, idx, idxchar, idx % 8, (int) map[idxchar], map[idxchar] & (1 << (idx%8)) );
    return ( map[idxchar] & (1 << (idx%8)));
  }


#endif // __XEM_PERSISTENCE_PERSISTENTDOCUMENTALLOCATOR_WRITABLEPAGECACHE__USE_STDMAP

};
