/*
 * bufferedwriter.h
 *
 *  Created on: 6 janv. 2010
 *      Author: francois
 */

#ifndef __XEM_IO_BUFFEREDWRITER_H_
#define __XEM_IO_BUFFEREDWRITER_H_

#include <Xemeiah/kern/format/core_types.h>
#include <Xemeiah/kern/encoding.h>
#include <Xemeiah/kern/utf8.h>

namespace Xem
{
  /**
   * Buffered Output Writer
   */
  class BufferedWriter
  {
  protected:
    /*
     * Encoding selected for this writer
     */
    Encoding encoding;

    /**
     * Total number of bytes written
     */
    __ui64 sz;

    /**
     * Size of write cache
     */
    int cacheSize;

    /**
     * Current position in write cache
     */
    int cacheIdx;


    /**
     * Write cache
     */
    char* cache;

  public:
    /**
     * Create a new instance of buffered writer
     */
    BufferedWriter ( int defaultCacheSize = 1024 );

    /**
     * Buffered writer destructor
     */
    virtual ~BufferedWriter ();

    /**
     * Set encoding
     * @param encoding the selected encoding
     */
    void setEncoding ( Encoding encoding );

    /**
     * Get encoding
     * @return the selected encoding
     */
    Encoding getEncoding ();

    /**
     * Get total number of bytes written
     */
    __ui64 getTotalWritten () const;

    /**
     * Flush the buffer when buffer is full
     */
    virtual void flushBuffer ();

    /**
     * extend buffer up to some bytes
     */
    virtual void extendBuffer ( int upTo );

    /**
     * Append a character
     * @param c the character to append
     */
    inline void addChar ( char c )
    {
      if ( cacheIdx == cacheSize - 2 )
        flushBuffer();
      cache[cacheIdx] = c ;
      cache[cacheIdx+1] = '\0';
      cacheIdx++ ;
    }

    /**
     * Append a string
     * @param str the string to append
     */
    inline void addStr ( const char* str )
    {
      for ( const char* s = str ; *s ; s++ )
        {
          addChar ( *s );
        }
    }

    /**
     * Append an utf8 value
     */
    inline void addUtf8 ( int code )
    {
      if (code < 0x80)
        {
          addChar ( (char) code );
          return;
        }
      int bytes = utf8CodeSize(code);
      AssertBug ( bytes > 0, "Unable to serialize.\n" );
      if ( cacheIdx + bytes+1 >= cacheSize )
        {
          extendBuffer(bytes+1);
        }
      int res = utf8CodeToChar(code, (unsigned char*) &(cache[cacheIdx]), bytes + 1);
      Assert ( res == bytes, "Diverging number of bytes ?\n" );      
#if 0 // DEBUG
      Log ( "Written UTF8 : code=%d, bytes=%d, cacheIdx=%d, cacheSz=%d, res=%d\n", code, bytes, cacheIdx, cacheSize, res );
      Log ( "cache[0]=%d, cache[1]=%d\n", (unsigned char) cache[cacheIdx], (unsigned char) cache[cacheIdx+1] );
#endif
      cacheIdx += bytes;
    }

    /**
     * Append a string
     * @param str the string to append
     */
    inline void addStr ( const unsigned char* str )
    {
      addStr ( (const char*) str );
    }

    /**
     * Print a formated string
     * @param format the printf-style format to use
     */
    void doPrintf ( const char* format, ... ) __attribute__((format(printf,2,3)));

    /**
     * Append a string, protecting it using HTML URL protection scheme (see B.2.1. of HTML 4.0 specifications)
     * @param text the string to append
     */
    void addStrProtectURI ( const char* text );

    /**
     * Append a string, with various options to protect stuff
     * @param text the text to write
     * @param protectLTHT protect '<' and '>' as XML entities
     * @param protectQuote protect the '"' character as XML entity
     * @param protectAmp protect the '&' character as XML entity
     * @param htmlEntity convert UTF-8 character to HTML entity
     * @param isCData write as a CData section
     */
    void serializeText ( const char* text, bool protectLTGT, bool protectQuote, bool protectAmp, bool htmlEntity, bool isCData = false );

    /**
     * Get current buffer size
     */
    __ui64 getBufferSize();

    /**
     * Get current buffer
     */
    const char* getBuffer();

    /**
     * Get as String
     */
    String toString();
  };
};

#endif /* __XEM_IO_BUFFEREDWRITER_H_ */
