/*
 * sessionmanagermodule.cpp
 *
 *  Created on: 25 nov. 2009
 *      Author: francois
 */
#include <Xemeiah/webserver/sessionmanagermodule.h>
#include <Xemeiah/webserver/sessionmanager.h>
#include <Xemeiah/kern/servicemanager.h>

#include <Xemeiah/auto-inline.hpp>

namespace Xem
{
  SessionManagerModule::SessionManagerModule ( XProcessor& xproc, SessionManagerModuleForge& moduleForge )
  : XProcessorModule ( xproc, moduleForge ),
    xem_web(moduleForge.xem_web),
    xem_session(moduleForge.xem_session),
    xem_event(moduleForge.xem_event)
  {}

  bool SessionManagerModule::hasSessionManager()
  {
    String serviceName = "SessionManager";
    Service* service = getServiceManager().getService ( serviceName );
    return ( service != NULL );
  }

  SessionManager& SessionManagerModule::getSessionManager ()
  {
    String serviceName = "SessionManager";
    Service* service = getServiceManager().getService ( serviceName );
    if ( ! service )
      {
        throwException ( Exception, "Session Manager : no service named '%s' !\n", serviceName.c_str() );
      }
    SessionManager& sessionManager = dynamic_cast<SessionManager&> ( *service );

    if ( ! sessionManager.isRunning() )
      {
        throwException ( Exception, "Session Manager has stopped !\n" );
      }
    return sessionManager;
  }

  void SessionManagerModule::functionGetSession ( __XProcFunctionArgs__ )
  {
    String sessionId = args[0]->toString ();
    Document* session = NULL;

    SessionManager& sessionManager = getSessionManager();

    if ( sessionId.size() )
      {
        session = sessionManager.getSession ( sessionId );
        if ( session == NULL )
          {
            /**
             * Only return with an empty nodeset
             */
            return;
          }
      }

#if 0
    if ( session == NULL )
      {
        session = sessionManager.createSession ();
        XemProcessor& xemProc = XemProcessor::getMe(getXProcessor());
        ElementRef sessionElement = sessionManager.getSessionElement(session);
        xemProc.callMethod ( sessionElement, "Constructor" );
      }
#endif

    AssertBug ( session, "Could not get or create session !\n" );

    getXProcessor().bindDocument ( session, true );
    ElementRef sessionElement = sessionManager.getSessionElement(session);

    result.pushBack ( sessionElement );
  }

  void SessionManagerModule::functionCreateSession ( __XProcFunctionArgs__ )
  {
#if 0
    XemUserModule& xemUserModule = XemUserModule::getMe(getXProcessor());
    String userName = xemUserModule.getUserName();
    Info ( "Create session for user '%s'\n", userName.c_str() );
#endif

    Document* session = getSessionManager().createSession ();
    getXProcessor().bindDocument ( session, true );

    XemProcessor& xemProc = XemProcessor::getMe(getXProcessor());
    ElementRef sessionElement = getSessionManager().getSessionElement(session);
    xemProc.callMethod ( sessionElement, "Constructor" );

    result.pushBack ( sessionElement );
  }

  void SessionManagerModule::instructionService ( __XProcHandlerArgs__ )
  {
    SessionManagerModuleForge& sessionManagerModuleForge = getTypedModuleForge<SessionManagerModuleForge&>();
    SessionManager* sessionManager = new SessionManager ( getXProcessor(), sessionManagerModuleForge, item );

    sessionManager->registerMyself(getXProcessor());
  }

  void
  SessionManagerModule::triggerEvent(KeyId eventId, KeyIdList& arguments)
  {
    if ( ! hasSessionManager() )
      {
        Info ( "Skipping triggerEvent : no running SessionManager service !\n" );
        return;
      }
    SessionManager& sessionManager = getSessionManager();
    sessionManager.broadcastEvent(getXProcessor(),eventId, arguments);
  }

  void SessionManagerModule::instructionWaitNews ( __XProcHandlerArgs__ )
  {
    ElementRef sessionRef = getXProcessor().getVariable(xem_session.session())->toElement();

    SessionDocument& sessionDocument = dynamic_cast<SessionDocument&> ( sessionRef.getDocument() );
    sessionDocument.waitNews( getSessionManager() );
  }

  void SessionManagerModule::instructionProcessEvent ( __XProcHandlerArgs__ )
  {
    ElementRef event = getCurrentNode().toElement();
    if ( event.getKeyId() != xem_event.event() )
      {
        throwException ( Exception, "Event is not a typed event !\n" );
      }
    SessionManager& sessionManager = getSessionManager();

    getXProcessor().setElement(xem_event.event(), event, false);

    ElementId subscribedEventId = event.getAttrAsElementId(xem_event.subscribed_event_id());

    KeyIdList arguments;
    for ( AttributeRef attr = event.getFirstAttr() ; attr ; attr = attr.getNext() )
      {
        if ( attr.getNamespaceId() == xem_event.ns() )
          {
            continue;
          }
        String value = attr.toString();
        getXProcessor().setString(attr.getKeyId(),value);
        arguments.push_back(attr.getKeyId());
      }

    try
    {
      ElementRef subscribedEvent = event.getDocument().getElementById(subscribedEventId);
      sessionManager.processEvent(getXProcessor(),subscribedEvent,arguments);
    }
    catch ( Exception *e )
    {
      Error ( "Could not process asynchronous event : %s ! \n", e->getMessage().c_str() );
      // throw ( e );
      delete ( e );
    }
  }

  void SessionManagerModule::install ( )
  {

  }

  void SessionManagerModuleForge::install ()
  {
    registerAsEventTriggerHandler ();

    registerHandler ( xem_session.service(), &SessionManagerModule::instructionService );
    registerHandler ( xem_session.wait_news(), &SessionManagerModule::instructionWaitNews );
    registerHandler ( xem_session.process_event(), &SessionManagerModule::instructionProcessEvent );

    registerFunction ( xem_session.create_session(), &SessionManagerModule::functionCreateSession );
    registerFunction ( xem_session.get_session(), &SessionManagerModule::functionGetSession );
  }

  void SessionManagerModuleForge::registerEvents ( Document& doc )
  {

  }

};
