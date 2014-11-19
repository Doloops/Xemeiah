#include <Xemeiah/kern/store.h>
#include <Xemeiah/kern/keycache.h>
#include <Xemeiah/kern/namespacealias.h>
#include <Xemeiah/trace.h>

#define Log_KeyCacheHPP Debug

namespace Xem
{
  __INLINE LocalKeyId KeyCache::getFromCache ( const char* keyName )
  {
#if PARANOID
    AssertBug ( strchr(keyName,':') == NULL, "Semicolumn in key !\n" );
#endif
    return keysBucket.get ( keyName );
  }

  __INLINE KeyId KeyCache::getKeyId ( NamespaceAlias& nsAlias, const char* keyName, bool create )
  {
    NamespaceId nsId = 0;
    AssertBug ( keyName, "Null keyName provided !\n" );

    LocalKeyId prefixId, localKeyId;
    
    parseKey ( String(keyName), prefixId, localKeyId );
    
    if ( prefixId )
      {
        nsId = nsAlias.getNamespaceIdFromPrefix ( prefixId );
        if ( ! nsId )
          {
            throwException ( Exception, "Could not resolve prefix in key '%s'\n", keyName ); 
          }
      }    
    else
      {
        nsId = nsAlias.getDefaultNamespaceId ();
      }    
    return getKeyId ( nsId, localKeyId );
  }

  __INLINE KeyId KeyCache::getKeyId ( NamespaceAlias& nsAlias, const String& keyName, bool create )
  {
    return getKeyId ( nsAlias, keyName.c_str(), create );
  }

  __INLINE const char* KeyCache::getKey ( NamespaceAlias& nsAlias, KeyId keyId )
  {
    
    NamespaceId nsId = getNamespaceId( keyId );
    LocalKeyId localKeyId = getLocalKeyId ( keyId );
#if PARANOID
    AssertBug ( localKeyMap.get ( localKeyId ), "Could not get local part of key '%x'.\n", localKeyId );
#endif

    Log_KeyCacheHPP ( "getKey (%x) : nsId=%x, alias prefix=%x (xmlns=%x), default=%x\n",
        keyId, nsId, nsAlias.getNamespacePrefix ( nsId ), getBuiltinKeys().nons.xmlns(), nsAlias.getDefaultNamespaceId() );

    if ( !nsId )
      return getLocalKey ( localKeyId );
    
    LocalKeyId prefixId = nsAlias.getNamespacePrefix ( nsId );
    if ( prefixId == getBuiltinKeys().nons.xmlns() && nsId != getBuiltinKeys().xmlns.ns() )
      return getLocalKey ( localKeyId );

    if ( ! prefixId )
      {
        throwException ( Exception, "Namespace Alias has no prefix defined for namespace '%s' (%x), for key '%s'\n",
          getNamespaceURL(nsId), nsId, dumpKey(keyId).c_str() );
      }
      
    return getKey ( prefixId, localKeyId );
  }

  __INLINE const char* KeyCache::getLocalKey ( LocalKeyId localKeyId )
  {
    return localKeyMap.get ( localKeyId );
  }

  __INLINE const char* KeyCache::getKey ( LocalKeyId prefixId, LocalKeyId localKeyId )
  {
    AssertBug ( prefixId, "No prefixId !\n" );
    AssertBug ( localKeyId, "No localKeyId !\n" );

    const char* key = qnameMap.get ( prefixId, localKeyId );
    if ( key ) return key;

    const char* prefix = getLocalKey ( prefixId );
    AssertBug ( prefix, "Could not get prefix !\n" );
    const char* localKey = getLocalKey ( localKeyId );
    AssertBug ( localKey, "Could not get local key !\n" );

    size_t keyLen = strlen(prefix) + strlen(localKey) + 2;    
    char* newKey = (char*) malloc ( keyLen );
    sprintf ( newKey, "%s:%s", prefix, localKey );
    qnameMap.put ( prefixId, localKeyId, newKey );
    return newKey;
  }

#if 0
  __INLINE KeyId KeyCache::getKeyIdWithElement ( ElementRef* parsingFromElementRef, const String& keyName )
  {
    AssertBug ( parsingFromElementRef, "Invalid null parsingFromElementRef !\n" );
    return getKeyIdWithElement ( *parsingFromElementRef, keyName );
  }
#endif
};
