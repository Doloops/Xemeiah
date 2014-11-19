#include <Xemeiah/kern/store.h>
#include <Xemeiah/kern/document.h>
#include <Xemeiah/trace.h>
#include <Xemeiah/nodeflow/nodeflow.h>

#include <Xemeiah/xprocessor/env.h>

#define Log_PushEnv Debug
#define Log_EnvHPP Debug

namespace Xem
{
  __INLINE Env::EnvEntry::~EnvEntry ()
  {
    if ( documentAllocator ) delete ( documentAllocator );
  }
  

  __INLINE void Env::pushEnv ()
  {
    currentEnvId++;
    Log_PushEnv ( "%p PUSH : Setting Env to %lx\n", this, currentEnvId );
  }

  __INLINE void Env::popEnv ()
  {
    AssertBug ( currentEnvId, "Could not pop Env !\n" );
    if ( headEntry && headEntry->envId == currentEnvId )
        freeEnvEntry ();
    currentEnvId --;
    Log_PushEnv ( "%p POP  : Setting Env to %lx\n", this, currentEnvId );
  }


  __INLINE void Env::pushIterator ( NodeSet::iterator& iterator )
  {
    iteratorStack.push_front ( &iterator );  
  }

  __INLINE void Env::popIterator ( NodeSet::iterator& iterator )
  {
    AssertBug ( iteratorStack.front() == &iterator, "Invalid iterator popped !\n" );
    iteratorStack.pop_front();
  }
 
  __INLINE bool Env::hasIterator ()
  {
    if ( iteratorStack.size() == 0 ) return false;
    return ( iteratorStack.front() != NULL );  
  }
  
  __INLINE void Env::setXPathBaseNode ( )
  {
    xpathBaseNode = &(getCurrentNode());
  }
  
  __INLINE NodeRef& Env::getXPathBaseNode ()
  {
    AssertBug ( xpathBaseNode, "Empty baseNode set !\n" );
    return *xpathBaseNode;
  }

  __INLINE NodeRef& Env::getCurrentNode()
  {
    if ( iteratorStack.empty() )
      {
        Bug ( "Current Node is Empty !" );
        throwException ( Exception, "Empty iterator stack !\n" );
      }
    NodeSet::iterator* iter = iteratorStack.front ();
    if ( ! iter )
      {
        Bug ( "." );
        throwException ( Exception, "Empty iterator stack !\n" );
      }
    return (*iter)->toNode ();
  }
  
#ifdef __XEM_ENV_ENVENTRY_HAS_POOLALLOCATOR
  __INLINE Env::EnvItem* Env::EnvEntry::newEnvItem ()
  {
    Log_EnvHPP ( "new EnvItem : sizeof()=%lu (0x%lx)\n", sizeof(EnvItem), sizeof(EnvItem) );
    EnvItem* envItem = defaultPoolAllocator.allocate ( 1 );
    return new(envItem) EnvItem ();  
  }
 
  __INLINE void Env::EnvEntry::releaseEnvItem ( EnvItem* envItem )
  {
  
  }
  
#else
  __INLINE Env::EnvItem* Env::EnvEntry::newEnvItem ()
  { return new EnvItem(); }

  __INLINE void Env::EnvEntry::releaseEnvItem ( EnvItem* envItem )
  { delete ( envItem ); }
#endif

  __INLINE Env::EnvEntry* Env::getHeadEntry () 
  {
    if ( ! headEntry || headEntry->envId != currentEnvId )
      createEnvEntry ();
    return headEntry;
  }
 
  __INLINE Env::EnvEntry* Env::getBehindEntry ()
  {
    if ( !currentEnvId ) { Bug ( "Could not get behind, zero currentEnvId !\n" ); }
    if ( ! headEntry )
      /*
       * No headEntry, we can easily create one for last envId.
       */
      {
        Log_EnvHPP ( "Create root EnvEntry !\n" );
        headEntry = new EnvEntry();
        headEntry->lastEntry = NULL;
        headEntry->envId = currentEnvId - 1;
        headEntry->firstItem = NULL;
        headEntry->previousDocumentAllocator = currentDocumentAllocator;
        return headEntry;
      }
    else if ( headEntry->envId == currentEnvId - 1 )
      /*
       * Current headEntry is the last envId entry.
       */
      {
        return headEntry;
      }
    else if ( headEntry && headEntry->lastEntry 
	      && headEntry->lastEntry->envId == currentEnvId - 1 )
      /*
       * Previous entry is the last envId entry.
       */
      {
        return headEntry->lastEntry;
      }

    /*
     * Most complex case : we have to create an entry between the current one
     * and the previous ones.
     */
#if PARANOID
    AssertBug ( headEntry, "Must have had an headEntry here !\n" );
    AssertBug ( ! headEntry->lastEntry || 
            ( headEntry->lastEntry->envId != currentEnvId-1),
            "Invalid lastEntry ! Must have handled this before !\n" );
#endif
    EnvEntry* newEntry = new EnvEntry ();
    newEntry->envId = currentEnvId - 1;
    newEntry->firstItem = NULL;
    if ( headEntry->envId == currentEnvId - 1 )
     {
       Bug ( "Already handled this case !\n" );
     }
    else if ( headEntry->envId < currentEnvId - 1 )
      {
        newEntry->lastEntry = headEntry;
        newEntry->previousDocumentAllocator = headEntry->previousDocumentAllocator;
        headEntry = newEntry;
      }
    else if ( headEntry->envId == currentEnvId )
      {
        AssertBug ( ! headEntry->lastEntry 
          || headEntry->lastEntry->envId < newEntry->envId, "Invalid last entry !\n" );

        newEntry->lastEntry = headEntry->lastEntry;
        headEntry->lastEntry = newEntry;
        newEntry->previousDocumentAllocator = headEntry->previousDocumentAllocator;
        // NotImplemented ( "!" );
      }
    else
      {
        Bug ( "." );
        throwException ( Exception, "[FIXME][ENV] Invalid entry=%lx, current=%lx\n",
          headEntry->envId, currentEnvId );
      }
    return newEntry;
  }


  __INLINE void Env::assignEnvItem ( EnvEntry* envEntry, EnvItem* newItem )
  {
    Log_EnvHPP ( "%p setVariable envId=%lx/current=%lx, keyId=%x:%s\n",
          this, envEntry->envId, currentEnvId, newItem->keyId, store.getKeyCache().dumpKey(newItem->keyId).c_str() );
#if PARANOID
    AssertBug ( envEntry, "No envEntry defined !\n" );
    AssertBug ( envEntry->envId == currentEnvId 
        || envEntry->envId == ( currentEnvId-1 ),
        "Invalid envEntry->envId=%lx, currentEnvId=%lx\n",
        envEntry->envId, currentEnvId );

    AssertBug ( envEntry->firstItem != newItem, "EnvItem already added !\n" );
#endif
    EnvItem* previousItem = NULL;
    ItemMap::iterator previousItemIterator = itemMap.find ( newItem->keyId );
    if ( previousItemIterator != itemMap.end () )
      previousItem = previousItemIterator->second;

    newItem->envId = envEntry->envId;
    newItem->nextInEntry = envEntry->firstItem;
    newItem->previousItem = previousItem;
    envEntry->firstItem = newItem;
    itemMap[newItem->keyId] = newItem;
    

  }

  __INLINE NodeSet* Env::setVariable ( KeyId keyId, bool behind )
  {
    NodeSet* nodeSet = allocateVariable ( keyId, behind );
    assignVariable ( keyId, nodeSet, behind );
    return nodeSet;
  }

  __INLINE NodeSet* Env::allocateVariable ( KeyId keyId, bool behind )
  {
    EnvEntry* entry = NULL;
    if ( behind )
      {
        entry = getBehindEntry ();      
      }
    else
      {
        entry = getHeadEntry ();
      }
    EnvItem* newItem = entry->newEnvItem ();
    newItem->keyId = keyId;
    return newItem;
  }

  __INLINE void Env::assignVariable ( KeyId keyId, NodeSet* nodeSet, bool behind )
  {
    EnvEntry* entry = NULL;
    if ( behind )
      {
        entry = getBehindEntry ();      
      }
    else
      {
        entry = headEntry;
      }
    EnvItem* item = static_cast<EnvItem*> ( nodeSet );
#if PARANOID
    AssertBug ( entry, "No entry for '%x'\n", keyId );
    AssertBug ( item, "Invalid type of NodeSet %p\n", nodeSet );
    AssertBug ( item->keyId == keyId, "Diverging keyId : %x != %x\n", item->keyId, keyId );
#endif
    assignEnvItem ( entry, item );  
  }

  __INLINE bool Env::hasVariable ( KeyId keyId )
  {
    if ( ! keyId ) return false;
    return ( itemMap.find(keyId) != itemMap.end() );
  }

  __INLINE NodeSet* Env::getVariable ( KeyId keyId )
  {
    if ( ! hasVariable ( keyId ) )
      {
        Error ( "No variable 0x%x '%s'\n", keyId, store.getKeyCache().dumpKey(keyId).c_str() );
        throwException ( VariableNotFoundException, 
                 "No variable 0x%x '%s'\n", keyId, store.getKeyCache().dumpKey(keyId).c_str() );
        return NULL;
      }
    return itemMap[keyId];
  }

  __INLINE void Env::setVariableNode ( KeyId keyId, NodeRef& nodeRef, bool behind )
  {
    NodeSet* nodeSet = setVariable ( keyId, behind );
    nodeSet->pushBack ( nodeRef );
  }
};
