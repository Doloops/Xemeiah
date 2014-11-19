#include <Xemeiah/webserver/sessionmanager.h>

#include <Xemeiah/kern/store.h>
#include <Xemeiah/kern/document.h>
#include <Xemeiah/kern/volatiledocument.h>
#include <Xemeiah/kern/volatiledocumentallocator.h>

#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/dom/attributeref.h>
#include <Xemeiah/dom/childiterator.h>

#include <Xemeiah/xemprocessor/xemprocessor.h>
#include <Xemeiah/xemprocessor/xemservicemodule.h>

#include <Xemeiah/xprocessor/xprocessorlib.h>

#include <Xemeiah/auto-inline.hpp>

#include <stdlib.h>

#define Log_SessionManager Debug
#define Log_SessionManagerEvent Debug

namespace Xem
{
  __XProcessorLib_REGISTER_MODULE ( WebServer, SessionManagerModuleForge );

#include <Xemeiah/kern/builtin_keys_prolog_inst.h>
#include <Xemeiah/webserver/builtin-keys/xem_session>
#include <Xemeiah/kern/builtin_keys_postlog.h>

  SessionManager::SessionManager ( XProcessor& xproc, SessionManagerModuleForge& _sessionManagerModuleForge,
          ElementRef& configurationElement )
  : XemService (xproc, configurationElement),
    sessionManagerMutex("SessionManager"),
    sessionManagerModuleForge(_sessionManagerModuleForge),
    xem_session(sessionManagerModuleForge.xem_session),
    xem_event(sessionManagerModuleForge.xem_event)
  {
    /**
     * We have to provide really random sessionIds
     */
    srand ( time ( NULL ) );
  }
  
  SessionManager::~SessionManager ()
  {
  }
  
  void SessionManager::start ()
  {
    startThread ( boost::bind(&SessionManager::garbageCollectorThread, this) );
  }
  
  void SessionManager::stop ()
  {
  }

  bool SessionManager::hasSessionExpired ( Document* session )
  {
    ElementRef sessionElement = getSessionElement ( session );
    
    time_t now = time ( NULL );
    time_t lastAccess = strtoul ( sessionElement.getAttr ( xem_session.lastAccessTime() ).c_str(), NULL, 0 );
    time_t ttl = strtoul ( sessionElement.getAttr ( xem_session.ttl() ).c_str(), NULL, 0 );
    
    if ( lastAccess > now )
      {
        Error ( "Session has lastAccess time in the future !\n" );
        return false;
      }
    if ( lastAccess + ttl < now )
      return true;
    return false;
  }

  void SessionManager::garbageCollectorThread ()
  {
    setStarted();
    sleep ( 1 );
    while ( true )
      {
        __ui64 stillReferencedSessions = 0;
        
        sessionManagerMutex.lock ();
        Log_SessionManager ( "[SESSION] SessionManager garbage collector : %lu openned sessions.\n", (unsigned long) sessionMap.size() );

        for ( SessionMap::iterator iter = sessionMap.begin () ; iter != sessionMap.end() ; iter++ )
          {
            Document* session = iter->second;
            if ( isRunning() && !hasSessionExpired ( session ) )
              continue;
            if ( session->getRefCount() == 1 )
              {
                Info ( "[SESSION] Deleting Session (%p) %llx : refCount=%llx\n",
                    session, iter->first, session->getRefCount() );
                getStore().releaseDocument ( session );
                sessionMap.erase ( iter );
              }
            else
              {
                Info ( "[SESSION] Session (%p) %llx still referenced %llu times !\n",
                    session, iter->first, session->getRefCount() );
                stillReferencedSessions++;
              }
          }
        sessionManagerMutex.unlock ();

        if ( isStopping() )
          {
            if ( stillReferencedSessions )
              {
                Info ( "Could not stop session manager : still %llu sessions referenced\n", stillReferencedSessions );
              }
            else
              {
                Info ( "Stopping SessionManager Garbage Collector Thread!\n" );
                return;
              }
          }
        sleep (1);
      }
  }

  Document* SessionManager::getSession ( SessionId sessionId )
  {
    if ( sessionMap.find ( sessionId ) == sessionMap.end() )
      return NULL;
    return sessionMap[sessionId];
  }

  bool SessionManager::doesSessionExist ( SessionId sessionId )
  {
    return sessionMap.find ( sessionId ) != sessionMap.end();
  }
  
  Document* SessionManager::createSession ()
  {
    SessionId sessionId = 0;
    Lock lock ( sessionManagerMutex );
    do
      {
        sessionId = rand();
      }
    while ( sessionId && doesSessionExist ( sessionId ) );
    
    Document* session = createSessionDocument ( sessionId );
    
    Warn ( "Created session %llu, sessions=%lu\n", sessionId, (unsigned long) sessionMap.size() );

    sessionMap[sessionId] = session;
    session->incrementRefCount ();

    ElementRef sessionElement = createSessionElement ( session, sessionId );
    session->unlockWrite ();
    return session;
  }

  Document* SessionManager::createSessionDocument ( SessionId sessionId )
  {
    VolatileDocumentAllocator* volatileDocumentAllocator = new VolatileDocumentAllocator (getStore());
    SessionDocument* session = new SessionDocument(getStore(), *volatileDocumentAllocator, sessionId);
    session->bindDocumentAllocator ( volatileDocumentAllocator );
    session->lockWrite ();
    session->createRootElement();
    session->setRole ( "session" );
    return session;    
  }
  
  
  ElementRef SessionManager::createSessionElement ( Document* session, SessionId sessionId )
  {
    String defaultTTL = "3600";
  
    ElementRef rootElement = session->getRootElement ();
    ElementRef sessionElement = session->createElement ( rootElement, xem_session.session() );
    rootElement.appendLastChild ( sessionElement );

#if 0
    sessionElement.addNamespaceAlias ( xem_web.defaultPrefix(), xem_web.ns() );
    sessionElement.addNamespaceAlias ( __builtinKey.xem->defaultPrefix(), __builtinKey.xem->ns() );
    sessionElement.addNamespaceAlias ( __builtinKey.xem_role->defaultPrefix(), __builtinKey.xem_role->ns() );
#endif
    sessionElement.addNamespaceAlias ( "xem", "http://www.xemeiah.org/ns/xem" );
    sessionElement.addNamespaceAlias ( "xem-role", "http://www.xemeiah.org/ns/xem-role" );
    sessionElement.addNamespaceAlias ( xem_session.defaultPrefix(), xem_session.ns() );
    sessionElement.addNamespaceAlias ( xem_event.defaultPrefix(), xem_event.ns() );
    
    String sessionIdStr; stringPrintf ( sessionIdStr, "%llx", sessionId );
    sessionElement.addAttr ( xem_session.sessionId(), sessionIdStr );

    time_t creationTime; time ( &creationTime );
    String creationTimeStr; stringPrintf ( creationTimeStr, "%lu", creationTime );
    sessionElement.addAttr ( xem_session.creationTime(), creationTimeStr );
    sessionElement.addAttr ( xem_session.lastAccessTime(), creationTimeStr );
    sessionElement.addAttr ( xem_session.ttl(), defaultTTL );

    return sessionElement;
  }
  
  ElementRef SessionManager::getSessionElement ( Document* session )
  {
    return session->getRootElement().getChild ();
  }
      
  Document* SessionManager::getSession ( String sessionId )
  {
    SessionId sId;
    if ( sscanf ( sessionId.c_str(), "%llx", &sId ) != 1 )
      {
        Warn ( "Invalid sessionId '%s'\n", sessionId.c_str() );
        return NULL;
      }
    Log_SessionManager ( "[SESSION] get Session '%llx'\n", sId );

    Lock lock ( sessionManagerMutex );
    Document* session = getSession ( sId );

    if ( ! session ) 
      {
        return NULL;
      }
      
    ElementRef sessionElement = getSessionElement ( session );
    time_t lastAccessTime; time ( &lastAccessTime );
    String lastAccessTimeStr; stringPrintf ( lastAccessTimeStr, "%lu", lastAccessTime );
    sessionElement.getDocument().lockWrite();
    sessionElement.addAttr ( xem_session.lastAccessTime(), lastAccessTimeStr );
    sessionElement.getDocument().unlockWrite();
    return session;
  }

  void SessionManager::enqueueEvent ( XProcessor& xproc, ElementRef& eventQueue, ElementRef& subscribedEvent,
      KeyId eventId, KeyIdList& arguments )
  {
    eventQueue.getDocument().grantWrite();
    XemProcessor& xemProc = XemProcessor::getMe(xproc);

    try
    {
      String classIdStr = subscribedEvent.getAttr(xemProc.xem.classId());
      KeyId classId = classIdStr.toUI64();
      KeyId methodId = subscribedEvent.getAttrAsKeyId(xemProc.xem.method());
      xemProc.controlMethodArguments(classId,methodId,arguments);
    }
    catch ( Exception* e )
    {
      Error ( "Could not control method arguments : exception %s\n", e->getMessage().c_str() );
      throw ( e );
    }
    ElementRef newEvent = eventQueue.getDocument().createElement(eventQueue,xem_event.event());
    newEvent.addAttrAsKeyId(xem_event.event_name(), eventId);

    newEvent.addAttrAsElementId(xem_event.subscribed_event_id(), subscribedEvent.getElementId() );
    newEvent.addAttr(xem_event.event_name(), xproc.getKeyCache().dumpKey(eventId) );

    for ( KeyIdList::iterator arg = arguments.begin() ; arg != arguments.end() ; arg++ )
      {
        KeyId argId = *arg;
        NodeSet* nodeSet = xproc.getVariable(argId);
        if ( nodeSet->size() != 1 )
          {
            throwException ( Exception, "Invalid multiple value for event argument %s\n", xproc.getKeyCache().dumpKey(argId).c_str());
          }
        Item& front = nodeSet->front();
        switch ( front.getItemType() )
        {
        case Item::Type_Element:
          NotImplemented ( "Event argument type Type_Element\n" );
        case Item::Type_Attribute:
          {
            AttributeRef& attr = front.toAttribute();
            switch ( attr.getAttributeType() )
            {
            case AttributeType_Integer:
              newEvent.addAttrAsInteger(argId,attr.toInteger());
              break;
            case AttributeType_String:
            default:
              newEvent.addAttr(argId,attr.toString());
            }
            break;
          }
        case Item::Type_Bool:
        case Item::Type_Integer:
        case Item::Type_Number:
        case Item::Type_String:
          newEvent.addAttr(argId,front.toString());
          break;
        default:
          NotImplemented ( "Item::ItemType %x\n", front.getItemType());
        }
      }
    eventQueue.appendLastChild(newEvent);
    eventQueue.getDocument().unlockWrite();
  }

  ElementRef SessionManager::resolveElement ( XemProcessor& xemProc, ElementRef& subscribedEvent )
  {
    __BUILTIN_NAMESPACE_CLASS(xem)& xem = xemProc.xem;
    NodeSet thisNodeSet;
    xemProc.resolveNodeAndPushToNodeSet(thisNodeSet, subscribedEvent.getAttr(xem.this_()));

    ElementRef thisElement = thisNodeSet.toElement();
    return thisElement;
  }

  void SessionManager::processEvent ( XProcessor& xproc, ElementRef& subscribedEvent, KeyIdList& arguments )
  {
    XemProcessor& xemProc = XemProcessor::getMe(xproc);
    __BUILTIN_NAMESPACE_CLASS(xem)& xem = xemProc.xem;
    xproc.setElement(xem_event.subscribed_event(), subscribedEvent, false);

    ElementRef thisElement = resolveElement ( xemProc, subscribedEvent );
    KeyId methodId = subscribedEvent.getAttrAsKeyId(xem.method());

    xemProc.controlMethodArguments(thisElement,methodId,arguments);

    try
    {
      xemProc.callMethod(thisElement, methodId);
    }
    catch ( Exception *e )
    {
      detailException ( e, "Could not process event='%s' this='%s' !\n",
          subscribedEvent.generateVersatileXPath().c_str(),
          thisElement.generateVersatileXPath().c_str() );
      throw ( e );
    }
  }

  void SessionManager::broadcastEvent(XProcessor& xproc, KeyId eventId, KeyIdList& arguments)
  {
    Lock lock ( sessionManagerMutex );
    SessionId currentSessionId = 0;
    if ( xproc.hasVariable(xem_session.session()) )
      {
        NodeSet* sessionNodeSet = xproc.getVariable(xem_session.session());
        ElementRef sessionElement = sessionNodeSet->toElement();
        currentSessionId = (dynamic_cast<SessionDocument&> ( sessionElement.getDocument()) ).getSessionId();
      }

    try
      {
        for ( SessionMap::iterator iter = sessionMap.begin() ; iter != sessionMap.end() ; iter++ )
          {
            ElementRef sessionElement = getSessionElement ( iter->second );

            ElementRef subscribedEvents (sessionElement.getDocument());
            ElementRef eventQueue (sessionElement.getDocument());

            for( ChildIterator child(sessionElement) ; child ; child++ )
              {
                if ( child.getKeyId() == xem_event.subscribed_events() )
                  { subscribedEvents = child;  }
                else if ( child.getKeyId() == xem_event.event_queue() )
                  { eventQueue = child;  }
              }
            if ( ! subscribedEvents )
              {
                throwException(Exception,"No subscribed events in session %s!\n", sessionElement.generateVersatileXPath().c_str() );
              }
            if ( ! eventQueue )
              {
                throwException(Exception,"No event queue in session %s!\n", sessionElement.generateVersatileXPath().c_str() );
              }

            bool hasEnqueued = false;

            for ( ChildIterator subscribedEvent(subscribedEvents) ; subscribedEvent ; subscribedEvent ++ )
              {
                if ( subscribedEvent.getAttrAsKeyId(xem_event.event_name()) != eventId )
                  continue;
                if ( subscribedEvent.hasAttr(xem_event.restrict_to_session ())
                    && subscribedEvent.getAttr(xem_event.restrict_to_session ()) == "yes" )
                  {
                    if ( currentSessionId != iter->first )
                      continue;
                  }

                Log_SessionManagerEvent ( "Matches event : %s\n", subscribedEvent.generateVersatileXPath().c_str() );
                if ( subscribedEvent.hasAttr(xem_event.synchronous())
                    && subscribedEvent.getAttr(xem_event.synchronous()) == "yes" )
                  {
                    processEvent(xproc, subscribedEvent, arguments);
                  }
                else
                  {
                    enqueueEvent(xproc, eventQueue, subscribedEvent, eventId, arguments);
                    hasEnqueued = true;
                  }
              }
            if ( hasEnqueued )
              {
                SessionDocument* sessionDocument = dynamic_cast<SessionDocument*> ( iter->second );
                AssertBug ( sessionDocument, "No document ?\n" );
                sessionDocument->postNews();
              }
          }
      }
    catch ( Exception *e )
    {
      detailException(e,"Could not broadcast event %s (%x)\n",
          xproc.getKeyCache().dumpKey(eventId).c_str(), eventId );
      throw ( e );
    }
  }
};
