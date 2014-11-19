#ifndef __XEM_KERN_UTF8_H
#define __XEM_KERN_UTF8_H

#include <stdio.h>
#include <stdlib.h>
#include <Xemeiah/trace.h>

/** 
 * \file Low-level UTF8 conversion
 */
 
namespace Xem
{
  /*
   * ************ LOWLEVEL **********
   */

  /**
   * Convert an encoded UTF8 to its int representation
   */
  INLINE int utf8CharToCode ( const unsigned char* vl, int& bytes );
  
  /**
   * Convert a UTF8 int representation to a UTF8-encoded char*
   */
  INLINE int utf8CodeToChar ( int code, unsigned char* vl, int bytes );

  /**
   * Get the utf-8 representation of a long code
   * @return the size when serialized
   */
  INLINE int utf8CodeSize ( int code );

  /**
   * Get an utf8CharSize from its first character
   */
  INLINE int utf8CharSize ( unsigned char firstChar );

  /**
   * HTML entity to UTF-8 Code
   */
  int htmlEntityToUtf8 ( const char* entity );
   
  /**
   * UTF-8 code to HTML entity
   */
  const char* utf8ToHtmlEntity ( int utf8Code );

  /*
   * ************ HIGHER LEVEL **********
   */
  class String;
  String iso8859ToUtf8 ( const unsigned char* str, int size );
};
#endif // __XEM_KERN_UTF8_H

