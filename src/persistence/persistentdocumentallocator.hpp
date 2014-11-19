#include <Xemeiah/kern/store.h>
#include <Xemeiah/kern/exception.h>
#include <Xemeiah/persistence/persistentdocumentallocator.h>
#include <Xemeiah/trace.h>

#include <Xemeiah/persistence/format/indirection.h>

#define Log_PDAHPP Debug
#define Log_PDA_AbsolutePage Debug

// #define __XEM_PERSISTENTDOCUMENTALLOCATOR_PAGEINFOPAGETABLE_PARANOID //< Option : Paranoid checks for PageInfoPageTable

namespace Xem
{

#define KnownFixedSegmentSize(__type) template<> __ui64 __INLINE DocumentAllocator::getFixedSegmentSize<__type> () \
  { return sizeof(__type); }
    KnownFixedSegmentSize(IndirectionHeader)
#undef KnownFixedSegmentSize

    __INLINE RevisionPage*
    PersistentDocumentAllocator::getRevisionPage () const
    {
        AssertBug(revisionPage, "NULL RevisionPage !!!\n");
        return revisionPage;
    }

    __INLINE BranchRevId
    PersistentDocumentAllocator::getBranchRevId ()
    {
        static const BranchRevId NullBranchRevId =
            { 0, 0 };
        if (!revisionPage)
            return NullBranchRevId;
        AssertBug(revisionPage, "NULL RevisionPage !!!\n");
        return revisionPage->branchRevId;
    }

    __INLINE void
    PersistentDocumentAllocator::alterRevisionPage ()
    {
#if PARANOID
        AssertBug ( revisionPage, "NULL revisionPage !\n" );
#endif
#ifdef XEM_MEM_PROTECT_SYS
        getPersistentStore().alterPage ( revisionPage );
#endif  
    }

    __INLINE void
    PersistentDocumentAllocator::protectRevisionPage ()
    {
#if PARANOID
        AssertBug ( revisionPage, "NULL revisionPage !\n" );
#endif
#ifdef XEM_MEM_PROTECT_SYS
        getPersistentStore().protectPage ( revisionPage );
#endif  
    }

    __INLINE void
    PersistentDocumentAllocator::alterPageInfo (PageInfo& pageInfo)
    {
#ifdef XEM_MEM_PROTECT_SYS
        AssertBug ( &pageInfo, "NULL pageInfo !\n" );
        PageInfoPage* pageInfoPage = (PageInfoPage*) ((__ui64)&pageInfo & PagePtr_Mask);
        getPersistentStore().alterPage(pageInfoPage);
#endif
    }

    __INLINE void
    PersistentDocumentAllocator::protectPageInfo (PageInfo& pageInfo)
    {
#ifdef XEM_MEM_PROTECT_SYS
        AssertBug ( &pageInfo, "NULL pageInfo !\n" );
        PageInfoPage* pageInfoPage = (PageInfoPage*) ((__ui64)&pageInfo & PagePtr_Mask);
        getPersistentStore().protectPage(pageInfoPage);
#endif
    }

    __INLINE PageInfoPagePtr
    PersistentDocumentAllocator::getPageInfoPagePtr (__ui64 indirectionOffset, bool write)
    {
        PageInfoPagePtr pageInfoPagePtr = fetchPageInfoPagePtr(indirectionOffset, write);
        return pageInfoPagePtr;
    }

    __INLINE bool
    PersistentDocumentAllocator::doGetPageInfoPage (RelativePagePtr relativePagePtr, PageInfoPage*& pageInfoPage, __ui64& index, bool write )
    {
        mapMutex.assertLocked();
        AssertBug ( relativePagePtr % PageSize == 0, "Relative page pointer not aligned !\n" );
        /*
         * The index of the page in the PageInfoPage global vector
         */
        __ui64 pageIndex = (relativePagePtr >> InPageBits);

        /*
         * The position of the page in its very own PageInfoPage
         */
        index = pageIndex % PageInfo_pointerNumber;

        /*
         * The index of the first page in the PageInfoPage of the provided page
         */
        __ui64 indirectionOffset = pageIndex - index;

#ifdef __XEM_PERSISTENTDOCUMENTALLOCATOR_HAS_PAGEINFOPAGETABLE
        PageInfoPageTable::iterator iter = pageInfoPageTable.find(indirectionOffset);
        if ( iter != pageInfoPageTable.end() && (!write || ((__ui64)iter->second & PageFlags_Stolen) ) )
        {
            Log_PDAHPP ( "PIPT [%llx] : [%llx] => %p\n", relativePagePtr, iter->first, iter->second );
            pageInfoPage = (PageInfoPage*) (((__ui64)iter->second) & PagePtr_Mask);
#ifdef __XEM_PERSISTENTDOCUMENTALLOCATOR_PAGEINFOPAGETABLE_PARANOID
            bool found = false;
            for ( AbsolutePages::iterator it2 = absolutePages.begin(); it2 != absolutePages.end(); it2++ )
            {
                if ( pageInfoPage == it2->second )
                {
                    found = true; break;
                }
            }
            if ( ! found )
            {
                Bug ( "PageInfoPageTable : PageInfopage %p for indirectionOffset %llx not in absolutePages map (%lu pages)\n",
                pageInfoPage, indirectionOffset, (unsigned long) absolutePages.size() );
            }
#endif
            return true;
        }
        Log_PDAHPP ( "PIPT [%llx] CACHE MISS (indirectionOffset=%llx, pageIndex=%llx, write=%d)\n",
        relativePagePtr, indirectionOffset, pageIndex, write );
#endif

        Log_PDAHPP ( "getPageInfoPage(relativePagePtr=%llx) : pageIndex=%llx, index=%llx, indirectionOffset=%llx\n",
        relativePagePtr, pageIndex, index, indirectionOffset );

        /**
         * Fetch the PageInfoPage
         * pageInfoPagePtr is the absolute page pointer for this pageInfoPage
         */
        PageInfoPagePtr pageInfoPagePtr = getPageInfoPagePtr ( indirectionOffset, write );

        AssertBug ( pageInfoPagePtr, "Null pageInfoPagePtr !\n" );

        pageInfoPage = getPageInfoPage(pageInfoPagePtr);

        if ( ! pageInfoPage ) return false;

#ifdef __XEM_PERSISTENTDOCUMENTALLOCATOR_HAS_PAGEINFOPAGETABLE
        Log_PDAHPP ( "PIPT [%llx] CACHE SET (indirectionOffset=%llx, pageIndex=%llx, pageInfoPage=%p, write=%d)\n",
        relativePagePtr, indirectionOffset, pageIndex, pageInfoPage, write );
        pageInfoPageTable[indirectionOffset] = (PageInfoPage*)
        ( ((__ui64)pageInfoPage) | (write ? PageFlags_Stolen : 0) );
#endif

#if PARANOID
        AssertBug ( pageInfoPage, "Null PageInfoPage !!!\n" );
        if ( pageInfoPage->pageInfo[index].branchRevId.branchId == 0 && ! write )
        {
            Bug ( "Invalid zero branchId on relPageIdx=%llx, pageInfoPage=%p, index=%llx, brid=%llx:%llx, abs=%llx\n",
            relativePagePtr, pageInfoPage, index,
            _brid(pageInfoPage->pageInfo[index].branchRevId), pageInfoPage->pageInfo[index].absolutePagePtr );
        }
#endif

        return true;
    }

    __INLINE PageInfo&
    PersistentDocumentAllocator::getPageInfo (RelativePagePtr relativePagePtr, bool write)
    {
        mapMutex.assertLocked();
        PageInfoPage* pageInfoPage;
        __ui64 index;
        if (!doGetPageInfoPage(relativePagePtr, pageInfoPage, index, write))
        {
            throwException(PageInfoException, "Could not getPageInfo(relativPagePtr=%llx, write=%d)\n", relativePagePtr, write);
        }
        PageInfo& pageInfo = pageInfoPage->pageInfo[index];
        return pageInfo;
    }

    template<typename PageClass>
        __INLINE PageClass*
        PersistentDocumentAllocator::getAbsolutePage (AbsolutePagePtr absPagePtr)
        {
            AssertBug(absPagePtr, "NULL absPagePtr provided !\n");
            mapMutex.assertLocked();
            AssertBug(( absPagePtr & PagePtr_Mask ) == absPagePtr, "Erroneous absPagePtr %llx\n", absPagePtr);
            AbsolutePages::iterator iter = absolutePages.find(absPagePtr);
            if (iter != absolutePages.end())
            {
                Log_PDAHPP ( "Absolute page %llx already claimed : at %p !\n", absPagePtr, iter->second );
                return (PageClass*) iter->second;
            }
            PageClass* page = getPersistentStore().getAbsolutePage<PageClass>(absPagePtr);
            absolutePages[absPagePtr] = (void*) page;
            Log_PDA_AbsolutePage ( "Absolute page %llx set to %p !\n", absPagePtr, page );
            return page;
        }

    __INLINE void
    PersistentDocumentAllocator::releasePage (AbsolutePagePtr absPagePtr)
    {
        mapMutex.assertLocked();
        Log_PDAHPP ( "releasePage %llx : absPagePtr\n", absPagePtr );

        absPagePtr &= PagePtr_Mask;
        AbsolutePages::iterator iter = absolutePages.find(absPagePtr);
        AssertBug(iter != absolutePages.end(), "Page %llx not recorded !\n", absPagePtr);
#ifdef __XEM_PERSISTENTDOCUMENTALLOCATOR_HAS_PAGEINFOPAGETABLE
#ifdef __XEM_PERSISTENTDOCUMENTALLOCATOR_PAGEINFOPAGETABLE_PARANOID
        void* page = iter->second;
        for ( PageInfoPageTable::iterator it2 = pageInfoPageTable.begin(); it2 != pageInfoPageTable.end(); it2++ )
        {
            void* pageInfoPage = (PageInfoPage*) (((__ui64)it2->second) & PagePtr_Mask);
            if ( pageInfoPage == page )
            {
                Bug ( "releasePage() but the page is recorded in PageInfoPageTable !\n" );
            }
        }
#endif // __XEM_PERSISTENTDOCUMENTALLOCATOR_PAGEINFOPAGETABLE_PARANOID
#endif // __XEM_PERSISTENTDOCUMENTALLOCATOR_HAS_PAGEINFOPAGETABLE
        absolutePages.erase(iter);
        Log_PDAHPP ( "releasePage %llx : absPagePtr : Ok\n", absPagePtr );
        getPersistentStore().releasePage(absPagePtr);
        Log_PDA_AbsolutePage ( "releasePage %llx : absPagePtr : Ok from store\n", absPagePtr );
    }

    __INLINE IndirectionPage*
    PersistentDocumentAllocator::getIndirectionPage (IndirectionPagePtr indirectionPagePtr)
    {
        AssertBug(__getPageType(indirectionPagePtr) == PageType_Indirection, "Invalid page type !\n");
        return getAbsolutePage<IndirectionPage>(indirectionPagePtr & PagePtr_Mask);
    }

    __INLINE PageInfoPage*
    PersistentDocumentAllocator::getPageInfoPage (AbsolutePagePtr pageInfoPagePtr)
    {
        AssertBug(__getPageType(pageInfoPagePtr) == PageType_PageInfo, "Invalid page type !\n");
        return getAbsolutePage<PageInfoPage>(pageInfoPagePtr & PagePtr_Mask);
    }

    __INLINE PageList*
    PersistentDocumentAllocator::getPageList (AbsolutePagePtr pageListPtr)
    {
        return getAbsolutePage<PageList>(pageListPtr & PagePtr_Mask);
    }

    __INLINE RevisionPage*
    PersistentDocumentAllocator::getRevisionPage (AbsolutePagePtr revisionPagePtr)
    {
        return getAbsolutePage<RevisionPage>(revisionPagePtr & PagePtr_Mask);
    }

    __INLINE SegmentPage*
    PersistentDocumentAllocator::getSegmentPage (AbsolutePagePtr absPagePtr)
    {
        AssertBug(__getPageType(absPagePtr) == PageType_Segment, "Invalid page type !\n");
        return getAbsolutePage<SegmentPage>(absPagePtr & PagePtr_Mask);
    }

}
;
