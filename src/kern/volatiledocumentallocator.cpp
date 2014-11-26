#include <Xemeiah/kern/volatiledocumentallocator.h>
#include <Xemeiah/kern/document.h>
#include <Xemeiah/kern/store.h>
#include <malloc.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_VolDocAlloc Log
#define Log_VolDoc_GetFreeRelativePages Debug

namespace Xem
{
    VolatileDocumentAllocator::VolatileDocumentAllocator (Store& _store) :
            DocumentAllocator(_store), store(_store)
    {
        __assertDocumentAlwaysWritable = true;
        __assertDocumentHasCoalesce = false;

        documentAllocationHeader = (DocumentAllocationHeader*) getStore().getVolatileArea();

        Log_VolDocAlloc ( "(this=%p) DocumentAllocationHeader at %p\n", this, documentAllocationHeader );
        initDocumentAllocationHeader(1);
    }

    VolatileDocumentAllocator::~VolatileDocumentAllocator ()
    {
        Log_VolDocAlloc ( "Deleting VolatileDocumentAllocator at %p, areas=%p, areasAlloced=%llu (0x%llx)\n", this, areas, areasAlloced, areasAlloced );
        if (areas)
        {
            for (__ui64 idx = 0; idx < areasAlloced; idx++)
            {
                void* area = areas[idx];
                if (!area)
                {
                    continue;
                }
                Log_VolDocAlloc ( "[VDA] Unmap idx=0x%llx, area=%p\n", idx, area );
                getStore().releaseVolatileArea(area);
            }
            free(areas);
            areas = NULL;
            areasAlloced = 0;
        }
        getStore().releaseVolatileArea(documentAllocationHeader);
    }

    /**
     * A segment in a volatile document is always read/write
     * Because branch mechanism and inheritance is not available for VolatileDocument
     * We are sure, by design, that all the Pages belong to the actual Revision.
     */
    void
    VolatileDocumentAllocator::authorizePageWrite (RelativePagePtr relPagePtr)
    {
    }

    void
    VolatileDocumentAllocator::mapDirectPage (RelativePagePtr relPagePtr)
    {
        __ui64 areaIdx = relPagePtr >> InAreaBits;
        mapArea(areaIdx);
    }

    void
    VolatileDocumentAllocator::mapArea (__ui64 areaIdx)
    {
        if (areasAlloced <= areaIdx)
        {
            __ui64 toAlloc = areaIdx + 4;
            areas = (void**) realloc(areas, sizeof(void*) * (toAlloc));
            for (__ui64 i = areasAlloced; i < toAlloc; i++)
            {
                areas[i] = NULL;
            }
            areasAlloced = toAlloc;
        }
        if (areas[areaIdx] == NULL)
        {
            areasMapped++;
            areas[areaIdx] = getStore().getVolatileArea();
            Log_VolDocAlloc ( "[VDA] this=%p, areaIdx=0x%llx -> MMapped to %p, areasAlloced=0x%llx\n",
                    this, areaIdx, areas[areaIdx], areasAlloced );
        }
    }

    RelativePagePtr
    VolatileDocumentAllocator::getFreeRelativePages (__ui64 askedNumber, __ui64& allocedNumber, AllocationProfile allocProfile )
    {
        allocedNumber = askedNumber;
        if ( allocedNumber*PageSize > AreaSize )
        {
            Bug ( "Segment exceeds Area size !\n" );
        }
        // Check that we don't come across an Area
        RelativePagePtr relativePagePtr = getDocumentAllocationHeader().nextRelativePagePtr;
        if ( ( getDocumentAllocationHeader().nextRelativePagePtr >> InAreaBits )
        != ( ( getDocumentAllocationHeader().nextRelativePagePtr+(PageSize*allocedNumber)) >> InAreaBits ) )
        {
            Log_VolDocAlloc ( "Coming accross the area : next=0x%llx, allocedNumber=0x%llx (size=0x%llx), "
            "across=0x%llx, left area=0x%llx, right=0x%llx\n",
            getDocumentAllocationHeader().nextRelativePagePtr, allocedNumber, PageSize*allocedNumber,
            getDocumentAllocationHeader().nextRelativePagePtr + (PageSize*allocedNumber),
            ( getDocumentAllocationHeader().nextRelativePagePtr >> InAreaBits ),
            ( (getDocumentAllocationHeader().nextRelativePagePtr+(PageSize*allocedNumber)) >> InAreaBits ) );
            /*
             * \todo we have to record the big hole !
             */
            __ui64 nextAreaIdx = (getDocumentAllocationHeader().nextRelativePagePtr+(PageSize*allocedNumber)) >> InAreaBits;
            relativePagePtr = nextAreaIdx << InAreaBits;

            alterDocumentAllocationHeader ();
            getDocumentAllocationHeader().nextRelativePagePtr = relativePagePtr + (PageSize*allocedNumber);
            protectDocumentAllocationHeader ();
        }
        else
        {
            Log_VolDoc_GetFreeRelativePages ( "Setting nextRelativePagePtr=%p, %llx\n",
            &(getDocumentAllocationHeader().nextRelativePagePtr), getDocumentAllocationHeader().nextRelativePagePtr );
            alterDocumentAllocationHeader ();
            getDocumentAllocationHeader().nextRelativePagePtr += (PageSize*allocedNumber);
            protectDocumentAllocationHeader ();
        }
        for ( __ui64 i = 0; i < allocedNumber; i++ )
        {
            mapDirectPage ( relativePagePtr + i );
        }

        Log_VolDoc_GetFreeRelativePages ( "VOLATILE : Alloced 0x%llx pages (size=0x%llx) from relPagePtr=0x%llx, areaIdx=%llx, area=%p\n",
        allocedNumber, PageSize*allocedNumber, relativePagePtr,
        relativePagePtr>>InAreaBits, areas[relativePagePtr>>InAreaBits] );

        return relativePagePtr;
    }

    AllocationProfile
    VolatileDocumentAllocator::getAllocationProfile (SegmentPtr segmentPtr)
    {
        return 0;
    }

}
;

