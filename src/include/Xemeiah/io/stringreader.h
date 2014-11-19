/*
 * stringreader.h
 *
 *  Created on: 6 janv. 2010
 *      Author: francois
 */

#ifndef __XEM_IO_STRINGREADER_H_
#define __XEM_IO_STRINGREADER_H_

#include <Xemeiah/io/bufferedreader.h>
#include <Xemeiah/dom/string.h>

namespace Xem
{
  /**
   * Simple BufferedReader from a String
   */
  class StringReader : public BufferedReader
  {
  protected:
    /**
     * Fill implementation : no need to fill, we finished the String !
     */
    virtual bool fill () { return false; }
  public:
    /**
     * StringReader constructor
     * @param str the String to read
     */
    StringReader ( const String& str )
    {
      buffer = (char*) str.c_str();
      bufferSz = strlen(buffer);
    }

    /**
     * StringReader destructor
     */
    virtual ~StringReader ()
    {

    }
  };
};


#endif /* __XEM_IO_STRINGREADER_H_ */
