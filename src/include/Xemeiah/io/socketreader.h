/*
 * blobreader.h
 *
 *  Created on: 5 févr. 2010
 *      Author: francois
 */

#ifndef __XEM_IO_SOCKETREADER_H
#define __XEM_IO_SOCKETREADER_H

#include <Xemeiah/io/bufferedreader.h>

namespace Xem
{
  /**
   * Socket as provided by socket()
   */
  typedef int Socket;

  /**
   * Binary stream from a socket
   */
  class SocketReader : public BufferedReader
  {
  protected:
    /**
     * The socket provided
     */
    Socket sock;

    /**
     * Size of input buffer
     */
    __ui64 bufferAlloc;
  public:

    /**
     * SocketReader constructor
     * @param sock_ the socket to read from
     * @param bufferAlloc_ the window size to read
     */
    SocketReader(Socket sock_, __ui64 bufferAlloc_ = 4096 )
    {
      sock = sock_;
      bufferAlloc = bufferAlloc_;
    }

    /**
     * SocketReader destructor
     */
    ~SocketReader()
    {
      if ( buffer ) free ( buffer );
    }

    /**
     * Fill : read from Socket
     * @return true upon success, false otherwise
     */
    virtual bool fill ()
    {
      if ( ! buffer )
        {
          buffer = (char*) malloc ( bufferAlloc );
        }
      bufferIdx = 0;
      int result = ::read(sock, buffer, bufferAlloc);
      if ( result < 0 )
        {
          Error ( "Could not read : err=%d:%s\n", errno, strerror(errno));
          return false;
        }
      if ( result == 0 )
        {
          return false;
        }
      bufferSz = (__ui64) result;
      bufferIdx = 0;
      return true;
    }
  };
};

#endif /* __XEM_IO_SOCKETREADER_H */
