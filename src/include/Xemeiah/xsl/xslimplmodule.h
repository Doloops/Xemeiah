/*
 * xslimplmodule.h
 *
 *  Created on: 9 nov. 2009
 *      Author: francois
 */

#ifndef __XEM_XSL_XSLIMPLMODULE_H_
#define __XEM_XSL_XSLIMPLMODULE_H_

#include <Xemeiah/xprocessor/xprocessormodule.h>
#include <Xemeiah/xprocessor/xprocessormoduleforge.h>
#include <Xemeiah/xsl/xslmoduleforge.h>

#include <Xemeiah/kern/exception.h>

namespace Xem
{
  class XSLImplModuleForge;

  /**
   * XSLImplModule is the module responsible for low-level XSL implementation (basically Dom Events)
   */
  class XSLImplModule : public XProcessorModule
  {
    friend class XSLImplModuleForge;
  protected:
    void domEventStylesheet ( DomEventType domEventType, ElementRef& domEventElement, NodeRef& nodeRef );

  public:
    __BUILTIN_NAMESPACE_CLASS(xslimpl) &xslimpl;

    XSLImplModule ( XProcessor& xproc, XSLImplModuleForge& moduleForge );
    ~XSLImplModule () {}

    void install ();

  };

  /**
   * Forge for the xslimpl namespace
   */
  class XSLImplModuleForge : public XProcessorModuleForge
  {
  protected:

  public:

    __BUILTIN_NAMESPACE_CLASS(xslimpl) xslimpl;

    XSLImplModuleForge ( Store& store );
    ~XSLImplModuleForge ();

    NamespaceId getModuleNamespaceId ();

    void install ();

    void instanciateModule ( XProcessor& xprocessor )
    {
      XProcessorModule* module = new XSLImplModule ( xprocessor, *this );
      xprocessor.registerModule ( module );
    }

    void registerEvents ( Document& doc )
    {

    }
  };
};

#endif /* __XEM_XSL_XSLIMPLMODULE_H_ */
