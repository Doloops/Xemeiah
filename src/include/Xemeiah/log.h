/*
 * log.h
 *
 * \file Xemeiah logging capabilities
 *
 *  Created on: 18 janv. 2010
 *      Author: francois
 */
#ifndef __XEM_TRACE_LOG_H
#define __XEM_TRACE_LOG_H

// #define __XEM_ENABLE_DEBUG_MESSAGES //< Option : enable debug messages at compile-time
#define __XEM_NOTIMPLEMENTED_THROWS_EXCEPTION //< Option : NotImplemented() throws exception or calls Fatal()

#include <syslog.h>
#include <stdarg.h>

namespace Xem
{
    typedef int LogLevel;
#if 0
    static const LogLevel LogLevel_Debug = 0;
    static const LogLevel LogLevel_Log = 1;
    static const LogLevel LogLevel_Info = 2;
    static const LogLevel LogLevel_Warn = 3;
    static const LogLevel LogLevel_Error = 4;
    static const LogLevel LogLevel_Fatal = 5;
    static const LogLevel LogLevel_Bug = 6;
    static const LogLevel LogLevel_MAX = 7;
#endif

    void
    doThrowXemException (const char* file, const char* function, int line, const char* format, ...)
            __attribute__((format(printf,4,5)));

    void
    doTraceMessageVA (LogLevel level, const char* file, const char* function, int line, const char* format,
                      va_list a_list);

    void
    doTraceMessage (LogLevel level, const char* file, const char* function, int line, const char* format, ...)
            __attribute__((format(printf,5,6)));

    void
    doAssertFails (const char* file, const char* function, int line, const char* format, ...)
            __attribute__((format(printf,4,5)));

    void
    setXemSysLog (const char* ident, int option, int facility);

#define DoTrace(level, ...) Xem::doTraceMessage ( level, __FILE__,__FUNCTION__,__LINE__, __VA_ARGS__ );

#ifdef __XEM_ENABLE_DEBUG_MESSAGES
#define Debug(...) DoTrace( LOG_DEBUG,__VA_ARGS__)
#else
#define Debug(...) do {} while(0)
#endif

#ifdef __XEM_DISABLE_LOG_MESSAGES
#define Log(...) do {} while(0)
#else
#define Log(...)    DoTrace( LOG_DEBUG,__VA_ARGS__)
#endif

#define Info(...)   DoTrace( LOG_INFO,__VA_ARGS__)
#define Error(...)  DoTrace( LOG_ERR,__VA_ARGS__)
#define Warn(...)   DoTrace( LOG_WARNING,__VA_ARGS__)

#define Fatal(...) { DoTrace( LOG_CRIT,__VA_ARGS__); exit(1); }
#define Bug(...)   { DoTrace( LOG_EMERG,__VA_ARGS__); char *p = NULL; p[0] = 1; }

#define Message(...) DoTrace( LOG_INFO,"[MESSAGE]" __VA_ARGS__)

#define Assert(__cond,...) { if ( ! (__cond) ) { Xem::doAssertFails (__FILE__,__FUNCTION__,__LINE__, __VA_ARGS__); } }

#if PARANOID
#define AssertBug Assert
#else
#define AssertBug(...) do{} while(0)
#endif

#ifdef __XEM_NOTIMPLEMENTED_THROWS_EXCEPTION
// Throw Exception
#define NotImplemented(...) throwException ( Xem::Exception, "NOT IMPLEMENTED : " __VA_ARGS__ )
#else
// Stop Xem Execution
#define NotImplemented(...) Fatal ( "NOT IMPLEMENTED : " __VA_ARGS__ )
#endif

}
;

#endif // __XEM_TRACE_LOG_H
