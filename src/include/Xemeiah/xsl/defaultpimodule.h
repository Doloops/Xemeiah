#ifndef __XEM_XPROCESSOR_DEFAULTPIMODULE_H
#define __XEM_XPROCESSOR_DEFAULTPIMODULE_H

#include <Xemeiah/xprocessor/xprocessormoduleforge.h>
#include <Xemeiah/xprocessor/xprocessormodule.h>
#include <Xemeiah/xprocessor/xprocessor.h>

namespace Xem
{
  class Document;

  /**
   * Default PI Module handler for XProcessor
   */
  class DefaultPIModule : public XProcessorModule
  {
  protected:
    /**
     * Default handler : do nothing with Processing Instructions
     */
    void instructionDefault ( __XProcHandlerArgs__ ) {}

  public:
    /**
     * Default PI Module constructor
     */
    DefaultPIModule ( XProcessor& xproc, XProcessorModuleForge& moduleForge ) : XProcessorModule ( xproc, moduleForge ) {}
    ~DefaultPIModule () {}
    
    void install ()
    {
      defaultNSHandler = (XProcessorHandler) (&DefaultPIModule::instructionDefault);      
    }
  
  };

  /**
   * Default PI Module handler for XProcessor :
   */
  class DefaultPIModuleForge : public XProcessorModuleForge
  {
  protected:
  
  public:
    DefaultPIModuleForge ( Store& store ) : XProcessorModuleForge ( store ) {}
    ~DefaultPIModuleForge () {}
    
    NamespaceId getModuleNamespaceId ( )
    {
      return store.getKeyCache().getBuiltinKeys().xemint_pi.ns();
    }
    
    void install () {}

    void instanciateModule ( XProcessor& xprocessor )
    {
      XProcessorModule* module = new DefaultPIModule ( xprocessor, *this ); 
      xprocessor.registerModule ( module );
    }

    void registerEvents ( Document& doc )
    {

    }
  };
};

#endif //  __XEM_XPROCESSOR_DEFAULTPIMODULE_H

