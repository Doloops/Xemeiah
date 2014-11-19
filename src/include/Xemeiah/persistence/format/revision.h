#ifndef __XEM_KERN_FORMAT_CORE_TYPES_H
#error Shall include <Xemeiah/core/core_types.h> first !
#endif

#ifndef __XEM_PERSISTENCE_FORMAT_REVISION_H
#define __XEM_PERSISTENCE_FORMAT_REVISION_H

#include <Xemeiah/kern/format/document_head.h>
#include <Xemeiah/kern/format/journal.h>
#include <Xemeiah/persistence/format/indirection.h>

namespace Xem
{
  /**
   * Revision Pages.
   *
   * Revisions have a set of data pages. 
   * Each data page (later refered as SegmentPage) is typed to reflect
   * the page content.
   *
   * The Blob PageType refers to a PageSize-large page with binary
   * data (large attributes, ...). These pages are not considered
   * as SegmentPages, and do not come with a page header.
   */
  struct RevisionPage
  {
    /**
     * The revision branchRevId
     */
    BranchRevId branchRevId;

    /**
     * The head information for document
     */
    SegmentPtr documentHeadPtr;

    /**
     * Indirection mechanism, that may be inherited from a previous revision
     * with the Indirection_StolenBit set.
     */
    IndirectionHeader indirection;
    
    /**
     * Journal head
     */
    JournalHead journalHead;

    /**
     * freePageList is a list of free pages.
     * It must *not* be inherited in any case.
     * And it must be freed at commitRevision().
     */
    AbsolutePagePtr freePageList;

    /**
     * Coherency and reverse mapping : pointer to the absolute page ptr of a revision
     */
    AbsolutePagePtr revisionAbsolutePagePtr;
    
    /**
     * Pointer to the last revision page
     */
    AbsolutePagePtr lastRevisionPage;

    /**
     * The time when the Revision was created
     */
    __ui64 creationTime;
    
    /**
     * The time when the Revision was committed
     */
    __ui64 commitTime;

    /**
     * The total number of owned pages
     */
    __ui64 ownedPages;

    /**
     * Total number of pages types
     */
    static const __ui64 PageTypeNumber = 0x10;
    
    
    /**
     * The total number of owned pages, by type
     */
    __ui64 ownedTypedPages[PageTypeNumber];


    /**
     * The free segments header
     */
    DocumentAllocationHeader documentAllocationHeader;
    
    /**
     * There must be nothing after the DocumentAllocationHeader because its size is dynamic !!!!
     */
  };

};

#endif // __XEM_PERSISTENCE_FORMAT_REVISION_H
