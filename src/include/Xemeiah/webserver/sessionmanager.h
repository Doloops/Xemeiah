#ifndef __XEM_KERN_SESSIONMANAGER_H
#define __XEM_KERN_SESSIONMANAGER_H

#include <Xemeiah/webserver/sessiondocument.h>
#include <Xemeiah/webserver/sessionmanagermodule.h>
#include <Xemeiah/kern/store.h>
#include <Xemeiah/kern/mutex.h>
#include <Xemeiah/dom/string.h>

#include <Xemeiah/xemprocessor/xemservice.h>

#include <map>

namespace Xem
{

  class Store;
  class Document;
  class ElementRef;
  class XemProcessor;
  class SessionManagerModuleForge;

  void* SessionManagerGarbageCollectorThread ( void* );

  /**
   * SessionManager : session creation and retrival
   */  
  class SessionManager : public XemService
  {
  protected:
    /**
     * Our sessionManagerMutex
     */
    Mutex sessionManagerMutex;

    /**
     * Our SessionManagerModuleForge
     */
    SessionManagerModuleForge& sessionManagerModuleForge;

    /**
     * Our session map
     */
    typedef std::map<SessionId,Document*> SessionMap;
    
    /**
     * Our Instance of SessionMap
     */
    SessionMap sessionMap;

    /**
     * Checks if sessionId exists
     */    
    virtual bool doesSessionExist ( SessionId sessionId );

    /**
     * Instanciates a new session docuemnt
     */
    virtual Document* createSessionDocument ( SessionId sessionId );
    
    /**
     * Creates session contents
     */ 
    virtual ElementRef createSessionElement ( Document* session, SessionId sessionId );

    /**
     * Internal function to fetch Session from SessionMap
     * @param sessionId the sessionId to lookup
     * @return the Session if found, NULL if none found.
     */
    virtual Document* getSession ( SessionId sessionId );
    
    /**
     * Our garbage collector function
     */
    virtual void garbageCollectorThread ();
    
    /**
     * Checks if Session has expired
     */
    bool hasSessionExpired ( Document* session );

    /**
     * start and stop services
     */
    void start ();
    void stop ();


    /**
     * Enqueue an event in the event queue
     * @param xproc the XProcessor to use
     * @param eventQUeue the event queue
     * @param subscribedEvent the subscribed event register
     * @param eventId the QName id of the event
     * @param arguments the list of argument QNames
     */
    void enqueueEvent ( XProcessor& xproc, ElementRef& eventQueue, ElementRef& subscribedEvent,
          KeyId eventId, KeyIdList& arguments );

    /**
     * Resolve an element call
     */
    ElementRef resolveElement ( XemProcessor& xemProc, ElementRef& subscribedEvent );
  public:
    __BUILTIN_NAMESPACE_CLASS(xem_session) &xem_session;
    __BUILTIN_NAMESPACE_CLASS(xem_event) &xem_event;
    /**
     * SessionManager constructor, with a reference to the binded store.
     */
    SessionManager ( XProcessor& xproc, SessionManagerModuleForge& sessionManagerModuleForge, ElementRef& configurationElement );
    
    /**
     * SessionManager destructor
     */
    ~SessionManager ();

    /**
     * Create a new session, incrementing refCount
     */
    Document* createSession ();
    
    /**
     * Gets a session, incrementing refCount
     */
    Document* getSession ( String sessionId );
    
    /**
     * Gets the session Element, xemws:session
     */
    ElementRef getSessionElement ( Document* session );


    /**
     * Session-wide Event Triggering
     */
    void broadcastEvent(XProcessor& xproc, KeyId eventId, KeyIdList& arguments);

    /**
     * Process synchronous event
     * @param xproc the XProcessor to use
     * @param subscribedEvent the event subscribed
     * @param arguments the arguments for this synchronous call
     */
    void processEvent ( XProcessor& xproc, ElementRef& subscribedEvent, KeyIdList& arguments );

  };
}; 

#endif // __XEM_KERN_SESSIONMANAGER_H

