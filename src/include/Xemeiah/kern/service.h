#ifndef __XEM_KERN_SERVICE_H
#define __XEM_KERN_SERVICE_H

#include <pthread.h>
#include <semaphore.h>
#include <list>

#include <Xemeiah/kern/mutex.h>
#include <Xemeiah/dom/nodeset.h> //< For the NodeSet::iterator declaration
#include <boost/function.hpp>
#include <boost/bind.hpp>

namespace Xem
{
  class String;
  class XProcessor;
  class Store;
  class ElementRef;


  /**
   * Xem Service API, able to run multiple threads and handle them
   */
  class Service
  {
  protected:
    /**
     * The thread destructor callback function
     */
    friend void __xemServiceThreadInfoDestructor(void* d);

    /**
     * States
     */
    enum State
    {
      State_Stopped  = 0x0,
      State_Starting = 0x1,
      State_Running  = 0x2,
      State_Stopping = 0x3
    };
    
    /**
     * Current state
     */
    State state;
    
    /**
     * Constructor
     */
    Service ();

    /**
     * The starter function
     */
    virtual void start() = 0;

    /**
     * The stopper function
     */
    virtual void stop() = 0;

    /**
     * The Semaphore used to check that the Service finished to start
     */
    sem_t starterSemaphore;

    /**
     * The Semaphore used to check that the service is stopping
     */
    sem_t stopperSemaphore;

    /**
     * State that the service is started
     */
    void setStarted ();

    /**
     * Wait for the service to start
     */
    void waitStarted ();

    /**
     * Run the stop thread
     */
    void runStopThread ( bool restart );

    /*
     * This is run after the effective start of the service
     */
    virtual void postStart() {}

    /*
     * This is run after the effective stop of all threads of service
     */
    virtual void postStop() {}

    /**
     *
     */
    void initThreadInfoKey();

    /**
     * Per-thread instance
     */
    class ThreadInfo
    {
    protected:
      /**
       * Our own service
       */
      Service& service;

      /**
       * The XProcessor we have instanciated
       */
      XProcessor* xproc;

      /**
       * For XProcessor : the initial NodeSet we have instanciated
       */
      NodeSet* initialNodeSet;

      /**
       * For XProcessor : the initial NodeSet iterator we have instanciated
       */
      NodeSet::iterator* initialNodeSetIterator;

      /**
       * Thread which is running
       */
      pthread_t thread;
    public:
      /**
       * Simple TheadInfo constructor
       */
      ThreadInfo ( Service& service );

      /**
       * Simple ThreadInfo destructor
       */
      virtual ~ThreadInfo ();

      /**
       * Get my Service
       */
      Service& getService() const { return service; }

      /**
       * Get my per-thread XProcessor
       */
      XProcessor& getXProcessor ();

      /**
       * Get the running thread
       */
      pthread_t getThread() const { return thread; }

      /**
       * Cleanup my XProcessor
       */
      void cleanupXProcessor ();

    };

    /**
     * Type for list of running threads
     */
    typedef std::map<pthread_t,ThreadInfo*> ThreadMap;
    
    /**
     * List of running threads
     */
    ThreadMap threads;

    /**
     * Mutex for list of running threads
     */
    Mutex threadsMutex;

    /**
     * Get per-thread structure ThreadInfo
     */
    ThreadInfo& getThreadInfo ();

    /**
     * Clear ThreadInfo
     */
    void clearThreadInfo ();

    /**
     * ThreadInfo creator callback
     */
    virtual ThreadInfo* createThreadInfo ( );

    /**
     * Notify that a thread has stopped
     */
    void notifyThreadStopped ( pthread_t thread );
    
    /**
     * Get Service State Name from Service State
     */
    const char* getStateName ( State st );

    /**
     * Get my Store
     */
    virtual Store& getStore() const;

    /**
     *
     */
    virtual ElementRef getXProcessorDefaultCurrentNode ();

    /**
     *
     */
    XProcessor& getPerThreadXProcessor ();

    /**
     *
     */
    void cleanupPerThreadXProcessor ();

    /**
     * Initialize a newly created XProcessor
     */
    virtual void initXProcessor ( XProcessor& xproc );

    /**
     *
     */
    void callThreadFunction ( boost::function<void()>& function );
  public:
    /**
     * Service Destructor
     */
    virtual ~Service();
    
    /**
     * Start this service
     */
    void startService ();

    /**
     * Stop this service
     */
    void stopService ( bool restart = false );

    /**
     * Restart this service : call stopService(), wait for service stops, and then call startService()
     */
    void restartService ();
    
    /**
     * Get current State
     */
    String getState ();

    /**
     * Is Service starting ?
     */
    bool isStarting ();

    /**
     * Is Service started ?
     */
    bool isStarted ();

    /**
     * Is Service started ? (alias)
     */
    bool isRunning () { return isStarted(); }

    /**
     * Is Service stopping ?
     */
    bool isStopping ();

    /**
     * Is Service stopped ?
     */
    bool isStopped ();
    
    /**
     * Get number of running threads
     */
    size_t getRunningThreads() { return threads.size(); }
    
    /**
     * Start thread using function
     */
    void startThread ( const boost::function<void()>& functor );
  };

};

#endif // __XEM_KERN_XEMSERVICE_H

