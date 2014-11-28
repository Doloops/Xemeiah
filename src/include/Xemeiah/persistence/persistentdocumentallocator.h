#ifndef __XEM_PERSISTENCE_PERSISTENTDOCUMENTALLOCATOR_H
#define __XEM_PERSISTENCE_PERSISTENTDOCUMENTALLOCATOR_H

#include <Xemeiah/kern/documentallocator.h>
#include <Xemeiah/kern/branchmanager.h>
#include <Xemeiah/kern/mutex.h>
#include <Xemeiah/persistence/persistentstore.h>
#include <Xemeiah/persistence/writablepagecache.h>

#define __XEM_PERSISTENTDOCUMENTALLOCATOR_HAS_PAGEINFOPAGETABLE
// #define __XEM_PERSISTENTDOCUMENTALLOCATOR_HAS_PAGEINFOPAGEPTRTABLE
#define __XEM_PERSISTENTDOCUMENTALLOCATOR_HAS_WRITABLEPAGECACHE

namespace Xem
{
    XemStdException(PageInfoException);

    /**
     * Persistent Document Allocator : Document Allocator
     */
    class PersistentDocumentAllocator : public DocumentAllocator
    {
        friend class PersistentDocument;
        friend class PersistentStore;
        friend class PageInfoIterator;
    protected:
        /**
         * Reference to our store
         */
        PersistentStore& persistentStore;

        /**
         * Qualified reference accessor to our store
         */
        PersistentStore&
        getPersistentStore () const
        {
            return persistentStore;
        }

        /**
         * Non-Qualified reference
         */
        Store&
        getStore () const
        {
            return persistentStore;
        }

        /**
         * Reference to the RevisionPage
         * Document class has a DocumentHead and FreeSegmentsHeader which must refer to this RevisionPage
         */
        AbsolutePageRef<RevisionPage> revisionPageRef;

        /**
         * Reference to the revision page
         * @return our RevisionPage
         */
        RevisionPage*
        getRevisionPage ();

        /**
         * Alter the RevisionPage
         */
        INLINE
        void
        alterRevisionPage ();

        /**
         * Protect the RevisionPage
         */
        INLINE
        void
        protectRevisionPage ();

        /**
         *
         */
        BranchRevId
        getBranchRevId ();

        /**
         * Get the BranchName
         */
        String
        getBranchName ();

        /**
         * Get the current branch's flags
         */
        BranchFlags
        getBranchFlags ();

        /**
         * allocateAreaMap grants that the areas table is large enough to match areaIdx pointer
         * @param areaIdx the minimum area index the areas table shall be allocated to
         */
        void
        allocateAreaMap (__ui64 areaIdx);

        /**
         * Preliminary call to mapArea() : maps the area from file using PersistentStore::mapArea()
         * @param areaIdx the area to map.
         * @param offset the offset to use.
         */
        void*
        mapAreaFromFile (__ui64 areaIdx, AbsolutePagePtr offset);

        /**
         * Get a SegmentPage from its relPagePtr
         */
        SegmentPage*
        getRelativePage (RelativePagePtr relPagePtr);

        /**
         * Maps the corresponding Area, minimizing calls to mapRelativePages(), LOCKLESS
         * This maps the area using mapAreaFromFile() and then calls mapRelativePages() to make the relative pages match with the file.
         * @param areaIdx the area to map.
         */
        void
        doMapArea (__ui64 areaIdx);

        /**
         * Document hook : Maps the corresponding Area, minimizing calls to mapRelativePages() ; LOCKING
         * This maps the area using mapAreaFromFile() and then calls mapRelativePages() to make the relative pages match with the file.
         * @param areaIdx the area to map.
         */
        void
        mapArea (__ui64 areaIdx);

        /**
         * Gets the area
         */
        void*
        getArea (__ui64 areaIdx);

        /**
         * Unmap all mapped areas
         */
        void
        unmapAllAreas ();

        /**
         * (Testing) Maps all the pages/areas.
         */
        void
        doMapAllPages () DEPRECATED;

        /**
         * Perform all post-instanciation initializations
         * @param revisionPagePtr the Revision Page Pointer to use
         */
        void
        doInitialize ();

        /**
         * Maps a bunch of contiguous pages, ie a linear mapping between relative and absolute pages ; frontend to remap_file_pages()
         * @param relPagePtr the first relative page pointer
         * @param absPagePtr the first absolute page pointer
         * @param count the number of contiguous pages to allocate
         */
        void
        mapRelativePages (void* area, RelativePagePtr relPagePtr, AbsolutePagePtr absPagePtr, __ui64 count);

        /**
         * returns a free page alloced by the Store.
         * each revision has a local free page cache, which is filled using Store::getFreePageList()
         * @param pageType (unused)
         * @return the absolute ptr of the page to use.x
         */
        AbsolutePagePtr
        __getFreePagePtr (PageType pageType);

#if 0
        /**
         * Map of all AbsolutePages claimed to Store
         */
        typedef std::map<AbsolutePagePtr, void*> AbsolutePages;

        /**
         * Map instance of AbsolutePages claimed
         */
        AbsolutePages absolutePages;
#endif

#if 0
        /**
         *  Access stub to AbsolutePagePtr
         */
        template<typename PageClass>
        INLINE PageClass*
        getAbsolutePage (AbsolutePagePtr absPagePtr);
#endif

        /**
         * Access stub for IndirectionPage pages
         * @param indirectionPagePtr the indirection absolute page pointer
         * @return the in-memory page pointer to the indirection page
         */
        INLINE
        AbsolutePageRef<IndirectionPage>
        getIndirectionPage (IndirectionPagePtr indirectionPagePtr);

        /**
         * Access stub for PageInfoPage pages
         * @param pageInfoPagePtr the PageInfoPage absolute page pointer
         * @return the in-memory page pointer to the PageInfoPage page
         */
        INLINE
        AbsolutePageRef<PageInfoPage>
        getPageInfoPage (AbsolutePagePtr pageInfoPagePtr);

        /**
         *
         */
        INLINE
        void
        alterPageInfo (PageInfo& pageInfo);

        /**
         *
         */
        INLINE
        void
        protectPageInfo (PageInfo& pageInfo);

        /**
         * Access stub for PageList pages
         * @param pageListPtr the PageList absolute page pointer
         * @return the in-memory page pointer to the PageList page
         */
        INLINE
        AbsolutePageRef<PageList>
        getPageList (AbsolutePagePtr pageListPtr);

        /**
         * Access stub for RevisionPage pages
         * @param revisionPagePtr the RevisionPage absolute page pointer
         * @return the in-memory page pointer to the RevisionPage page
         */
        INLINE
        AbsolutePageRef<RevisionPage>
        getRevisionPage (AbsolutePagePtr revisionPagePtr);

        /**
         * Access stub for SegmentPage pages
         * @param segmentPagePtr the SegmentPage absolute page pointer
         * @return the in-memory page pointer to the SegmentPage page
         */
        INLINE
        AbsolutePageRef<SegmentPage>
        getSegmentPage (AbsolutePagePtr segmentPagePtr);

#if 0
        /**
         * Release an absolute page ptr
         */
        INLINE
        void
        releasePage (AbsolutePagePtr absPagePtr);
#endif // 0

        /**
         * allocates a new IndirectionPage page.
         * @param reset set to true to perform a memset() on it
         * @return a brand new indirection page absolute page pointer.
         */
        IndirectionPagePtr
        getFreeIndirectionPagePtr (bool reset = false);

        /**
         * allocates a new PageInfoPage page.
         * @param reset set to true to perform a memset() on it
         * @return a brand new PageInfoPage absolute page pointer.
         */
        PageInfoPagePtr
        getFreePageInfoPagePtr (bool reset = false);

        /**
         * Get the PageInfoPage containing the provided RelativePagePtr
         * @param relativePagePtr the relative page ptr we want to see
         * @param pageInfoPage the resulting pageInfo
         * @param index the index, for this relative page, in the pageInfo
         * @param write must we steal indirections if we have to, enlarge indirection if we may to...
         * @return true on succes, false if not.
         */
        INLINE
        AbsolutePageRef<PageInfoPage>
        doGetPageInfoPage (RelativePagePtr relativePagePtr, __ui64& index, bool write );

    /**
     * Increase covered area
     */
    void increaseIndirectionCoveredArea ();

#ifdef __XEM_PERSISTENTDOCUMENTALLOCATOR_HAS_PAGEINFOPAGEPTRTABLE
                           /**
                            * PageInfoPage Pointer Table
                            */
                           typedef std::map<__ui64,PageInfoPagePtr> PageInfoPagePtrTable;

                           /**
                            * PageInfoPage Pointer Table instance
                            */
                           PageInfoPagePtrTable pageInfoPagePtrTable;
#endif // __XEM_PERSISTENTDOCUMENTALLOCATOR_HAS_PAGEINFOPAGEPTRTABLE

#ifdef __XEM_PERSISTENTDOCUMENTALLOCATOR_HAS_PAGEINFOPAGETABLE

                           class PageInfoPageItem
                           {
                           private:
                               AbsolutePageRef<PageInfoPage> pageInfoPageRef;
                               bool stolen;
                           public:
                               PageInfoPageItem(const AbsolutePageRef<PageInfoPage>& _pageInfoPageRef, bool _stolen)
                               : pageInfoPageRef(_pageInfoPageRef)
                               {
                                   stolen = _stolen;
                               }

                               AbsolutePageRef<PageInfoPage>& getPageInfoPageRef()
                               {
                                   return pageInfoPageRef;
                               }
                               bool isStolen()
                               {
                                   return stolen;
                               }
                           };
                           /**
                            * PageInfoPage Table
                            */
                           typedef std::map<RelativePagePtr, PageInfoPageItem*> PageInfoPageTable;

                           /**
                            * PageInfoPage Table instance
                            */
                           PageInfoPageTable pageInfoPageTable;
#endif // __XEM_PERSISTENTDOCUMENTALLOCATOR_HAS_PAGEINFOPAGETABLE

                           /**
                            * Get a given PageInfoPage, using caching if necessary
                            * @param indirectionOffset the index of the page to fetch considering all PageInfoPage as a linear vector : divid
                            * @param write set to true for a writable PageInfoPage, false if dont care
                            * @return the corresponding PageInfoPage, or NULL if a problem arose
                            */
                           INLINE PageInfoPagePtr getPageInfoPagePtr ( __ui64 indirectionOffset, bool write );

                           /**
                            * Fetch a given PageInfoPage
                            * @param indirectionOffset the index of the page to fetch considering all PageInfoPage as a linear vector : divid
                            * @param write set to true for a writable PageInfoPage, false if dont care
                            * @return the corresponding PageInfoPage, or NULL if a problem arose
                            */
                           PageInfoPagePtr fetchPageInfoPagePtr ( __ui64 indirectionOffset, bool write );

                           /**
                            * Fetch a given PageInfoPage from an indirection page
                            */
                           PageInfoPagePtr fetchPageInfoPagePtrFromIndirection ( IndirectionPagePtr indirectionPagePtr,
                           __ui32 currentLevel, __ui64 indirectionOffset, bool write );

#if 0
                           /**
                            * returns the PageInfo active for a given RelativePagePtr
                            * @param relativePagePtr the relative page ptr we want to see
                            * @param pageInfo the resulting (read-only !) pageInfo
                            * @return true on succes, false if not.
                            */
                           bool getPageInfo ( RelativePagePtr relativePagePtr, PageInfo& pageInfo ) DEPRECATED;
#endif

                           /**
                            * Provide a reference to a pageInfo, must be called with mapMutex locked
                            * @param relativePagePtr the relative page pointer to fetch PageInfo for
                            * @param write (optional) set to true if you want to modify this PageInfo
                            * @return a reference to the PageInfo
                            */
                           INLINE PageInfo& getPageInfo ( AbsolutePageRef<PageInfoPage> &pageInfoPageRef, __ui64 index);

                           /**
                            * Sets info inside of a PageInfo page
                            */
                           // void setPageInfo ( RelativePagePtr relativePagePtr, PageInfo& pageInfo );
                           /**
                            * returns a copy of a stolen indirection page. The copy is owned by this revision, and can be altered.
                            * @param stolenPagePtr the location of the page to copy.
                            * @return the absolut page pointer of the copy of the indirection page.
                            */
                           AbsolutePagePtr stealIndirectionPage ( AbsolutePagePtr stolenPagePtr );

                           /**
                            * returns a copy of a stolen PageInfoPage page. The copy is owned by this revision, and can be altered.
                            * @param stolenPagePtr the location of the page to copy.
                            * @return the absolut page pointer of the copy of the indirection page.
                            */
                           PageInfoPagePtr stealPageInfoPage ( PageInfoPagePtr stolenPagePtr );

                           /**
                            * returns a copy of a stolen segment page. The copy is owned by this revision, and can be altered.
                            * @param segPage the in-mem pointer of the segment page.
                            * @param pageType the corresponding page type (unused).
                            * @return the absolute index of the copy.
                            */
                           AbsolutePagePtr stealSegmentPage ( SegmentPage* segPage, PageType pageType );

                           /**
                            * Get the allocation profile of a given segment using PageInfo on its Page
                            * @param segmentPtr the relative segment pointer, which will be converted to a RelativePagePtr
                            * @return the allocation profile of the page
                            */
                           AllocationProfile getAllocationProfile ( SegmentPtr segmentPtr );

                           /**
                            * Get the first free segment offset of a given page
                            * @param relPagePtr the RelativePagePtr of the page
                            * @return the offset of the first free segment in page, or a value greater or equal to PageSize if the page has no free segment
                            */
                           __ui32 getFirstFreeSegmentOffset ( RelativePagePtr relPagePtr );

                           /**
                            * Set the first free segment offset of a given page
                            * @param relPagePtr the RelativePagePtr of the page
                            * @param offset the offset of the first free segment in page, or a value greater or equal to PageSize to mark the absence of a free segment
                            */
                           void setFirstFreeSegmentOffset ( RelativePagePtr relPagePtr, __ui32 offset );

                           /**
                            * returns the absolute index of the page pointed by a in-mem pointer.
                            * (May be deprecated).
                            */
                           AbsolutePagePtr getAbsolutePagePtr ( void* page ) DEPRECATED;

                           /**
                            * Allocates a continuous chunk of free relative relative pages
                            * @param askedNumber number of pages to allocate.
                            * @param allocedNumber number of pages allocated (shall be more than or equals to askedNumber)
                            * @param allocProfile the allocation profile of the pages to allocate
                            * @return the relative page pointer to the first page of the continuous chunk.
                            */
                           RelativePagePtr getFreeRelativePages ( __ui64 askedNumber, __ui64& allocedNumber,AllocationProfile allocProfile );

    /**
     * Promotes a page read-write
     * @param relPagePtr the relative page ptr to promote read-write.
     */
    void authorizePageWrite ( RelativePagePtr relPagePtr );

#ifdef __XEM_PERSISTENTDOCUMENTALLOCATOR_HAS_WRITABLEPAGECACHE
    /**
     * Writable page cache
     */
     WritablePageCache writablePageCache;
#endif // __XEM_PERSISTENTDOCUMENTALLOCATOR_HAS_WRITABLEPAGECACHE

    /**
     * Debug function : really check that a page in alterable, ie a authorizePageWrite() has been called ; a Bug() is issued otherwise.
     * @param page an in-memory pointer, which shall correspond to a page inside of a mapped Area
     */
    void checkPageIsAlterable ( void* page );

    /*
     * Whole pages actions
     */
    /**
     * Action to be performed on each owned page.
     * @param relPagePtr the relative page ptr of the page
     * @param arg the void* argument provided to PersistentDocument::forAllOwnedPages
     * @return if false, will stop iterating over pages.
     */
    typedef bool ( PersistentDocumentAllocator::*SegmentPageAction ) 
        ( RelativePagePtr relPagePtr, PageInfo& pageInfo, PageType pageType, bool isStolen, void* arg );

  
    /**
     * Action to be performed on each owned indirection page
     */
    typedef bool ( PersistentDocumentAllocator::*IndirectionPageAction ) 
        ( RelativePagePtr relPagePtr, AbsolutePagePtr absolutePagePtr, PageType pageType, bool isStolen, void* arg );

    /**
     * Performs an action for each page of a PageInfoPage
     * @param pageInfoPage the current PageInfoPage to work on
     * @param offset the RelativePagePtr offset of the first page in PageInfoPage
     * @param isStolen true if the PageInfoPage is part of a stolen part of the indirection tree
     * @param action the PersistentDocument::PageAction to be called for each page.
     * @param arg the argument to provide to the action function
     * @param ownedSegmentOnly skip stolen segment pages
     * @return true upon success, false otherwise
     */
    bool forPageInfoPage ( PageInfoPage* pageInfoPage, RelativePagePtr offset, bool isStolen, 
        SegmentPageAction action, void *arg, bool ownedSegmentOnly );   
        
    /**
     * Performs an action for each page of an IndirectionPage
     * @param indirPage the current IndirectionPage to work on
     * @param level the current indirection level of the IndirectionPage
     * @param offset the RelativePagePtr offset of the first page in PageInfoPage
     * @param isStolen true if the PageInfoPage is part of a stolen part of the indirection tree
     * @param indirAction the PersistentDocument::PageAction to be called for each sub-indirection page.
     * @param segmentAction the PersistentDocument::PageAction to be called for each segment page.
     * @param arg the argument to provide to the action function
     * @param ownedIndirOnly if true, skip stolen indirection pages
     * @param ownedSegmentOnly if true, skip stolen segment pages
     * @return true upon success, false otherwise
     */
    bool forIndirectionPages ( IndirectionPage* indirPage, __ui32 level, RelativePagePtr offset, bool isStolen,
        IndirectionPageAction indirAction, SegmentPageAction segmentAction, void *arg, 
        bool ownedIndirOnly, bool ownedSegmentOnly );   

    /**
     * Performs an action for each page of an IndirectionPage
     * @param indirAction the PersistentDocument::PageAction to be called for each sub-indirection page.
     * @param segmentAction the PersistentDocument::PageAction to be called for each segment page.
     * @param arg the argument to provide to the action function
     * @param ownedIndirOnly if true, skip stolen indirection pages
     * @param ownedSegmentOnly if true, skip stolen segment pages
     * @return true upon success, false otherwise
     */
    bool forAllIndirectionPages ( IndirectionPageAction indirAction, SegmentPageAction segmentAction, void *arg, 
        bool ownedIndirOnly, bool ownedSegmentOnly );

    /**
     * PersistentDocument::PageAction callback to drop pages.
     * @param relPagePtr the relative page ptr of the page
     * @param absPagePtr the absolute page ptr of the page
     * @param arg the void* argument provided to PersistentDocument::forAllOwnedPages
     * @return if false, will stop iterating over pages.
     */
    bool dropOwnedIndirectionPage ( RelativePagePtr relPagePtr, AbsolutePagePtr absPagePtr, PageType pageType, bool isStolen, void* arg );

    bool dropOwnedSegmentPage ( RelativePagePtr relPagePtr, PageInfo& pageInfo, PageType pageType, bool isStolen, void* arg );

    /**
     * Drops all owned pages of the revision.
     * This is only relevant for volatile contexts or revisions that have no revision after.
     * For complex scenarii, see Store::dropRevisions ().
     * \todo may also drop all non-owned pages ?
     */
    bool dropAllPages ();

    /**
     * Immediate drop of the revision behind this document (dangerous)
     * All contents are destroyed, and all NodeRef or XPath instanciated on this document are unusable
     */
    void drop ();
        
    /**
     * Sync the page back to storage.
     * @param page Page pointer to sync.
     */
    void syncPage ( void* page );

    /**
     * PersistentDocument::PageAction callback to sync pages.
     * @param relPagePtr the relative page ptr of the page
     * @param absPagePtr the absolute page ptr of the page
     * @param arg the void* argument provided to PersistentDocument::forAllOwnedPages
     * @return if false, will stop iterating over pages.
     */
    bool syncOwnedSegmentPage ( RelativePagePtr relPagePtr, PageInfo& pageInfo, PageType pageType, bool isStolen, void* arg );

    bool syncOwnedIndirectionPage ( RelativePagePtr relPagePtr, AbsolutePagePtr absPagePtr, PageType pageType, bool isStolen, void* arg );
    /**
     * Gives back the local cache of free pages to the Store.
     */
    bool flushFreePageList ();

    /**
     * Check all segment pages allocation
     */ 
    bool checkSegmentPage ( RelativePagePtr relPagePtr, PageInfo& pageInfo, PageType pageType, bool isStolen, void* arg );

    bool checkIndirectionPage ( RelativePagePtr relPagePtr, AbsolutePagePtr absPagePtr, PageType pageType, bool isStolen, void* arg );
    
    bool checkPages ( PersistentStore::AllocationStats& stats );
    
    bool checkRelativePages ( PersistentStore::AllocationStats& stats );
    
    /**
     * Check our contents
     */
    virtual bool checkContents ( );
   
    class ContentsCheck
    {
    public:
        __ui64 errorCount;

        ContentsCheck()
        {
            errorCount = 0;
        }
    };

    void checkPageInfos ( ContentsCheck& contentsCheck, __ui64 *nbPagesPerAllocationProfile, __ui64& totalPages);

    /**
     * Flush all in-memory caches
     */
    void flushInMemCaches ();
   
    /**
     * Release all Absolute Pages
     */
    void releaseAllAbsolutePages ();

    /**
     * Persistence Operations
     */
    /**
     * Commits the current Context (and the revision behind).
     * The Context must be openned Write, and must not be volatile.
     * After the commit, the Context remains open Read-only.
     */
    void commit ();

    /**
     * Checks if this is worth commiting.
     */
    bool mayCommit ();

    /**
     * Re-opens a previously commited Context.
     * This may fail for obvious reasons, such as a revison has been created
     * inbetween.
     */
    void reopen ( BranchFlags branchFlags );

    /**
     * Forks the current revision to a new branch, keeping all current mappings intact (conservative) ; but the revision *MUST NOT* be writable
     * @param branchName the new branch name
     * @param branchFlags the new branch's flag
     * @return true upon success, false otherwise
     */
    void fork ( String& branchName, BranchFlags branchFlags );

    /**
     * Quickly merge a branch to a document
     */
    void quickMerge ( PersistentDocumentAllocator& targetAllocator );

    /**
     * Quick merge : Indirection Page
     */
    bool quickMergeIndirectionPage ( RelativePagePtr relPagePtr, AbsolutePagePtr absPagePtr, PageType pageType, bool isStolen, void* arg );

    /**
     * Quick merge : Segment Page
     */
    bool quickMergeSegmentPage ( RelativePagePtr relPagePtr, PageInfo& pageInfo, PageType pageType, bool isStolen, void* arg );


    /**
     * Merge current document to the forked branch
     */
    void merge ( XProcessor& xproc );

    /**
     * Mutex locked when we are modifying mapped areas
     */
    Mutex mapMutex;
    void lockMutex_Map() { mapMutex.lock(); }
    void unlockMutex_Map() { mapMutex.unlock(); }

#ifdef __XEM_DOCUMENTALLOCATOR_HAS_ALLOC_MUTEX
    /**
     * Mutex locked when we are trying to allocate something
     */
    Mutex allocMutex;
    void lockMutex_Alloc() { allocMutex.lock(); }
    void unlockMutex_Alloc() { allocMutex.unlock(); }
    void assertMutexLocked_Alloc() { allocMutex.assertLocked(); }
#else
    void assertMutexLocked_Alloc() {}
#endif

    /**
     * housewife
     */
    virtual void housewife();
  public:
    /**
     * Constructor
     */
    PersistentDocumentAllocator ( PersistentStore& store, AbsolutePagePtr revisionPagePtr );

    /**
     * Destructor
     */
    ~PersistentDocumentAllocator ();
  };
  
}

#endif // __XEM_PERSISTENCE_PERSISTENTDOCUMENTALLOCATOR_H

