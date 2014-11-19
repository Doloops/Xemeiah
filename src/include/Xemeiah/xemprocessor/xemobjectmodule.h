#if 0 // DEPRECATED

#ifndef __XEM_XEMOBJECTMODULE_H
#define __XEM_XEMOBJECTMODULE_H

#include <Xemeiah/xprocessor/xprocessormodule.h>
#include <Xemeiah/xprocessor/xprocessor.h>

#include <Xemeiah/kern/exception.h>

namespace Xem
{
  /**
   * XemObjectModule implements the call to Constructor when instanciating a Xem Object directly
   */
  class XemObjectModule : public XProcessorModule
  {
  protected:

  public:
    /**
     * Fake object
     */
    XemObjectModule ( XProcessor& xproc, XProcessorModuleForge& moduleForge );
    ~XemObjectModule ();

    void xemInstructionInstanciate ( __XProcHandlerArgs__ );
    void install();
    
  };
};

#endif // __XEM_XEMOBJECTMODULE_H

#endif // DEPRECATED
