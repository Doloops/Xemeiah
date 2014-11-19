#include <Xemeiah/xprocessor/xprocessormoduleforge.h>
#include <Xemeiah/xprocessor/xemintmodule.h>
#include <Xemeiah/kern/format/domeventtype.h>
#include <Xemeiah/dom/domeventmask.h>
#include <Xemeiah/dom/documentmeta.h>
#include <Xemeiah/xprocessor/xprocessorlib.h>
#include <Xemeiah/xprocessor/xprocessorlibs.h>

#include <Xemeiah/auto-inline.hpp>

#include <list>

namespace Xem
{
  __XProcessorLib_DECLARE_LIB_INTERNAL ( XProcessorDefaultModules, "xprocessor-default-modules" );

  void XProcessorModuleForge::registerXPathAttribute ( Document& doc, KeyId keyId )
  {
    KeyCache& keyCache = doc.getKeyCache();
    doc.getDocumentMeta().getDomEvents().registerEvent(DomEventMask_Attribute,
        keyId, keyCache.getBuiltinKeys().xemint.domevent_xpath_attribute());
  }

  void XProcessorModuleForge::registerQNameAttribute ( Document& doc, KeyId keyId )
  {
    KeyCache& keyCache = doc.getKeyCache();
    doc.getDocumentMeta().getDomEvents().registerEvent(DomEventMask_Attribute,
        keyId, keyCache.getBuiltinKeys().xemint.domevent_qname_attribute());
  }

  void XProcessorModuleForge::registerQNameListAttribute ( Document& doc, KeyId keyId, KeyId targetId )
  {
    NotImplemented ( "registerQNameListAttribute()" );
  }

  void XProcessorModuleForge::registerNamespaceListAttribute ( Document& doc, KeyId keyId, KeyId targetId )
  {
    KeyCache& keyCache = doc.getKeyCache();
    DomEvent event = doc.getDocumentMeta().getDomEvents().registerEvent(DomEventMask_Attribute,
        keyId, keyCache.getBuiltinKeys().xemint.domevent_namespacelist_attribute());
    event.addAttrAsQName(keyCache.getBuiltinKeys().xemint.target(), targetId );
  }

  void XProcessorModuleForge::registerHandler ( KeyId keyId, XProcessorModule::XProcessorHandler handler, bool checkNamespace )
  {
    if ( checkNamespace && KeyCache::getNamespaceId(keyId) != getModuleNamespaceId() )
      {
        Bug ( "Diverging namespaces !\n" );
      }
    localHandlerMap[KeyCache::getLocalKeyId(keyId)] = handler;
  }

  void XProcessorModuleForge::registerFunction ( KeyId keyId, XProcessorModule::XProcessorFunction function )
  {
    if ( KeyCache::getNamespaceId(keyId) != getModuleNamespaceId() )
      {
        Bug ( "Diverging namespaces !\n" );
      }
    localFunctionMap[KeyCache::getLocalKeyId(keyId)] = function;
  }

  void XProcessorModuleForge::registerDomEventHandler ( KeyId keyId, XProcessorModule::DomEventHandler handler )
  {
    if ( KeyCache::getNamespaceId(keyId) != getModuleNamespaceId() )
      {
        Bug ( "Diverging namespaces !\n" );
      }
    localDomEventMap[KeyCache::getLocalKeyId(keyId)] = handler;
  }

  XProcessorModule::XProcessorHandler XProcessorModuleForge::getHandler ( LocalKeyId localKeyId )
  {
    return localHandlerMap[localKeyId];
  }

  XProcessorModule::XProcessorFunction XProcessorModuleForge::getFunction ( LocalKeyId localKeyId )
  {
    return localFunctionMap[localKeyId];
  }

  XProcessorModule::DomEventHandler XProcessorModuleForge::getDomEventHandler ( LocalKeyId localKeyId )
  {
    return localDomEventMap[localKeyId];
  }

  void XProcessorModuleForge::registerAsEventTriggerHandler ()
  {
    store.getXProcessorLibs().registerModuleAsEventTriggerHandler ( getModuleNamespaceId() );
  }

  void XProcessorModuleForge::dumpExtensions ()
  {
    Info ( "\tNamespace %s\n", getStore().getKeyCache().getNamespaceURL(getModuleNamespaceId()) );
    for ( XProcessorModule::LocalHandlerMap::iterator iter = localHandlerMap.begin() ;
        iter != localHandlerMap.end() ; iter++ )
      {
        Info ( "\t\tInstruction {%s}:%s\n",
            getStore().getKeyCache().getNamespaceURL(getModuleNamespaceId()),
            getStore().getKeyCache().getLocalKey(iter->first) );
      }
    for ( XProcessorModule::LocalFunctionMap::iterator iter = localFunctionMap.begin() ;
        iter != localFunctionMap.end() ; iter++ )
      {
        Info ( "\t\tFunction {%s}:%s\n",
            getStore().getKeyCache().getNamespaceURL(getModuleNamespaceId()),
            getStore().getKeyCache().getLocalKey(iter->first) );
      }
    for ( XProcessorModule::LocalDomEventMap::iterator iter = localDomEventMap.begin() ;
        iter != localDomEventMap.end() ; iter++ )
      {
        Info ( "\t\tDomEvent {%s}:%s\n",
            getStore().getKeyCache().getNamespaceURL(getModuleNamespaceId()),
            getStore().getKeyCache().getLocalKey(iter->first) );
      }
  }
};

