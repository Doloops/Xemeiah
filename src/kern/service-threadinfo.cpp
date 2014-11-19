/*
 * service-threadinfo.cpp
 *
 *  Created on: 5 nov. 2009
 *      Author: francois
 */

#include <Xemeiah/kern/service.h>
#include <Xemeiah/xprocessor/xprocessor.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_ServiceTI Debug

namespace Xem
{
  static pthread_key_t xemServiceThreadInfoKey;
  static bool xemServicePthreadKeysCreated = false;

  void
  __xemServiceThreadInfoDestructor(void* d)
  {
    Service::ThreadInfo* threadInfo = (Service::ThreadInfo*) d;
    pthread_setspecific(xemServiceThreadInfoKey, NULL);

    Log_ServiceTI ( "[SERVICETHREAD] Delete thread %lx threadInfo=%p\n", pthread_self(), threadInfo );
    if ( threadInfo );
      delete ( threadInfo );
  }

  void Service::initThreadInfoKey()
  {
    if ( ! xemServicePthreadKeysCreated )
      {
        int res = pthread_key_create(&xemServiceThreadInfoKey, &__xemServiceThreadInfoDestructor);
        if (res)
          {
            Bug ( "Could not create Service Thread Key : error %d\n", res );
          }
        xemServicePthreadKeysCreated = true;
      }
  }

  Service::ThreadInfo* Service::createThreadInfo ( )
  {
    return new ThreadInfo(*this);
  }

  Service::ThreadInfo& Service::getThreadInfo ()
  {
    ThreadInfo* threadInfo = (ThreadInfo*) pthread_getspecific(xemServiceThreadInfoKey);
    if ( threadInfo )
      {
        AssertBug ( &(threadInfo->getService()) == this, "Thread %lx recorded with service=%p, but is at %p\n", pthread_self(),
            &(threadInfo->getService()), this );
        return *threadInfo;
      }
    Log_ServiceTI ( "[SERVICETHREAD] : initialize new thread %lx for service %p\n", pthread_self(), this );

    threadInfo = createThreadInfo();
    pthread_setspecific(xemServiceThreadInfoKey, (void*) threadInfo);

    AssertBug ( threads[pthread_self()] == NULL, "Already have a threadInfo for thread %lx !\n", pthread_self() );
    threadsMutex.assertUnlocked();
    Lock lock ( threadsMutex );
    threads[pthread_self()] = threadInfo;
    Log_ServiceTI ( "Thread thread '%lx' added to service %p (%lu threads now)!\n", pthread_self(), this,
        (unsigned long) threads.size() );

    return *threadInfo;
  }

  void Service::clearThreadInfo ()
  {
    ThreadInfo* threadInfo = (ThreadInfo*) pthread_getspecific(xemServiceThreadInfoKey);
    if ( threadInfo )
      {
        Log_ServiceTI ( "Clear threadInfo at %p\n", threadInfo );
        pthread_setspecific(xemServiceThreadInfoKey, (void*) NULL);
        delete ( threadInfo );
      }
  }

  Service::ThreadInfo::ThreadInfo ( Service& _service )
  : service(_service)
  {
    xproc = NULL;
    initialNodeSet = NULL;
    initialNodeSetIterator = NULL;
    thread = pthread_self();
  }

  Service::ThreadInfo::~ThreadInfo ()
  {
    cleanupXProcessor();
    getService().notifyThreadStopped ( pthread_self() );
    // pthread_setspecific(xemServiceThreadInfoKey, (void*) NULL);
  }

  XProcessor& Service::ThreadInfo::getXProcessor ()
  {
    if ( xproc ) return *xproc;

    xproc = new XProcessor(getService().getStore());

    initialNodeSet = new NodeSet();

    ElementRef defaultCurrentNode = getService().getXProcessorDefaultCurrentNode();
    initialNodeSet->pushBack(defaultCurrentNode);
    initialNodeSetIterator = new NodeSet::iterator(*initialNodeSet, *xproc);

    getService().initXProcessor(*xproc);

    return *xproc;
  }

  void Service::ThreadInfo::cleanupXProcessor()
  {
    if ( initialNodeSetIterator ) delete (initialNodeSetIterator);
    initialNodeSetIterator = NULL;
    if ( initialNodeSet ) delete (initialNodeSet);
    initialNodeSet = NULL;
    if ( xproc ) delete (xproc);
    xproc = NULL;
  }
};
