#ifndef __XEM_PERSISTENCE_SUPERBLOCK_FORMAT_H
#define __XEM_PERSISTENCE_SUPERBLOCK_FORMAT_H

namespace Xem
{
#define XEM_SB_MAGIC "XEM_MMapFile_0"

#define __XEM_COUNT_FREEPAGES
#define XEM_SB_VERSION		 \
    "0.5.3-ui64-LE"		 \
    " flatFreePage-0.4.0-rc3"    \
    " dom-0.4.0-rc2"             \
    " relativePage-0.4.0-rc2"	 \
    " XPath-0.4.8"               \
    " SKMap-0.3.83-typed"	 \
    " Blob-skmap-0.5.3-mime"     \
    " metaElement-0.4.5"         \
    " count-freepages-0.4.0-rc3" \
    " documentHead-0.4.15"

  /**
   * Xem Store SuperBlock
   *
   * SuperBlock is the critical first entry of the storage model.
   *
   * \todo make copies of superblocks ?
   */
  struct SuperBlock
  {
    static const size_t magic_length = 32;
    char magic[magic_length];
    static const size_t version_length = 256;
    char version[version_length];
    
    __ui64 pageSize;
    __ui64 segmentSizeMax;
    __ui64 freeSegmentSize;

    /**
     * keyPage refers to the first key page in key storage.
     */
    AbsolutePagePtr keyPage;
   
    /**
     * namespacePage refers to the first namespace page in namespace storage
     */
    AbsolutePagePtr namespacePage;
   
    /**
     * Allocatable, Free and Available Pages lifecycle.
     *
     * Only Branch, Revision, Segment and Blob pages may be deleted.
     * When deleting a revision, the whole revision contents 
     * (id est all the pages that are owned by this revision 
     * and are not used by next revision) becomes available, 
     * so these pages are tracked to be free from deletion.
     * When a free page is asked, we try to find a free page
     * and mark it pre-allocated.
     *
     * \todo implement FreePageHeader cleanly, with a strong tracking 
     * mechanism of pages that are pre-allocated per-revision, to be able to
     * free them on crash recovery if the revision has not been commited.
     * On the other hand, we cannot make the SuperBlock be a bottleneck
     * in a multi-threaded (multi-branch) context, so revision committing
     * must remain fast & efficient.
     *
     * All data relative to free pages is stored in the FreePageHeader
     * free page.
     */
    AbsolutePagePtr freePageHeader;
    
    /**
     * noMansLand is the first page that never has been alloced.
     * So a pagePtr higher than noMansLand will always be wrong.
     */
    AbsolutePagePtr noMansLand;

    /**
     * lastBranchPage refers to the last created branch page.
     * Next branch created will have a branchId = lastBranchPage->branchId+1.
     * If branch lookup is a bottleneck, in-mem store may build a temporary
     * branch lookup tree.
     */
    AbsolutePagePtr lastBranch;

    /**
     * Next BranchId refers to the next usable branch Id
     */
    BranchId nextBranchId;

    /**
     * ElementIds are assigned for all branches and all revisions, so that
     * they can remain consistent in case of branch merges.
     *
     * In order to keep the SuperBlock from becoming a bottleneck, revisions
     * *must* pre-reserve element ids.
     */
    ElementId nextElementId;

    // Debug stuff
#ifdef __XEM_COUNT_FREEPAGES
    __ui64 nbFreePages;
#endif
  };
  
  /**
   * FreePageHeader : SuperBlock free page referencer.
   *
   * We must be able to provide a list of free pages to revisions
   * (pre-allocation), but keep a track of these pre-allocated free pages
   * in case of crash with non-committed revisions.
   */
  struct FreePageHeader
  {
    /**
     * List of pages that are free to use.
     */
    AbsolutePagePtr firstFreePageList; 

    /**
     * Currently built list of free pages
     * When this pageList is full, it is pushed to the firstFreePageList.
     */
    AbsolutePagePtr currentFreedPageList;  

    /**
     * Free Pages list for common purposes (branches, revisions, ...)
     */
    AbsolutePagePtr ownFreePageList; 

    /**
     * pre-allocation tracking...
     * (unused)
     */
    AbsolutePagePtr preAllocatedPageList;
    
    /**
     * Non-referenced pages list.
     * This pages list is candidate to being considered as free one day or another.
     */
    AbsolutePagePtr atticPageList;
  };
};

#endif // __XEM_PERSISTENCE_SUPERBLOCK_FORMAT_H

