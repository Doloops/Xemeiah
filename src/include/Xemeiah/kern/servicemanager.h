/*
 * servicemanager.h
 *
 *  Created on: 5 nov. 2009
 *      Author: francois
 */

#ifndef __XEM_KERN_SERVICEMANAGER_H_
#define __XEM_KERN_SERVICEMANAGER_H_

#include <Xemeiah/kern/service.h>
#include <Xemeiah/kern/store.h>

#include <semaphore.h>

namespace Xem
{
  /**
   * ServiceManager is the class responsible for handling Xem::Service background service classes
   */
  class ServiceManager
  {
    friend void* __callStopServiceManagerThread(void* arg );

    friend class XemServiceModule;

  protected:
    /**
     * Reference to the store we instanciated against
     */
    Store& store;

    /**
     * Class : Currently instanciated services Map
     */
    typedef std::map<String, Service*> ServiceMap;

    /**
     * Instance : Currently instanciated services Map
     */
    ServiceMap serviceMap;

    /**
     * The semaphore used for service termination
     */
    sem_t serviceTerminationSemaphore;

    /**
     * Start the finish service function
     */
    void doStopServiceManager ();

    /**
     * Check if all services have stopped
     */
    bool areAllServicesStopped ();

    /**
     * Broadcast stop order to all services
     */
    void broadcastStopService ();
  public:
    /**
     * Constructor holds a reference to the Store
     */
    ServiceManager ( Store& store );

    /**
     * Destructor
     */
    ~ServiceManager ();

    /**
     * Register a service
     */
    void registerService ( const String& serviceName, Service* service );

    /**
     * Un-register a service
     */
    void unregisterService ( const String& serviceName );

    /**
     * Get a service given its name
     * @param serviceName the service to lookup
     * @return the Service provided, or NULL if lookup failed
     */
    Service* getService ( const String& serviceName );

    /**
     * Call for all services stop
     */
    void stopServiceManager ();

    /**
     * Wait for Service Termination
     */
    void waitTermination ();
  };
};

#endif /* __XEM_KERN_SERVICEMANAGER_H_ */
