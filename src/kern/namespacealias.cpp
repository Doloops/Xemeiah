#include <Xemeiah/kern/store.h>
#include <Xemeiah/kern/namespacealias.h>
#include <Xemeiah/kern/keycache.h>
#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/dom/attributeref.h>
#include <Xemeiah/trace.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_NamespaceAlias Debug

namespace Xem
{
  NamespaceAlias::NamespaceAlias ( KeyCache& _keyCache, bool noInstanciation ) 
  : keyCache(_keyCache)
  {
    Log_NamespaceAlias( "----------- New Namespace Alias %p --------------\n", this );
    stackHead = NULL;
    currentStackLevel = 0;
    push ();
  }

  NamespaceAlias::NamespaceAlias ( KeyCache& _keyCache ) 
  : keyCache(_keyCache)
  {
    Log_NamespaceAlias( "----------- New Namespace Alias %p --------------\n", this );
    stackHead = NULL;
    currentStackLevel = 0;

    push ();
#define __setDefaultNamespacePrefix(__ns) \
    setNamespacePrefix ( keyCache.getBuiltinKeys().__ns.defaultPrefix(), keyCache.getBuiltinKeys().__ns.ns() );

    __setDefaultNamespacePrefix ( xml );
    __setDefaultNamespacePrefix ( xemint );
#undef __setDefaultNamespacePrefix
  }

  NamespaceAlias::~NamespaceAlias () 
  {
    if ( canPop() )
      {
        pop ();
      }
    else
      {
        Error ( "At NamespaceAlias destructor : could not pop !\n" );
      }
  }

  NamespaceAlias::StackHeader* NamespaceAlias::createStackHeader ()
  {
    if ( ! stackHead || stackHead->level < currentStackLevel )
      {
        StackHeader* stack = new StackHeader ();
        stack->level = currentStackLevel;
        stack->firstDeclaration = NULL;
        stack->previousHeader = stackHead;
        stackHead = stack;
      }
    return stackHead;
  }
  
  void NamespaceAlias::setNamespacePrefix ( KeyId aliasId, NamespaceId namespaceId, bool overwrite )
  {
    LocalKeyId localKeyId = keyCache.getLocalKeyId ( aliasId );
    Log_NamespaceAlias( "Level=%x : Setting key prefix %x (%s) to namespaceId %x (%s)\n",
        currentStackLevel, aliasId, keyCache.dumpKey(aliasId).c_str(), namespaceId, keyCache.getNamespaceURL ( namespaceId ) );

#if PARANOID
    AssertBug ( keyCache.getLocalKeyId ( aliasId ), "Null local key id provided : %x\n", aliasId );
    if ( aliasId != keyCache.getBuiltinKeys().nons.xmlns() )
      {
        AssertBug ( keyCache.getNamespaceId ( aliasId ) == keyCache.getBuiltinKeys().xmlns.ns(),
          "Invalid aliasId %x : wrong namespace %x, expected %x.\n", 
          aliasId, keyCache.getNamespaceId ( aliasId ), keyCache.getBuiltinKeys().xmlns.ns() );
        AssertBug ( namespaceId, "Invalid null namespaceId for full prefix declaration !\n" );
      }
#endif

    if ( ! overwrite )
      {
        if ( stackHead && stackHead->level == currentStackLevel && localKeyIdToNamespaceIdMap[localKeyId] )
          {
            for ( AliasDeclaration* alias = stackHead->firstDeclaration ; alias ; alias = alias->nextDeclaration )
              {
                if ( alias->localKeyId == localKeyId )
                  {
                    if ( alias->namespaceId == namespaceId ) return;
                    NotImplemented ( "[NSAlias][Check] Already defined prefix '%x' for this NamespaceAlias stackLevel !\n",
                      localKeyId );
                  }
                if ( alias->namespaceId == namespaceId )
                  {
                    /*
                     * We know that alias->localKeyId != localKeyId
                     */
                    Log_NamespaceAlias ( "[NSAlias][Check] Already defined prefix %x:%s for namespace '%s' for this NamespaceAlias stackLevel, "
                      "overriding it with %x:%s\n",
                      alias->localKeyId, keyCache.getLocalKey ( alias->localKeyId ), 
                      keyCache.getNamespaceURL(namespaceId),
                      localKeyId, keyCache.getLocalKey ( localKeyId ) );
                    
                    namespaceIdToLocalKeyIdMap[namespaceId] = localKeyId;
                    localKeyIdToNamespaceIdMap[localKeyId] = namespaceId;
                    return;
                  }
              }            
          }
      }
    StackHeader* stack = createStackHeader ();
    AliasDeclaration* alias = new AliasDeclaration();
    
    
    alias->localKeyId = localKeyId;
    alias->namespaceId = namespaceId;
    
    alias->previousLocalKeyId = namespaceIdToLocalKeyIdMap[namespaceId];
    alias->previousNamespaceId = localKeyIdToNamespaceIdMap[localKeyId];
    
    alias->nextDeclaration = stack->firstDeclaration;
    stack->firstDeclaration = alias;
    
    namespaceIdToLocalKeyIdMap[namespaceId] = localKeyId;
    localKeyIdToNamespaceIdMap[localKeyId] = namespaceId;
  }
  
  NamespaceId NamespaceAlias::getNamespaceIdFromAlias ( KeyId aliasId )
  {
#if PARANOID
    if ( keyCache.getNamespaceId ( aliasId ) )
      {
        AssertBug ( keyCache.getNamespaceId ( aliasId ) == keyCache.getBuiltinKeys().xmlns.ns(),
          "Invalid aliasId %x\n", aliasId );
        AssertBug ( keyCache.getLocalKeyId ( aliasId ), "Null local key id provided : %x\n", aliasId );
      }
    else
      {
        AssertBug ( aliasId == keyCache.getBuiltinKeys().nons.xmlns(), "Invalid aliasId %x\n", aliasId );
      }  
#endif
    LocalKeyId localKeyId = keyCache.getLocalKeyId ( aliasId );
    if ( keyCache.getNamespaceId ( aliasId ) )
      {
        AssertBug ( keyCache.getNamespaceId ( aliasId ) == keyCache.getBuiltinKeys().xmlns.ns(),
          "Invalid aliasId %x\n", aliasId );
      }
    else
      {
        AssertBug ( aliasId == keyCache.getBuiltinKeys().nons.xmlns(),
          "Invalid aliasId %x\n", aliasId );
        aliasId = 0;
      }
    return localKeyIdToNamespaceIdMap[localKeyId];
  }
  
  NamespaceId NamespaceAlias::getNamespaceIdFromPrefix ( LocalKeyId prefixId )
  {
    return localKeyIdToNamespaceIdMap[prefixId];
  }

  LocalKeyId NamespaceAlias::getNamespacePrefix ( NamespaceId namespaceId )
  {
    if ( namespaceId == keyCache.getBuiltinKeys().xmlns.ns() )
      return keyCache.getBuiltinKeys().nons.xmlns();
    return namespaceIdToLocalKeyIdMap[namespaceId];
  }

  NamespaceId NamespaceAlias::getDefaultNamespaceId ()
  {
    return localKeyIdToNamespaceIdMap[keyCache.getBuiltinKeys().nons.xmlns()];
  }

  void NamespaceAlias::setDefaultNamespaceId ( NamespaceId nsId )
  {
    setNamespacePrefix ( keyCache.getBuiltinKeys().nons.xmlns(), nsId );
  }

  
  void NamespaceAlias::push ()
  {
    currentStackLevel ++;
    Log_NamespaceAlias( "Push : currentStackLevel = %x, stackHead=%x\n",
      currentStackLevel, stackHead ? stackHead->level : 0 );
  }
   
  bool NamespaceAlias::canPop ()
  {
    return (currentStackLevel > 0);
  }

  void NamespaceAlias::pop ()
  {
    Log_NamespaceAlias( "Pop : currentStackLevel = %x, stackHead=%x\n",
      currentStackLevel, stackHead ? stackHead->level : 0 );
    AssertBug ( currentStackLevel > 0, "Null stackLevel !!!\n" );
    if ( stackHead && stackHead->level == currentStackLevel )
      {
        Log_NamespaceAlias( "Rewind stack, currentStackLevel='%u'\n", currentStackLevel );
        StackHeader* previous = stackHead->previousHeader;

        AliasDeclaration* alias = stackHead->firstDeclaration;
        while ( alias )
          {
            /**
             * Restore precedent values.
             */
            namespaceIdToLocalKeyIdMap[alias->namespaceId] = alias->previousLocalKeyId;
            localKeyIdToNamespaceIdMap[alias->localKeyId] = alias->previousNamespaceId;
            
            AliasDeclaration* next = alias->nextDeclaration;
            free ( alias );
            alias = next;
          }
        free ( stackHead );
        stackHead = previous;
      }
    currentStackLevel--;
  }
};

