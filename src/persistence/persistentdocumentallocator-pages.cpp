#include <Xemeiah/persistence/persistentdocumentallocator.h>
#include <Xemeiah/trace.h>

#include <Xemeiah/auto-inline.hpp>
#include <Xemeiah/persistence/auto-inline.hpp>

#define Log_PDAForAllPages Debug

namespace Xem
{
    bool
    PersistentDocumentAllocator::forPageInfoPage (PageInfoPage* pageInfoPage, RelativePagePtr offset, bool isStolen,
                                                  SegmentPageAction action, void *arg, bool ownedSegmentOnly)
    {
        for (__ui32 index = 0; index < PageInfo_pointerNumber; index++)
        {
            PageInfo* pageInfo = &(pageInfoPage->pageInfo[index]);
            if (pageInfo->absolutePagePtr == NullPage)
            {
                Log_PDAForAllPages ( "(skipping page %llx : no absolute)\n", offset );
                offset += PageSize;
                continue;
            }
            bool willProcess = (!ownedSegmentOnly) || ( bridcmp ( getBranchRevId(), pageInfo->branchRevId ) == 0);
            Log_PDAForAllPages ( "Page %x : pageInfo rel=%llx/%llx, abs=%llx, brid=%llx:%llx %s\n",
                    index, offset, getDocumentAllocationHeader().nextRelativePagePtr,
                    pageInfo->absolutePagePtr, _brid(pageInfo->branchRevId),
                    willProcess ? "-> process" : "" );

            if (willProcess)
            {
                // Log_PDAForAllPages ( "Process this page !\n" );
                (this->*action)(offset, *pageInfo, PageType_Segment,
                                isStolen || ( bridcmp ( getBranchRevId(), pageInfo->branchRevId ) != 0), arg);
#if 0            
                (this->*action) ( offset, pageInfo->absolutePagePtr | PageType_Segment,
                        isStolen || ( bridcmp ( getRevisionPage()->branchRevId, pageInfo->branchRevId ) != 0 ),
                        arg );
#endif
            }
            offset += PageSize;
        }
        return true;
    }

    bool
    PersistentDocumentAllocator::forIndirectionPages (IndirectionPage* indirPage, __ui32 level, RelativePagePtr offset,
                                                      bool isStolen, IndirectionPageAction indirAction,
                                                      SegmentPageAction segmentAction, void *arg, bool ownedIndirOnly,
                                                      bool ownedSegmentOnly)
    {
        // Log_PDAForAllPages ( "forIndirectionPages : indirPage=%p, level=%u, offset=0x%llx\n", indirPage, level, offset );
        AssertBug(level > 1, "Invalid level : %u\n", level);
        RelativePagePtr area = PageInfo_pointerNumber;
        // if ( level > 2 ) area *= Indirection_pointerNumber * ( level - 2 );
        for (__ui32 l = 2; l < level; l++)
            area *= Indirection_pointerNumber;
        area *= PageSize;

        Log_PDAForAllPages ( "forIndirectionPages : level=%u, offset=%llx -> area=%llx\n", level, offset, area );

        for (__ui32 index = 0; index < Indirection_pointerNumber; index++)
        {
            AbsolutePagePtr absPagePtr = indirPage->pointers[index];

            Log_PDAForAllPages ( "level=%x : index=%x : absPagePtr=%llx\n", level, index, absPagePtr );

            if (absPagePtr == NullPage)
                continue;
            if ( __isStolen(absPagePtr) && ownedIndirOnly)
                continue;

            if (level == 2)
            {
                AssertBug(__getPageType(absPagePtr) == PageType_PageInfo, "Invalid page type !\n");
                AbsolutePageRef<PageInfoPage> pageRef = getPageInfoPage(absPagePtr);
                forPageInfoPage(pageRef.getPage(), offset, isStolen || (__isStolen(absPagePtr)), segmentAction, arg,
                                ownedSegmentOnly);
            }
            else
            {
                AssertBug(__getPageType(absPagePtr) == PageType_Indirection, "Invalid page type !\n");
                AbsolutePageRef<IndirectionPage> pageRef = getIndirectionPage(absPagePtr);
                forIndirectionPages(pageRef.getPage(), level - 1, offset, isStolen || (__isStolen(absPagePtr)),
                                    indirAction, segmentAction, arg, ownedIndirOnly, ownedSegmentOnly);
            }
            if (indirAction)
            {
                (this->*indirAction)(offset, absPagePtr, __getPageType(absPagePtr),
                                     isStolen || (__isStolen(absPagePtr)), arg);
            }
            // releasePage ( absPagePtr );
            offset += area;
        }
        return true;
    }

    bool
    PersistentDocumentAllocator::forAllIndirectionPages (IndirectionPageAction indirAction,
                                                         SegmentPageAction segmentAction, void *arg,
                                                         bool ownedIndirOnly, bool ownedSegmentOnly)
    {
        Lock lock(mapMutex);

        Log_PDAForAllPages ( "forAllIndirections : firstPage=%llx, stolen=%s, ownedIndirOnly=%s, ownedSegmentOnly=%s\n",
                getRevisionPage()->indirection.firstPage,
                __isStolen ( getRevisionPage()->indirection.firstPage ) ? "true" : "false",
                ownedIndirOnly ? "true" : "false",
                ownedSegmentOnly ? "true" : "false" );
        Log_PDAForAllPages ( "\tat start : level=%u, firstPage=%llx, type=%llx\n",
                getRevisionPage()->indirection.level,
                getRevisionPage()->indirection.firstPage & PagePtr_Mask,
                __getPageType(getRevisionPage()->indirection.firstPage) );

        if (!__isStolen(getRevisionPage()->indirection.firstPage) || !ownedIndirOnly)
        {
            if (getRevisionPage()->indirection.level > 1)
            {
                AbsolutePageRef<IndirectionPage> firstPageRef = getIndirectionPage(
                        getRevisionPage()->indirection.firstPage);
                forIndirectionPages(firstPageRef.getPage(), getRevisionPage()->indirection.level, NullPtr,
                                    __isStolen(getRevisionPage()->indirection.firstPage), indirAction, segmentAction,
                                    arg, ownedIndirOnly, ownedSegmentOnly);
            }
            else if (getRevisionPage()->indirection.level == 1)
            {
                AbsolutePageRef<PageInfoPage> firstPageRef = getPageInfoPage(getRevisionPage()->indirection.firstPage);
                forPageInfoPage(firstPageRef.getPage(), NullPtr, __isStolen(getRevisionPage()->indirection.firstPage),
                                segmentAction, arg, ownedSegmentOnly);
            }
            else
            {
                Warn("No indirection level, skipping !\n");
            }
            if (getRevisionPage()->indirection.firstPage && indirAction)
            {
                (this->*indirAction)( NullPtr, getRevisionPage()->indirection.firstPage,
                                     __getPageType(getRevisionPage()->indirection.firstPage),
                                     __isStolen(getRevisionPage()->indirection.firstPage), arg);
            }
        }
        // releasePage ( getRevisionPage()->indirection.firstPage );
        return true;
    }
}
;

