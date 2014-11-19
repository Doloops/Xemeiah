#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/dom/childiterator.h>
#include <Xemeiah/xprocessor/xprocessor.h>
#include <Xemeiah/xprocessor/xprocessormodule.h>
#include <Xemeiah/xprocessor/xprocessormoduleforge.h>
#include <Xemeiah/xprocessor/xprocessorlibs.h>
#include <Xemeiah/xpath/xpath.h>

#define __XEM_XPROC_CATCH_EXCEPTION

#define Log_XProcessorHPP Debug
#define Log_XProcessor_Step Debug
#define Log_AutoInstallModule Debug

// #define __XEM_XPROCESSOR_EXPLICIT_NAMESPACE_ALIASING      
#define __XEM_XPROCESSOR_SHOW_PROCESSING_TIME 1

#if __XEM_XPROCESSOR_SHOW_PROCESSING_TIME == 2
#define __XPROC_BEGIN() \
  NTime __startProcessTime = getntime (); \
  String versatileXPath = ""; /* actionItem.generateVersatileXPath(); */\
  if (1) fprintf ( stderr, "[XPROCTIME] E%012lu,L%04lu,I%06llu|%s\t%s\n", \
    (unsigned long int)0, getCurrentEnvId(), actionItem.getElementId(), \
    actionItem.getKey().c_str(), getCurrentNode().getKey().c_str() );
#define __XPROC_END()\
  do { \
  NTime __endProcessTime = getntime (); \
  unsigned long timespent = diffntime ( &(__startProcessTime.tp_cpu), &(__endProcessTime.tp_cpu)); \
  if (timespent) fprintf ( stderr, "[XPROCTIME] T%012lu,L%04lu,I%06llu|%s\t%s\n", \
    timespent, getCurrentEnvId(), actionItem.getElementId(), \
    actionItem.getKey().c_str(), getCurrentNode().getKey().c_str() ); \
  } while ( 0 ) ;
#elif __XEM_XPROCESSOR_SHOW_PROCESSING_TIME == 1
#define __XPROC_BEGIN() \
    Log_XProcessor_Step ( "XPROC BEGIN : Level=%lu, Action=(%llx,'%s'), CurrentNode=(%llx,'%s')\n", \
      currentLevel, actionItem.getElementId(), actionItem.getKey().c_str(), \
      getCurrentNode().isElement() ? getCurrentNode().toElement().getElementId() : 0, getCurrentNode().getKey().c_str() );
#define __XPROC_END() \
    Log_XProcessor_Step ( "XPROC END   : Level=%lu, Action=(%llx,'%s'), CurrentNode=(%llx,'%s')\n", \
      currentLevel, actionItem.getElementId(), actionItem.getKey().c_str(), \
      getCurrentNode().isElement() ? getCurrentNode().toElement().getElementId() : 0, getCurrentNode().getKey().c_str() );
#elif __XEM_XPROCESSOR_SHOW_PROCESSING_TIME == 0
#define __XPROC_BEGIN()
#define __XPROC_END()
#endif

namespace Xem
{
  __INLINE bool XProcessor::hasNodeFlow()
  {
    return ( currentNodeFlow != NULL);
  }

  __INLINE NodeFlow& XProcessor::getNodeFlow()
  {
#if 0
    if ( ! currentNodeFlow ) throwException ( Exception, "No NodeFlow defined !\n" );
#else
    if ( ! currentNodeFlow ) Bug ( "No NodeFlow defined !\n" );
#endif
    return *currentNodeFlow;
  }

  __INLINE XProcessorModule* XProcessor::getModule ( NamespaceId moduleNSId, bool mayAutoInstall )
  {
    ModuleMap::iterator iter = moduleMap.find ( moduleNSId );
    if ( iter != moduleMap.end() )
      return iter->second;
    if ( !mayAutoInstall || !settings.autoInstallModules )
      return NULL;

    AssertBug ( moduleNSId, "Zero moduleNSId provided !\n" );
    Log_AutoInstallModule ( "Auto-installing module '%s' (%x)\n",
        getKeyCache().getNamespaceURL(moduleNSId), moduleNSId );

    XProcessorModuleForge* moduleForge = store.getXProcessorLibs().getModuleForge ( moduleNSId );
    if ( ! moduleForge )
      {
        Log_AutoInstallModule ( "Could not find module that namespace : '%s' (%x)\n",
            getKeyCache().getNamespaceURL(moduleNSId), moduleNSId );
        return NULL;
      }
    moduleForge->instanciateModule ( *this );

    iter = moduleMap.find ( moduleNSId );
    if ( iter == moduleMap.end() )
      {
        Log_AutoInstallModule ( "Could not find module for that namespace : '%s' (%x)\n",
            getKeyCache().getNamespaceURL(moduleNSId), moduleNSId );
        return NULL;
      }
    Log_AutoInstallModule ( "Auto-installed module '%s' (%x)\n",
        getKeyCache().getNamespaceURL(moduleNSId), moduleNSId );
    return iter->second;
  }

  __INLINE XProcessor::XProcessorHandler XProcessor::getXProcessorHandler ( KeyId keyId )
  {
    if ( ! KeyCache::getNamespaceId ( keyId ) )
      return defaultHandler;
    XProcessorHandler handler;
    handler.module = getModule ( KeyCache::getNamespaceId ( keyId ), true );
    Log_XProcessorHPP ( "Found module at %p\n", handler.module );
    if ( ! handler.module ) return handler;
    handler.hook = handler.module->getHandler ( KeyCache::getLocalKeyId ( keyId ) );
    if ( ! handler.hook ) return handler;
    return handler;
  }

  __INLINE XProcessor::XProcessorFunction XProcessor::getXProcessorFunction ( KeyId keyId )
  {
    if ( ! KeyCache::getNamespaceId ( keyId ) )
      return defaultFunction;
    XProcessorFunction function;
    function.module = getModule ( KeyCache::getNamespaceId ( keyId ), true );
    if ( ! function.module ) return defaultFunction;
    function.hook = function.module->getFunction ( KeyCache::getLocalKeyId ( keyId ) );
    if ( ! function.hook ) return defaultFunction;
    return function;
  }

  __INLINE XProcessor::DomEventHandler XProcessor::getDomEventHandler ( KeyId keyId )
  {
    DomEventHandler handler;
    handler.module = getModule ( KeyCache::getNamespaceId ( keyId ), true );
    if ( ! handler.module ) return handler;
    handler.hook = handler.module->getDomEventHandler ( KeyCache::getLocalKeyId ( keyId ) );
    return handler;
  }

  __INLINE void XProcessor::process ( ElementRef& actionItem )
  {
#if PARANOID  
    AssertBug ( actionItem, "Been given an empty actionItem !\n" );
#endif

    KeyId keyId = actionItem.getKeyId ();

    if ( keyId == getKeyCache().getBuiltinKeys().xemint.textnode() )
      {
        processTextNode(actionItem);
        return;
      }

    if ( keyId == getKeyCache().getBuiltinKeys().xemint.comment() ) return;

#if 0
    if ( keyId == getKeyCache().getBuiltinKeys().xsl.param() ) return;
    if ( keyId == getKeyCache().getBuiltinKeys().xsl.variable() )
      {
        processInstructionVariable ( actionItem, 
            getKeyCache().getBuiltinKeys().xsl.name(),
            getKeyCache().getBuiltinKeys().xsl.select() );
        return;
      }
#endif

    XProcessorHandler handler = getXProcessorHandler ( actionItem.getKeyId() );

    if ( ! handler )
      {
        Log_XProcessorHPP ( "Setting defaultHandler !\n" );
        handler = defaultHandler;
      }
    if ( ! handler )
      {
        throwXProcessorException("No hook for %s\n", getKeyCache().dumpKey(actionItem.getKeyId()).c_str());
      }

    if ( getCurrentEnvId() >= ( maxLevel - 2 ) )
      {
        throwXProcessorException ( "Maximum level reached : level=%lu max=%lu\n",
                       getCurrentEnvId(), maxLevel );
      }

    pushEnv ();

    Env::EnvId currentLevel = getCurrentEnvId(); (void) currentLevel;

    __XPROC_BEGIN();

#ifdef __XEM_XPROCESSOR_EXPLICIT_NAMESPACE_ALIASING
    std::list<KeyId> namespaceAliases;
    NSKeyId xmlnsId = actionItem.getKeyCache().getNSKeyId(actionItem.getKeyCache().getBuiltinKeys().xmlns_xml());
    for ( AttributeRef attrRef = actionItem.getFirstAttr () ; attrRef ; attrRef = attrRef.getNext() )
      {
        if ( actionItem.getKeyCache().getNSKeyId(attrRef.getKeyId()) != xmlnsId )
            continue;
        actionItem.getKeyCache().setNamespaceAlias ( attrRef.getKeyId(), attrRef.toString().c_str() );
        namespaceAliases.push_back ( attrRef.getKeyId() );
      }
#endif // __XEM_XPROCESSOR_EXPLICIT_NAMESPACE_ALIASING
    try 
      {
        (handler.module->*handler.hook) ( actionItem );
      }
    catch ( Exception * xpe )
      {
        AssertBug ( currentLevel == getCurrentEnvId(), "Wrong current level !\n" );
#ifdef __XEM_XPROCESSOR_EXPLICIT_NAMESPACE_ALIASING
        for ( std::list<KeyId>::iterator iter = namespaceAliases.begin () ; 
            iter != namespaceAliases.end () ; iter++ )
            actionItem.getKeyCache().removeNamespaceAlias ( *iter );
#endif // __XEM_XPROCESSOR_EXPLICIT_NAMESPACE_ALIASING
        handleException ( xpe, actionItem, false );
      }

#ifdef __XEM_XPROCESSOR_EXPLICIT_NAMESPACE_ALIASING
    for ( std::list<KeyId>::iterator iter = namespaceAliases.begin () ; 
        iter != namespaceAliases.end () ; iter++ )
        actionItem.getKeyCache().removeNamespaceAlias ( *iter );
#endif // __XEM_XPROCESSOR_EXPLICIT_NAMESPACE_ALIASING
    
#if PARANOID
    AssertBug ( getCurrentEnvId() == currentLevel,
        "Level does not match current one : "
       "env.envId=%lu, current=%lu. "
       "At item : %llx %x:'%s'\n",
       getCurrentEnvId(), currentLevel,
       actionItem.getElementId(),
       actionItem.getKeyId(),
       actionItem.getKey().c_str() );
#endif
       
    __XPROC_END();
    popEnv ();
  }

  __INLINE void XProcessor::processChildren ( ElementRef& actionItem )
  {
    AssertBug ( actionItem, "Null actionItem provided !\n" );
    pushEnv();
    try
      {
        for ( ChildIterator child(actionItem) ; child ; child++ )
          {
            process ( child );
          }
      }
    catch ( Exception * xpe )
      {
        handleException ( xpe, actionItem, true ); /* , currentNode */
      }
    popEnv();
  }
 
};
