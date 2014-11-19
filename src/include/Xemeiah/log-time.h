/*
 * log-time.h
 *
 *  Created on: 18 janv. 2010
 *      Author: francois
 */

#ifndef __XEM_TRACE_LOG_TIME_H
#define __XEM_TRACE_LOG_TIME_H

#include <sys/timeb.h>

inline void __getntime ( struct timespec* tp )
{
#ifdef __XEM_HAS_RT
  clock_gettime (
                 CLOCK_THREAD_CPUTIME_ID
                 // CLOCK_REALTIME
                 , tp );
#endif

}

/**
 * NTime contains multiple time values measured with multiple clocks (CPU clock, realtime clock), using getntime().
 */
struct NTime
{
  struct timespec tp_cpu;
  struct timespec tp_realtime;
};

inline NTime getntime ( )
{
  NTime ntime;
#ifdef __XEM_HAS_RT
  clock_gettime ( CLOCK_PROCESS_CPUTIME_ID, &(ntime.tp_cpu) );
  clock_gettime ( CLOCK_REALTIME, &(ntime.tp_realtime) );
#endif
  return ntime;
}


inline unsigned long  diffntime ( struct timespec* from, struct timespec* to )
{
  unsigned long r = to->tv_sec - from->tv_sec;
  r *= 1000000000;
  r += to->tv_nsec;
  r -= from->tv_nsec;
  return r;
}

#define __udiff(_d,_k) (_d / _k ) % 1000
#define __LogTime(__level,__text, __start,__end)                          \
  { \
    unsigned long __diff_cpu = diffntime ( &(__start.tp_cpu), &(__end.tp_cpu) );        \
    unsigned long __diff_realtime = diffntime ( &(__start.tp_realtime), &(__end.tp_realtime) ); \
    __level ( __text "CPU : %lu,%03lu,%03lu,%03lu ns, " \
           "Realtime : %lu,%03lu,%03lu,%03lu ns\n",  \
           __udiff(__diff_cpu,1000000000),           \
           __udiff(__diff_cpu,1000000),              \
           __udiff(__diff_cpu,1000),                 \
           __diff_cpu % 1000,                        \
           __udiff(__diff_realtime,1000000000),      \
           __udiff(__diff_realtime,1000000),                 \
           __udiff(__diff_realtime,1000),                    \
           __diff_realtime % 1000                            \
           ); \
  }


#define __BeautyPrintTimeDiff(__buff, __start,__end)                      \
  { \
    unsigned long __diff_cpu = diffntime ( &(__start.tp_cpu), &(__end.tp_cpu) );        \
    unsigned long __diff_realtime = diffntime ( &(__start.tp_realtime), &(__end.tp_realtime) ); \
    sprintf ( buff, "CPU : %lu,%03lu,%03lu,%03lu ns, " \
           "Realtime : %lu,%03lu,%03lu,%03lu ns",  \
           __udiff(__diff_cpu,1000000000),           \
           __udiff(__diff_cpu,1000000),              \
           __udiff(__diff_cpu,1000),                 \
           __diff_cpu % 1000,                        \
           __udiff(__diff_realtime,1000000000),      \
           __udiff(__diff_realtime,1000000),                 \
           __udiff(__diff_realtime,1000),                    \
           __diff_realtime % 1000                            \
           ); \
  }
#define WarnTime(...) __LogTime(Warn,__VA_ARGS__)

#ifdef LOG
#define LogTime(...) __LogTime(Log,__VA_ARGS__)
#else
#define LogTime(...)
#endif

#endif // __XEM_TRACE_LOG_TIME_H
