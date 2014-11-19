#include <Xemeiah/xprocessor/xprocessormodule.h>
#include <Xemeiah/xprocessor/xprocessormoduleforge.h>
#include <Xemeiah/xprocessor/xprocessor.h>
#include <Xemeiah/dom/documentmeta.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_XProc Debug

namespace Xem
{
  XProcessorModule::XProcessorModule ( XProcessor& _xprocessor, XProcessorModuleForge& _moduleForge )
  : xprocessor(_xprocessor), moduleForge(_moduleForge)
  {
    defaultNSHandler = NULL;
  }

  /*
   * TODO here is a bunch of stubs that HAVE to be inlined
   */
  NodeFlow& XProcessorModule::getNodeFlow() { return getXProcessor().getNodeFlow(); }
  KeyCache& XProcessorModule::getKeyCache() { return getXProcessor().getKeyCache(); }
  NodeRef& XProcessorModule::getCurrentNode() { return getXProcessor().getCurrentNode(); }
  ServiceManager& XProcessorModule::getServiceManager() { return getXProcessor().getStore().getServiceManager(); }

#ifdef __XEM_XPROCESSORMODULE_HAS_LOCALMAPS
  void XProcessorModule::registerHandler ( KeyId keyId, XProcessorHandler handler, bool checkNamespace )
  {
    if ( checkNamespace && KeyCache::getNamespaceId(keyId) != moduleForge.getModuleNamespaceId() )
      {
        Bug ( "Diverging namespaces !\n" );
      }
    localHandlerMap[KeyCache::getLocalKeyId(keyId)] = handler;
  }
  
  void XProcessorModule::registerFunction ( KeyId keyId, XProcessorFunction function )
  {
    if ( KeyCache::getNamespaceId(keyId) != moduleForge.getModuleNamespaceId() )
      {
        Bug ( "Diverging namespaces !\n" );
      }
    localFunctionMap[KeyCache::getLocalKeyId(keyId)] = function;
  }
  
  void XProcessorModule::registerDomEventHandler ( KeyId keyId, DomEventHandler handler )
  {
    if ( KeyCache::getNamespaceId(keyId) != moduleForge.getModuleNamespaceId() )
      {
        Bug ( "Diverging namespaces !\n" );
      }
    localDomEventMap[KeyCache::getLocalKeyId(keyId)] = handler;
  }
#endif

  XProcessorModule::XProcessorHandler XProcessorModule::getHandler ( LocalKeyId localKeyId )
  {
    XProcessorHandler handler = moduleForge.getHandler(localKeyId);
    if ( handler ) return handler;
#ifdef __XEM_XPROCESSORMODULE_HAS_LOCALMAPS
    return localHandlerMap[localKeyId];
#else
    Log_XProc ( "Getting defaultNSHandler !\n" );
    return defaultNSHandler; // (XProcessorHandler) NULL;
#endif
  }

  XProcessorModule::XProcessorFunction XProcessorModule::getFunction ( LocalKeyId localKeyId )
  {
    XProcessorFunction function = moduleForge.getFunction(localKeyId);
    if ( function ) return function;
#ifdef __XEM_XPROCESSORMODULE_HAS_LOCALMAPS
    return localFunctionMap[localKeyId];
#else
    return (XProcessorFunction) NULL;
#endif
  }

  XProcessorModule::DomEventHandler XProcessorModule::getDomEventHandler ( LocalKeyId localKeyId )
  {
    DomEventHandler handler = moduleForge.getDomEventHandler(localKeyId);
    if ( handler ) return handler;
#ifdef __XEM_XPROCESSORMODULE_HAS_LOCALMAPS
    return localDomEventMap[localKeyId];
#else
    return (DomEventHandler) NULL;
#endif
  }

  void XProcessorModule::getXProcessorModuleProperty ( NodeSet& nodeSet, KeyId ptyId )
  {
    throwException(Exception, "Could not get XProcessorModule property : this module '%s' does not support it !\n",
        getKeyCache().getNamespaceURL(getXProcessorModuleForge().getModuleNamespaceId()));
  }

  void XProcessorModule::triggerEvent(KeyId eventId, KeyIdList& arguments)
  {
    throwException(Exception, "Could not trigger event : this module '%s' does not support it !\n",
        getKeyCache().getNamespaceURL(getXProcessorModuleForge().getModuleNamespaceId()));
  }
};

