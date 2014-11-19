/*
 * querydocument-parsecontents.cpp
 *
 *  Created on: 6 févr. 2010
 *      Author: francois
 */
#include <Xemeiah/webserver/querydocument.h>
#include <Xemeiah/dom/blobref.h>

#include <Xemeiah/auto-inline.hpp>

#include <errno.h>
#include <sys/mman.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define Log_QParseContents Debug

namespace Xem
{
  void QueryDocument::parseToBlob ( BlobRef& blob, __ui64 contentLength, __ui64 offset )
  {
    Log_QParseContents ( "Parsing to blob, contentLength=%llu, offset=%llu\n", contentLength, offset );
    if ( contentLength == 0 )
      {
        return;
      }
    __ui64 remains = contentLength;
    __ui64 originalOffset = offset;
    while ( remains )
      {
        unsigned char* window;
        __ui64 windowSize = blob.allowWritePiece((void**)&window, remains, offset);
        offset += windowSize;
        remains -= windowSize;
        for ( __ui64 i = 0 ; i < windowSize ; i++ )
          {
            if ( reader.isFinished() )
              throwException ( Exception, "Premature end of blob !\n" );
            int c = reader.getNextChar();
            if ( c >= 0x100 )
              {
                throwException ( Exception, "Very long UTF-8 in blob !\n" );
              }
            window[i] = (unsigned char) c;
          }
        Log_QParseContents ( "Read ok : offset=%llu, remains=%llu\n", offset, remains );
      }
    AssertBug ( originalOffset + contentLength == offset, "Invalid.\n" );
    AssertBug ( remains == 0, "Invalid.\n" );
  }

  void QueryDocument::parseChunkedToBlob ( BlobRef& blob )
  {
    __ui64 offset = 0;
    while ( true )
      {
        __ui64 chunkSize = 0;
        bool hasRead = false;
        while ( ! reader.isFinished() )
          {
            int r = reader.getNextChar();
            if ( r == '\r' )
              continue;
            if ( r == '\n' )
              {
                if ( hasRead )
                  break;
                else
                  continue;
              }
            hasRead = true;
            if ( ( '0' <= r && r <= '9' ) )
              {
                chunkSize *= 16;
                chunkSize += (r - '0');
              }
            else if ( ( 'a' <= r && r <= 'f' ) )
              {
                chunkSize *= 16;
                chunkSize += (r + 10 - 'a');
              }
            else if ( ( 'A' <= r && r <= 'F' ) )
              {
                chunkSize *= 16;
                chunkSize += (r + 10 - 'A');
              }
            else
              {
                throwException ( Exception, "Invalid character %d !\n", r );
              }
          }
        Log_QParseContents ( "ChunkSize : %llu\n", chunkSize );
        if ( chunkSize == 0 ) break;
        parseToBlob(blob, chunkSize, offset);
        offset += chunkSize;
      }
  }
};
