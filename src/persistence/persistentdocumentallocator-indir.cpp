#include <Xemeiah/persistence/persistentdocumentallocator.h>
#include <Xemeiah/trace.h>

#include <Xemeiah/auto-inline.hpp>
#include <Xemeiah/persistence/auto-inline.hpp>

#define Log_Indir Debug

namespace Xem
{
#define __alterPtr(__ptr,__size) getPersistentStore().alterPage ( (void*) (((__ui64)__ptr)&PagePtr_Mask) )
#define __protectPtr(__ptr,__size) getPersistentStore().protectPage ( (void*) (((__ui64)__ptr)&PagePtr_Mask) )

  IndirectionPagePtr PersistentDocumentAllocator::getFreeIndirectionPagePtr ( bool reset )
  {
    alterRevisionPage ();
    getRevisionPage()->ownedPages++;
    getRevisionPage()->ownedTypedPages[PageType_Indirection] ++;    
    protectRevisionPage ();
    IndirectionPagePtr newIndirectionPagePtr = getPersistentStore().getFreePagePtr () | PageType_Indirection; 
    
    if ( reset )
      {
        IndirectionPage* indirectionPage = getIndirectionPage ( newIndirectionPagePtr );
        getPersistentStore().alterPage ( indirectionPage );
        memset ( indirectionPage, 0, PageSize );
        getPersistentStore().protectPage ( indirectionPage );
        releasePage ( newIndirectionPagePtr );
      }    
    Log_Indir ( "[%llx:%llx] New indirectionPage=%llx (reset=%d)\n", 
        _brid(getBranchRevId()), newIndirectionPagePtr, reset );
    return newIndirectionPagePtr;
  }

  PageInfoPagePtr PersistentDocumentAllocator::getFreePageInfoPagePtr ( bool reset )
  {
    alterRevisionPage ();
    getRevisionPage()->ownedPages++;
    getRevisionPage()->ownedTypedPages[PageType_PageInfo] ++;    
    protectRevisionPage ();

    PageInfoPagePtr newPageInfoPagePtr = getPersistentStore().getFreePagePtr () | PageType_PageInfo;

    if ( reset )
      {
        PageInfoPage* pageInfoPage = getPageInfoPage ( newPageInfoPagePtr );
        getPersistentStore().alterPage ( pageInfoPage );
        memset ( pageInfoPage, 0, PageSize );
        getPersistentStore().protectPage ( pageInfoPage );
        /*
         * TODO Keep that page in absolute cache (shall we ?)
         */
        releasePage ( newPageInfoPagePtr );
      }    
    Log_Indir ( "[%llx:%llx] New pageInfoPage=%llx (reset=%d)\n", 
        _brid(getBranchRevId()), newPageInfoPagePtr, reset );    
    return newPageInfoPagePtr;
  }
  
  bool PersistentDocumentAllocator::getPageInfo ( RelativePagePtr relativePagePtr, PageInfo& pageInfo )
  {
    /*
     * This function is quite expensive, because we handle a reference to that PageInfo
     */
    mapMutex.lock ();
    PageInfoPage* pageInfoPage; __ui64 index;
    if ( ! doGetPageInfoPage ( relativePagePtr, pageInfoPage, index, false ) )
      {
        mapMutex.unlock ();
        return false;
      }
    pageInfo = pageInfoPage->pageInfo[index];
    mapMutex.unlock ();
    return true;
  }
  
  void PersistentDocumentAllocator::setPageInfo ( RelativePagePtr relativePagePtr, PageInfo& pageInfo )
  {
    mapMutex.assertLocked();
    PageInfoPage* pageInfoPage; __ui64 index;
    if ( ! doGetPageInfoPage ( relativePagePtr, pageInfoPage, index, true ) )
      {
        Bug ( "Could not get page info page at 0x%llx\n", relativePagePtr );
      }
    getPersistentStore().alterPage ( pageInfoPage );
    pageInfoPage->pageInfo[index] = pageInfo;
    getPersistentStore().protectPage ( pageInfoPage );
  }

  void PersistentDocumentAllocator::increaseIndirectionCoveredArea ()
  {
    IndirectionPagePtr headIndirectionPagePtr = getFreeIndirectionPagePtr ();
    IndirectionPage* headIndirectionPage = getIndirectionPage ( headIndirectionPagePtr );
    
    getPersistentStore().alterPage ( headIndirectionPage );
    memset ( headIndirectionPage, 0, PageSize );
    headIndirectionPage->pointers[0] = revisionPage->indirection.firstPage;
    getPersistentStore().protectPage ( headIndirectionPage );
    
    alterRevisionPage ();
    revisionPage->indirection.level ++;
    revisionPage->indirection.firstPage = headIndirectionPagePtr;
    protectRevisionPage ();
    
    Log_Indir ( "Increased : level=%u, new Indirection head at %llx\n", revisionPage->indirection.level, headIndirectionPagePtr );
  }
  
  PageInfoPagePtr PersistentDocumentAllocator::fetchPageInfoPagePtr ( __ui64 indirectionOffset, bool write )
  {
    if ( revisionPage->indirection.level == 0 )
      {
        Warn ( "Brid %llx:%llx : no indirection at all !\n", _brid(getBranchRevId()) );
        return NULL;
      }
    AssertBug ( revisionPage->indirection.firstPage, "No first page set !\n" );
  
    /**
     * Compute and test initial coverage test
     */
    __ui64 coveredArea = PageInfo_pointerNumber;
    for ( __ui32 l = 2 ; l <= revisionPage->indirection.level ; l++ ) coveredArea *= Indirection_pointerNumber;
  
    if ( coveredArea <= indirectionOffset )
      {
        if ( ! write )
          {
            Bug ( "Overbound the Covered area ! indirOffset=0x%llx, coveredArea=0x%llx\n", indirectionOffset, coveredArea );
          }
        AssertBug ( indirectionOffset < coveredArea * Indirection_pointerNumber, "Invalid multi-hop over-covered fetch !\n" );
        
        increaseIndirectionCoveredArea ();
    
        /**
         * Recompute coveredArea from scratch (we could just do coveredArea *= Indirection_pointerNumber here)
         */
        coveredArea = PageInfo_pointerNumber;
        for ( __ui32 l = 2 ; l <= revisionPage->indirection.level ; l++ ) coveredArea *= Indirection_pointerNumber;
        
        AssertBug (  coveredArea > indirectionOffset, "Did not increase covered area !\n" );
      }
      
    if ( revisionPage->indirection.level == 1 )
      {
        PageInfoPagePtr headPageInfoPagePtr = revisionPage->indirection.firstPage;
        if ( __isStolen(headPageInfoPagePtr) && write )
          {
            /*
             * We must steal the PageInfo we have on the head
             */
            headPageInfoPagePtr = stealPageInfoPage ( headPageInfoPagePtr );
            AssertBug ( !__isStolen(headPageInfoPagePtr), "Stupid.\n" );
            AssertBug ( __getPageType(headPageInfoPagePtr) == PageType_PageInfo, "Invalid page type !\n" );

            alterRevisionPage ();
            revisionPage->indirection.firstPage = headPageInfoPagePtr;
            protectRevisionPage ();

            Log_Indir ( "Stolen pageInfo at %llx\n", headPageInfoPagePtr );
          }
        Log_Indir ( "Getting head PageInfoPage at %llx\n", revisionPage->indirection.firstPage );
        return revisionPage->indirection.firstPage;
      }
    IndirectionPagePtr headIndirectionPagePtr = revisionPage->indirection.firstPage;
    if ( __isStolen(headIndirectionPagePtr) && write )
      {
        headIndirectionPagePtr = stealIndirectionPage ( headIndirectionPagePtr );
        AssertBug ( __getPageType(headIndirectionPagePtr) == PageType_Indirection, "Invalid page type !\n" );
        AssertBug ( !__isStolen(headIndirectionPagePtr), "Stupid.\n" );
        alterRevisionPage ();
        revisionPage->indirection.firstPage = headIndirectionPagePtr;
        protectRevisionPage ();
        
        Log_Indir ( "[%llx:%llx] Stolen indirection at %llx\n", _brid(getBranchRevId()), headIndirectionPagePtr );
      }
    return fetchPageInfoPagePtrFromIndirection ( headIndirectionPagePtr, revisionPage->indirection.level, indirectionOffset, write );
  }

  PageInfoPagePtr PersistentDocumentAllocator::fetchPageInfoPagePtrFromIndirection ( IndirectionPagePtr indirectionPagePtr, __ui32 currentLevel,
          __ui64 indirectionOffset, bool write )
  {
    AssertBug ( __getPageType(indirectionPagePtr) == PageType_Indirection, "Invalid page type !\n" );

    __ui64 localOffset = indirectionOffset / PageInfo_pointerNumber;
    for ( __ui32 l = 2 ; l < currentLevel ; l++ )
      localOffset = localOffset/Indirection_pointerNumber;

    AssertBug ( localOffset < Indirection_pointerNumber, "Local offset=%llx out of range !\n", localOffset );

    Log_Indir ( "Fetching [%llx:%llx] : indir=%llx, currentLevel=%u, indirOffset=%llx->localOffset=%llx, write=%d\n",
        _brid(getBranchRevId()), indirectionPagePtr, currentLevel, indirectionOffset, localOffset, write );
    
    IndirectionPage* indirectionPage = getIndirectionPage ( indirectionPagePtr );

    if ( !indirectionPage->pointers[localOffset] )
      {
        Log_Indir ( "--> pointer is empty !\n" );
        if ( !write ) return NULL;
        if ( currentLevel == 2 )
          {
            /*
             * TODO : call getFreePageInfoPagePtr(false), get the PageInfoPage*, and associate it with pageInfoPageTable directly
             */
            PageInfoPagePtr newPageInfoPagePtr = getFreePageInfoPagePtr ( true );
            getPersistentStore().alterPage ( indirectionPage );
            indirectionPage->pointers[localOffset] = newPageInfoPagePtr;
            getPersistentStore().protectPage ( indirectionPage );
          }
        else
          {
            PageInfoPagePtr newIndirectionPtr = getFreeIndirectionPagePtr ( true );
            getPersistentStore().alterPage ( indirectionPage );
            indirectionPage->pointers[localOffset] = newIndirectionPtr;
            getPersistentStore().protectPage ( indirectionPage );
          }
      } 
    
    if ( __isStolen(indirectionPage->pointers[localOffset]) && write )
      {
        AssertBug ( !__isStolen(indirectionPagePtr), "Father was stolen !\n" );
        if ( currentLevel == 2 )
          {
            PageInfoPagePtr newPageInfoPagePtr = stealPageInfoPage ( indirectionPage->pointers[localOffset] );
            getPersistentStore().alterPage ( indirectionPage );
            indirectionPage->pointers[localOffset] = newPageInfoPagePtr;
            getPersistentStore().protectPage ( indirectionPage );           
          }
        else
          {
            IndirectionPagePtr newIndirectionPagePtr = stealIndirectionPage ( indirectionPage->pointers[localOffset] );
            getPersistentStore().alterPage ( indirectionPage );
            indirectionPage->pointers[localOffset] = newIndirectionPagePtr;
            getPersistentStore().protectPage ( indirectionPage );
          }
      }
    if ( currentLevel > 2 )
      {
        /*
         * We have pointers to IndirectionPage, make them recursive
         */
        __ui64 coveredArea = PageInfo_pointerNumber;
        for ( __ui32 l = 2 ; l < currentLevel ; l++ )
            coveredArea *= Indirection_pointerNumber;
        __ui64 newIndirectionOffset = indirectionOffset & (coveredArea-1);
        Log_Indir ( "[NEWOFF] indir=%llx(local=%llx), level=%x, covered=%llx => new=%llx\n",
            indirectionOffset, localOffset, currentLevel, coveredArea, newIndirectionOffset );
        return fetchPageInfoPagePtrFromIndirection ( indirectionPage->pointers[localOffset], currentLevel - 1, newIndirectionOffset, write );
      }

    Log_Indir ( "Fetching [%llx:%llx] : indir=%llx, currentLevel=%u, indirOffset=%llx->localOffset=%llx, write=%d-->%llx\n",
        _brid(getBranchRevId()), indirectionPagePtr, currentLevel, indirectionOffset, localOffset, write,
        indirectionPage->pointers[localOffset] );

    PageInfoPagePtr pageInfoPagePtr = indirectionPage->pointers[localOffset];
    return pageInfoPagePtr;
  }
  
  AbsolutePagePtr PersistentDocumentAllocator::stealSegmentPage ( SegmentPage* oldPage, PageType pageType )
  {
    AssertBug ( pageType == PageType_Segment, "Invalid type !\n" );
    /*
     * We have to do a copy of the existing page before assigning it to the relative page offset.
     * To do so, make the Store mmap() this page, and perform the copy here.
     * This costs us two syscalls (one mmap(), one munmap()), maybe a write(2) would be cheaper.
     * \todo Test COW using write(2) instead of PersistentStore::getAbsolutePage().
     */
    AbsolutePagePtr newPagePtr = __getFreePagePtr ( pageType ) | PageType_Segment;
    SegmentPage *newPage = getSegmentPage ( newPagePtr );
    
    Log_Indir ( "Steal segmentPage : oldPage=%p, newPage=%p, newPagePtr=0x%llx\n", oldPage, newPage, newPagePtr );
    getPersistentStore().alterPage ( newPage );
    memcpy ( newPage, oldPage, PageSize );
    getPersistentStore().protectPage ( newPage );
    syncPage ( newPage );
    releasePage ( newPagePtr );
    return newPagePtr;
  }

  PageInfoPagePtr PersistentDocumentAllocator::stealPageInfoPage ( PageInfoPagePtr stolenPagePtr )
  {
    AssertBug ( __getPageType(stolenPagePtr) == PageType_PageInfo, "Invalid page type !\n" );
  
    AbsolutePagePtr newPagePtr = getFreePageInfoPagePtr ();

    AssertBug ( __getPageType(newPagePtr) == PageType_PageInfo, "Invalid page type !\n" );

    Log_Indir ( "COW : copying PageInfo %llx->%llx\n", stolenPagePtr, newPagePtr );
    
    PageInfoPage* oldPage = getPageInfoPage ( stolenPagePtr );
    PageInfoPage* newPage = getPageInfoPage ( newPagePtr );
    
    getPersistentStore().alterPage ( newPage );
    memcpy ( newPage, oldPage, PageSize );
    getPersistentStore().protectPage ( newPage );    
    
    syncPage ( newPage );
    releasePage ( newPagePtr & PagePtr_Mask );
    return newPagePtr;
  }


  AbsolutePagePtr PersistentDocumentAllocator::stealIndirectionPage ( AbsolutePagePtr stolenPagePtr )
  {
    AssertBug ( __getPageType(stolenPagePtr) == PageType_Indirection, "Invalid page type !\n" );

    IndirectionPage* oldPage = getIndirectionPage ( stolenPagePtr );

    AbsolutePagePtr newPagePtr = getFreeIndirectionPagePtr();

    AssertBug ( __getPageType(newPagePtr) == PageType_Indirection, "Invalid page type !\n" );

    IndirectionPage* newPage = getIndirectionPage ( newPagePtr );

    getPersistentStore().alterPage ( newPage );
    for ( __ui64 index = 0 ; index < Indirection_pointerNumber ; index ++ )
      {
        newPage->pointers[index] = oldPage->pointers[index];
        if ( newPage->pointers[index] )
          {
            Log_Indir ( "Indirection steal : %llx->%llx, steal at=%llx\n", stolenPagePtr, newPagePtr, index );
            newPage->pointers[index] |= PageFlags_Stolen;
          }
        else
          {
            Log_Indir ( "Indirection steal : %llx->%llx, empty at=%llx\n", stolenPagePtr, newPagePtr, index );
          }
      }
    getPersistentStore().protectPage ( newPage );
    syncPage ( newPage );
    releasePage ( newPagePtr );
    return newPagePtr | PageType_Indirection;
  }

};

