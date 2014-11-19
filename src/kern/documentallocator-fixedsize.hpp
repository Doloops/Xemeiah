#include <Xemeiah/trace.h>
#include <Xemeiah/kern/document.h>
#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/kern/format/skmap.h>
#include <Xemeiah/kern/format/blob.h>
#include <Xemeiah/kern/format/journal.h>

namespace Xem
{
#define KnownFixedSegmentSize(__type) \
  template<> __ui64 __INLINE DocumentAllocator::getFixedSegmentSize<__type> () \
  { return sizeof(__type); }

    KnownFixedSegmentSize(FreeSegment)
    KnownFixedSegmentSize(ElementSegment)
    KnownFixedSegmentSize(AttributeSegment)
    KnownFixedSegmentSize(SKMapHeader)
    KnownFixedSegmentSize(BlobHeader)
    KnownFixedSegmentSize(SKMapList)
    KnownFixedSegmentSize(DocumentHead)
    
    KnownFixedSegmentSize(JournalItem)
    KnownFixedSegmentSize(JournalHead)    
#undef KnownFixedSegmentSize    

#define UnKnownFixedSegmentSize(__type) \
  template<> __INLINE __ui64 DocumentAllocator::getFixedSegmentSize<__type> () \
  { Bug ( "." ); return 0; }
    
    UnKnownFixedSegmentSize(__ui64)
    UnKnownFixedSegmentSize(__ui32)
    UnKnownFixedSegmentSize(void)
#undef UnKnownFixedSegmentSize
};


