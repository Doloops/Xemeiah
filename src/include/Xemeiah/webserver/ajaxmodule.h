/*
 * ajaxmodule.h
 *
 *  Created on: 14 nov. 2009
 *      Author: francois
 */

#ifndef __XEM_WEBSERVER_AJAXMODULE_H
#define __XEM_WEBSERVER_AJAXMODULE_H

#include <Xemeiah/xprocessor/xprocessormoduleforge.h>
#include <Xemeiah/xprocessor/xprocessormodule.h>
#include <Xemeiah/xprocessor/xprocessor.h>
#include <Xemeiah/xemprocessor/xemprocessor.h>

#include <Xemeiah/webserver/webserver.h>

namespace Xem
{
  class AjaxModuleForge;

  class AjaxModule : public XProcessorModule
  {
    friend class AjaxModuleForge;
  protected:
    void ajaxUpdates ( __XProcHandlerArgs__ );
    void ajaxUpdate ( __XProcHandlerArgs__ );
    void ajaxScript ( __XProcHandlerArgs__ );

    void writeProtectedJS ( NodeFlow& nodeFlow, const String& value );

  public:
    __BUILTIN_NAMESPACE_CLASS(xem_ajax) &xem_ajax;

    AjaxModule ( XProcessor& xproc, AjaxModuleForge& moduleForge );
    ~AjaxModule ();

    void install ();
  };

  class AjaxModuleForge : public XProcessorModuleForge
  {
  protected:
  public:
    __BUILTIN_NAMESPACE_CLASS(xem_ajax) xem_ajax;

    AjaxModuleForge ( Store& store ) : XProcessorModuleForge(store),
    xem_ajax(store.getKeyCache())
    {
    }
    ~AjaxModuleForge () {}

    NamespaceId getModuleNamespaceId ( ) { return xem_ajax.ns(); }

    void install ();

    void instanciateModule ( XProcessor& xprocessor )
    {
      XProcessorModule* module = new AjaxModule ( xprocessor, *this );
      xprocessor.registerModule ( module );
    }

    /**
     * Register default DomEvents for this document
     */
    void registerEvents ( Document& doc );
  };

};

#endif /* __XEM_WEBSERVER_AJAXMODULE_H */
