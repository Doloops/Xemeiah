#include <Xemeiah/xemprocessor/xemservice.h>
#include <Xemeiah/xemprocessor/xemservicemodule.h>
#include <Xemeiah/xemprocessor/xemprocessor.h>
#include <Xemeiah/nodeflow/nodeflow-dom.h>
#include <Xemeiah/xprocessor/xprocessorlib.h>
#include <Xemeiah/dom/childiterator.h>
#include <Xemeiah/kern/volatiledocument.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_XemServiceModule Debug

namespace Xem
{

  __XProcessorLib_REGISTER_MODULE ( XemProcessor, XemServiceModuleForge );

#include <Xemeiah/kern/builtin_keys_prolog_inst.h>
#include <Xemeiah/xemprocessor/builtin-keys/xem_service>
#include <Xemeiah/kern/builtin_keys_postlog.h>

  XemServiceModuleForge::XemServiceModuleForge ( Store& store ) 
  : XProcessorModuleForge ( store ), xem_service(store.getKeyCache())
  {
    // finishService = false;
  }

  XemServiceModuleForge::~XemServiceModuleForge ()
  {
  
  
  }

  void XemServiceModuleForge::instanciateModule ( XProcessor& xprocessor )
  {
    XProcessorModule* module = new XemServiceModule ( xprocessor, *this ); 
    xprocessor.registerModule ( module );
  }

  void XemServiceModule::instructionRegisterService ( __XProcHandlerArgs__ )
  {
    if ( ! item.hasAttr(xem_service.name()))
      {
        throwException ( Exception, "Service has no xem-service:name defined : %s\n",
            item.generateVersatileXPath().c_str() );
      }
    String serviceName = item.getEvaledAttr ( getXProcessor(), xem_service.name() );
    Log_XemServiceModule ( "Registering Xem Service : %s\n", serviceName.c_str() );

    if ( serviceName.isSpace() )
      throwException ( Exception, "Invalid name for a service : '%s' (at %s)!\n",
          serviceName.c_str(),
          item.generateVersatileXPath().c_str() );

    Service* service = getServiceManager().getService ( serviceName );
    if ( service )
      throwException ( Exception, "Service '%s' already registered !\n", serviceName.c_str() );

    XemProcessor& xemProcessor = XemProcessor::getMe(getXProcessor());
    KeyId classId = item.getAttrAsKeyId(xemProcessor.xem.class_());

    if ( xemProcessor.hasCurrentCodeScope() && xemProcessor.getObjectConstructor(classId) )
      {
        Log_XemServiceModule ( "Register service : %s (%x) is a Constructor function\n",
            getKeyCache().dumpKey(classId).c_str(), classId );

        ElementRef serviceItem = item;
        XemService* service = new XemService(getXProcessor(), serviceItem);
        service->registerMyself(getXProcessor());
      }
    else
      {
        Log_XemServiceModule ( "Register service : %s (%x) is a builtin function\n",
            getKeyCache().dumpKey(classId).c_str(), classId );
      }
    getXProcessor().processElement(item,classId);
  }

  void XemServiceModule::instructionUnregisterService ( __XProcHandlerArgs__ )
  {
    if ( ! item.hasAttr(xem_service.name()))
      {
        throwException ( Exception, "Service has no xem-service:name defined : %s\n",
            item.generateVersatileXPath().c_str() );
      }
    String serviceName = item.getEvaledAttr ( getXProcessor(), xem_service.name() );

    if ( serviceName.isSpace() )
      throwException ( Exception, "Invalid name for a service : '%s' (at %s)!\n",
          serviceName.c_str(),
          item.generateVersatileXPath().c_str() );

    Log_XemServiceModule ( "Unregistering Xem Service : %s\n", serviceName.c_str() );

    getServiceManager().unregisterService(serviceName);
  }

  void XemServiceModule::instructionStartService ( __XProcHandlerArgs__ )
  {
    String serviceName = item.getEvaledAttr ( getXProcessor(), xem_service.name() );
    Log_XemServiceModule ( "Starting Xem Service : %s\n", serviceName.c_str() );

    Service* service = getServiceManager().getService ( serviceName );
    if ( ! service ) throwException ( Exception, "Could not get service '%s'\n", serviceName.c_str() );
    service->startService ();
  }

  void XemServiceModule::instructionStopService ( __XProcHandlerArgs__ )
  {
    String serviceName = item.getEvaledAttr ( getXProcessor(), xem_service.name() );
    Log_XemServiceModule ( "Stopping Xem Service : %s\n", serviceName.c_str() );
    __ui64 timeout = item.getEvaledAttr ( getXProcessor(), xem_service.timeout() ).toUI64();

    Service* service = getServiceManager().getService ( serviceName );
    if ( ! service ) throwException ( Exception, "Could not get service '%s'\n", serviceName.c_str() );
    service->stopService ();
    if ( timeout )
      {
        __ui64 iterations = 0;
        while ( ! service->isStopped() )
          {
            if ( iterations == timeout )
              {
                throwException ( Exception, "Timeout while waiting for service '%s' to stop\n", serviceName.c_str() );
              }
            Log_XemServiceModule ( "Waiting for service '%s' to stop\n", serviceName.c_str() );
            sleep ( 1 );
            iterations++;
          }
        Log_XemServiceModule ( "Successfully waited for service '%s' to stop.\n", serviceName.c_str() );
      }
  }

  void XemServiceModule::instructionRestartService ( __XProcHandlerArgs__ )
  {
    String serviceName = item.getEvaledAttr ( getXProcessor(), xem_service.name() );
    Log_XemServiceModule ( "Starting Xem Service : %s\n", serviceName.c_str() );

    Service* service = getServiceManager().getService ( serviceName );
    if ( ! service ) throwException ( Exception, "Could not get service '%s'\n", serviceName.c_str() );
    service->restartService ();
  }

  void XemServiceModule::instructionStopServiceManager ( __XProcHandlerArgs__ )
  {
    Log_XemServiceModule ( "Calling Stop Service Manager ...\n" );
    getServiceManager().stopServiceManager ( );
    Log_XemServiceModule ( "Calling Stop Service Manager ... OK\n" );
  }

  void XemServiceModule::xemFunctionGetService ( __XProcFunctionArgs__ )
  {
    if ( ! args.size() == 1 )
      {
        throwException ( Exception, "Invalid number of arguments for xem-service:get-service()" );
      }
    String serviceName = args[0]->toString();
    XemService* service = dynamic_cast<XemService*> ( getServiceManager().getService(serviceName) );
    if ( ! service )
      {
        throwException ( Exception, "Invalid Service or not a XemService : '%s'", serviceName.c_str() );
      }
    ElementRef serviceElement = service->getConfigurationElement();
#if 0
    Document& serviceDocument = serviceElement.getDocument();
    getXProcessor().bindDocument(serviceDocument, true);
#endif
    result.pushBack(serviceElement);
  }

  void XemServiceModule::xemFunctionGetServices ( __XProcFunctionArgs__ )
  {
    ElementRef rootElement = getXProcessor().createVolatileDocument(true);

    NodeFlowDom nodeFlow(getXProcessor(), rootElement);

    nodeFlow.newElement ( xem_service.services() );

    for ( ServiceManager::ServiceMap::iterator iter = getServiceManager().serviceMap.begin () ;
        iter != getServiceManager().serviceMap.end () ; iter++ )
      {
        Service* service = iter->second;

        nodeFlow.newElement ( xem_service.service() );
        nodeFlow.newAttribute ( xem_service.name(), iter->first );
        nodeFlow.newAttribute ( xem_service.state(), service->getState() );
        
        String id; stringPrintf ( id, "%p", service );
        nodeFlow.newAttribute ( xem_service.id(), id );        

        String threads; stringPrintf ( threads, "%lu", (unsigned long) service->getRunningThreads() );
        Log_XemServiceModule ( "[THREADNB] %p has %s running threads.\n", service, threads.c_str() );
        nodeFlow.newAttribute ( xem_service.threads(), threads );        

        nodeFlow.elementEnd ( xem_service.service() );
        
      }    
    nodeFlow.elementEnd ( xem_service.services() );

    result.pushBack ( rootElement );
  }

  void XemServiceModule::instructionStartThread ( __XProcHandlerArgs__ )
  {
    String serviceName;
    bool genericService = false;

    if ( item.hasAttr(xem_service.name()) )
      {
        serviceName = item.getEvaledAttr ( getXProcessor(), xem_service.name() );
      }
    else
      {
        serviceName = "Generic Asynchronous Method Service";
        genericService = true;
      }
    Service* service = getServiceManager().getService ( serviceName );
    if ( ! service )
      {
        if ( genericService )
          {
            service = new XemServiceGeneric(getXProcessor(), item.getRootElement());
            getServiceManager().registerService(serviceName, service);
            service->startService();
          }
        else
          throwException ( Exception, "Could not get service '%s'\n", serviceName.c_str() );
      }

    XemService* xemService = dynamic_cast<XemService*> ( service );
    if ( ! xemService ) throwException ( Exception, "Service '%s' is not a XemService !\n", serviceName.c_str() );

    XemProcessor& xemProc = XemProcessor::getMe ( getXProcessor() );
    __BUILTIN_NAMESPACE_CLASS(xem)& xem = xemProc.xem;
    KeyId keyId = item.getAttrAsKeyId(xemProc.xem.method());

    Log_XemServiceModule ( "------------------ startMethodThread (xproc=%p) ----------------------\n", &getXProcessor() );

    XemService::StartMethodThreadArgumentsMap* argsMap = new XemService::StartMethodThreadArgumentsMap();

    for ( ChildIterator child(item) ; child ; child++ )
      {
        if ( child.getKeyId() != xem.with_param() ) continue;
        KeyId keyId = child.getAttrAsKeyId(xem.name());

        String value;
        if ( child.hasAttr(xem.select()))
          {
            XPath xpath(getXProcessor(),child,xem.select());
            value = xpath.evalString();
          }
        else
          {
            NotImplemented ( "Not implemented.\n" );
          }
        String val = stringFromAllocedStr(strdup(value.c_str()));
        argsMap->insert(std::pair<KeyId,String>(keyId,val));

        Log_XemServiceModule ( "[METHODTHREAD] arg %s (%x) = %s\n",
            getKeyCache().dumpKey(keyId).c_str(), keyId, val.c_str() );

        // (*argsMap)[keyId] = val;
      }
    Log_XemServiceModule ( "[METHODTHREAD] argsMap size %lu\n", (unsigned long) argsMap->size() );


    if ( genericService )
      {
        ElementRef this_ =  getXProcessor().getVariable(getKeyCache().getBuiltinKeys().nons.this_())->toElement();
        xemService->startMethodThread ( this_, KeyCache::getLocalKeyId(keyId), argsMap );
      }
    else
      {
        ElementRef this_ = xemService->getConfigurationElement();
        xemService->startMethodThread ( this_, KeyCache::getLocalKeyId(keyId), argsMap );
      }
    Log_XemServiceModule ( "------------------ startMethodThread : End (xproc=%p) ----------------------\n", &getXProcessor() );
  }

  XemServiceModule& XemServiceModule::getMe ( XProcessor& xproc )
  {
    return dynamic_cast<XemServiceModule&> ( *xproc.getModule ( "http://www.xemeiah.org/ns/xem/service", true ) );
  }

  void XemServiceModule::install ()
  {
  }

  void XemServiceModuleForge::install ()
  {
    registerHandler ( xem_service.register_service(), &XemServiceModule::instructionRegisterService );
    registerHandler ( xem_service.unregister_service(), &XemServiceModule::instructionUnregisterService );
    registerHandler ( xem_service.start_service(), &XemServiceModule::instructionStartService );
    registerHandler ( xem_service.stop_service(), &XemServiceModule::instructionStopService );
    registerHandler ( xem_service.restart_service(), &XemServiceModule::instructionRestartService );
    registerHandler ( xem_service.stop_service_manager(), &XemServiceModule::instructionStopServiceManager );

    registerHandler ( xem_service.start_thread(), &XemServiceModule::instructionStartThread );

    registerFunction ( xem_service.get_services(), &XemServiceModule::xemFunctionGetServices );
    registerFunction ( xem_service.get_service(), &XemServiceModule::xemFunctionGetService );
  }

  
  
};

