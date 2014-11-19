#ifndef __XEM_XEMSERVICEMODULE_H
#define __XEM_XEMSERVICEMODULE_H

#include <Xemeiah/xprocessor/xprocessormoduleforge.h>
#include <Xemeiah/xprocessor/xprocessormodule.h>
#include <Xemeiah/xprocessor/xprocessor.h>
#include <Xemeiah/dom/string.h>

#include <Xemeiah/xemprocessor/xemservice.h>
#include <Xemeiah/kern/servicemanager.h>

/**
 * Service Module, able to run multiple services
 */

#include <map>

namespace Xem
{
#include <Xemeiah/kern/builtin_keys_prolog.h>
#include <Xemeiah/xemprocessor/builtin-keys/xem_service>
#include <Xemeiah/kern/builtin_keys_postlog.h>

  class XemServiceModule;

  class XemServiceModuleForge : public XProcessorModuleForge
  {
    friend class XemServiceModule;
  protected:
  public:
    __BUILTIN_NAMESPACE_CLASS(xem_service) xem_service;
  
    XemServiceModuleForge ( Store& store );
    ~XemServiceModuleForge ();
    
    NamespaceId getModuleNamespaceId ( ) { return xem_service.ns(); }
    
    void install ();

    void instanciateModule ( XProcessor& xprocessor );

    /**
     * Register default DomEvents for this document
     */
    void registerEvents ( Document& doc ) {}
  };


  class XemServiceModule : public XProcessorModule
  {
    friend class XemServiceModuleForge;
  protected:
    void instructionRegisterService ( __XProcHandlerArgs__ );
    void instructionUnregisterService ( __XProcHandlerArgs__ );

    void instructionStartService ( __XProcHandlerArgs__ );
    void instructionStopService ( __XProcHandlerArgs__ );
    void instructionRestartService ( __XProcHandlerArgs__ );
    
    void instructionStopServiceManager ( __XProcHandlerArgs__ );
    void xemFunctionGetServices ( __XProcFunctionArgs__ );    
    void xemFunctionGetService ( __XProcFunctionArgs__ );

    void instructionService ( __XProcHandlerArgs__ );

    void instructionStartThread ( __XProcHandlerArgs__ );

  public:
    __BUILTIN_NAMESPACE_CLASS(xem_service) &xem_service;

    XemServiceModule ( XProcessor& xproc, XemServiceModuleForge& moduleForge ) 
    : XProcessorModule ( xproc, moduleForge ), xem_service(moduleForge.xem_service)
    {}
    
    ~XemServiceModule () {}

    void install ();
    
    static XemServiceModule& getMe ( XProcessor& xproc );
    
    XemServiceModuleForge& getModuleForge () const { return getTypedModuleForge<XemServiceModuleForge>(); }
  };

};

#endif //  __XEM_XEMSERVICEMODULE_H

