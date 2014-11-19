/*
 * sessiondocument.cpp
 *
 *  Created on: 25 nov. 2009
 *      Author: francois
 */

#include <Xemeiah/webserver/sessiondocument.h>
#include <Xemeiah/webserver/sessionmanager.h>

#include <Xemeiah/auto-inline.hpp>

#include <errno.h>
#include <string.h>

#define Log_WaitNews Debug

namespace Xem
{
  SessionDocument::SessionDocument ( Store& store, DocumentAllocator& allocator, SessionId _sessionId )
  : VolatileDocument ( store, allocator ), sessionMutex("Session Mutex", _sessionId)
  {
    sessionMutex.setLogLevel(0);
    sessionId = _sessionId;
    sem_init ( &newsSemaphore, 0, 0 );
  }

  SessionDocument::~SessionDocument ()
  {
    sem_destroy(&newsSemaphore);
  }

  bool SessionDocument::mayIndex ()
  {
    return true;
  }

  void SessionDocument::waitNews ( SessionManager& sessionManager )
  {
    Log_WaitNews ( "[SESSION-NEWS] Waiting news !!!\n" );
    struct timespec ts;
    int res;
    int iterations = 0;
    int maxIterations = 100;
  retry_new:
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 1;

  retry_intr:
    res = sem_timedwait(&newsSemaphore,&ts);

    if ( !sessionManager.isRunning() )
      {
        Log_WaitNews ( "[SESSION-NEWS] While waiting for news, session manager stopped !\n" );
        return;
        throwException ( Exception, "While waiting for news, session manager stopped !\n" );
      }

    if ( res == -1 )
      {
        if ( errno == ETIMEDOUT )
          {
            Log_WaitNews ( "[SESSION-NEWS] ETIMEOUT while waiting news...\n" );
            iterations ++;
            if ( iterations == maxIterations )
              {
                Log_WaitNews ( "[SESSION-NEWS] Exceeded delay...\n" );
                return;
              }
            goto retry_new;
          }
        else if ( errno == EINTR )
          {
            Log_WaitNews ( "[SESSION-NEWS] EINTR : interrupted after waiting %ld.%ld seconds\n",
                ts.tv_sec, ts.tv_nsec );
            goto retry_intr;
          }
        else
          {
            Bug ( "[SESSION-NEWS] Could not timewait : err=%d:%s\n", errno, strerror(errno) );
          }
        return;
      }
    Log_WaitNews ( "[SESSION-NEWS] Had news, broadcasting them !!!\n" );
    /*
     * Take a small grace period, trying to handle multiple events at a time
     */
    usleep ( 1000 );
  }

  void SessionDocument::postNews ()
  {
    int news = 0;
    if ( sem_getvalue(&newsSemaphore,&news) == -1 )
      {
        Error ( "Could not sem_getvalue : err=%d:%s\n", errno, strerror(errno) );
      }
    if ( news > 4 )
      {
        Log_WaitNews ( "[SESSION-NEWS] Skipping postNews(), already spammed up to %d !!!\n", news );
        return;
      }
    sem_post ( &newsSemaphore );
  }

  bool SessionDocument::isLockedWrite ()
  {
    return sessionMutex.isLocked();
  }

  void SessionDocument::lockWrite ()
  {
    sessionMutex.lock();
  }

  void SessionDocument::unlockWrite ()
  {
    sessionMutex.unlock();
  }

  void SessionDocument::grantWrite ()
  {
    if ( ! isLockedWrite() )
      {
        lockWrite ();
      }
  }
};
