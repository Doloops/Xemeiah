#ifndef __XEM_WEBSERVER_MODULE_H
#define __XEM_WEBSERVER_MODULE_H

#include <Xemeiah/xprocessor/xprocessormoduleforge.h>
#include <Xemeiah/xprocessor/xprocessormodule.h>
#include <Xemeiah/xprocessor/xprocessor.h>
#include <Xemeiah/xemprocessor/xemprocessor.h>

#include <Xemeiah/webserver/webserver.h>

namespace Xem
{
  class WebServerModuleForge;

  class WebServerModule : public XProcessorModule
  {
    friend class WebServerModuleForge;
  protected:
    void webInstructionService ( __XProcHandlerArgs__ );
    void webInstructionPutParametersInEnv ( __XProcHandlerArgs__ );
    void webInstructionResponseFile ( __XProcHandlerArgs__ );
    void webInstructionAddResponseParam ( __XProcHandlerArgs__ );
    void webInstructionSetContentLength ( __XProcHandlerArgs__ );
    void webInstructionSetContentType ( __XProcHandlerArgs__ );
    void webInstructionSetResultCode ( __XProcHandlerArgs__ );
    void webInstructionListen ( __XProcHandlerArgs__ );
    void webInstructionSerializeQueryToBlob ( __XProcHandlerArgs__ );
    void webInstructionSerializeBlobToResponse ( __XProcHandlerArgs__ );

    String serializeQueryHeader ( ElementRef& queryDocument );

    void webInstructionSerializeQueryResponse ( __XProcHandlerArgs__ );

    void webInstructionRedirectQuery ( __XProcHandlerArgs__ );

    void webFunctionURLEncode ( __XProcFunctionArgs__ );
    void webFunctionProtectJS ( __XProcFunctionArgs__ );

    void webFunctionSendQuery ( __XProcFunctionArgs__ );
        
  public:
    __BUILTIN_NAMESPACE_CLASS(xem_web) &xem_web;
    __BUILTIN_NAMESPACE_CLASS(xem_role) &xem_role;

    WebServerModule ( XProcessor& xproc, WebServerModuleForge& moduleForge );
    ~WebServerModule ();
    
    void install ();
  };

  class WebServerModuleForge : public XProcessorModuleForge
  {
  protected:
  
  public:
    __BUILTIN_NAMESPACE_CLASS(xem_web) xem_web;
    __BUILTIN_NAMESPACE_CLASS(xem_role) xem_role;
    
    WebServerModuleForge ( Store& store ) : XProcessorModuleForge(store),
    xem_web(store.getKeyCache()), xem_role(store.getKeyCache())
    {
    }
    ~WebServerModuleForge () {}
    
    NamespaceId getModuleNamespaceId ( ) { return xem_web.ns(); }
    
    void install ();

    void instanciateModule ( XProcessor& xprocessor )
    {
      XProcessorModule* module = new WebServerModule ( xprocessor, *this ); 
      xprocessor.registerModule ( module );
    }

    /**
     * Register default DomEvents for this document
     */
    virtual void registerEvents ( Document& doc );
  };
};

#endif //  __XEM_WEBSERVER_MODULE_H

