#ifndef __XEM_PERSISTENCE_INDIRECTION_FORMAT_H
#define __XEM_PERSISTENCE_INDIRECTION_FORMAT_H

namespace Xem
{

    /*
     * SegmentPtr and relative addressing theory :
     *
     * - All AbsolutePagePtr defined are absolute to the beginning of the store
     * (Store::store_ptr).
     * So accessing data pointed by a AbsolutePagePtr is simply done by computing the
     * offset = PagePtr * PageSize.
     *
     * - But *ALL* SegmentPtr are relative to the per-Revision page addressing
     * indirection plan, with the help of the IndirectionHeader that maps
     * relative page pointers to absolute ones.
     * That means, SegmentPtrs are stable over time ; the only thing that
     * changes from a revision to another (or for forks), is the mapping
     * between the relative page and the absolute page.
     * Relative Page pointers are chosen linearly using
     * IndirectionHeader::nextRelativePagePtr.
     *
     * In this context, a zero relative page pointer *always* refers to
     * the RevisionPage (this is a convention, and may not be asserted in code).
     *
     */

    /**
     * Page Information structure
     * Each relative page (i.e. SegmentPage) has a record in the indirection plan.
     * This record is the PageInfo structure
     */
    struct PageInfo
    {
        /**
         * absolutePagePtr refers to the absolute page offset in storage
         * It also contains page flags, ...
         * Use Indirection_FlagsMask and Indirection_PageMask to extract one of the two parts of the data.
         */
        AbsolutePagePtr absolutePagePtr;

        /**
         * Branch and Revision in which the page was created.
         * This is usefull to know when to copy the page in COW mechanism
         */
        BranchRevId branchRevId;

        /**
         * Group (segments, pages) allocation profile for this page
         */
        AllocationProfile allocationProfile;

        /**
         * First segment in the page to be marked free (> PageSize if no segment defined)
         */
        __ui32 firstFreeSegmentInPage;
    };

    /**
     * PageInfo_pointerNumber defines the number of pages handled by a single PageInfoPage
     */
#define PageInfo_pointerNumber (PageSize/sizeof(PageInfo))

    /**
     * Page Information page
     */
    struct PageInfoPage
    {
        PageInfo pageInfo[PageInfo_pointerNumber];
    };

    /**
     * Indirection Mechanism :
     * IndirectionHeader defines the head of the Indirection Mechanism.
     *
     * First level ist made up of PageInfoPage pages.
     * The upper levels (2+) are IndirectionPages to a PageInfoPage.
     *
     * Base Scenario, 1st level coverage :
     * - sizeof(PageInfo) = 32.
     * - PageInfo_pointerNumber = 4096 / 32 = 128
     * - General 1st level coverage : 128 * 4096 = 512kbytes of data.
     * - General 1st level overhead : 0.78%
     * 2nd level coverage :
     * - sizeof(AbsolutePagePtr) = 8
     * - Indirection_pointerNumber = 4096 / 8 = 512
     * - General 2nd level coverage = 512 * 128 = 65k pages, 128k * 4096 = 256Mo
     * - 128 PageInfoPage, 1 IndirectionPage = 129 pages overhead,
     * - General 2nd level overhead = 0.19%
     */

#define Indirection_pointerNumber (PageSize/sizeof(AbsolutePagePtr))

    /**
     * The IndirectionPage stores, for each level, the sub-level AbsolutePagePtr that maps
     * Use Indirection_FlagsMask and Indirection_PageMask to extract one of the two parts of the data.
     */
    struct IndirectionPage
    {
        /**
         * pointers : The pointers to the next page in indirection plan.
         * If the indirection level for *this* page is 2, then a pointer points to a PageInfoPage.
         * If the indirection level is higher, the a pointer points to an IndirectionPage, which
         * will have a (level-1) level.
         */
        AbsolutePagePtr pointers[Indirection_pointerNumber];
    };

    /**
     * We have 8 bytes to store in each PageSize
     * 8 = 2^3.
     * PageSize / 8 = 2^InPageBits - 2^3 = 2^(InPageBits-3)
     * Nota :
     * For 4096 PageSize, Indirection pages are 512, and 512 = 2^9 = 2^(12-3).
     * For 65536 PageSize, indir. pages are 8192, and 8192 = 2^13 = 2^(16-3)
     *
     * \todo we must check at run-time that Indirection_pointerNumber == (1 << Indirection_LevelShift)
     */
#define Indirection_LevelShift ((__ui64)InPageBits - 3)
    /**
     * \deprecated use a (maybe less performant but) more readable algorithm for indirection computations.
     */
#define Indirection_firstLevelMask (((__ui64)1 << Indirection_LevelShift) - 1)

    /**
     * IndirectionHeader and indirection levels :
     *
     * - At Branch/First Revision creation time, indirection is inited with an indirection page
     * and a lastRelativePagePtr = 1.
     * - At new Revision creation time, the first indirection page is inherited from the preceding
     * Revision, and marked Stolen. If the indirections must be altered, a new copy of the
     * first indirection page will be done, and the Stolen bit will be unset.
     *
     * We can assert after Revision Creation that indirection page already
     * exist.
     * lastRelativePagePtr allows us to compute the level of indirection.
     * - When level=1, firstPage directly refers to a indirection page that stores
     * page maps.
     * - When level=2, firstPage refers to a higher-level of indirection that maps
     * to indirection pages.
     * ..
     *.
     * Taking PageSize=4096 and sizeof(__ui64)=8, we have 512 page pointers per
     * derivation Page.
     *
     * - Level 1 : 512 covered pages, 2M bytes covered.
     * - Level 2 : 256k covered pages, 1G bytes covered.
     * - Level 3 : 128M covered pages, 512G bytes covered.
     * - Level 4 : 64G covered pages, 256T bytes covered.
     *
     * Level may be limited to 4 in implementation, but could be easily extended
     * if the limit is exceeded.
     *
     * Use Indirection_coveredRelativePages[] to compute the corresponding level,
     * but implementation *may* use home-made macros to speed up level
     * computation.
     *
     * When lastRelativePagePtr == Indirection_coveredRelativePages[level], then we must
     * add another level in indirection map.
     *
     * Note : lastRelativePagePtr is never shrinked. Or if it is, make sure to rebuild the
     * firstPage and indirection levels accordingly.
     */
    struct IndirectionHeader
    {
        /**
         * first page in the indirection tree.
         */
        AbsolutePagePtr firstPage;

        /**
         * Level of indirection.
         */
        __ui32 level;
    };
}
;

#endif // __XEM_PERSISTENCE_INDIRECTION_FORMAT_H
