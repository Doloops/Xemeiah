#include <Xemeiah/persistence/persistentstore.h>
#include <Xemeiah/kern/keycache.h>

#include <Xemeiah/kern/exception.h>

#include <Xemeiah/trace.h>

#include <Xemeiah/kern/format/core_types.h>
#include <Xemeiah/persistence/format/keys.h>

#include <Xemeiah/auto-inline.hpp>
#include <Xemeiah/persistence/auto-inline.hpp>

#define Log_Keys Debug

namespace Xem
{
  /*
   * Load namespaces and keys from an (already formatted) Persistence file.
   * Namespaces and Keys are inserted manually in the keyCache.
   */
  bool PersistentStore::loadKeysFromStore ()
  {
    if ( getSB()->keyPage == 0 )
      {
        Log_Keys ( "No keys pages !\n" );
      }
    Log_Keys ( "------------------------ Loading Keys ---------------------------\n" );
    for ( KeyPage* keyPage = getAbsolutePage<KeyPage> ( getSB()->keyPage ) ; 
        keyPage ; keyPage = getAbsolutePage<KeyPage> ( keyPage->nextPage ) )
      {
        for ( __ui64 idx = 0 ; 
              idx < KeyPage_keyNumber && keyPage->keys[idx].id ; 
              idx++ )
          {
            Log_Keys ( "Load key %x : %s\n", keyPage->keys[idx].id, keyPage->keys[idx].name );
            char* name = strdup ( keyPage->keys[idx].name );
            keyCache.localKeyMap.put ( keyPage->keys[idx].id, name );
            keyCache.keysBucket.put ( keyPage->keys[idx].id, name );
          }
      }
    Log_Keys ( "------------------------ Loading Namespaces ---------------------------\n" );
    for ( NamespacePage* namespacePage = getAbsolutePage<NamespacePage> ( getSB()->namespacePage ) ; 
        namespacePage ; namespacePage = getAbsolutePage<NamespacePage> ( namespacePage->nextPage ) )
      {
        for ( __ui64 idx = 0 ; 
              idx < NamespacePage_namespaceNumber && namespacePage->namespaces[idx].namespaceId ; 
              idx++ )
          {
            Log_Keys ( "Load namespace %x : %s\n",
                   namespacePage->namespaces[idx].namespaceId, namespacePage->namespaces[idx].url );
            char* url = strdup ( namespacePage->namespaces[idx].url );
            keyCache.namespaceBucket.put ( namespacePage->namespaces[idx].namespaceId, url );
            keyCache.namespaceMap.put ( namespacePage->namespaces[idx].namespaceId, url );
          }
      }
    Log_Keys ( "------------------------ End of Namespaces Loading ---------------------------\n" );
    return buildKeyCacheBuiltinKeys ();
  }


  LocalKeyId PersistentStore::addKeyInStore ( const char* keyName )
  /*
   * Called with the KeyStore key_lock held, and returns with the key_lock held...
   */
  {
    if ( strlen ( keyName ) >= KeySegment::nameLength )
      {
        Bug ( "KeyName too long : '%s'\n", keyName );
        return 0;
      }
    KeyPage* keyPage = NULL;
    if ( getSB()->keyPage == 0 )
      {
        /*
         * No key page, shall create one before
         */
        AbsolutePagePtr keyPagePtr = getFreePagePtr ();
        keyPage = getAbsolutePage<KeyPage> ( keyPagePtr );
        alterPage ( keyPage );
        memset ( keyPage, 0, PageSize );
        protectPage ( keyPage );
        lockSB ();
        getSB()->keyPage = keyPagePtr;
        unlockSB ();
      }
    else
      keyPage = getAbsolutePage<KeyPage> ( getSB()->keyPage );
    LocalKeyId nextKeyId = 0;
    for ( ; keyPage ; )
      {
        /*
         * It is asserted that keyPages have no hole, because keys are created continuously, 
         * and never deleted.
         */
        for ( __ui64 idx = 0; idx < KeyPage_keyNumber ; idx++ )
          if ( keyPage->keys[idx].id == 0 )
            {
              Log_Keys ( "Empty slot %llx at page %p\n", idx, keyPage );
              nextKeyId++;

              alterPage ( keyPage );
              keyPage->keys[idx].id = nextKeyId;
              strcpy ( keyPage->keys[idx].name, keyName );
              protectPage ( keyPage );

              return nextKeyId;
            }
          else
            nextKeyId = keyPage->keys[idx].id;
        if ( keyPage->nextPage )
          {
            keyPage = getAbsolutePage<KeyPage> ( keyPage->nextPage );
            continue;
          }
        else if ( ! keyPage->nextPage )
          {
            Log_Keys ( "Creating keyPage..\n" );
            AbsolutePagePtr nextPagePtr = getFreePagePtr ();
            KeyPage* nextPage = getAbsolutePage<KeyPage> ( nextPagePtr );

            Log_Keys ( "Created new keyPage=%llx/%p, continuation of %p\n", nextPagePtr, nextPage, keyPage );
            alterPage ( nextPage );
            memset ( nextPage, 0, PageSize );
            protectPage ( nextPage );
            alterPage ( keyPage );
            keyPage->nextPage = nextPagePtr;
            protectPage ( keyPage );
            keyPage = nextPage;
            continue;
          }
        Bug ( "Shall not be here !!!\n" );
      }
    Bug ( "Shall not be here.\n" );
    return 0;
  }
  
  
  NamespaceId PersistentStore::addNamespaceInStore ( const char* namespaceURL )
  /*
   * Called with the KeyStore key_lock held, and returns with the key_lock held...
   */
  {
    NamespacePage* namespacePage = NULL;
    __ui32 len = strlen(namespaceURL) + 1;
    if ( len > NamespaceSegment::urlLength )
      {
        Bug ( "Namespace too large : '%s'\n", namespaceURL );
      }
    if ( getSB()->namespacePage == 0 )
      {
        /*
         *  No namespace page, shall create the first one
         */
        AbsolutePagePtr namespacePagePtr = getFreePagePtr ();
        namespacePage = getAbsolutePage<NamespacePage> ( namespacePagePtr );
        alterPage ( namespacePage );
        memset ( namespacePage, 0, PageSize );
        protectPage ( namespacePage );
        lockSB ();
        getSB()->namespacePage = namespacePagePtr;
        unlockSB ();
      }
    else
      namespacePage = getAbsolutePage<NamespacePage> ( getSB()->namespacePage );

    NamespaceId nextNamespaceId = 1;
    for ( ; namespacePage ; )
      {
        /*
         * It is asserted that namespace pages have no hole, because all namespaces are created
         * continuously, and never deleted.
         */
        if ( namespacePage->nextPage )
          {
            nextNamespaceId = namespacePage->namespaces[NamespacePage_namespaceNumber-1].namespaceId + 1;
            namespacePage = getAbsolutePage<NamespacePage> ( namespacePage->nextPage );
            continue;
          }
        for ( __ui64 idx = 0; idx < NamespacePage_namespaceNumber ; idx++ )
          {
            if ( namespacePage->namespaces[idx].namespaceId == 0 )
              {
                Log_Keys ( "Empty slot %llx at page %p\n", idx, namespacePage );
                alterPage ( namespacePage );
                namespacePage->namespaces[idx].namespaceId = nextNamespaceId;
                strcpy ( namespacePage->namespaces[idx].url, namespaceURL );
                protectPage ( namespacePage );
                return nextNamespaceId;
              }
            nextNamespaceId = namespacePage->namespaces[idx].namespaceId + 1;
          }
          
        Log_Keys ( "Creating new namespacePage..\n" );
        AbsolutePagePtr nextPagePtr = getFreePagePtr ();
        NamespacePage* nextPage = getAbsolutePage<NamespacePage> ( nextPagePtr );
        Log_Keys ( "Created new namespacePage=%llx/%p, continuation of %p\n", nextPagePtr, nextPage, namespacePage );
        alterPage ( nextPage );
        memset ( nextPage, 0, PageSize );
        protectPage ( nextPage );
        alterPage ( namespacePage );
        namespacePage->nextPage = nextPagePtr;
        protectPage ( namespacePage );
        namespacePage = nextPage;
      }
    Bug ( "Shall not be here.\n" );
    return 0;
  }
};

