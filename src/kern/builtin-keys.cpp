#include <Xemeiah/kern/keycache.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_Keys Debug

namespace Xem
{
  void KeyCache::processBuiltinKeyId ( KeyId& keyId, NamespaceId nsId, const char* localName )
  {
    Log_Keys ( "Getting builtinKey : ns='%s' (%x) : '%s'\n", getNamespaceURL(nsId), nsId, localName );
    char* keyName = strdup(localName);
    for ( char* c = keyName ; *c != '\0' ; c++ )
      {
        if ( *c == '_' ) 
          {
            /*
             * Remove trailing '_' to disambiguate with C++ reserved keywords
             */
            if ( c[1] == '\0' )
              {
                *c = '\0';
                break;
              }
            else
              *c = '-';
          }
      }
    Log_Keys ( "BuiltinKey : mod='%s'\n", keyName );
    LocalKeyId localKeyId = getKeyId ( 0, keyName, true );
    keyId = getKeyId ( nsId, localKeyId );
    Log_Keys ( "-> Result : %x\n", keyId );
    free ( keyName );
  }

  void KeyCache::processBuiltinNamespaceId ( KeyId& nsKeyId, NamespaceId& nsId, 
        const char* _prefixName, const char* url )
  {
    char* prefixName = strdup(_prefixName);
    for ( int i = 0 ; ; i++ )
      {
        prefixName[i] = ( _prefixName[i] == '_' ) ? '-' : _prefixName[i];
        if (! prefixName[i] ) break;
      }

    Log_Keys ( "prefixName='%s', _prefixName='%s'\n", prefixName, _prefixName );
    
    nsId = getNamespaceId ( url );
  
    NamespaceId xmlnsNsId = getNamespaceId ( "http://www.w3.org/2000/xmlns/" );

    if ( nsId )
      {    
        Log_Keys ( "Builtin Namespace prefix='%s', url='%s'\n", prefixName, url );
        nsKeyId = getKeyId ( xmlnsNsId, prefixName, true );
        Log_Keys ( "Builtin Namespace prefix='%s', url='%s' : nsId=%x, nsKeyId=%x\n", prefixName, url, nsId, nsKeyId );
      }
    free ( prefixName );
  }

  bool KeyCache::buildBuiltinKeys ()
  {
    Log_Keys ( "Building builtin keys !\n" );
    Log_Keys ( "Builtin pointers at ref=%p\n", &builtinKeys );

    builtinKeys = new BuiltinKeys ( *this );   
    return true;
  }

  KeyCache::BuiltinKeys::BuiltinKeys ( KeyCache& keyCache )
  : dummy(0)
#define __STARTNAMESPACE(__globalid,__url)
#define __ENDNAMESPACE(__globalid,__prefixid) , __globalid(keyCache)
#define __KEY(...)
#include <Xemeiah/kern/builtin_keys.h>
#undef __KEY
#undef __ENDNAMESPACE
#undef __STARTNAMESPACE
  {

  }

  KeyCache::BuiltinKeys::~BuiltinKeys ()
  { 
  }


#ifndef STRINGIFY 
#define STRINGIFY(__x) #__x
#endif

  bool KeyCache::BuiltinKeys::__build ( KeyCache& keyCache )
  {
    return true;
  }

#define __STARTNAMESPACE(__globalid,__url) \
  bool __BUILTIN__##__globalid##_CLASS::__build ( KeyCache& keyCache ) \
  { \
    keyCache.processBuiltinNamespaceId ( xmlns_PREFIX_VALUE, ns_VALUE, STRINGIFY(__globalid), __url ); \
    Log_Keys ( "Building class '%s', nsId='%x' (namespace='%s')\n", \
        STRINGIFY(__globalid), ns_VALUE,__url );

#define __KEY(__key) keyCache.processBuiltinKeyId( __key##_VALUE, ns_VALUE, STRINGIFY(__key) );
#define __ENDNAMESPACE(__globalid,__prefixid) \
    Log_Keys ( "Built class '%s' '%s' (ns=%x)\n", STRINGIFY(__globalid), STRINGIFY(__prefixid), ns_VALUE ); \
    return true; \
 }
#include <Xemeiah/kern/builtin_keys.h>
#undef __KEY
#undef __ENDNAMESPACE
#undef __STARTNAMESPACE


};
