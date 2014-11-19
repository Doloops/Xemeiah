#include <Xemeiah/persistence/persistentbranchmanager.h>
#include <Xemeiah/persistence/persistentdocumentallocator.h>
#include <Xemeiah/trace.h>

#include <Xemeiah/auto-inline.hpp>
#include <Xemeiah/persistence/auto-inline.hpp>

#include <map>

#define Log_PDADrop Debug

namespace Xem
{

  void PersistentDocumentAllocator::drop ()
  {
    AssertBug ( revisionPage, "Null RevsionPage !\n" );
    
    Warn ( "Dropping '%llx:%llx'\n", _brid(revisionPage->branchRevId) );

    AbsolutePagePtr revisionPagePtr = revisionPage->revisionAbsolutePagePtr;
    AbsolutePagePtr lastRevisionPagePtr = revisionPage->lastRevisionPage;
  
    BranchPage* branchPage = NULL; // getPersistentStore().getPersistentBranchManager().getBranchPage ( getBranchRevId().branchId );
    Bug ( "." );
    
    if ( branchPage->lastRevisionPage != revisionPagePtr )
      {
        throwException ( Exception, "Could not drop revision '%llx:%llx' : "
          "branchPage->lastRevisionPage=%llx whereas revisionPage->revisionAbsolutePagePtr=%llx\n",
          _brid(revisionPage->branchRevId), branchPage->lastRevisionPage, revisionPagePtr );
      }

#if 0
    if ( scheduledRevisionRemoval && revisionPage->lastRevisionPage == NullPage && branchPage->lastRevisionPage == revisionPagePtr )
      {
        Log_PDADrop ( "At revision drop for '%llx:%llx' : I am the only revision of the branch, scheduling this branch for removal.\n",
          _brid(revisionPage->branchRevId) );
        scheduleBranchForRemoval ();      
      }
#endif

    /*
     * Update the last revision of the branch's revision list
     */
    getPersistentStore().alterPage ( branchPage );
    branchPage->lastRevisionPage = lastRevisionPagePtr;
    getPersistentStore().protectPage ( branchPage );

    /*
     * Drop the freePageList local cache
     */
    flushFreePageList ();
    
    /**
     * Flush in-mem caches
     */
    flushInMemCaches ();
    
    /*
     * Drop all owned pages
     */
    dropAllPages ();
  }

  class DropStats
  {
  public:
    __ui64 droppedPages[PageType_Mask];
    typedef std::map<AbsolutePagePtr,bool> DropMap;
    DropMap dropMap;
  };

  bool PersistentDocumentAllocator::dropOwnedIndirectionPage ( RelativePagePtr relPagePtr, AbsolutePagePtr absPagePtr, PageType pageType, bool isStolen, void* arg )
  {
    DropStats* stats = (DropStats*) arg;
    if ( isStolen )
      {
        Log_PDADrop ( "dropOwnedPage : will not drop page %llx (type %llx) because it is stolen\n",
          absPagePtr, pageType );
        return false;
      }
    switch ( pageType )
    {
    case PageType_Revision:
      Warn ( "dropOwnedPage : will not drop revisionPage at %llx\n", absPagePtr );
      return true;
    case PageType_Indirection:
      if ( __isStolen ( absPagePtr ) )
        {
          Log_PDADrop ( "dropOwnedPage : Will not drop page %llx, because it is stolen !\n", absPagePtr );
          return true;
        }
      break;
    case PageType_PageInfo:
      if ( __isStolen ( absPagePtr ) )
        {
          Log_PDADrop ( "dropOwnedPage : Will not drop page %llx, because it is stolen !\n", absPagePtr );
          return true;
        }
      break;
    }  
    stats->droppedPages[pageType] ++;
    Log_PDADrop ( "Dropping page abs=%llx, type=%llx\n", absPagePtr, pageType );
    stats->dropMap[absPagePtr & PagePtr_Mask] = true;
    return true;
  }


  bool PersistentDocumentAllocator::dropOwnedSegmentPage ( RelativePagePtr relPagePtr, PageInfo& pageInfo, PageType pageType, bool isStolen, void* arg )
  {
    DropStats* stats = (DropStats*) arg;
    AbsolutePagePtr absPagePtr = pageInfo.absolutePagePtr;

    if ( bridcmp(getBranchRevId(),pageInfo.branchRevId) )
      {
        Warn ( "dropOwnedSegmentPage : told it was dropped, but it was not !\n" );
      }
    if ( isStolen )
      {
        Log_PDADrop ( "dropOwnedPage : will not drop page %llx (type %llx) because it is stolen\n",
          absPagePtr, pageType );
        return false;
      }
    switch ( pageType )
    {
    case PageType_Segment:
      break;
    default:
      Bug ( "Invalid pageType for dropped page abs=%llx, type=%llx\n", absPagePtr, pageType );
    }
    stats->droppedPages[pageType] ++;
    Log_PDADrop ( "Dropping page abs=%llx, type=%llx\n", absPagePtr, pageType );
    stats->dropMap[absPagePtr & PagePtr_Mask] = true;
    return true;
  }


  bool PersistentDocumentAllocator::dropAllPages ()
  {
    AssertBug ( revisionPage, "Null RevsionPage !\n" );
    // AssertBug ( revisionPage->writable == true, "Previous RevisionPage is writable ! commit first !\n" );
  
    DropStats dropStats;
    memset ( &(dropStats.droppedPages), 0, sizeof(dropStats.droppedPages) );

#if 1
    if ( revisionPage->freePageList )
      {
        PageList* pageList = getPageList ( revisionPage->freePageList );
        for ( __ui64 index = 0 ; index < pageList->number ; index++ )
          dropStats.dropMap[pageList->pages[index]] = true;
        releasePage ( revisionPage->freePageList );
        dropStats.dropMap[revisionPage->freePageList] = true;
      }
#endif

    Info ( "Revision %llx:%llx has %llu owned pages, and %lu free pages:\n", 
      _brid(getBranchRevId()), getRevisionPage()->ownedPages, (unsigned long) dropStats.dropMap.size() );

    forAllIndirectionPages ( &PersistentDocumentAllocator::dropOwnedIndirectionPage, &PersistentDocumentAllocator::dropOwnedSegmentPage, 
          &dropStats, false, true );

    for ( __ui64 type = 0 ; type < PageType_Mask ; type++ )
      {
        if ( getRevisionPage()->ownedTypedPages[type] == 0
          && dropStats.droppedPages[type] == 0 )
          continue;
        Info ( "\ttype=%llx:%s : owned=%llu, dropped=%llu, %s\n", 
          type, PersistencePageTypeName[type],
          getRevisionPage()->ownedTypedPages[type],
          dropStats.droppedPages[type],
          getRevisionPage()->ownedTypedPages[type] == dropStats.droppedPages[type] ? "Ok." : "! ERROR !"
          );
      }
      
    __ui64 droppedPages = dropStats.droppedPages[PageType_Indirection]
      + dropStats.droppedPages[PageType_PageInfo]
      + dropStats.droppedPages[PageType_Segment];
    __ui64 ownedPages = getRevisionPage()->ownedPages - getRevisionPage()->ownedTypedPages[PageType_Revision];

    if ( ownedPages != droppedPages )
      {
        Warn ( "Could not drop all pages : ownedPages=%llu, but dropped %llu pages\n",
          ownedPages, droppedPages );
      }
    Info ( "Dropping %lu pages.\n", (unsigned long) dropStats.dropMap.size() );
    for ( DropStats::DropMap::iterator iter = dropStats.dropMap.begin ();
      iter != dropStats.dropMap.end () ; iter++ )
      {
        AssertBug ( iter->second, "A drop=false page has been inserted for page '%llx'\n", iter->first );
        Log_PDADrop ( "Dropping : 0x%llx\n", iter->first );
        getPersistentStore().freePage ( iter->first );
      }

    AbsolutePagePtr revisionPagePtr = revisionPage->revisionAbsolutePagePtr;

    /*
     * Make sure no further (corrupted) reference will be done using this revision page.
     */
    alterRevisionPage ();
    memset ( revisionPage, 0, PageSize );
    protectRevisionPage ();

    /*
     * Unmap all contents, including revisionPage pointer
     */
    unmapAllAreas ();

    /*
     * Put the revision page out of absolute pages cache
     */
    if ( false )
      {
        Lock lock ( mapMutex );
        releasePage ( revisionPagePtr );
      }
    /*
     * Free the revision page
     */
    getPersistentStore().freePage ( revisionPagePtr );

    return true;
  }

};

