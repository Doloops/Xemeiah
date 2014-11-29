/*
 * log.cpp
 *
 *  Created on: 18 janv. 2010
 *      Author: francois
 */

#include <Xemeiah/log.h>
#include <Xemeiah/kern/exception.h>

#include <stdlib.h>
#include <stdio.h>
#include <sys/timeb.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <stdarg.h>

namespace Xem
{
#if 0
    static const char* levelName[] =
    {
        "DBG",
        "LOG",
        "INF",
        "WRN",
        "ERR",
        "FTL",
        "BUG",
        NULL
    };
#endif

    static const char*
    getLogLevelName (LogLevel level)
    {
        switch (level)
        {
            case LOG_DEBUG:
                return "DBG";
            case LOG_INFO:
                return "INF";
            case LOG_ERR:
                return "ERR";
            case LOG_WARNING:
                return "WRN";
            case LOG_CRIT:
                return "CRT";
            case LOG_EMERG:
                return "BUG";
        }
        return "???";
    }
    ;

    static bool useSysLog = false;

    void
    doTraceMessageVA (LogLevel level, const char* file, const char* function, int line, const char* format,
                      va_list a_list)
    {
        if (useSysLog)
        {
            if (*format == '\t')
                format++;
            vsyslog(level, format, a_list);
            return;
        }
        struct timeb tb;
        ftime(&tb);

        fprintf( stderr, "%4d:%3d|%lx|%s:%s:%d|%s|", (int) (tb.time % 10000), tb.millitm, pthread_self(), file,
                function, line, getLogLevelName(level));

        vfprintf(stderr, format, a_list);
    }

    void
    doTraceMessage (LogLevel level, const char* file, const char* function, int line, const char* format, ...)
    {
        va_list a_list;
        va_start(a_list, format);
        doTraceMessageVA(level, file, function, line, format, a_list);
        va_end(a_list);
    }

    void
    setXemSysLog (const char* ident, int option, int facility)
    {
        openlog(ident, option, facility);
        useSysLog = true;
    }

    void
    doThrowXemException (const char* file, const char* function, int line, const char* format, ...)
    {
        Exception* e = new Exception("Exception");
        va_list a_list;
        va_start(a_list, format);
        e->doAppendMessageVA(file, function, line, format, a_list);
        va_end(a_list);

        throw(e);
    }

    static const bool assertThrowsException = true;

    void
    doAssertFails (const char* file, const char* function, int line, const char* format, ...)
    {
        if (assertThrowsException)
        {
            Exception* e = new Exception("Exception");
            va_list a_list;
            va_start(a_list, format);
            e->doAppendMessageVA(file, function, line, format, a_list);
            va_end(a_list);
            throw(e);
        }
        else
        {
            va_list a_list;
            va_start(a_list, format);
            doTraceMessageVA(LOG_ERR, file, function, line, format, a_list);
            va_end(a_list);

            char *p = NULL; p[0] = 1;
        }
    }
}
