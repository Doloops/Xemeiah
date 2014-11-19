#ifndef __XEM_PERSISTENCE_PAGES_FORMAT_H
#define __XEM_PERSISTENCE_PAGES_FORMAT_H

namespace Xem
{
  /**
   * Simple PageList structure.
   */
  struct PageList
  {
    AbsolutePagePtr nextPage;
    __ui64 number;
    static const __ui64 maxNumber = 
      (PageSize - (sizeof(AbsolutePagePtr) + sizeof(__ui64)))
      / sizeof(AbsolutePagePtr);
    AbsolutePagePtr pages[maxNumber];
  };
    
  /**
   * Branch Pages
   * \deprecated Branch pages will be deprecated in favor of a system Revision (called MetaRevision),
   * This revision will handle all Branch and Information data (maybe).
   */
  struct BranchPage
  {
    /**
     * The branchId of this Branch.
     */
    BranchId branchId;

    /**
     * forkedFrom stores the Branch and revision from the previously 
     * forked branch.
     */
    BranchRevId forkedFrom;
    
    /**
     * Last branch in the single-linked-list of branches
     */
    AbsolutePagePtr lastBranch;

    /**
     * Last revision in the single-linked-list of revisions
     * This is also the last revision chronologically.
     */
    AbsolutePagePtr lastRevisionPage;

    /**
     * Branch flags
     */
    BranchFlags branchFlags;
     
    /**
     * Maximum size of a branch name
     */
    static const size_t name_length = 256;
    /**
     * The branch name
     */
    char name[name_length];
  };

  /**
   * PageFlags are made up of PageType, StolenBit, FreeBit and AllocatedBit.
   *
   * PageFlags are always used while dealing with the whole page, so the in-page
   * segment pointer part is used to store the page flags.
   *
   * That is why the PageFlags_Mask has the same mask than the SegmentPtr_Mask.
   */
  typedef __ui64 PageFlags;
#define PageFlags_Mask SegmentPtr_Mask

  typedef __ui64 PageType;

#define PageType_FreePageList ((__ui64)0x01)
#define PageType_FreePage     ((__ui64)0x02)
#define PageType_Segment      ((__ui64)0x04)

#define PageType_Key          ((__ui64)0x0a)
#define PageType_Branch       ((__ui64)0x0b)
#define PageType_Revision     ((__ui64)0x0c)
#define PageType_PageInfo     ((__ui64)0x0d)
#define PageType_Indirection  ((__ui64)0x0e)
#define PageType_Mask         ((__ui64)0x0f) // See RevisionPage::PageTypeNumber

#define __getPageType(__pagePtr) ((__pagePtr) & PageType_Mask)

#define PageFlags_Stolen            ((__ui64)0x10)
#define PageFlags_Free              ((__ui64)0x20)
#define PageFlags_Alloced           ((__ui64)0x40)
#define PageFlags_InheritedFlags    ( PageFlags_Stolen )

#define __inheritedPageFlags(__pagePtr) ( (__pagePtr) & PageFlags_InheritedFlags )
#define __isStolen(__pagePtr)  ( ( (__pagePtr) & PageFlags_Stolen )  == PageFlags_Stolen )
#define __isFree(__pagePtr)    ( ( (__pagePtr) & PageFlags_Free )    == PageFlags_Free )
#define __isAlloced(__pagePtr) ( ( (__pagePtr) & PageFlags_Alloced ) == PageFlags_Alloced )

};

#endif // __XEM_PERSISTENCE_PAGES_FORMAT_H
