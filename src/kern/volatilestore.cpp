#include <Xemeiah/kern/volatilestore.h>

#include <Xemeiah/auto-inline.hpp>

namespace Xem
{
  VolatileStore::VolatileStore ()
  {
    if ( ! buildKeyCacheBuiltinKeys() )
      {
    	  Fatal ( "VolatileStore : unable to get builtin keys !\n" );
      }
    // xprocessorStoredModules = new XProcessorStoredModules ( *this );
  }

  VolatileStore::~VolatileStore ()
  {
  
  }

  BranchManager& VolatileStore::getBranchManager()
  {
    return volatileBranchManager;
    // throwException ( Exception, "VolatileStore has no Branch Manager defined !\n" );
    // return *((BranchManager*)NULL);
  }
  
  
  LocalKeyId standaloneLocalKeyId = 0x3;

  LocalKeyId VolatileStore::addKeyInStore ( const char* keyName )
  {
    return ++standaloneLocalKeyId;
  }

  NamespaceId standaloneNamespaceId = 0x4;

  NamespaceId VolatileStore::addNamespaceInStore ( const char* namespaceURL )
  {
    return ++standaloneNamespaceId;    
  }

  ElementId standaloneNextElementId = 0x2;

  bool VolatileStore::reserveElementIds ( ElementId& nextId, ElementId& lastId )
  {
    nextId = standaloneNextElementId;
    standaloneNextElementId += 256;
    lastId = standaloneNextElementId;
    return true;
  }
  
};
