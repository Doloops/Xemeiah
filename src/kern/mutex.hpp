/*
 * mutex.hpp
 *
 *  Created on: 16 déc. 2009
 *      Author: francois
 */

#include <Xemeiah/kern/mutex.h>

#define Log_Mutex(...) do{ if (logLevel) { Debug ( "[MUTEX]" __VA_ARGS__ ); } } while (0)

// #define __XEM_MUTEX_COMPUTE_RETENTION

namespace Xem
{
  __INLINE void Mutex::init ()
  {
    logLevel = 0;
    pthread_mutex_init ( &mutex, NULL );
    owner = 0;
  }

  __INLINE Mutex::Mutex ()
  {
    init ();
    stringPrintf(name, "Lock at %p", this );
  }

  __INLINE Mutex::Mutex ( const String& _name )
  {
    init ();
    name = stringFromAllocedStr(strdup(_name.c_str()));
  }

  __INLINE Mutex::Mutex ( const String& _name, void* obj )
  {
    init ();
    stringPrintf(name,"%s for %p :", _name.c_str(), obj );
  }

  __INLINE Mutex::Mutex ( const String& _name, __ui64 idx )
  {
    init ();
    stringPrintf(name,"%s for %llx :", _name.c_str(), idx );
  }

  __INLINE Mutex::~Mutex ()
  {
    pthread_mutex_destroy ( &mutex );
  }

  __INLINE void Mutex::setLogLevel ( int i ) { logLevel = i; }

  __INLINE void Mutex::lock ()
  {
    AssertBug ( owner != pthread_self(), "DEADLOCK %s, me %lx == owner %lx\n", name.c_str(), owner, pthread_self() );
    if ( pthread_mutex_trylock ( &mutex ) == 0 )
      {

      }
    else
      {
        Log_Mutex ( "WAITING Lock %s by %lx, hold by %lx\n", name.c_str(), pthread_self(), owner );
        pthread_mutex_lock ( &mutex );
      }
    owner = pthread_self();
    Log_Mutex ( "LOCKED  %s by %lx\n", name.c_str(), owner );
#ifdef __XEM_MUTEX_COMPUTE_RETENTION
    clock_gettime(CLOCK_REALTIME, &retention);
#endif
  }

  __INLINE void Mutex::unlock ()
  {
    AssertBug ( owner == pthread_self(), "STOLEN %s, me %lx => owner %lx\n", name.c_str(), owner, pthread_self() );
    owner = 0;
#ifdef __XEM_MUTEX_COMPUTE_RETENTION
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    __ui64 diff = (ts.tv_sec - retention.tv_sec);
    diff *= (__ui64) 1000000000ULL;
    diff += (ts.tv_nsec - retention.tv_nsec);
    Log_Mutex ( "UNLOCK  %s by %lx (retention : %'llu ns)\n",
        name.c_str(), pthread_self(), diff );
#endif
    pthread_mutex_unlock ( &mutex );
  }

  __INLINE bool Mutex::isLocked ()
  {
    return ( owner == pthread_self() );
  }

  __INLINE void Mutex::assertLocked ()
  {
    AssertBug ( isLocked(), "%s Not locked by %lx, owner is %lx\n", name.c_str(), pthread_self(), owner );
  }

  __INLINE void Mutex::assertUnlocked ()
  {
    AssertBug ( !isLocked(), "%s Locked by %lx, owner is %lx\n", name.c_str(), pthread_self(), owner );
  }

  __INLINE Lock::Lock(Mutex& _mutex) : mutex(_mutex)
  {
    mutex.lock ();
  }

  __INLINE Lock::~Lock()
  {
    mutex.unlock ();
  }

};
