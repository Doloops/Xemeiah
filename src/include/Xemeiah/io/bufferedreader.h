/*
 * bufferedreader.h
 *
 *  Created on: 6 janv. 2010
 *      Author: francois
 */

#ifndef __XEM_IO_BUFFEREDREADER_H
#define __XEM_IO_BUFFEREDREADER_H

#include <Xemeiah/kern/format/core_types.h>
#include <Xemeiah/kern/encoding.h>
#include <Xemeiah/dom/string.h>

namespace Xem
{
  /**
   * Buffered input reader
   */
  class BufferedReader
  {
  protected:
    /**
     * The current encoding of the input stream
     */
    Encoding encoding;

    /**
     * The read window
     */
    char* buffer;

    /**
     * Current window size
     */
    __ui64 bufferSz;

    /**
     * Current window position
     */
    __ui64 bufferIdx;

    /**
     * Count number of (utf8) bytes parsed
     */
    __ui64 totalParsed;

    /**
     * Count number of lines parsed
     */
    __ui64 totalLinesParsed;

    /**
     * BufferedReader constructor
     */
    BufferedReader ();

    /**
     * Fill the buffer
     * @return true if the fill was ok, otherwise return false
     */
    virtual bool fill () = 0;

    /**
     * Get next char as a non-us-ascii char
     */
    INLINE void getNextCharHigh ( int& byte ) __FORCE_INLINE;

  public:

    /**
     * BufferedReader destructor
     */
    virtual ~BufferedReader ();

    /**
     * Set encoding
     * @param encoding the selected encoding
     */
    void setEncoding ( Encoding _encoding );

    /**
     * Get encoding
     * @return the selected encoding
     */
    Encoding getEncoding ();

    /**
     * Get next character
     */
    INLINE int getNextChar() __FORCE_INLINE;

    /**
     * Check that the reading process is finished
     */
    INLINE bool isFinished() __FORCE_INLINE;

    /**
     * Get current URI
     */
    virtual String getCurrentURI () { return ""; }

    /**
     * Get base URI
     */
    virtual String getBaseURI () { return ""; }

    /**
     * Get total number of bytes parsed
     */
    __ui64 getTotalParsed() const { return totalParsed; }

    /**
     * Get total number of lines parsed
     */
    __ui64 getTotalLinesParsed() const { return totalLinesParsed; }

    /**
     * Dump current position context
     */
    String dumpCurrentContext ();
  };
};

#endif // __XEM_IO_BUFFEREDREADER_H
