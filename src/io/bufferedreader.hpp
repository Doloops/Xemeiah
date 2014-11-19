/*
 * bufferedreader.hpp
 *
 *  Created on: 12 janv. 2010
 *      Author: francois
 */
#define Log_BufferedReaderHPP Debug

namespace Xem
{
  __INLINE bool BufferedReader::isFinished()
  {
    if ( bufferIdx < bufferSz ) return false;
    return !fill();
  }

  __INLINE int BufferedReader::getNextChar()
  {
#if PARANOID
    AssertBug ( buffer, "No buffer !\n" );
    AssertBug ( bufferIdx < bufferSz, "Reached end of buffer !\n" );
#endif

    int byte = ((unsigned char*)buffer)[bufferIdx];

#if PARANOID
    AssertBug ( byte >= 0, "Invalid negative byte !\n" );
#endif

    if ( byte >= 0x80 )
      {
        getNextCharHigh ( byte );
      }
    else
      {
        bufferIdx++;
      }
    if ( byte == '\n' )
      {
        totalLinesParsed++;
      }
    totalParsed ++;
    return byte;
  }

  __INLINE void BufferedReader::getNextCharHigh ( int& byte )
  {
    switch ( encoding )
    {
    case Encoding_Unknown:
      {
        /*
         * First, try to eliminate known problematic character encodings
         */
        if ( bufferIdx + 4 < bufferSz )
          {
            unsigned const char* s = (unsigned const char*) &(buffer[bufferIdx]);
            if ( s[0] == 0xC3 && s[1] == 0x83 && s[2] == 0xC2 && s[3] && 0xA9 )
              {
                byte = 233;
                bufferIdx += 4;
                return;
              }
          }
      }
    case Encoding_UTF8:
      {
        int nb_bytes = utf8CharSize ( byte );
        if ( nb_bytes == -1 )
          {
            // throwException ( Exception, "Could not parse UTF-8 size !\n" );
            Warn ( "Could not parse UTF-8 size (byte=%d, bufferIdx=%llu) !\n", byte, bufferIdx );
            bufferIdx++;
            // byte = '?';
            break;
          }
        if ( bufferIdx + nb_bytes > bufferSz )
          {
            Warn ( "Multi-byte UTF-8 character crosses boundaries ! nb_bytes=%d\n", nb_bytes );
            bufferIdx++;
            break;
          }
        int parsedByte = utf8CharToCode ( &(((unsigned char*)buffer)[bufferIdx]), nb_bytes );
        if ( parsedByte == -1 )
          {
            // throwException ( Exception, "Could not parse UTF-8 size !\n" );
            Warn ( "Could not parse UTF-8 (byte=%d, bufferIdx=%llu) !\n", byte, bufferIdx );
            if ( encoding == Encoding_Unknown )
              {
                Warn ( "Falling back to Encoding_ISO_8859_1 (char=%d, %c).\n", byte, byte );
                encoding = Encoding_ISO_8859_1;
              }
            else
              {
                parsedByte = '?';
              }
            bufferIdx++;
            break;
          }
        byte = parsedByte;
        bufferIdx += nb_bytes;
        Log_BufferedReaderHPP ( "=> Jump to %llu (nb_bytes=%d)\n", bufferIdx, nb_bytes );
      }
      break;
    case Encoding_ISO_8859_1:
      /*
       * Nothing to do, we already converted this.
       */
      bufferIdx++;
      break;
    default:
      bufferIdx++;
      break;
    }
  }


};
