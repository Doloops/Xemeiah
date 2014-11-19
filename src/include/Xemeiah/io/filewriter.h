/*
 * filewriter.h
 *
 *  Created on: 6 janv. 2010
 *      Author: francois
 */

#ifndef __XEM_IO_FILEWRITER_H_
#define __XEM_IO_FILEWRITER_H_

#include <Xemeiah/io/bufferedwriter.h>

namespace Xem
{
  /**
   * Buffered output writer to a file descriptor
   */
  class FileWriter : public BufferedWriter
  {
  protected:
    /**
     * The file descriptor to use
     */
    int fd;

    /**
     * Do I own my file descriptor ?
     */
    bool ownsFD;

  public:
    /**
     * Instanciate a new FileWriter
     * @param fd the file descriptor to use
     */
    FileWriter ( int fd = -1 );

    /**
     * Destructor for FileWriter
     */
    virtual ~FileWriter ();

    /**
     * Perform flush operation
     */
    virtual void flushBuffer ();

    /**
     *
     */
    void setFD ( int fd );

    /**
     *
     */
    void setFile ( const String& path, bool allowMkdir = true );
  };
};

#endif /* __XEM_IO_FILEWRITER_H_ */
