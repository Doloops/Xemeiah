/*
 * sessionmanagermodule.h
 *
 *  Created on: 25 nov. 2009
 *      Author: francois
 */

#ifndef __XEM_WEBSERVER_SESSIONMANAGERMODULE_H
#define __XEM_WEBSERVER_SESSIONMANAGERMODULE_H

#include <Xemeiah/xprocessor/xprocessormodule.h>
#include <Xemeiah/xprocessor/xprocessormoduleforge.h>

#include <Xemeiah/xemprocessor/xemprocessor.h>
#include <Xemeiah/webserver/webserver.h>

namespace Xem
{
#include <Xemeiah/kern/builtin_keys_prolog.h>
#include <Xemeiah/webserver/builtin-keys/xem_session>
#include <Xemeiah/kern/builtin_keys_postlog.h>

  class SessionManager;
  class SessionManagerModuleForge;

  /**
   * XProcessor module for webserver session handling
   */
  class SessionManagerModule : public XProcessorModule
  {
    friend class SessionManagerModuleForge;
  protected:
    void instructionService ( __XProcHandlerArgs__ );
    void functionCreateSession ( __XProcFunctionArgs__ );
    void functionGetSession ( __XProcFunctionArgs__ );
    void instructionWaitNews ( __XProcHandlerArgs__ );
    void instructionProcessEvent ( __XProcHandlerArgs__ );

    void triggerEvent(KeyId eventId, KeyIdList& arguments);

    SessionManager& getSessionManager ();

    bool hasSessionManager();
  public:
    __BUILTIN_NAMESPACE_CLASS(xem_web) &xem_web;
    __BUILTIN_NAMESPACE_CLASS(xem_session) &xem_session;
    __BUILTIN_NAMESPACE_CLASS(xem_event) &xem_event;

    SessionManagerModule ( XProcessor& xproc, SessionManagerModuleForge& moduleForge );
    ~SessionManagerModule () {}

    void install ();
  };

  /**
   * XProcessor module forge for webserver session handling
   */
  class SessionManagerModuleForge : public XProcessorModuleForge
  {
  protected:

  public:
    __BUILTIN_NAMESPACE_CLASS(xem_web) xem_web;
    __BUILTIN_NAMESPACE_CLASS(xem_session) xem_session;
    __BUILTIN_NAMESPACE_CLASS(xem_event) xem_event;

    SessionManagerModuleForge ( Store& store )
    : XProcessorModuleForge ( store ),
      xem_web(store.getKeyCache()),
      xem_session(store.getKeyCache()),
      xem_event(store.getKeyCache())
    { }
    ~SessionManagerModuleForge () {}

    NamespaceId getModuleNamespaceId ( )
    {
      return xem_session.ns();
    }
    void install ();

    void instanciateModule ( XProcessor& xprocessor )
    {
      XProcessorModule* module = new SessionManagerModule ( xprocessor, *this );
      xprocessor.registerModule ( module );
    }

    /**
     * Register default DomEvents for this document
     */
    void registerEvents ( Document& doc );
  };
};

#endif /* __XEM_WEBSERVER_SESSIONMANAGERMODULE_H */
