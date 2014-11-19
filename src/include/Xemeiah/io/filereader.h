/*
 * filereader.h
 *
 *  Created on: 6 janv. 2010
 *      Author: francois
 */

#ifndef __XEM_IO_FILEREADER_H
#define __XEM_IO_FILEREADER_H

#include <Xemeiah/io/bufferedreader.h>
#include <Xemeiah/dom/string.h>

namespace Xem
{
  /**
   * Buffered input reader from a file
   */
  class FileReader : public BufferedReader
  {
  protected:
    /**
     * The file descriptor of the openned file
     */
    int fd;

    /**
     * Total file length
     */
    __ui64 fileLength;

    /**
     * The current offset in the file
     */
    __ui64 offset;

    /**
     * Read-window size
     */
    __ui64 windowSize;

    /**
     * Do I own my fd ?
     */
    bool ownsMyFd;

    /**
     * URI of the file
     */
    String uri;

    /**
     * Base URI of the file
     */
    String baseURI;

    /**
     * Initialize members
     */
    void init();

    /**
     *
     */
    virtual bool fill ();

    /**
     * Set URI
     */
    void setURI ( const String& path );

    /**
     * Set file length according to the file descriptor provided
     */
    void setFileLength ( );
  public:
    /**
     * New FileReader
     * @param fd the file descriptor to use
     */
    FileReader ( int fd );

    /**
     * New FileReader
     * @param path the path to the file to read
     */
    FileReader ( const String& path );

    /**
     * FileReader destructor
     */
    virtual ~FileReader ();

    /**
     * Get current URI
     */
    virtual String getCurrentURI () { return uri; }

    /**
     * Get base URI
     */
    virtual String getBaseURI () { return baseURI; }

  };
};

#endif // __XEM_IO_FILEREADER_H
