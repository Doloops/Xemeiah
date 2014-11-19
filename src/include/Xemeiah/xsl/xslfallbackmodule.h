#ifndef __XEM_XSL_FALLBACK_MODULE_H
#define __XEM_XSL_FALLBACK_MODULE_H

#include <Xemeiah/xprocessor/xprocessormoduleforge.h>
#include <Xemeiah/xprocessor/xprocessormodule.h>
#include <Xemeiah/xprocessor/xprocessor.h>
#include <Xemeiah/xsl/xslprocessor.h>

#include <Xemeiah/kern/exception.h>

namespace Xem
{
  class XSLProcessor;

  /**
   * XemObjectModule implements the call to Constructor when instanciating a Xem Object directly
   */
  class XSLFallbackModule : public XProcessorModuleForge, public XProcessorModule
  {
  protected:
    void instructionFallback ( __XProcHandlerArgs__ );

  public:
    __BUILTIN_NAMESPACE_CLASS(xsl) &xsl;

    /**
     * Fake object
     */
    XSLFallbackModule ( XProcessor& xproc, XSLProcessor& xslProcessor );
    ~XSLFallbackModule ();

    void install();
    
    NamespaceId getModuleNamespaceId();
    void instanciateModule(Xem::XProcessor&);
    void registerEvents(Xem::Document&);
  };
};

#endif // __XSL_FALLBACK_MODULE_H

