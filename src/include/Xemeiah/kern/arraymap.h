#ifndef __XEM_CORE_ARRAYMAP_H
#define __XEM_CORE_ARRAYMAP_H

#include <Xemeiah/trace.h>

#include <stdlib.h>

namespace Xem
{
#define Log_ArrayMap Debug

  /**
   * ArrayMap is a Array-based associative container, efficient when keys are linear ;
   * it best fits for the KeyCache and generally KeyId-based mappings, because KeyIds are created linearly, are not too many,
   * and a fast lookup matters.
   */
  template<typename TKey, typename TValue>
  class ArrayMap
  {
  protected:
    TValue* map;
    TKey alloced;
  public:
    ArrayMap ()
    {
      map = 0;
      alloced = 0;
    }
    ~ArrayMap ()
    {
      if ( map )
        {
          Log_ArrayMap ( "Freeing map=%p, alloced = 0x%x\n", map, alloced );
          for ( TKey i = 0 ; i < alloced ; i++ )
            if ( map[i] )
              {
                Log_ArrayMap ( "Freeing at %p\n", map[i] );
                delete ( map[i] );
              }
          ::free ( map );
          map = NULL;
        }
      
    }
    /**
     * insert a new keyId->key conversion.
     * @param key the key of the association
     * @param value the value to set for the key.
     */
    void put ( TKey key, TValue value )
    {
      if ( alloced <= key )
        {
          TKey newSize = key * 2;
          map = (TValue*) realloc ( map, newSize * sizeof (TValue) );
          if ( ! map )
            {
              Bug ( "Could not realloc !\n" );
            }
          for ( LocalKeyId i = alloced; i < newSize ; i++ )
            map[i] = NULL;
          alloced = newSize;
        }
      Log_ArrayMap ( "Put key=0x%x, value=%p\n", key, value );
      map[key] = value;    
    }
    
    /**
     * get a key from its LocalKeyId.
     * @param keyId the local KeyId
     * @return the char* key. The pointer provided is from a strdupped() char*, so it is persistent and *must* not be freed.
     */
    TValue get ( TKey key )
    {
      if ( alloced <= key ) 
        return (TValue)NULL;
      return map[key];    
    }
    
    /*
     * get the highest key alloced.
     */
    TKey getLastKey ()
    {
      if ( ! alloced ) return 0;
      TKey last = alloced - 1;
      while ( map[last] == NULL ) last--;
      return last;    
    }
  };
};

#endif // __XEM_CORE_LOCALKEYIDTOKEYMAP_H

