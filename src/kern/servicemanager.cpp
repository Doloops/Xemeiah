/*
 * servicemanager.cpp
 *
 *  Created on: 5 nov. 2009
 *      Author: francois
 */

#include <Xemeiah/kern/servicemanager.h>

#include <Xemeiah/auto-inline.hpp>

#include <signal.h>
#include <errno.h>
#include <string.h>

#define Log_ServiceManager Debug

// #define __XEM_SERVICEMANAGER_INTERCEPTS_SIGINT

namespace Xem
{
    ServiceManager::ServiceManager (Store& _store) :
            store(_store)
    {
        sem_init(&serviceTerminationSemaphore, 0, 1);
    }

    ServiceManager::~ServiceManager ()
    {
        AssertBug(serviceMap.size() == 0, "Deleting ServiceManager with non-zero Service Map !\n");
    }

    void*
    __callStopServiceManagerThread (void* arg);

    static void
    __callStopServiceManager (void* arg)
    {
        pthread_t threadid;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

        if (pthread_create(&threadid, &attr, __callStopServiceManagerThread, arg))
        {
            Error("Could not create the finish service function !!!\n");
        }
    }

    void
    ServiceManager::stopServiceManager ()
    {
        void* arg = (void*) this;
        __callStopServiceManager(arg);
    }

    static void* xemServiceSignalArg = NULL;
    static int xemServiceSIGINTHandlerCalled = 0;

    void
    xemServiceSIGINTHandler (int sig, siginfo_t* info, void* ucontext)
    {
        Error("SIGINT Handler !\n");
        xemServiceSIGINTHandlerCalled++;
        if (xemServiceSIGINTHandlerCalled == 2)
        {
            Warn("SIGINT Handler called %d times, stopping.\n", xemServiceSIGINTHandlerCalled);
            exit(0);
        }
        __callStopServiceManager(xemServiceSignalArg);
    }

#ifdef __XEM_SERVICEMANAGER_INTERCEPTS_SIGINT
    static void
    registerSignalSIGINT ()
    {
        struct sigaction sigIntAction;
        memset(&sigIntAction, 0, sizeof(struct sigaction));
        sigIntAction.sa_handler = NULL;
        sigIntAction.sa_sigaction = xemServiceSIGINTHandler;
        sigIntAction.sa_restorer = NULL;
        sigIntAction.sa_flags = SIGINT;

        int res = sigaction( SIGINT, &sigIntAction, NULL);
        if (res == -1)
        {
            Fatal("Could not set SIGINT handler !\n")
        }
        else
        {
            Log_ServiceManager ( "sigIntAction configured OK for SIGINT.\n" );
        }
    }
#endif

    void
    ServiceManager::registerService (const String& serviceName, Service* service)
    {
        if (serviceName == "" || serviceName.size() == 0)
        {
            throwException(Exception, "Invalid empty name to register a service !\n");
        }
        AssertBug(service, "Null Service provided !\n");
        if (serviceMap.size() == 0)
        {
#ifdef __XEM_SERVICEMANAGER_INTERCEPTS_SIGINT
            Info("At least one service registered, locking store !\n");
            AssertBug(!xemServiceSignalArg, "xemServiceSignalArg already registered !!!\n");
            xemServiceSignalArg = (void*) this;
            registerSignalSIGINT();
#endif
            sem_wait(&serviceTerminationSemaphore);
        }
        String _serviceName = stringFromAllocedStr(strdup(serviceName.c_str()));
        serviceMap[_serviceName] = service;
    }

    void
    ServiceManager::unregisterService (const String& serviceName)
    {
        ServiceMap::iterator iter = serviceMap.find(serviceName);
        if (iter == serviceMap.end())
        {
            throwException(Exception, "Could not lookup service '%s'\n", serviceName.c_str());
        }
        Service* service = iter->second;
        if (service->isStopped())
        {
            AssertBug(service->getRunningThreads() == 0, "Service '%s' still has running threads !\n",
                      serviceName.c_str());
            serviceMap.erase(iter);
            delete (service);
            Info("Service '%s' successfully unregistered !\n", serviceName.c_str());
            return;
        }
        throwException(Exception, "Could not unregister service '%s' : status=%s\n", serviceName.c_str(),
                       service->getState().c_str());
    }

    Service*
    ServiceManager::getService (const String& serviceName)
    {
        if (serviceName.isSpace())
        {
            throwException(Exception, "Will not get service named '%s'\n", serviceName.c_str());
        }
        ServiceMap::iterator iter = serviceMap.find(serviceName);
        if (iter == serviceMap.end())
        {
            return NULL;
            throwException(Exception, "Could not get service named '%s'\n", serviceName.c_str());
        }
        return iter->second;
    }

    bool
    ServiceManager::areAllServicesStopped ()
    {
        bool remains = false;
        for (ServiceMap::iterator iter = serviceMap.begin(); iter != serviceMap.end(); iter++)
        {
            Service* service = iter->second;
            if (!service->isStopped())
            {
                Info("\tService '%s' still not stopped, status=%s.\n", iter->first.c_str(),
                     iter->second->getState().c_str());
                Info("\t\tService has %lu running threads.\n", (unsigned long ) iter->second->getRunningThreads());
                remains = true;
            }
        }
        return !remains;
    }

    void
    ServiceManager::broadcastStopService ()
    {
        for (ServiceMap::iterator iter = serviceMap.begin(); iter != serviceMap.end(); iter++)
        {
            Service* service = iter->second;
            if (service->isRunning())
            {
                Info("\tService %s at %p\n", iter->first.c_str(), iter->second);
                try
                {
                    service->stopService();
                }
                catch (Exception *e)
                {
                    Error("Could not stop service '%s' : err=%s\n", iter->first.c_str(), e->getMessage().c_str());
                    delete (e);
                }
            }
        }
    }

    void
    ServiceManager::doStopServiceManager ()
    {
        Info("doStopServiceManager() : stopping services :\n");
        broadcastStopService();

        while (true)
        {
            Info("Checking if all services have stopped...\n");
            if (areAllServicesStopped())
            {
                Info("Finished waiting all services, unlocking Store.\n");
                sem_post(&serviceTerminationSemaphore);
                return;
            }
            Info("Still some services stopping, waiting...\n");
            sleep(1);
        }
    }

    void*
    __callStopServiceManagerThread (void* arg)
    {
        try
        {
            ServiceManager* serviceManager = (ServiceManager*) arg;
            serviceManager->doStopServiceManager();
        }
        catch (Exception* e)
        {
            Error("At ServiceManager : doStopServiceManager() : received exception : %s\n", e->getMessage().c_str());
            delete (e);
        }
        return NULL;
    }

    void
    ServiceManager::waitTermination ()
    {
        usleep(1);

        Info("Checking that at least one service is started...\n");

        bool started = false;

        for (ServiceMap::iterator iter = serviceMap.begin(); iter != serviceMap.end(); iter++)
        {
            Service* service = iter->second;
            if (!service)
            {
                Warn("NULL service provided '%s' !\n", iter->first.c_str());
                continue;
            }
            AssertBug(service, "Null Service provided '%s' !\n", iter->first.c_str());
            if (service->isStarting() || service->isRunning())
            {
                Info("\tService %s started.\n", iter->first.c_str());
                started = true;
                break;
            }
        }

        if (!started)
        {
            Info("Finished waiting for termination : no service started on starting.\n");
            return;
        }
        Info("Waiting for ServiceManager termination...\n");

        while (true)
        {
            int res = sem_wait(&serviceTerminationSemaphore);
            if (res == -1 && errno == EINTR)
            {
                Info("ServiceManager termination interrupted !\n");
                continue;
            }
            break;
        }

        Info("ServiceManager termination received : waiting for grace period...\n");

        sleep(1);

        Info("ServiceManager termination received : now stopping.\n");
        xemServiceSignalArg = NULL;
    }

}
