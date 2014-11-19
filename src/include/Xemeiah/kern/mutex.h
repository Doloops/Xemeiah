/*
 * mutex.h
 *
 *  Created on: 6 nov. 2009
 *      Author: francois
 */

#ifndef __XEM_KERN_MUTEX_H
#define __XEM_KERN_MUTEX_H

#include <Xemeiah/trace.h>
#include <pthread.h>
#include <time.h>

#include <Xemeiah/dom/string.h>

namespace Xem
{
  /**
   * Mutex class : a pthread-based controlled mutex class
   */
  class Mutex
  {
  protected:
    /**
     * The name of the mutex, initialized at Mutex construction time
     */
    String name;

    /**
     * The thread owner of the mutex
     */
    pthread_t owner;

    /**
     * The pthread mutex
     */
    pthread_mutex_t mutex;

    /**
     * Level of debugging
     */
    int logLevel;

    /**
     * Retention time
     */
    struct timespec retention;

    /**
     * Initialize Mutex values
     */
    INLINE void init ();
  public:
    /**
     * Simple constructor
     */
    INLINE Mutex ();

    /**
     * Named mutex constructor
     * @param name the name of the mutex
     */
    INLINE Mutex ( const String& name );

    /**
     * Named and assigned mutex constructor
     * @param name the name of the mutex
     * @param obj a pointer to an object the mutex works for
     */
    INLINE Mutex ( const String& name, void* obj );

    /**
     * Named mutex constructor
     * @param name the name of the mutex
     * @param idx the value this mutex represents
     */
    INLINE Mutex ( const String& name, __ui64 idx );

    /**
     * Simple destructor
     */
    INLINE ~Mutex () __FORCE_INLINE;

    /**
     * Set log level
     * @param loglevel set to zero for no log, non-zero otherwise
     */
    INLINE void setLogLevel ( int loglevel );

    /**
     * Lock this mutex
     */
    INLINE void lock ();

    /**
     * Unlock this mutex
     */
    INLINE void unlock ();

    /**
     * Is the mutex locked
     */
    INLINE bool isLocked ();

    /**
     * Assert that the mutex is locked (a Bug() will be triggered if not)
     */
    INLINE void assertLocked ();

    /**
     * Assert that the mutex is not locked (a Bug() will be triggered if locked)
     */
    INLINE void assertUnlocked ();
  };

  /**
   * Simple Lock constructor
   */
  class Lock
  {
  protected:
    /**
     * The mutex assigned to this Lock
     */
    Mutex& mutex;
  public:
    /**
     * Lock constructor : instanciate on stack to lock this mutex
     */
    INLINE Lock(Mutex& mutex);

    /**
     * Lock destructor : at stack unwinding, the mutex will be unlocked
     */
    INLINE ~Lock();
  };
};

#endif /* __XEM_KERN_MUTEX_H */
