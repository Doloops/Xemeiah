#ifndef __XEM_KERN_HASHBUCKET_H
#define __XEM_KERN_HASHBUCKET_H

#include <Xemeiah/kern/format/core_types.h>
#include <Xemeiah/trace.h>
#include <string.h>

#define Log_HashBucket Debug

namespace Xem
{

  template<typename KeyType>
  class HashBucketTemplate
  {
  protected:
    typedef unsigned int KeyHash;
    static const __ui32 hashBucketSize = 0x100;

    /**
     * Defines a HashEntry for a given Hash.
     * A HashEntry is a single-linked list of keys sharing the same hash (i.e. collisions).
     */
    struct HashEntry
    {
      KeyType keyId;
      const char* key;
      HashEntry* next;
    };

    HashEntry* hashEntries[hashBucketSize];

    /**
     * Hash the key string
     * @param key the key to hash
     * @return the corresponding KeyHash.
     */
    INLINE static KeyHash getHash ( const char* key )
    {
      KeyHash h = 0x245;
      for ( const unsigned char* c = (const unsigned char*) key ; *c ; c++ )
        {
          h = (*c) + (h<<6) + (h<<16) - h;
        }
      return h % hashBucketSize;
    }
  public:
    HashBucketTemplate ()
    {
      memset ( hashEntries, 0, sizeof(hashEntries) );
    }
    
    ~HashBucketTemplate ()
    {
    
    }
    
    /**
     * get a KeyType from a string value.
     * @param keyName the char* value of the string to parse
     * @return KeyType the unique KeyType corresponding to the keyName.
     */
    INLINE KeyType get ( const char* keyName )
    {
      KeyHash hash = getHash ( keyName );
      for ( HashEntry* entry = hashEntries[hash] ;
          entry ; entry = entry->next )
        {
          if ( strcmp ( entry->key, keyName ) == 0 )
            return entry->keyId;
        }
      Log_HashBucket ( "HashBucket has no '%s'.\n", keyName );
      return 0;
    }
  
    /**
     * Add a new keyName to KeyType mapping.
     * @param keyId the KeyType of the mapping.
     * @param keyName the char* value of the mapping. The keyName must be alloced properly, HashBucket will not strdup() it.
     */
    INLINE void put ( KeyType keyId, const char* keyName )
    {
      KeyHash hash = getHash ( keyName );
      HashEntry* currentEntry = hashEntries[hash];
      HashEntry* newEntry = new HashEntry();
      newEntry->keyId = keyId;
      newEntry->key = keyName;
      newEntry->next = currentEntry;
      hashEntries[hash] = newEntry;
    }

    /**
     * Remove a keyName to KeyType mappine, the keyName and keyId *must* match !!!
     * @param keyId the KeyType to remove
     * @param keyName the char* value to remove
     */
    INLINE void remove ( KeyType keyId, const char* keyName )
    {
      KeyHash hash = getHash ( keyName );
      HashEntry* last = NULL;
      for ( HashEntry* entry = hashEntries[hash] ;
          entry ; entry = entry->next )
        {
          if ( strcmp ( entry->key, keyName ) == 0 && entry->keyId == keyId )
            {
              if ( last ) 
                {
                  last->next = entry->next;
                }
              else
                {
                  hashEntries[hash] = entry->next;
                }
              delete ( entry );
              return;
            }
          last = entry;
        }
      Bug ( "HashBucket::remove() : could not remove keyId=%llx, keyName=%s!!!\n",
          (__ui64) keyId, keyName );
    }
    
    /**
     * clears all contents. Does not free the keyNames provided.
     */
    void clear ()
    {
      for ( __ui32 i = 0 ; i < hashBucketSize ; i++ )
        if ( hashEntries[i] )
          {
            HashEntry* entry = hashEntries[i];
            while ( entry )
              {
                HashEntry* nextEntry = entry->next;
                delete ( entry );
                entry = nextEntry;
              }
          }  
    }
  };

};


#endif // __XEM_KERN_HASHBUCKET_H
