#include <Xemeiah/xsl/xslfallbackmodule.h>
#include <Xemeiah/xsl/xslprocessor.h>
#include <Xemeiah/dom/childiterator.h>

#include <Xemeiah/auto-inline.hpp>

namespace Xem
{
  XSLFallbackModule::XSLFallbackModule ( XProcessor& xproc, XSLProcessor& xslProcessor )
  : XProcessorModuleForge ( xproc.getStore()), XProcessorModule ( xproc, *this ), xsl(xslProcessor.xsl)
  {}
  
  XSLFallbackModule::~XSLFallbackModule () {}

  NamespaceId XSLFallbackModule::getModuleNamespaceId()
  {
    Bug ( "Shall not be called !\n" );
    return 0;
  }

  void XSLFallbackModule::instanciateModule(Xem::XProcessor&)
  {
    Bug ( "Shall not be called !\n" );
  }

  void XSLFallbackModule::registerEvents(Xem::Document&)
  {
    Bug ( "Shall not be called !\n" );
  }


  void XSLFallbackModule::instructionFallback ( __XProcHandlerArgs__ )
  {
    for ( ChildIterator iter(item) ; iter ; iter++ )
      {
        if ( iter.getKeyId() == xsl.fallback() )
          {
            getXProcessor().process ( iter );
            return;
          }
      }
    throwException ( Exception, "No fallback !\n" );
  }

  void XSLFallbackModule::install() 
  {
    defaultNSHandler = (XProcessorHandler) (&XSLFallbackModule::instructionFallback);
    Debug ( "Installed default NS Handler this=%p !\n",
        this );
  }

};

