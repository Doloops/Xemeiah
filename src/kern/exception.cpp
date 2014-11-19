/*
 * exception.cpp
 *
 *  Created on: 18 janv. 2010
 *      Author: francois
 */

#include <Xemeiah/kern/exception.h>
#include <Xemeiah/io/bufferedwriter.h>
#include <Xemeiah/auto-inline.hpp>

#include <stdarg.h>

// #define __XEM_EXCEPTION_BUG_ON_EXCEPTION //< Option : send Bug() on exception

namespace Xem
{
  Exception::Exception ( const char* _exceptionClass )
  {
    silent = false;
    exceptionClass = _exceptionClass;
  }

  Exception::~Exception ()
  {

  }

  void Exception::doAppendMessage ( const char* file, const char* function, int line, const char* format, ...)
  {
    if ( silent ) return;

    va_list a_list;

    messages.push_back(MessageFragment());
    MessageFragment& fragment = (messages.back());

    fragment.file = file;
    fragment.function = function;
    fragment.line = line;
    va_start(a_list,format);
    int sz = vsnprintf(NULL, 0, format, a_list);
    va_end(a_list);

    char* msg = (char*) malloc ( sz + 2 );

    va_start(a_list,format);
    vsnprintf(msg, sz + 1, format, a_list);
    va_end(a_list);

    fragment.fragment = stringFromAllocedStr(msg);
  }

  String Exception::getMessage ()
  {
    BufferedWriter writer;
    for ( MessageList::iterator iter = messages.begin() ; iter != messages.end() ; iter++ )
      {
        MessageFragment& fragment = *iter;
        writer.doPrintf("[%s:%s:%d] %s\n", fragment.file, fragment.function, fragment.line, fragment.fragment.c_str() );
      }
    return writer.toString();
  }

  String Exception::getMessageHTML ()
  {
    BufferedWriter writer;
    writer.addStr("<html><header><title>Xemeiah : Exception Occured</title></header>");
    writer.addStr("<body>");
    writer.addStr("<b>An exception occured : </b><br/>");
    writer.addStr("<table style='border: 1px solid black;'>");
    writer.addStr("<tr><th>File</th><th>Function</th><th>Line</th><th>Message</th></tr>");

    int line = 0;
    for ( MessageList::iterator iter = messages.begin() ; iter != messages.end() ; iter++ )
      {
        MessageFragment& fragment = *iter;
        line++;
        writer.doPrintf("<tr style='border: 1px solid black; background: %s;'>", line % 2 ? "white": "lightgrey");
        writer.doPrintf("<td>%s</td>", fragment.file);
        writer.doPrintf("<td>%s</td>", fragment.function);
        writer.doPrintf("<td>%d</td>", fragment.line);
        writer.doPrintf("<td>%s</td>", fragment.fragment.c_str() );
        writer.addStr("</tr>");
      }
    writer.addStr("</table>");
    writer.addStr("</body>");
    writer.addStr("</html>");

    return writer.toString();
  }
};
