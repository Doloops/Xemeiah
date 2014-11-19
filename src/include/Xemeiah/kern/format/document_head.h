#ifndef __XEM_KERN_FORMAT_CORE_TYPES_H
#error Shall include <Xemeiah/kern/format/core_types.h> first !
#endif

#ifndef __XEM_KERN_FORMAT_DOCUMENT_HEAD_H
#define __XEM_KERN_FORMAT_DOCUMENT_HEAD_H

namespace Xem
{

#define NullPage ((AbsolutePagePtr)0)
#define NullPtr ((SegmentPtr)0)

  /**
   * FreeSegment dummy KeyId (magic number to check consistency).
   */
  static const KeyId FreeSegment_dummy = 0xffffffff;

  /**
   * FreeSegment structure.
   */
  struct FreeSegment
  {
    KeyId dummy;       // 4 
    __ui32 nextInPage; // 4
    SegmentPtr last;   // 8 
    SegmentPtr next;   // 8
    __ui64 size;       // 8 
  }; // Size = 4+4+8+8+8 = 8*4 = 32 bytes.

  /**
   * Allocation Header for free segments, for a given length level
   */
  struct FreeSegmentsLevelHeader
  {
    /**
     * Hole squad
     *
     * levels 0 to levelNumber-2 are for levelSize*(level+1)
     * levelOther are for sizes > to levelSize * levelNumber-2;
     * levelNumber = 0 : 32 bytes sizeof(FreeSegment)
     * levelNumber = 1 : 64
     * levelNumber = 2 : 96
     * levelNumber = 3 : 128
     */
    static const __ui32 levelNumber = 5;
    static const __ui64 levelSize = sizeof(FreeSegment);
    static const __ui32 levelOther = levelNumber - 1;
    SegmentPtr firstFreeHole[levelNumber];
  };

#define computeFreeLevel(__level, __size ) \
  do { __level = ( __size / FreeSegmentsLevelHeader::levelSize ) - 1; \
    if ( __level > FreeSegmentsLevelHeader::levelOther ) \
      __level = FreeSegmentsLevelHeader::levelOther; \
  } while (0)


  /**
   * Header of segments for all level-based segment lists
   */
  struct DocumentAllocationHeader
  {
    /**
     * The next available relative page ptr for this revision. Always a multiple of PageSize.
     */
    RelativePagePtr nextRelativePagePtr;

    /**
     * States if the Allocation Profile allows writing or not
     */
    __ui64 writable;

    /**
     * Number of alloced (and initialized) freeListHeaders
     */
    AllocationProfile nbAllocationProfiles;

    /**
     * freeSegmentsLevelHeaders is a size-sorted list of free Segments
     * This has variable length, so take good care of what your are doing with it...
     */
    FreeSegmentsLevelHeader freeSegmentsLevelHeaders[0];
    
    /*** THERE MUST BE NOTHING ELSE AFTER freeSegmentsLevelHeaders BECAUSE SIZE IS DYNAMIC !!!! ****/
  };
  
  /**
   * Main structure for a document head, reflecting information for its purpose
   */
  struct DocumentHead
  {
    /**
     * Elements
     *
     * ElementIds are pre-reserved from the superblock.
     * This pre-reservation is inherited from revision to revision.
     * Only in case of an ElementId starvation may a revision ask for
     * more ElementIds from the SuperBlock.
     *
     * Pre-reservation starts at nextReservedElementId, and ends at
     * lastReservedElementId (including).
     */
    ElementId firstReservedElementId;
    ElementId lastReservedElementId;

    /**
     * The Element ptr to the root element of this document.
     */
    ElementPtr rootElementPtr;

    /**
     * The Element ptr to the meta element of this document.
     */
    ElementPtr metaElementPtr;

    /**
     * Total number of elements in the revision (unused).
     * \deprecated If we really need this, we have the SKMap index mechanism.
     */
    __ui64 elements;

  };

  /**
   * This defines the size of the largest segment allocatable.
   * 
   * \todo Record this in SuperBlock
   */
  static const SegmentPtr SegmentSizeMax = (1<<20);
  
  /**
   * Document instanciation flags
   */
  enum DocumentOpeningFlags
  {
    DocumentOpeningFlags_NotSet        = 0x00,
    DocumentOpeningFlags_Read          = 0x01, //< Read with an opportunity to change the document to be writable
    DocumentOpeningFlags_ExplicitRead  = 0x02, //< Read with no opportunity to write
    DocumentOpeningFlags_Write         = 0x03, //< Create a writable revision at the end of the branch (branch must not have a writable one !)
    DocumentOpeningFlags_ReuseWrite    = 0x04, //< Only take the last writable revision, fail if not existent
    DocumentOpeningFlags_AsRevision    = 0x05, //< Open as the revision is : if it is writable, open it writable, otherwise open it readable
    DocumentOpeningFlags_UnManaged     = 0x06, //< Not managed by the BranchManager : direct deletion, no cache, no reuse.
    DocumentOpeningFlags_ResetRevision = 0x07, //< Force to reset the revision
    DocumentOpeningFlags_FollowBranch  = 0x08, //< Follow branch, do not lock write if the revision was writable. Document will be shared with others
    DocumentOpeningFlags_Invalid       = 0x09
  };

};

#endif // FORMAT_DOCUMENT_HEAD
