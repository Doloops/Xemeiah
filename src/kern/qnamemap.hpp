#include <Xemeiah/kern/qnamemap.h>

namespace Xem
{
  __INLINE QNameMap::QNameMap ()
  {
  
  }
  
  __INLINE QNameMap::~QNameMap ()
  {
  
  }
  
  __INLINE void QNameMap::put ( LocalKeyId prefixId, LocalKeyId localId, const char* key )
  {
    LocalMap* localMap = nsMap.get ( prefixId );
    if ( ! localMap )
      {
        localMap = new LocalMap ();
        nsMap.put ( prefixId, localMap );
      }
    localMap->put ( localId, key );
  }
  
  __INLINE const char* QNameMap::get ( LocalKeyId prefixId, LocalKeyId localId )
  {
    LocalMap* localMap = nsMap.get ( prefixId );
    if ( ! localMap ) return NULL;
    return localMap->get ( localId );
  }

};

