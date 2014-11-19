#ifndef __XEM_KERN_QNAMEMAP_H
#define __XEM_KERN_QNAMEMAP_H

#include <Xemeiah/kern/format/core_types.h>
#include <Xemeiah/kern/arraymap.h>

namespace Xem
{
  /**
   * Mapping from fully-qualified (prefixId, localKeyId) keys to their unique textual representation.
   */
  class QNameMap
  {
  protected:
    typedef ArrayMap<KeyId,const char*> LocalMap;
    typedef ArrayMap<KeyId,LocalMap*> NSMap;
    NSMap nsMap;
    
  public:
    INLINE QNameMap ();
    INLINE ~QNameMap ();
    
    INLINE void put ( LocalKeyId prefixId, LocalKeyId localId, const char* key );
    INLINE const char* get ( LocalKeyId prefixId, LocalKeyId localId );
  };
};

#endif // __XEM_CORE_QNAMEMAP_H

