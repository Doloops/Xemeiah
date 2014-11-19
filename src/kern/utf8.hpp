#include <Xemeiah/trace.h>
#include <Xemeiah/kern/exception.h>
#include <Xemeiah/kern/utf8.h>

namespace Xem
{
  __INLINE int utf8CharToCode ( const unsigned char* vl, int& bytes )
  {
    int n = 0; bytes = 1;
    if ( *vl <= 0x7F ) /* 0XXX XXXX one byte */
      {
        return (unsigned char) *vl;
      }
    if ((*vl & 0xE0) == 0xC0)  /* 110X XXXX 10XX XXXX two bytes */
      {
        n = *vl & 31;
        if ( ( vl[1] & 0xC0 ) != 0x80 ) return -1;
        n = (n<<6) | (vl[1]&0x3F); vl++;
        bytes = 2;
      }
    else if ((*vl & 0xF0) == 0xE0)  /* 1110 XXXX 10XX XXXX 10XX XXXX three bytes */
      {
        n = *vl & 15;
        if ( ( vl[1] & 0xC0 ) != 0x80 ) return -1;
        n = (n<<6) | (vl[1]&0x3F); vl++;
        if ( ( vl[1] & 0xC0 ) != 0x80 ) return -1;
        n = (n<<6) | (vl[1]&0x3F); vl++;
        bytes = 3;
      }
    else if ((*vl & 0xF8) == 0xF0 ) /* 1111 0XXX 10XX XXXX 10XX XXXX 10XX XXXX four bytes */
      {
        n = *vl & 7;
        if ( ( vl[1] & 0xC0 ) != 0x80 ) return -1;
        n = (n<<6) | (vl[1]&0x3F); vl++;
        if ( ( vl[1] & 0xC0 ) != 0x80 ) return -1;
        n = (n<<6) | (vl[1]&0x3F); vl++;
        if ( ( vl[1] & 0xC0 ) != 0x80 ) return -1;
        n = (n<<6) | (vl[1]&0x3F); vl++;
        bytes = 4;
      }
    else
      {
        /*
         * Input is not UTF8-conform, so cry a bit here
         */
        Warn ( "Long UTF8 Here ! vl=%s, firstChar=0x%x, sz=%d\n", "(disabled)" /* vl */, *vl, utf8CharSize(*vl) );
        bytes = -1;
        return -1;
      }
    return n;   
  }

  __INLINE int utf8CharSize ( unsigned char firstChar )
  {
    if (firstChar < 0x80) return 1;
    else if ((firstChar & 0xE0) == 0xC0) return 2; /* 110X XXXX 10XX XXXX two bytes */
    else if ((firstChar & 0xF0) == 0xE0) return 3; /* 1110 XXXX 10XX XXXX 10XX XXXX three bytes */
    else if ((firstChar & 0xF8) == 0xF0 ) return 4; /* 1111 0XXX 10XX XXXX 10XX XXXX 10XX XXXX four bytes */

    return -1;
  }

  __INLINE int utf8CodeSize ( int code )
  {
    if ( code < 0x80 )             return 1;
    else if ( code < 0x800 )       return 2;
    else if ( code < 0x10000 )     return 3;
    else if ( code < 0x200000 )    return 4;
    else if ( code < 0x4000000 )   return 5;

    Bug ( "Entity parsing to UTF-8 : Out of bounds code : 0x%x (%d)", code, code );
    throwException ( Exception, "Entity parsing to UTF-8 : Out of bounds code : 0x%x (%d)", code, code );
    return -1;
  }

  __INLINE int utf8CodeToChar ( int code, unsigned char* vl, int bytes )
  {
    AssertBug ( bytes, "Invalid zero bytes provided !\n" );
    if ( code < 0x80 )
      {
        if ( bytes < 2 ) return -1;
        vl[0] = (unsigned char) code; vl[1] = '\0';
        return 1;
      }
    else if ( code < 0x800 )
      {
        if ( bytes < 3 ) return -1;
        vl[0] = 0xc0 | (code >> 6); vl[1] = 0x80 | (code & 0x3f); vl[2] = '\0';
        return 2;
      }
    else if ( code < 0x10000 )
      {
        if ( bytes < 4 ) return -1;
        vl[0] = 0xe0 | (code >> 12); vl[1] = 0x80 | ((code>>6) & 0x3f); vl[2] = 0x80 | (code & 0x3f); vl[3] = '\0';
        return 3;
      }
    else if ( code < 0x200000 )
      {
        if ( bytes < 5 ) return -1;
        vl[0] = 0xf0 | (code >> 18); vl[1] = 0x80 | ((code>>12) & 0x3f); vl[2] = 0x80 | (code>>6 & 0x3f); vl[3] = 0x80 | (code & 0x3f); vl[4] = '\0';
        return 4;
      }
    else if ( code < 0x4000000 )
      {
        if ( bytes < 6 ) return -1;
        vl[0] = 0xf8 | (code >> 24); vl[1] = 0x80 | ((code>>18) & 0x3f); vl[2] = 0x80 | (code>>12 & 0x3f); vl[3] = 0x80 | (code>>6 & 0x3f); 
        vl[4] = 0x80 | (code & 0x3f); vl[5] = '\0';
        return 5;
      }
    Bug ( "Entity parsing to UTF-8 : Out of bounds code : 0x%x (%d)", code, code );
    throwException ( Exception, "Entity parsing to UTF-8 : Out of bounds code : 0x%x (%d)", code, code );
    return -1;
  }
};
