#ifndef __XEM_CORE_EXCEPTION_H
#define __XEM_CORE_EXCEPTION_H

#include <Xemeiah/dom/string.h>
#include <Xemeiah/trace.h>

#include <list>
#include <stdarg.h>

namespace Xem
{
    /**
     * Primary class for exception handling ; all exceptions thrown must inherit Xem::Exception.
     */
    class Exception
    {
    protected:

        /**
         * The Exception class
         */
        const char* exceptionClass;

        /**
         *
         */
        class MessageFragment
        {
        public:
            const char* file;
            const char* function;
            int line;

            String fragment;

            MessageFragment ()
            {
                file = function = NULL;
                line = 0;
            }
        };

        /**
         *
         */
        typedef std::list<MessageFragment> MessageList;

        MessageList messages;

        /**
         * The message
         */
        // String message;
        /**
         * Silent exception means no message on it
         */
        bool silent;
    public:
        /**
         * Simple constructor
         */
        Exception (const char* exceptionClass);

        /**
         * Virtual destructor : this class can be overloaded
         */
        virtual
        ~Exception ();

        /**
         * Check if we have a silent exception
         */
        bool
        isSilent () const
        {
            return silent;
        }

        /**
         * Set silent exception
         */
        void
        setSilent (bool b = true)
        {
            silent = b;
        }

        /**
         * detail exception
         */
        virtual void
        doAppendMessageVA (const char* file, const char* function, int line, const char* format, va_list a_list);

        void
        doAppendMessage (const char* file, const char* function, int line, const char* format, ...)
        __attribute__((format(printf,5,6)))
        {
            va_list a_list;
            va_start(a_list, format);
            doAppendMessageVA(file, function, line, format, a_list);
            va_end(a_list);
        }

        /**
         * Get message as string
         */
        virtual String
        getMessage ();

        /**
         * Get message as string, as HTML format
         */
        virtual String
        getMessageHTML ();

    };

#ifndef STRINGIFY
#define STRINGIFY(__x) #__x
#endif // STRINGIFY

#define XemStdException(__exceptionClass) \
  class __exceptionClass : public Exception { \
  public: \
  __exceptionClass() : Exception(STRINGIFY(__exceptionClass)) {} \
  __exceptionClass(const char* __class) : Exception(__class) {} \
    ~__exceptionClass() {} \
  }

#define throwException(__exceptionClass,...)	\
  do { \
    __exceptionClass* e = new __exceptionClass(STRINGIFY(__exceptionClass)); \
    e->doAppendMessage ( __FILE__, __FUNCTION__, __LINE__, "Exception '%s'\n", STRINGIFY(__exceptionClass) ); \
    e->doAppendMessage ( __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__ ); \
    Error ( "Exception '%s'\n", e->getMessage().c_str()); \
    throw ( e ); \
  } while (0)

#define detailException(__exception,...) \
  do { \
    __exception->doAppendMessage ( __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__ ); \
  } while (0)

}

#endif // __XEM_CORE_EXCEPTION_H
