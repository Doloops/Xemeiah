#ifndef __XEM_KERN_DOCUMENTALLOCATOR_H
#define __XEM_KERN_DOCUMENTALLOCATOR_H

#include <Xemeiah/trace.h>
#include <Xemeiah/kern/format/core_types.h>
#include <Xemeiah/kern/format/document_head.h>
#include <Xemeiah/kern/format/dom.h>

namespace Xem
{
  /**
   * A SegmentPage is a generic void* pointer
   */
  typedef void SegmentPage;
  class ElementRef;
  class Store;
  class Document;

  /**
   * Defines wether we want the Page ro or rw
   */
  enum PageCredentials
    {
      Read  = 0x01,
      Write = 0x02
    };

  /**
   * Class responsible for allocating Document objects (Nodes, Elements, Attributes, ...)
   */
  class DocumentAllocator
  {
  public:
    /**
     * The size in bit offset of an Area handled by this Allocator
     */
    static const __ui64 InAreaBits = 22; // 20; // 16 = 64Ko, 18=256Ko, 20=1Mo 22 = 4Mo 24=16, 26=64

    /**
     * The size in bits of an Area handled by this Allocator
     */
    static const __ui64 AreaSize = ( 1 << InAreaBits );
    static const __ui64 AreaPageMask = AreaSize - 1;
  protected:  
    /**
     * Function to force Template instanciation
     */
    void __foo__ (); 

    /**
     * Reference how much Documents use us !
     */
    __ui64 refCount;

    /**
     * Pointer to the DocumentAllocationHeader Header. 
     * This pointer *must* be aligned to PageSize and allocs PageSize bytes in memory
     */
    DocumentAllocationHeader* documentAllocationHeader;

    /**
     * Access to our documentAllocationHeader
     */
    INLINE DocumentAllocationHeader& getDocumentAllocationHeader() const { return *documentAllocationHeader; }

    /**
     * Assert the document allocator is always writable, and bypass the __authorizeWrite() stuff
     */
    bool __assertDocumentAlwaysWritable;

    /**
     * Assert that the document allocator has a Coalesce mechanism, with getFirstFreeSegmentOffset()
     */
    bool __assertDocumentHasCoalesce;

    /**
     * Document partial mmapping of the Store file.
     * The mapping is quite similar to the Store one.
     * THis mapping is based on the *relative* page address.
     */
    void** areas;
    __ui64 areasAlloced;
    __ui64 areasMapped;

    /**
     * Map a given area
     */
    virtual void mapArea ( __ui64 areaIdx ) = 0;

    /*
     * Page allocation
     */

    /**
     * Returns a set of linear free pages.
     * The first page can be located by the pointer returned.
     * @param askedNumber number of pages to allocate.
     * @param allocedNumber number of pages allocated (shall be more than or equals to askedNumber)
     * @return a free page, with a whole bunch after, or NullPtr on error.
     */
    virtual RelativePagePtr getFreeRelativePages ( __ui64 askedNumber,  __ui64& allocedNumber, AllocationProfile allocProfile ) = 0;
  
    /*
     * Segment allocation
     */

    /**
     * finds the allocation header from the allocation Profile provided.
     * @param allocProfile the profile index
     * @return the freeListHeader corresponding to the allocProfile index.
     */
    FreeSegmentsLevelHeader* getFreeSegmentsLevelHeader ( AllocationProfile allocProfile );

    /**
     * Get free segment from hole
     * @param size the size of segment to allocate
     * @param allocProfile the allocation profile to use
     * @param fslHeader FreeSegmentsLevelHeader to use
     * @param level the size level
     */
    SegmentPtr getFreeSegmentPtrFromHoles ( __ui64 size, 
      AllocationProfile allocProfile,
      FreeSegmentsLevelHeader* fslHeader, __ui32 level );
      
    /**
     * Try to allocate a segment from the holes in the owned pages
     * @param size the (aligned) size of the segment to allocate
     * @param allocProfile the allocation profile of the segment to allocate
     * @return the segment pointer, or null if failed
     */
    SegmentPtr getFreeSegmentPtrFromHoles ( __ui64 size, AllocationProfile allocProfile );
  
    /**
     * Allocate a segment from newly created pages
     * @param size the (aligned) size of the segment to allocate
     * @param allocProfile the allocation profile of the segment to allocate
     * @return the segment pointer, or null if failed (but may not fail)
     */
    SegmentPtr getFreeSegmentPtrFromNewPages ( __ui64 size, AllocationProfile allocProfile );

    /**
     * Get the first free segment offset of a given page
     * @param relPagePtr the relative page pointer of the segment page
     * @return the first free segment offset defined from the beginning of the page, > PageSize if no segment in page
     */
    virtual __ui32 getFirstFreeSegmentOffset ( RelativePagePtr relPagePtr );

    /**
     * Set the first free segment offset of a given page
     * @param relPagePtr the relative page pointer of the segment page
     * @param offset the first free segment offset defined from the beginning of the page, > PageSize if no segment in page
     */
    virtual void setFirstFreeSegmentOffset ( RelativePagePtr relPagePtr, __ui32 offset );

    /**
     * Mark a (supposed-to-be non-allocated) virgin segment as being free for now on (does not increment DocumentHead::allocedBytes)
     */
    void markSegmentAsFree ( SegmentPtr segPtr, __ui64 size, 
        AllocationProfile allocProfile );

    /**
     * markSegmentAsFree helper function : try to coalesce the segment with left and right free segments
     * @param segPtr the segment pointer to coalesce, will be changed to point at the left free segment if found
     * @param size the segment size to coalesce, will be changed if a coalescing is found
     * @param allocProfile the allocation profile of the impacted pages
     */
    void coalesceFreeSegment ( SegmentPtr& segPtr, __ui64& size, AllocationProfile allocProfile );
    
    /**
     * Insert the provided segment in the in-page sorted list of free segments
     * @param segPtr the pointer of the free segment to insert
     * @param size the size of the segment to add
     * @param freeSegment the free segment corresponding to the segPtr
     * @param allocProfile the allocation profile of the impacted pages
     */
    void insertFreeSegmentInPageList ( SegmentPtr segPtr, __ui64 size, FreeSegment* freeSegment, AllocationProfile allocProfile );
 
    /**
     * Mark a (supposed-to-be free) segment as not being free anymore
     */
    void unmarkSegmentAsFree ( SegmentPtr segPtr, AllocationProfile allocProfile );

    INLINE void unmarkSegmentAsFree ( SegmentPtr segPtr, AllocationProfile allocProfile,
        FreeSegmentsLevelHeader* fslHeader, __ui32 freeLevel ) __FORCE_INLINE;

    /**
     * Free a given segment.
     * No special control is done, except that the segPtr and the segment after are on legal pages.
     * @param segPtr the (relative) segment ptr.
     * @param size the size of the segment, as provided in the getFreeSegment() calls.
     * @param allocProfile the allocation profile of the segment to delete, which shall be the allocation profile of the page selected.
     */
    INLINE bool freeSegment ( SegmentPtr segPtr, __ui64 size, AllocationProfile allocProfile );

    /*
     * Segment Retrieving
     */

    /**
     * returns a pointer to the segmentPage of a given segment.
     * Each page is PageSize-aligned, so we can easily catch the SegmentPage header
     * from an in-mem segment.
     */
    INLINE SegmentPage* __getSegmentPage ( void* seg ) __FORCE_INLINE;

    /**
     * returns a read-only in-mem pointer to a segment.
     * If the segment must be altered, call authorizeWrite() on this segment, 
     * otherwise write will be illegal if the page is not owned by this revision.
     * @param segPtr the relative index of the segment.
     * @param size the size of the segment to fetch.
     * @return a read-only pointer to the segment.
     */
    INLINE void* __getSegment_Read ( SegmentPtr segPtr, __ui64 size ) __FORCE_INLINE;

    /**
     * Check at run-time the correctness of size computation for known structs.
     */
    template<typename T> __ui64 getFixedSegmentSize ( );
    
    /**
     * Effectively Checks that a in-mem segment is read/write.
     * For each page of the segment (a segment can be stored accross multiple pages)
     * the authorizePageWrite() will be called.
     * @param segPtr the relative pointer to the segment.
     * @param size the size of the segment to write to.
     */
    INLINE void __authorizeWrite ( SegmentPtr segPtr, __ui64 size ) __FORCE_INLINE;

    /**
     * Promotes a page read-write.
     * @param relPagePtr the page to promote read-write
     */
    virtual void authorizePageWrite ( RelativePagePtr relPagePtr ) = 0;

#ifdef XEM_MEM_PROTECT_SYS
    INLINE void alterSegmentPage ( void* page );
    INLINE void protectSegmentPage ( void* page );
    
    INLINE void __alter ( void* seg, __ui64 size );
    INLINE void __protect ( void* seg, __ui64 size );

    INLINE void alterFreeSegmentsLevelHeader ( AllocationProfile allocProfile );
    INLINE void protectFreeSegmentsLevelHeader ( AllocationProfile allocProfile );

    INLINE void alterDocumentAllocationHeader ();
    INLINE void protectDocumentAllocationHeader ();    
#else
    INLINE void alterSegmentPage ( void* page ) {}
    INLINE void protectSegmentPage ( void* page ) {}
    
    INLINE void __alter ( void* seg, __ui64 size ) {}
    INLINE void __protect ( void* seg, __ui64 size ) {}

    INLINE void alterFreeSegmentsLevelHeader ( AllocationProfile allocProfile ) {}
    INLINE void protectFreeSegmentsLevelHeader ( AllocationProfile allocProfile ) {}

    INLINE void alterDocumentAllocationHeader () {}
    INLINE void protectDocumentAllocationHeader () {}
#endif

    virtual void checkPageIsAlterable ( void* page ) {}
      
    virtual void lockMutex_Map() {}
    virtual void unlockMutex_Map() {}
      
    virtual void lockMutex_Alloc() {}
    virtual void unlockMutex_Alloc() {}

    DocumentAllocator ( Store& store );
    
    /**
     * Initialize Document allocation header
     */
    void initDocumentAllocationHeader ( AllocationProfile nbAllocationProfiles );

  public:
    /**
     * Destructor
     */
    virtual ~DocumentAllocator ();

    /**
     * Are we allowed to write ?
     */
    INLINE bool isWritable();

    /**
     * Get my refCount
     * @return the number of Document which uses this DocumentAllocator
     */
    __ui64 getRefCount() const { return refCount; }

    /**
     * Increment refCount, registering this Document
     */
    void incrementRefCount ( Document& document );

    /**
     * Increment refCount, registering this Document
     */
    void decrementRefCount ( Document& document );

    /**
     * Get the current allocated size
     */
    RelativePagePtr getNextRelativePagePtr() { return getDocumentAllocationHeader().nextRelativePagePtr; }

    /**
     * Align the requested segment size to a size we can allocate (ie sizeof(FreeSegment))
     * @param requestedSize the requested size to allocate
     * @return the aligned size used for allocation, as the next minimum multiple of sizeof(FreeSegment)
     */
    INLINE static __ui64 alignSize ( __ui64 requestedSize );

    /**
     * allocates a (writable) segment in the revision.
     * @param size the size (in bytes) of the segment to allocate
     * @param allocProfile the allocation profile to be used for allocation.
     * @return a pointer to the alloced segment. The pointer is writable, but protected.
     */
    SegmentPtr getFreeSegmentPtr ( __ui64 size, AllocationProfile allocProfile );

    /**
     * Free a given segment.
     * No special control is done, except that the segPtr and the segment after are on legal pages.
     * @param segPtr the (relative) segment ptr.
     * @param size the size of the segment, as provided in the getFreeSegment() calls.
     */
    INLINE bool freeSegment ( SegmentPtr segPtr, __ui64 size );

    /**
     * Obtain the AllocationProfile affinity for an element to be created.
     *
     * @param father Father node of the element to-be-created.
     * @param keyId KeyId of the elment to-be-created.
     * @return the resulting AllocationProfile
     */
    AllocationProfile getAllocationProfile ( ElementRef& father, KeyId keyId );

    /**
     * Gets the AllocationProfile of a given (relative) pointer
     * @param segmentPtr the segment pointer
     * @return the allocation profile of the segment pointer
     */
    virtual AllocationProfile getAllocationProfile ( SegmentPtr segmentPtr ) = 0;

    /**
     * Generic template segment accessor, with T being the type of segment to be returned,
     * which will be pointered (i.e. T=ElementSegment will return a ElementSegment* object).
     * how is the credentials to be used (Read or Write)
     * @param segPtr the relative pointer to the segment.
     * @param size the size of the segment.
     * @return a typed-pointer to the segment.
     */
    template<typename T, PageCredentials how> 
    INLINE T* getSegment ( SegmentPtr segPtr, __ui64 size ) __FORCE_INLINE;

    /**
     * Segment lookup, with T being the type of segment pointer to be returned
     * @param segPtr the SegmentPtr to lookup
     * @return a valid pointer to this segment
     */
    template<typename T, PageCredentials how> INLINE T* getSegment ( SegmentPtr segPtr ) // __FORCE_INLINE;
    { return getSegment<T, how> ( segPtr, getFixedSegmentSize<T> () ); }
    
    /**
     * Segment page accessor frontend
     */
    template<typename T> INLINE SegmentPage* getSegmentPage ( T* seg )
    { return __getSegmentPage ( seg ); }

    /**
     * ElementSegment frontend to DocumentAllocator::getSegment()
     * @param segPtr the relative SegmentPtr pointer to the segment.
     * @return a pointer to the ElementSegment structure
     */
    template<PageCredentials how>
    INLINE ElementSegment* getElement ( SegmentPtr segPtr )
    { return getSegment<ElementSegment,how> ( segPtr ); }

    /**
     * Alter a segment : template frontend
     * @param seg the segment to alter
     * @param size the segment size to alter
     */
    template<typename T> INLINE void alter ( T* seg, __ui64 size )   { __alter ( (void*) seg, size ); }

    /**
     * Protect a segment : template frontend
     * @param seg the segment to protect
     * @param size the segment size to protect
     */
    template<typename T> INLINE void protect ( T* seg, __ui64 size ) { __protect ( (void*) seg, size ); }

    /**
     * Alter a segment with a known structure size
     * @param seg the pointer to the structure
     */
    template<typename T> INLINE void alter ( T* seg )   { alter ( seg, getFixedSegmentSize<T> () ); }

    /**
     * Protected a segment with a known structure size
     * @param seg the pointer to the structure
     */
    template<typename T> INLINE void protect ( T* seg ) { protect ( seg, getFixedSegmentSize<T> () ); }

    /**
     * Authorize a given segment for writing, performing a COW if necessary
     * @param segPtr the segment pointer
     * @param size the segment size
     */
    void authorizeWrite ( SegmentPtr segPtr, __ui64 size ) { __authorizeWrite ( segPtr, size ); }

    /**
     * Authorize a given segment for writing, with a known structure size
     * @param segPtr the segment pointer
     * @param seg the segment structure, only provided to fetch the segment structure size
     */
    template<typename T> void authorizeWrite ( SegmentPtr segPtr, T* seg )
    { __authorizeWrite ( segPtr, getFixedSegmentSize<T> () ); }

    /**
     * Housewife in-memory temporary caches
     */
    virtual void housewife () {}

    /**
     * Misc stats : get number of areas alloced
     */
    __ui64 getNumberOfAreasAlloced () const { return areasAlloced; }
    
    /**
     * Misc stats : get number of areas mapped
     */
    __ui64 getNumberOfAreasMapped () const { return areasMapped; }
    
    /**
     * Misc stats : get area size
     */
    static __ui64 getAreaSize () { return AreaSize; }
    
    __ui64 getTotalDocumentSize () const { return documentAllocationHeader->nextRelativePagePtr; }

    /**
     * Check contents default function : do not check anything
     */
    virtual bool checkContents ( ) { return true; }
  };
};

#endif // KERN_DOCUMENTALLOCATOR

