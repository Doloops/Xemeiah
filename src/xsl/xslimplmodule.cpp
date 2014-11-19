/*
 * xslimplmodule.cpp
 *
 *  Created on: 9 nov. 2009
 *      Author: francois
 */

#include <Xemeiah/xsl/xslimplmodule.h>
#include <Xemeiah/xsl/xslprocessor.h>
#include <Xemeiah/xpath/xpathparser.h>
#include <Xemeiah/xprocessor/xprocessorlib.h>
#include <Xemeiah/dom/documentmeta.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_XSLI Debug

namespace Xem
{
  __XProcessorLib_REGISTER_MODULE ( XSL, XSLImplModuleForge );

  XSLImplModuleForge::XSLImplModuleForge ( Store& store )
  : XProcessorModuleForge(store), xslimpl(store.getKeyCache())
  {

  }

  XSLImplModuleForge::~XSLImplModuleForge ()
  {

  }


  NamespaceId XSLImplModuleForge::getModuleNamespaceId ()
  {
    return xslimpl.ns();
  }

  XSLImplModule::XSLImplModule ( XProcessor& xproc, XSLImplModuleForge& moduleForge )
  : XProcessorModule ( xproc, moduleForge ), xslimpl(moduleForge.xslimpl)
  {

  }

  void XSLImplModule::domEventStylesheet ( DomEventType domEventType, ElementRef& domEventElement, NodeRef& nodeRef )
  {
    try
      {
        XSLProcessor& xslProc = XSLProcessor::getMe(getXProcessor());
        xslProc.prepareStylesheet ( nodeRef.toElement() );
        Log_XSLI ( "XSL : End of Prepare !\n" );
      }
    catch ( Exception* e )
      {
        Error ( "Could not prepare stylesheet '%s' : error = %s\n",
            nodeRef.generateVersatileXPath().c_str(),
            e->getMessage().c_str() );
        detailException(e,"While trying to prepare stylesheet : %s\n", nodeRef.generateVersatileXPath().c_str() );
        throw ( e );
      }
  }

  void XSLImplModule::install ( )
  {

  }

  void XSLImplModuleForge::install ( )
  {
#undef __builtin
#define __builtin getStore().getKeyCache().getBuiltinKeys()
    registerDomEventHandler(xslimpl.domevent_prepare_stylesheet(), &XSLImplModule::domEventStylesheet );

  }
};
