/*
 * xemintmodule.h
 *
 *  Created on: 8 janv. 2010
 *      Author: francois
 */

#ifndef __XEM_XPROCESSOR_XEMINTMODULE_H_
#define __XEM_XPROCESSOR_XEMINTMODULE_H_

#include <Xemeiah/xprocessor/xprocessormoduleforge.h>
#include <Xemeiah/xprocessor/xprocessormodule.h>
#include <Xemeiah/xprocessor/xprocessor.h>

namespace Xem
{
  class Document;

  /**
   * Simple PI Module handler for XProcessor
   */
  class XemIntModule : public XProcessorModule
  {
    friend class XemIntModuleForge;
  protected:
    void domEventXPath ( DomEventType domEventType, ElementRef& domEventElement, NodeRef& nodeRef );
    void domEventQName ( DomEventType domEventType, ElementRef& domEventElement, NodeRef& nodeRef );
    void domEventQNameList ( DomEventType domEventType, ElementRef& domEventElement, NodeRef& nodeRef );
    void domEventNamespaceList ( DomEventType domEventType, ElementRef& domEventElement, NodeRef& nodeRef );
  public:
    XemIntModule ( XProcessor& xproc, XProcessorModuleForge& moduleForge ) : XProcessorModule ( xproc, moduleForge ) {}
    ~XemIntModule () {}

    void install () {}
  };

  /**
   * Simple PI Module Forge handler for XProcessor
   */
  class XemIntModuleForge : public XProcessorModuleForge
  {
  protected:

  public:
    XemIntModuleForge ( Store& store ) : XProcessorModuleForge ( store ) {}
    ~XemIntModuleForge () {}

    NamespaceId getModuleNamespaceId ( )
    {
      return store.getKeyCache().getBuiltinKeys().xemint.ns();
    }

    void install ();

    void instanciateModule ( XProcessor& xprocessor )
    {
      XProcessorModule* module = new XemIntModule ( xprocessor, *this );
      xprocessor.registerModule ( module );
    }

    void registerEvents ( Document& doc )
    {

    }
  };
};


#endif /* __XEM_XPROCESSOR_XEMINTMODULE_H_ */
