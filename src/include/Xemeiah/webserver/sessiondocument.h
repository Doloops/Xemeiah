/*
 * sessiondocument.h
 *
 *  Created on: 25 nov. 2009
 *      Author: francois
 */

#ifndef __XEM_WEBSERVER_SESSIONDOCUMENT_H
#define __XEM_WEBSERVER_SESSIONDOCUMENT_H

#include <Xemeiah/kern/volatiledocument.h>
#include <Xemeiah/kern/mutex.h>

#include <semaphore.h>

namespace Xem
{
  /**
   * The SessionId is stored on a 64bit pointer, which shall be enough
   */
  typedef __ui64 SessionId;

  class SessionManager;

  /**
   * SessionDocument : specially crafted VolatileDocument for Session handling
   */
  class SessionDocument : public VolatileDocument
  {
    friend class SessionManager;
  protected:
    /**
     * The session mutex, hold when the session is being modified
     */
    Mutex sessionMutex;

    /**
     * The effective sessionId
     */
    SessionId sessionId;

    /**
     * Semaphore used when broadcasting news
     */
    sem_t newsSemaphore;

    /**
     * SessionDocument constructor : protected because only SessionManager is allowed to instanciate us
     * @param store the store to bind to
     * @param allocator the allocator to use
     * @param sessionId the effective sessionId this document represents
     */
    SessionDocument ( Store& store, DocumentAllocator& allocator, SessionId sessionId );

    /**
     * SessionDocument destructor
     */
    ~SessionDocument ();

    /**
     * May index this document (answer shall be yes)
     */
    virtual bool mayIndex ();

    /**
     * Post news to the news semaphore
     */
    void postNews ();

  public:

    /**
     * Wait for news
     */
    void waitNews ( SessionManager& sessionManager );

    /**
     * Get my sessionId
     */
    SessionId getSessionId () { return sessionId; }

    /**
     * Document callback : grant write
     */
    virtual void grantWrite ();

    /**
     * Document callback : check if the document is locked write
     */
    virtual bool isLockedWrite ();

    /**
     * Document callback : lock document for write
     */
    virtual void lockWrite ();

    /**
     * Document callback : unlock document for write
     */
    virtual void unlockWrite ();
  };


};

#endif // __XEM_WEBSERVER_SESSIONDOCUMENT_H
