#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/dom/skmapref.h>

namespace Xem
{
    __INLINE bool
    DocumentAllocator::isWritable ()
    {
#if PARANOID
        AssertBug ( documentAllocationHeader, "No documentAllocationHeader !!!\n" );
#endif  
        return getDocumentAllocationHeader().writable;
    }

    __INLINE FreeSegmentsLevelHeader*
    DocumentAllocator::getFreeSegmentsLevelHeader (AllocationProfile allocProfile)
    {
#if PARANOID
        AssertBug ( documentAllocationHeader, "No declared freeSegmentsHeader !\n" );
        AssertBug ( allocProfile < getDocumentAllocationHeader().nbAllocationProfiles, "Alloc profile out of bounds : %x\n", allocProfile );
#endif    
        return &(getDocumentAllocationHeader().freeSegmentsLevelHeaders[allocProfile]);
    }

    __INLINE __ui64
    DocumentAllocator::alignSize (__ui64 size)
    {
        if (size % sizeof(FreeSegment))
        {
            size = ((size / sizeof(FreeSegment)) + 1) * sizeof(FreeSegment);
        }
        return size;
    }

    /**
     * \todo implement segmentAllocProfile fetching
     */
    __INLINE bool
    DocumentAllocator::freeSegment (SegmentPtr segPtr, __ui64 size)
    {
        AllocationProfile allocProfile = getAllocationProfile(segPtr);
        return freeSegment(segPtr, size, allocProfile);
    }

    __INLINE bool
    DocumentAllocator::freeSegment (SegmentPtr segPtr, __ui64 size, AllocationProfile allocProfile)
    {
#if 0
        if ( documentHead->allocedBytes < size )
        {
            Bug ( "Did not alloc so much space !\n" );
        }
        alterDocumentHead ();
        documentHead->allocedBytes -= size;
        protectDocumentHead ();
#endif
        /*
         * \todo : better error handling, and allocedBytes increment
         */
        markSegmentAsFree(segPtr, size, allocProfile);
        return true;
    }

}
