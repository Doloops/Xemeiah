/*
 * utf8.cpp
 *
 *  Created on: 11 nov. 2009
 *      Author: francois
 */

#include <Xemeiah/kern/utf8.h>
#include <Xemeiah/dom/string.h>

#include <Xemeiah/auto-inline.hpp>

namespace Xem
{
  String iso8859ToUtf8 ( const unsigned char* str, int size )
  {
    int buffsz = 32;
    int buffit = 0;
    unsigned char* buff = (unsigned char*) malloc ( buffsz );

    buff[0] = '\0';
    for ( int i = 0 ; i < size && str[i] ; i++ )
      {
        int d = str[i];
        while ( buffit + 8 >= buffsz )
          {
            buffsz *= 2;
            buff = (unsigned char*) realloc(buff,buffsz);
          }
        int res = utf8CodeToChar(d,&(buff[buffit]),8);
        buffit += res;
      }
    return stringFromAllocedStr((char*)buff);
  }
};
