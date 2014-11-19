/*
 * blobreader.h
 *
 *  Created on: 5 févr. 2010
 *      Author: francois
 */

#ifndef __XEM_IO_BLOBREADER_H
#define __XEM_IO_BLOBREADER_H

#include <Xemeiah/io/bufferedreader.h>
#include <Xemeiah/dom/blobref.h>

namespace Xem
{
  class BlobReader : public BufferedReader
  {
  protected:
    BlobRef blobRef;
    __ui64 blobOffset;
  public:
    BlobReader(BlobRef& blobRef_)
    : blobRef(blobRef_)
    {
      blobOffset = 0;
    }

    ~BlobReader()
    {

    }

    virtual bool fill ()
    {
      __ui64 blobSize = blobRef.getSize();
      bufferSz = 0;
      bufferIdx = 0;
      if ( blobOffset >= blobSize )
        {
          return false;
        }
      bufferSz = blobRef.getPiece((void**)&buffer,SegmentSizeMax,blobOffset);
      blobOffset += bufferSz;
      return bufferSz > 0;
    }
  };
};

#endif /* __XEM_IO_BLOBREADER_H */
