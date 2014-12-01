#include <Xemeiah/kern/documentallocator.h>
#include <Xemeiah/dom/elementref.h>

#include <Xemeiah/trace.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_DA Debug
#define Log_GF Debug
#define Log_GF_A Debug

namespace Xem
{
    DocumentAllocator::DocumentAllocator (Store& store)
    {
        Log_DA ( "New DA at %p\n", this );
        __assertDocumentAlwaysWritable = false;
        __assertDocumentHasCoalesce = true;
        documentAllocationHeader = NULL;

        areas = NULL;
        areasAlloced = 0;
        areasMapped = 0;
        refCount = 0;
    }

    DocumentAllocator::~DocumentAllocator ()
    {
        Log_DA ( "Delete DA at %p\n", this );
        AssertBug(refCount == 0, "Deleting with refCount=%llu\n", refCount);
    }

    void
    DocumentAllocator::initDocumentAllocationHeader (AllocationProfile nbAllocationProfiles)
    {
        alterDocumentAllocationHeader();
        getDocumentAllocationHeader().writable = true;
        getDocumentAllocationHeader().nbAllocationProfiles = nbAllocationProfiles;
        getDocumentAllocationHeader().nextRelativePagePtr = 0; // PageSize;
        for (AllocationProfile p = 0; p < getDocumentAllocationHeader().nbAllocationProfiles; p++)
            memset(&(getDocumentAllocationHeader().freeSegmentsLevelHeaders[p]), 0, sizeof(FreeSegmentsLevelHeader));
        protectDocumentAllocationHeader();
    }

    void
    DocumentAllocator::incrementRefCount (Document& document)
    {
        refCount++;
    }

    void
    DocumentAllocator::decrementRefCount (Document& document)
    {
        AssertBug(refCount, "Null refCount !\n");
        refCount--;
    }

    /**
     * \todo Implement this on a rule-based mechanism, not a hard-coded one !
     */
    AllocationProfile
    DocumentAllocator::getAllocationProfile (ElementRef& father, KeyId keyId)
    {
        AssertBug(getDocumentAllocationHeader().nbAllocationProfiles, "No free list header defined ???\n");
        if (true || getDocumentAllocationHeader().nbAllocationProfiles == 1)
        {
            return 0;
        }

        AllocationProfile fatherProfile = getAllocationProfile(father.getElementPtr());
        AllocationProfile allocProfile = fatherProfile;

        allocProfile = (fatherProfile + 1) % getDocumentAllocationHeader().nbAllocationProfiles;
#if 0
        if ( father.getElementId() % 2 )
        {
            allocProfile = (fatherProfile + 1) % (0x10);
        }
#endif
#if 0

        if ( father.getKeyId() == store.getKeyCache().builtinKeys.xem_folder() )
        allocProfile = 2;
        else if ( father.getKeyId() == store.getKeyCache().builtinKeys.xem_file() )
        allocProfile = 3;
        else
        {
            allocProfile = fatherProfile;
        }
        Log_DA ( "Alloc Profile : father %s (alloc %u), key %s -> alloc %u\n",
                father.getKey(), fatherProfile,
                store.getKeyCache().getKey ( keyId ), allocProfile );
#endif
        return allocProfile;
    }

    __ui32
    DocumentAllocator::getFirstFreeSegmentOffset (RelativePagePtr relPagePtr)
    {
        AssertBug(__assertDocumentHasCoalesce, "! DocumentHasCoalesce !");
        return PageSize * 2;
    }

    void
    DocumentAllocator::setFirstFreeSegmentOffset (RelativePagePtr relPagePtr, __ui32 offset)
    {
        AssertBug(__assertDocumentHasCoalesce, "! DocumentHasCoalesce !");
        Log_GF ( "[NOTHING TO DO] setFirstFreeSegmentOffset() relPagePtr(page=%llx)= %x\n", relPagePtr, offset );
    }

    SegmentPtr
    DocumentAllocator::getFreeSegmentPtr (__ui64 size, AllocationProfile allocProfile)
    {
        if (!isWritable())
        {
            throwException(Exception, "Document not writable !!!\n");
        }
        if (size > SegmentSizeMax)
        {
            throwException(Exception, "Asked segment size is too large : 0x%llx (maximum : 0x%llx)\n", size,
                           SegmentSizeMax);
        }
        /*
         * Normalize size : we always allocate multiples of FreeSegment, ie 32 bytes of data
         */
        size = alignSize(size);

        /*
         * Normalize allocProfile : if we are higher than the nbFreeListHeaders, then modulo it.
         */
        allocProfile %= getDocumentAllocationHeader().nbAllocationProfiles;

        Log_GF ( "Allocating %llu (0x%llx) bytes, allocProfile=0x%x\n", size, size, allocProfile );
        lockMutex_Alloc();

        /**
         * First, try to use holes to allocate this segment
         */
        SegmentPtr ptr = getFreeSegmentPtrFromHoles(size, allocProfile);

        if (!ptr)
        {
            /**
             * Allocating from holes failed. Allocate new pages.
             */
            ptr = getFreeSegmentPtrFromNewPages(size, allocProfile);
        }

#if 0
        alterDocumentHead ();
        documentHead->allocedBytes += size;
        protectDocumentHead ();
#endif
        unlockMutex_Alloc();
        Log_GF_A ( "Allocated %llu (0x%llx) bytes, allocProfile=0x%x at ptr=0x%llx\n", size, size, allocProfile, ptr );
        return ptr;
    }

#if 0
    void DocumentAllocator::log ()
    {
        Log_GF ( "----------\n" );
        for ( AllocationProfile prof = 0; prof < getDocumentAllocationHeader().nbAllocationProfiles; prof++ )
        for ( __ui32 level = 0; level < FreeSegmentsLevelHeader::levelNumber; level++ )
        Log_GF ( "firstFree %x/%x = %llx\n", prof, level, getFreeSegmentsLevelHeader(prof)->firstFreeHole[level] );
        Log_GF ( "----------\n" );
    }
#endif

    void
    DocumentAllocator::markSegmentAsFree (SegmentPtr segPtr, __ui64 size, AllocationProfile allocProfile, bool shouldCoalesce)
    {
#if PARANOID
        if (!isWritable())
        {
            throwException(Exception, "Document not writable !!!\n");
        }
#endif
        Log_GF ( "freeSegment : freeing segPtr=0x%llx, size=0x%llx, allocProfile=0x%x\n",
                segPtr, size, allocProfile );

        size = alignSize(size);

        AssertBug ( (segPtr % sizeof(FreeSegment)) == 0,
                "Invalid segptr %p (not aligned with FreeSegment %lu)\n",
                (void*) segPtr, (unsigned long) sizeof(FreeSegment) );

        Log_GF ( "freeSegment : seg=0x%llx, size=%llu (0x%llx), profile=%x\n",
                segPtr, size, size, allocProfile );

        /*
         * First, compute coalescion for this segment
         */
        if (shouldCoalesce && __assertDocumentHasCoalesce)
        {
            coalesceFreeSegment(segPtr, size, allocProfile);
        }

        /*
         * Then, update the per-level linked-list of free segments
         */
        __ui32 level;
        computeFreeLevel(level, size);

        Log_GF ( "freeSegment : effective free segment at ptr=%llx, "
                "size=%llx, level=%x, firstFreeSegment=%llx\n",
                segPtr, size, level,
                getFreeSegmentsLevelHeader(allocProfile)->firstFreeHole[level] );

        FreeSegment* freeSeg = getSegment<FreeSegment, Write>(segPtr);

        if (getFreeSegmentsLevelHeader(allocProfile)->firstFreeHole[level])
        {
            FreeSegment* previousFreeSeg = getSegment<FreeSegment, Write>(
                    getFreeSegmentsLevelHeader(allocProfile)->firstFreeHole[level]);
            alter(previousFreeSeg);
            previousFreeSeg->last = segPtr;
            protect(previousFreeSeg);
        }
        Log_GF ( "Alterating %p (0x%llx)\n", freeSeg, segPtr );
        alter(freeSeg);
        freeSeg->dummy = FreeSegment_dummy;
        freeSeg->last = NullPtr;
        freeSeg->next = getFreeSegmentsLevelHeader(allocProfile)->firstFreeHole[level];
        freeSeg->size = size;
        freeSeg->nextInPage = PageSize;
        protect(freeSeg);

        Log_GF ( "(%p) Setting first free hole for %x/%x -> %llx\n", this, allocProfile, level, segPtr );
        alterFreeSegmentsLevelHeader(allocProfile);
        getFreeSegmentsLevelHeader(allocProfile)->firstFreeHole[level] = segPtr;
        protectFreeSegmentsLevelHeader(allocProfile);

        /*
         * Finally, update the in-page linked-list of free segments accordingly
         */
        if (__assertDocumentHasCoalesce)
        {
            insertFreeSegmentInPageList(segPtr, size, freeSeg, allocProfile);
        }
    }

    void
    DocumentAllocator::unmarkSegmentAsFree (SegmentPtr segPtr, AllocationProfile allocProfile,
                                            FreeSegmentsLevelHeader* fslHeader, __ui32 freeLevel)
    {
        FreeSegment* freeSeg = getSegment<FreeSegment, Read>(segPtr);
        AssertBug(freeSeg->dummy == FreeSegment_dummy, "Invalid dummy.\n");

        if (fslHeader->firstFreeHole[freeLevel] == segPtr)
        {
            /*
             * We are using the first free hole in the list
             */
            AssertBug(freeSeg->last == NullPtr, "Invalid last !\n");
            Log_GF ( "Found at %llx, setting firstFreeHole(%x,%x) = %llx\n", segPtr, allocProfile, freeLevel, freeSeg->next );
            alterFreeSegmentsLevelHeader(allocProfile);
            fslHeader->firstFreeHole[freeLevel] = freeSeg->next;
            protectFreeSegmentsLevelHeader(allocProfile);
        }
        else
        {
            /*
             * We are using one hole in the list, use ->last to update
             */
            AssertBug(freeSeg->last, "Invalid last !\n");
            FreeSegment* lastSeg = getSegment<FreeSegment, Write>(freeSeg->last);
            AssertBug(lastSeg->next == segPtr, "Invalid last->next !\n");
            alter(lastSeg);
            lastSeg->next = freeSeg->next;
            protect(lastSeg);
        }
        if (freeSeg->next)
        {
            FreeSegment* nextSeg = getSegment<FreeSegment, Write>(freeSeg->next);
            alter(nextSeg);
            nextSeg->last = freeSeg->last;
            protect(nextSeg);
        }

        if (__assertDocumentHasCoalesce)
        {

            /*
             * Then, we have to update the per-page first segment pointer
             */
            RelativePagePtr basePagePtr = segPtr & PagePtr_Mask;
            __ui32 offset = getFirstFreeSegmentOffset(basePagePtr);
            SegmentPtr lastInPageSegPtr = NullPtr;

            while (offset < PageSize)
            {
                SegmentPtr inPageSegPtr = basePagePtr + offset;
                FreeSegment* inPageSeg = getSegment<FreeSegment, Read>(inPageSegPtr);

                if (inPageSegPtr == segPtr)
                {
                    if (lastInPageSegPtr)
                    {
                        FreeSegment* lastInPageSeg = getSegment<FreeSegment, Write>(lastInPageSegPtr);
                        alter(lastInPageSeg);
                        lastInPageSeg->nextInPage = inPageSeg->nextInPage;
                        protect(lastInPageSeg);
                    }
                    else
                    {
                        setFirstFreeSegmentOffset(basePagePtr, inPageSeg->nextInPage);
                    }
                    break;
                }
                offset = inPageSeg->nextInPage;
                lastInPageSegPtr = inPageSegPtr;
            }
        }
    }

    void
    DocumentAllocator::unmarkSegmentAsFree (SegmentPtr segPtr, AllocationProfile allocProfile)
    {
        /*
         * First, we have to remove the segment from the per-level linked-list
         */
        FreeSegmentsLevelHeader* fslHeader = getFreeSegmentsLevelHeader(allocProfile);

        FreeSegment* freeSeg = getSegment<FreeSegment, Read>(segPtr);

        __ui32 freeLevel;
        computeFreeLevel(freeLevel, freeSeg->size);

        unmarkSegmentAsFree(segPtr, allocProfile, fslHeader, freeLevel);
    }

    SegmentPtr
    DocumentAllocator::getFreeSegmentPtrFromNewPages (__ui64 size, AllocationProfile allocProfile)
    {
        bool isDocumentEmpty = (documentAllocationHeader->nextRelativePagePtr == NullPage);

        /*
         * If the document is empty, we must enlarge the size
         * In order to reserve the first bytes of the document
         */
        if (isDocumentEmpty)
        {
            size += sizeof(FreeSegment);
        }

        /*
         * Compute the number of pages required to allocate this segment
         */
        __ui64 pageNumber = size >> InPageBits;
        if (size % PageSize)
        {
            pageNumber++;
        }

        __ui64 allocedPageNumber = 0;

        RelativePagePtr relativePagePtr = getFreeRelativePages(pageNumber, allocedPageNumber, allocProfile);

        AssertBug(allocedPageNumber, "Could not allocate any page ?\n");
        AssertBug(isDocumentEmpty || relativePagePtr, "Could not allocate any page ?\n");

        Log_GF ( "Allocating %llu (0x%llx) bytes, would need pageNumber=%llu (0x%llx), will take allocedPageNumber=%llu (0x%llx)\n",
                size, size, pageNumber, pageNumber, allocedPageNumber, allocedPageNumber );

        /*
         * Make the free segment start from the new relativePagePtr
         */
        SegmentPtr ptr = relativePagePtr;

        /*
         * If we were empty (i.e. no page assigned), shift right the reserved area
         */
        if (isDocumentEmpty)
        {
            AssertBug(ptr == NullPtr, "Invalid non-null ptr with an empty document.\n");
            ptr = sizeof(FreeSegment);
            size -= sizeof(FreeSegment);
        }
        else
        {
            AssertBug(ptr, "Invalid null ptr with a non-empty document.\n");
        }

        /*
         * Mark the remains as free
         */
        __ui64 remainsSize = (allocedPageNumber * PageSize) - size;
        if (remainsSize)
        {
            if (isDocumentEmpty)
            {
                remainsSize -= sizeof(FreeSegment);
            }
            SegmentPtr remainsStart = ptr + size;
            markSegmentAsFree(remainsStart, remainsSize, allocProfile, false);
        }
        return ptr;
    }

    void
    DocumentAllocator::coalesceFreeSegment (SegmentPtr& segPtr, __ui64& size, AllocationProfile allocProfile )
    {
        AssertBug( __assertDocumentHasCoalesce, "! DocumentHasCoalesce");

        /*
         * First, we have to find the first segment in page, and compute collations
         */
        RelativePagePtr basePagePtr = segPtr & PagePtr_Mask;

        __ui32 segmentOffset = getFirstFreeSegmentOffset ( basePagePtr );

        if ( segmentOffset < PageSize )
        {
            Log_GF ( "freeSegment : first offset at %x\n", segmentOffset );
            while ( true )
            {
                SegmentPtr inPageSegPtr = basePagePtr + (SegmentPtr) segmentOffset;

                Log_GF ( "In page : base=%llx, offset=%x, inPageSegPtr=%llx\n",
                basePagePtr, segmentOffset, inPageSegPtr );

                FreeSegment* inPageSeg = getSegment<FreeSegment,Read> ( inPageSegPtr );

                Log_GF ( "In page seg : %llx : nextInPage=%x, last=%llx, next=%llx, size=%llx\n",
                inPageSegPtr, inPageSeg->nextInPage, inPageSeg->last, inPageSeg->next, inPageSeg->size );

                AssertBug ( inPageSeg->dummy == FreeSegment_dummy, "Invalid dummy %x.\n", inPageSeg->dummy );

                __ui64 inPageSegSize = inPageSeg->size;
                __ui32 nextOffset = inPageSeg->nextInPage;
                inPageSeg = NULL;

                AssertBug ( segmentOffset < nextOffset, "Cycling or reducing offsets : segmentOffset=%x, next=%x\n", segmentOffset, nextOffset );

                if ( inPageSegPtr + inPageSegSize == segPtr )
                {
                    Log_GF ( "Left-collation of free segment : segPtr=%llx, inPageSegPtr=%llx, size=%llx.\n",
                    segPtr, inPageSegPtr, inPageSegSize );

                    unmarkSegmentAsFree ( inPageSegPtr, allocProfile );

                    segPtr = inPageSegPtr;
                    size += inPageSegSize;
                }
                else if ( segPtr + size == inPageSegPtr )
                {
                    Log_GF ( "Right-collation of free segment : segPtr=%llx, inPageSegPtr=%llx, size=%llx.\n",
                    segPtr, inPageSegPtr, inPageSegSize );

                    unmarkSegmentAsFree ( inPageSegPtr, allocProfile );
                    size += inPageSegSize;
                    break;
                }

                segmentOffset = nextOffset;
                if ( segmentOffset >= PageSize ) break;
            }

        }
    }

    void
    DocumentAllocator::insertFreeSegmentInPageList (SegmentPtr segPtr, __ui64 size, FreeSegment* freeSeg,
                                                    AllocationProfile allocProfile)
    {
        AssertBug(__assertDocumentHasCoalesce, "! DocumentHasCoalesce !");

        RelativePagePtr basePagePtr = segPtr & PagePtr_Mask;
        __ui32 freeSegOffset = segPtr - basePagePtr;
        __ui32 segmentOffset = getFirstFreeSegmentOffset(basePagePtr);

        Log_GF ( "in-page : segPtr=%llx, basePagePtr=%llx, segmentOffset=%x, freeSegOffset=%x\n",
                segPtr, basePagePtr, segmentOffset, freeSegOffset );

        if (freeSegOffset < segmentOffset && segmentOffset < PageSize)
        {
            /*
             * The freed segment has an offset less than the head of the linked-list, insert it at head
             */
            alter(freeSeg);
            freeSeg->nextInPage = segmentOffset;
            protect(freeSeg);
            setFirstFreeSegmentOffset(basePagePtr, freeSegOffset);
        }
        else if (segmentOffset < PageSize)
        {
            /*
             * We have to find the spot where to add the free segment
             */
            while (true)
            {
                FreeSegment* previousInPage = getSegment<FreeSegment, Read>(basePagePtr + segmentOffset);

                AssertBug(previousInPage->dummy == FreeSegment_dummy, "Invalid dummy.\n");
                if (previousInPage->nextInPage > freeSegOffset)
                {
                    AssertBug(segmentOffset < freeSegOffset, "Too late ! segOffset=%x, nextInPage=%x, freeSeg=%x\n",
                            segmentOffset, previousInPage->nextInPage, freeSegOffset);
                    alter(freeSeg);
                    freeSeg->nextInPage = previousInPage->nextInPage;
                    protect(freeSeg);
                    alter(previousInPage);
                    previousInPage->nextInPage = freeSegOffset;
                    protect(previousInPage);
                    Log_GF ( "Set previousInPage->nextInPage = %x\n", previousInPage->nextInPage );
                    break;
                }
                segmentOffset = previousInPage->nextInPage;
            }

        }
        else
        {
            Log_GF ( "Inserting freeSeg freeSegOffset=%x for page basePagePtr=%llx (ptr=%llx)\n", freeSegOffset, basePagePtr, segPtr );
            setFirstFreeSegmentOffset(basePagePtr, freeSegOffset);
        }
    }

    SegmentPtr
    DocumentAllocator::getFreeSegmentPtrFromHoles (__ui64 size, AllocationProfile allocProfile,
                                                   FreeSegmentsLevelHeader* fslHeader, __ui32 level)
    {
        Log_GF ( "\tAt level %x, firstHole=0x%llx\n", level, fslHeader->firstFreeHole[level] );
        SegmentPtr segPtr = fslHeader->firstFreeHole[level];
        if (segPtr)
        {
            Log_GF ( "(%p) Found a free hole for %x/%x, ptr=0x%llx\n",
                    this, allocProfile, level, fslHeader->firstFreeHole[level] );
            FreeSegment* lastFreeSeg = getSegment<FreeSegment, Read>(segPtr);
#if PARANOID
            if (lastFreeSeg->dummy != FreeSegment_dummy)
            {
                Warn("Document (this=%p) : at segPtr=%llx, "
                     "Invalid hole ! stored=0x%x, real dummy=0x%x\n",
                     this, segPtr, lastFreeSeg->dummy, FreeSegment_dummy);
                Warn("\tDocument first free at %x\n", getFirstFreeSegmentOffset(segPtr & PagePtr_Mask));
                Warn("\tDump : \n");
                fprintf( stderr, "0x8%llx : ", segPtr + 0);
                for (__ui64 x = 0; x < 64; x++)
                {
                    if (x % 8 == 0 && x)
                    {
                        fprintf( stderr, "\n");
                        if (x != 64 - 8)
                            fprintf( stderr, "0x8%llx : ", segPtr + x);
                    }
                    fprintf( stderr, "%8x ", (*getSegment<__ui32, Read>(segPtr + x, sizeof(__ui32) )));
                }

                Bug("This is considered fatal.\n");
                Warn("Getting segment from free pages instead...\n");
                return NullPtr;
            }
#endif // PARANOID
            if (lastFreeSeg->size < size)
            {
                Log_GF ( "Free segment too narrow ! lastFreeSeg->size=0x%llx, requested=0x%llx\n",
                        lastFreeSeg->size, size );
                return NullPtr;
            }

            Log_GF ( "Free hole at 0x%llx (%p), size=0x%llx, remains=0x%llx\n",
                    segPtr, lastFreeSeg, lastFreeSeg->size, lastFreeSeg->size - size );

            unmarkSegmentAsFree(segPtr, allocProfile, fslHeader, level);

            /*
             * If the free segment is larger than the desired size,
             * we have to mark back the remains as free.
             */
            if (lastFreeSeg->size > size)
            {
                markSegmentAsFree(segPtr + size, lastFreeSeg->size - size, allocProfile, false);
            }
            return segPtr;
        }
        return NullPtr;
    }

    SegmentPtr
    DocumentAllocator::getFreeSegmentPtrFromHoles (__ui64 size, AllocationProfile allocProfile)
    {
        __ui32 freeLevel;
        computeFreeLevel(freeLevel, size);
        Log_GF ( "Allocating %llu (0x%llx) bytes, freeLevel=0x%x\n", size, size, freeLevel );
        FreeSegmentsLevelHeader* fslHeader = getFreeSegmentsLevelHeader(allocProfile);
        SegmentPtr segPtr;

        if (fslHeader->firstFreeHole[freeLevel])
        {
            segPtr = getFreeSegmentPtrFromHoles(size, allocProfile, fslHeader, freeLevel);
            if (segPtr)
            {
                return segPtr;
            }
        }
        segPtr = getFreeSegmentPtrFromHoles(size, allocProfile, fslHeader, FreeSegmentsLevelHeader::levelOther);
        return segPtr;
    }

}
