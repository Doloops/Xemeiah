/*
 * bufferedwriter.cpp
 *
 *  Created on: 6 janv. 2010
 *      Author: francois
 */

#include <Xemeiah/io/bufferedwriter.h>
#include <Xemeiah/kern/utf8.h>
#include <Xemeiah/trace.h>
#include <Xemeiah/kern/exception.h>

#include <Xemeiah/auto-inline.hpp>

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define Log_BW Debug

namespace Xem
{
  BufferedWriter::BufferedWriter ( int defaultCacheSize )
  {
    if ( defaultCacheSize < 256 )
      defaultCacheSize = 256;
    encoding = Encoding_UTF8;
    sz = 0;
    cacheIdx = 0;
    // cacheSize = PageSize * 8;
    cacheSize = defaultCacheSize;
    cache = (char*) malloc ( cacheSize );
    Assert ( cache, "Could not allocate BufferedWriter cache of %lu bytes\n", (unsigned long) cacheSize );
  }

  BufferedWriter::~BufferedWriter ()
  {
    if ( cache )
      {
        free ( cache );
        cache = NULL;
      }
  }

  void BufferedWriter::setEncoding ( Encoding _encoding )
  {
    encoding = _encoding;
  }

  Encoding BufferedWriter::getEncoding ()
  {
    return encoding;
  }

  __ui64 BufferedWriter::getTotalWritten () const
  {
    return sz;
  }

  void BufferedWriter::flushBuffer ()
  {
    AssertBug ( cacheSize, "Zero cacheSize !\n" );
    if ( cacheIdx < cacheSize / 2 )
      return;
    cacheSize *= 2;
    cache = (char*) realloc(cache, cacheSize);
    Assert ( cache, "Could not allocate BufferedWriter cache of %lu bytes\n", (unsigned long) cacheSize );
  }

  void BufferedWriter::extendBuffer ( int upTo )
  {
    int requiredSize = cacheSize;
    while ( cacheIdx + upTo >= requiredSize )
      {
        requiredSize *= 2;
      }
    cache = (char*) realloc ( cache, requiredSize );
    Assert ( cache, "Could not allocate BufferedWriter cache of %lu bytes\n", (unsigned long) requiredSize );
    Assert ( cacheIdx + upTo < requiredSize, "Internal bug.\n" );
    cacheSize = requiredSize;
  }

  void BufferedWriter::addStrProtectURI ( const char* text )
  {
    char s[32];
    for ( const unsigned char* vl = (const unsigned char*) text ; *vl ; vl++ )
      {
        if ( *vl > 0x7F )
          {
            addChar('%');
            sprintf(s, "%X", *vl);
            addStr(s);
          }
        else if ( *vl == '"' )
          {
            addStr ( "%22" );
          }
        else
          {
            addChar(*vl);
          }
      }
  }

  void BufferedWriter::serializeText ( const char* text, bool protectLTGT, bool protectQuote, bool protectAmp, bool htmlEntity, bool isCData )
  {
    Log_BW ( "[ST] %p %s\n", text, text );
    if ( isCData )
      addStr ( "<![CDATA[" );

    for ( const unsigned char* vl = (const unsigned char*) text ; *vl ; vl++ )
      {
        if ( isCData && strncmp ( (const char*)vl, "]]>", 3 ) == 0 )
          {
            addStr ( "]]]]><![CDATA[>" );
            vl = &(vl[2]);
            continue;
          }
        if ( *vl > 0x7F )
          {
            int bytes; int n = utf8CharToCode ( vl, bytes );
            if ( bytes == -1 )
              {
                Warn ( "Skipping non-utf8 char '%c'\n", *vl );
                addChar(*vl);
                continue;
              }
            Log_BW ( "vl='%p', encoding=%x, htmlEntity=%d, n=%d, bytes=%d\n", vl, encoding, htmlEntity, n, bytes );
            if ( htmlEntity )
              {
                const char* s = utf8ToHtmlEntity ( n );
                Log_BW ( "Convert %d to HTML entity : '%s'\n", n, s );
                if ( s )
                  {
                    addChar ( '&' );
                    addStr ( s ); vl = &(vl[bytes-1]);
                    addChar ( ';' );
                    continue;
                  }
                else
                  {

                  }
              }
            else if ( encoding != Encoding_UTF8 ) // encoding == Encoding_ISO_8859_1 )
              {
                unsigned char s[32];
                if ( ( n < 256 && encoding == Encoding_ISO_8859_1 )
                  || ( n < 128 && encoding == Encoding_US_ASCII ) )
                  {
                    s[0] = n; s[1] = '\0';
                    addStr ( s );
                  }
                else
                  {
                    Log_BW ( "Conversion from ISO-8859-1 to Decimal Entity, n=%d\n" , n);
                    sprintf ( (char*)s, "&#%d;", n );
                    if ( isCData ) { addStr ( "]]>" ); }
                    addStr ( s );
                    if ( isCData ) { addStr ( "<![CDATA[" ); }
                  }
                vl = &(vl[bytes-1]);
                continue;
              }
            /*
             * UTF-8 to UTF-8 copy, just group bytes while copying
             */
            for ( int i = 0 ; i < bytes ; i++ )
              {
                addChar ( *vl );
                vl++;
              }
            vl--; // Will be incremented in the loop
            continue;
          }
        if ( protectLTGT )
          switch ( *vl )
            {
            case '<': addStr ( "&lt;" ); continue;
            case '>': addStr ( "&gt;" ); continue;
            }

        if ( *vl == '"' && protectQuote )
          {
            addStr ( "&quot;" );
            continue;
          }
        if ( *vl == '&' && protectAmp && !( htmlEntity && vl[1] == '{' ) )
          {
            addStr ( "&amp;" );
            continue;
          }

        addChar ( *vl );
      }
    if ( isCData )
      addStr ( "]]>" );
  }

  void BufferedWriter::doPrintf ( const char* format, ... )
  {
    va_list a_list;
    va_start(a_list,format);
    int required = vsnprintf(NULL,0,format, a_list);
    va_end(a_list);

    extendBuffer(required + 1);

    va_start(a_list,format);
    vsnprintf(&(cache[cacheIdx]), required + 1, format, a_list);
    va_end(a_list);

    cacheIdx += required;
  }

  __ui64 BufferedWriter::getBufferSize()
  {
    return cacheIdx;
  }

  const char* BufferedWriter::getBuffer()
  {
    if ( cacheIdx == 0 )
      return "";
    AssertBug ( cacheIdx < cacheSize, "Too narrow to put this !\n" );
    if ( cache[cacheIdx] != '\0' )
      {
        addChar ( '\0' );
      }
    return cache;
  }

  String BufferedWriter::toString()
  {
    const char* buf = getBuffer();
    return stringFromAllocedStr(strdup(buf));
  }
};
