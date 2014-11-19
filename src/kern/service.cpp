#include <Xemeiah/kern/service.h>
#include <Xemeiah/kern/exception.h>
#include <Xemeiah/trace.h>

#include <Xemeiah/auto-inline.hpp>

#include <stdlib.h>
#include <errno.h>
#include <string.h>

#define Log_Service Debug

namespace Xem
{
  Service::Service ()
  : threadsMutex("Thread List")
  {
    state = State_Stopped;
    sem_init ( &starterSemaphore, 0, 0 );
    sem_init ( &stopperSemaphore, 0, 0 );
    initThreadInfoKey();
  }

  Service::~Service()
  {
    Log_Service ( "Deleting Xem Service at %p\n", this );
    AssertBug ( isStopped(), "Trying to delete a non-stopped service (status=%s)\n",
        getState().c_str() );
    AssertBug ( threads.size() == 0, "Deleting service with %lx threads active !\n",
        (unsigned long) threads.size() );
  }

  const char* Service::getStateName ( State st )
  {
    static const char* stateNames[] =
    { "Stopped", "Starting", "Running", "Stopping", NULL };
    AssertBug ( State_Stopped <= state && state <= State_Stopping, "Invalid state %d\n", st );
    return stateNames[st];
  }

  bool Service::isStarting () { return state == State_Starting; }
  bool Service::isStarted () { return state == State_Running; }
  bool Service::isStopping () { return state == State_Stopping; }
  bool Service::isStopped ()
  {
    threadsMutex.assertUnlocked();
    Lock lock ( threadsMutex );
    return state == State_Stopped;
  }

  String Service::getState()
  {
    if ( State_Stopped <= state && state <= State_Stopping )
      return getStateName(state);
    return "Unknown";
  }

  void Service::startService ()
  {
    if ( state != State_Stopped )
      {
        throwException ( Exception, "Could not start : Service is not in the 'Stopped' state (current state : %s)!\n",
          getStateName(state) );
      }
    state = State_Starting;
    try
    {
      start ();
    }
    catch ( Exception * e )
    {
      Warn ( "Could not start service : exception %s\n", e->getMessage().c_str() );
      state = State_Stopped;
      return;
      throw ( e );
    }
    Log_Service ( "Waiting for service to start...\n" );
    waitStarted();
    if ( state == State_Starting )
      {
        state = State_Running;
        Log_Service ( "Service running !\n" );
        postStart();
      }
    else if ( state == State_Stopping )
      {
        Warn ( "Could not start service : after waitStarted(), state is %s\n", getState().c_str() );
        State lastState = state;
        state = State_Stopped;
        throwException ( Exception, "Could not start service : after waitStarted(), state was %s\n",
            getStateName(lastState) );
      }
    else
      {
        Warn ( "Unexpected state while failing to start : %s\n", getState().c_str() );
        State lastState = state;
        state = State_Stopped;
          throwException ( Exception, "Unexpected state while failing to start : %s\n",
              getStateName(lastState) );
      }
  }
  
  void Service::runStopThread ( bool restart )
  {
    AssertBug ( isStopping(), "Service not stopping !\n" );

    sem_post ( &stopperSemaphore );
    stop();

    clearThreadInfo();

    while ( true )
      {
        threadsMutex.lock ();
        Warn ( "Waiting : still %lu threads !\n", (unsigned long) threads.size() );
        bool empty = threads.empty();
        if ( empty )
          state = State_Stopped;
        else
          {
          for ( ThreadMap::iterator th = threads.begin() ; th != threads.end() ; th++ )
            {
              ThreadInfo* thInfo = th->second;
              AssertBug ( thInfo, "Null thInfo provided !\n" );
              Warn ( "\tThread %lx\n", (unsigned long) thInfo->getThread() );
            }
          }
        threadsMutex.unlock ();

        if ( empty )
          {
            Info ( "Finished waiting thread !\n" );
            break;
          }
        sleep ( 1 );
      }
    if ( restart )
      {
        startService();
      }
  }

  void Service::stopService ( bool restart )
  {
    if ( state != State_Running )
      {
        throwException ( Exception, "Could not stop : Service is not in the 'Running' state (current state : %s).\n",
          getStateName(state) );
      }
    threadsMutex.lock ();
    state = State_Stopping;
    threadsMutex.unlock ();

    startThread( boost::bind(&Service::runStopThread, this, restart) );

    if ( sem_wait ( &stopperSemaphore ) )
      {
        Bug ( "Could not sem_wait(&stopperSemaphore) !\n" );
      }
  }

  void Service::restartService ()
  {
    stopService(true);
  }

  void Service::setStarted ()
  {
    sem_post(&starterSemaphore);
  }

  void Service::waitStarted ()
  {
    int res = sem_wait(&starterSemaphore);
    if (res)
      {
        throwException ( Exception, "Could not wait starterSemaphore !\n" );
      }
  }

  void* __startThreadFunctionGeneric ( void* v )
  {
    boost::function<void()>* f = (boost::function<void()>*) v;
    (*f) ();
    delete ( f );
    return NULL;
  }

  void Service::callThreadFunction ( boost::function<void()>& function )
  {
    Log_Service ( "Starting boost::function at %p\n", &function );
    getThreadInfo();
    try
    {
      AssertBug ( isStarting() || isRunning() || isStopping(), "Service not starting or running ?\n" );
      function ( );
    }
    catch ( Exception * e )
    {
      Error ( "Service %p : thread thrown an exception : %s\n",
          this, e->getMessage().c_str() );
      delete ( e );
    }
  }

  void Service::startThread ( const boost::function<void()>& functor )
  {
    bool allowStopping = true;

    Log_Service ( "Running thread !\n" );
    if ( ! allowStopping && isStopping() )
      {
        throwException ( Exception, "Will not start thread : service not running !\n" );
      }
    pthread_t threadid;
    pthread_attr_t attr;
    pthread_attr_init ( &attr );
    pthread_attr_setdetachstate ( &attr, PTHREAD_CREATE_DETACHED );

    Lock lock ( threadsMutex );
    if ( ! allowStopping && isStopping() )
      {
        throwException ( Exception, "Will not start thread : service not running !\n" );
      }

    boost::function<void()>* f = new boost::function<void()>(boost::bind(&Service::callThreadFunction,this,functor));

    if ( pthread_create ( &threadid, &attr, __startThreadFunctionGeneric, (void*) f ) )
      {
        Error ( "Could not create thread : err=%d:%s\n", errno, strerror(errno) );
        delete ( f );
        throwException ( Exception, "Could not create thread : err=%d:%s\n", errno, strerror(errno) );
      }
    Log_Service ( "Thread started '%lx' !\n", (long) threadid );
  }

  void Service::notifyThreadStopped ( pthread_t threadid )
  {
    Log_Service ( "Thread stopped '%lx'\n", (long) threadid );
    threadsMutex.lock ();

    ThreadMap::iterator iter = threads.find ( threadid );
    AssertBug ( iter != threads.end(), "Could not find thread %lx in thread map !\n", threadid );

    threads.erase(iter);
    Log_Service ( "Thread '%lx' removed from thread list\n", (long) threadid );
    
    threadsMutex.unlock ();
  }

  Store& Service::getStore() const
  {
    Bug ( "." );
    return *((Store*)NULL);
  }

  void Service::initXProcessor ( XProcessor& xproc )
  {

  }

  ElementRef Service::getXProcessorDefaultCurrentNode ()
  {
    Bug ( "." );
    return ElementRef(*((Document*)NULL));
  }

  XProcessor& Service::getPerThreadXProcessor ()
  {
    ThreadInfo& threadInfo = getThreadInfo();
    return threadInfo.getXProcessor();
  }

  void Service::cleanupPerThreadXProcessor ()
  {
    getThreadInfo().cleanupXProcessor();
  }
};

