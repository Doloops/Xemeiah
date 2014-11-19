/*
 * encoding.cpp
 *
 *  Created on: 6 janv. 2010
 *      Author: francois
 */

#include <Xemeiah/kern/encoding.h>
#include <Xemeiah/dom/string.h>
#include <Xemeiah/trace.h>
#include <Xemeiah/kern/exception.h>

#include <Xemeiah/auto-inline.hpp>

#include <string.h>

namespace Xem
{
  Encoding ParseEncoding ( const String encodingName_ )
  {
    Encoding encoding;
    String encodingName = stringToUpperCase(encodingName_);

    if ( encodingName == "UTF-8" ) encoding = Encoding_UTF8;
    else if ( encodingName == "ISO-8859-1" ) encoding = Encoding_ISO_8859_1;
    else if ( encodingName == "US-ASCII" ) encoding = Encoding_US_ASCII;
    else if ( encodingName == "SHIFT_JIS" || encodingName == "BIG5" || encodingName == "ISO-2022-JP" )
      {
        encoding = Encoding_UTF8;
      }
    else
      {
        Warn ( "Encoding not handled : '%s'\n", encodingName.c_str() );
        // encodingName = "UTF-8";
        // encoding = Encoding_UTF8;
        encoding = Encoding_Unknown;
      }
    return encoding;
  }
};
