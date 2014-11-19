#if 0 // DEPRECATED

#include <Xemeiah/xemprocessor/xemobjectmodule.h>
#include <Xemeiah/xemprocessor/xemprocessor.h>

#include <Xemeiah/auto-inline.hpp>

namespace Xem
{
  XemObjectModule::XemObjectModule ( XProcessor& xproc, XProcessorModuleForge& moduleForge ) : XProcessorModule ( xproc, moduleForge )
  {
  }
  
  XemObjectModule::~XemObjectModule ()
  {
  
  }


  void XemObjectModule::install() 
  {}  

  void XemObjectModule::xemInstructionInstanciate ( __XProcHandlerArgs__ )
  {
    XemProcessor& xemProc = XemProcessor::getMe ( getXProcessor() );
    xemProc.xemInstructionInstance ( item );
  }

};
#endif // DEPRECATED
