#ifndef __XEM_KERN_FORMAT_CORE_TYPES_H
#error Shall include <Xemeiah/kern/format/core_types.h> first !
#endif

#ifndef __XEM_KERN_FORMAT_BLOB_H
#define __XEM_KERN_FORMAT_BLOB_H

#include <Xemeiah/kern/format/skmap.h>

namespace Xem
{
  /**
   * Blob format is based on SKMap
   * - this allows for fast finding of the beginning of a blob piece
   * - piece size is computed using the next iterator size
   */
  struct BlobHeader
  {
    SKMapHeader skMapHeader;
    __ui64 totalSize;
    AllocationProfile pieceAllocationProfile;
    LocalKeyId mimeTypeId;
    LocalKeyId mimeSubTypeId;
    /*
     * BranchRevId lastUpdatedBranchRevId;
     */
  };
};

#endif // __XEM_KERN_FORMAT_BLOB_H
