#include <Xemeiah/persistence/persistentstore.h>
#include <Xemeiah/persistence/persistentbranchmanager.h>
#include <Xemeiah/persistence/persistentdocument.h>
#include <Xemeiah/trace.h>

#include <Xemeiah/persistence/format/keys.h>
#include <string.h> // For memset.

#include <list>
#include <map>

#include <Xemeiah/auto-inline.hpp>
#include <Xemeiah/persistence/auto-inline.hpp>

namespace Xem
{

#define __incrementAndRelease(__ptr,__member) \
 do { \
  AbsolutePagePtr __tmp = __member; \
  releasePage ( __ptr ); \
  __ptr = __tmp; } while ( 0 )

#define __printCheck(...) fprintf ( stderr, "[CHECK]" __VA_ARGS__ );

#define XEM_CHECK_LOG
#ifdef XEM_CHECK_LOG
#define __XEM_LOG_INDIR
#define __XEM_LOG_REV
#define __XEM_LOG_BRANCH

#define Log_Check __printCheck
#else
#define Log_Check __printCheck

#endif


#ifdef __XEM_LOG_INDIR
#define Log_Indir __printCheck
#else
#define Log_Indir(...)
#endif

#ifdef __XEM_LOG_SK
#define Log_SK __printCheck
#else
#define Log_SK(...)
#endif

#ifdef __XEM_LOG_REV
#define Log_Rev __printCheck
#else
#define Log_Rev(...)
#endif

#ifdef __XEM_LOG_BRANCH
#define Log_Branch __printCheck
#else
#define Log_Branch(...)
#endif

#define __Stats_Ratio(__nb,__total) \
  __nb, (__total) ? ((__nb)*100) / (__total) : 0


#define Log_Stats(__logger,__stats) \
  __logger ( "\tTotal Pages=%llu, Stolen=%llu\n",			\
	     __stats.getTotalPages(), __stats.getTotalStolenPages() );	\
  for ( __ui64 type = 0 ; type < PageType_Mask ; type++ )		\
    { if ( __stats.pages[type] )					\
	{ __logger ( "\tType %s : %llu pages (%llu%%).\n",		\
		     __getTypeName(type),				\
		     __Stats_Ratio(__stats.pages[type], __stats.getTotalPages() ) ); } \
      if ( __stats.stolenPages[type] )					\
	{ __logger ( "\tType %s : %llu stolen pages (%llu%% of stolen) (avg %g stolen/page).\n",		\
		     __getTypeName(type),				\
		     __Stats_Ratio(__stats.stolenPages[type], __stats.getTotalStolenPages() ), \
         __stats.pages[type] ? ( ((double) __stats.stolenPages[type] ) / (double)__stats.pages[type] ) : 0 ); } \
    }

#if 0
#define __Stats(__c,__s) "[TO IMPLEMENT]", __c

#define __Stats(__c, stats)						\
  ": \n"								\
  "\townedPages=%llu\n"							\
  "\tsegmentPages=%llu(%llu%%)\n"					\
  "\tindirectionPages=%llu(%llu%%)\n "					\
  "\tskPages=%llu(%llu%%)\n"						\
  "\tstolenPages=%llu\n",						\
    __c,								\
    stats.ownedPages,							\
    __Stats_Ratio(stats.pages[PageType_Segment], stats.ownedPages), \
    __Stats_Ratio(stats.pages[PageType_Indirection], stats.ownedPages), \
    __Stats_Ratio(stats.pages[PageType_SKIndex], stats.ownedPages), \
    stats.stolenPages						       
#endif
#define _Error Error

  #define __getTypeName(__t) (PersistencePageTypeName[__t])

  bool PersistentStore::check ( CheckFlag flag )
  {
    switch ( flag )
      {
      case Check_Internals:
      {
        checkFormat ();
        Log_Check ( "Check Step 1 :Basic in-mem structures check\n" );
        Log_Check ( "\tnoMansLand at %llu(0x%llx). Total alloced=%llu Mb.\n", 
	          getSB()->noMansLand, getSB()->noMansLand, 
	          (getSB()->noMansLand) / ( 1024 * 1024 ) );
        Log_Check ( "\tfileLength=%lu (0x%lx), 0x%llx pages.\n", 
            (unsigned long) fileLength, (unsigned long) fileLength, fileLength / PageSize );
        Log_Check ( "\tAlloced %llu%% Pages.\n",
		        ( getSB()->noMansLand * 100 ) / ( fileLength) );
#ifdef XEM_MEM_PROTECT_TABLE
        Log_Check ( "XMPC : Check for Altered Pages, table has %llx entries  \n",
	            mem_pages_table_size );
        __ui64 alteredPages = 0;
        for ( __ui64 idx = 0 ; idx < mem_pages_table_size ; idx++ )
          if ( mem_pages_table[idx] )
            {
              Log_Check ( "[!!] XMPC : Page %llx has a refCount=%u(0x%x)\n", 
		          idx, mem_pages_table[idx], mem_pages_table[idx] );
              alteredPages++;
            }
        Log_Check ( "XMPC : Found %llx pages altered over %llx entries.\n",
	            alteredPages, mem_pages_table_size );
#endif
	      AllocationStats stats;
	      checkKeys ( stats );
        break;
      }
      case Check_AllContents:
        Log_Check ( "Check Step 2 : Checking all contents.\n" );
        checkAllContents ();
        break;
      case Check_Clean:
        
        break;
      }
    return true;
  }


  bool PersistentStore::checkFormat ()
  {
    SuperBlock* sb = getSB ();
    Log_Check ( "sb at %p\n", sb );
    if ( strncmp ( sb->magic, XEM_SB_MAGIC, sb->magic_length ) != 0 )
      {
        Error ( "Invalid file magic '%s'\n", sb->magic );
        return false;
      }
    if ( strncmp ( sb->version, XEM_SB_VERSION, sb->version_length ) != 0 )
      {
        Error ( "Invalid file version '%s'\n", sb->version );
        return false;
      }
    Log_Check ( "Check : sb->magic : '%s', sb->version : '%s'\n", sb->magic, sb->version );
    // TODO : check SuperBlock sizes
    return true;
  }


  /*
   * Checking All Contents.
   */


  void PersistentStore::checkAllContents ( )
  {
    SuperBlock* sb = getSB ();
    Log_Check ( "SuperBlock : \n" );
    Log_Check ( "\tlastBranch=%llx, keyPage=%llx\n"
	        "\tfreePageHeader=%llx, noMansLand=%llx\n",
	        sb->lastBranch, sb->keyPage,
	        sb->freePageHeader, sb->noMansLand );
    Log_Check ( "\tnextElementId=%llx\n",
	        sb->nextElementId );
    AllocationStats allStats;
    allStats.initPageTable ( sb->noMansLand );
    checkFreePageHeader ( allStats );
    checkKeys ( allStats );

    for ( AbsolutePagePtr branchPagePtr = sb->lastBranch ; branchPagePtr ; )
      {
        BranchPage* branchPage = getAbsolutePage<BranchPage> ( branchPagePtr );
        checkBranch ( branchPagePtr, branchPage, allStats );
        __incrementAndRelease ( branchPagePtr, branchPage->lastBranch );
      }    
    Log_Check ( "\n" );
    Log_Check  ( "AllStats for sb=%p\n", getSB() );
    Log_Stats ( Log_Check, allStats );
    Log_Check ( "Allocation : total known pages=%llu, alloced=%llu\n",
	        allStats.getTotalPages(), sb->noMansLand >> InPageBits );
    __ui64 nbFreePages = 0, totalPages = allStats.getTotalPages();

    Log_Check ( "Allocation Stats : \n"
	        "\tUsed pages : keys=%llu, branches=%llu, revs=%llu,\n"
	        "\tindir=%llu, pageInfo=%llu, segment=%llu,\n"
	        "\ttotal used pages=%llu,\n"
	        "\tfree pages=%llu\n"
	        "\ttotal known pages=%llu, alloced=%llu\n",
	        allStats.pages[PageType_Key], 
	        allStats.pages[PageType_Branch], 
	        allStats.pages[PageType_Revision], 
	        allStats.pages[PageType_Indirection], 
	        allStats.pages[PageType_PageInfo], 
	        allStats.pages[PageType_Segment], 
	        totalPages,
	        nbFreePages,
	        totalPages + nbFreePages + 2, sb->noMansLand >> InPageBits );

#ifdef __XEM_COUNT_FREEPAGES
    Log_Check ( "\tSuperBlock has nbFreePages=%llu\n", getSB()->nbFreePages );
    if ( allStats.pages[PageType_FreePageList] + allStats.pages[PageType_FreePage]
	        != getSB()->nbFreePages )
	     {
	        Log_Check ( "Wrong number of free pages : SuperBlock has %llu, stats have %llu\n",
	          getSB()->nbFreePages, 
	          allStats.pages[PageType_FreePageList] + allStats.pages[PageType_FreePage] );
	     }
#endif
    bool doPutPagesInAttic = false;
    AllocationStats::AbsolutePagePtrList unsetPages;
    allStats.checkUnsetPages ( unsetPages );
    if ( doPutPagesInAttic && unsetPages.size() )
      {
        putPagesInAttic ( unsetPages );
      }
#if 0 // Clear up freePageHeader - WHAT FOR ????
    FreePageHeader* fpHeader = getFreePageHeader ();
    alterPage ( fpHeader );
    fpHeader->firstFreePageList = NullPage;
    fpHeader->currentFreedPageList = NullPage;
    fpHeader->ownFreePageList     = NullPage;
    protectPage ( fpHeader );
#endif
  }

 
  void PersistentStore::checkKeys (  AllocationStats& stats )
  {
    for ( AbsolutePagePtr keyPagePtr = getSB()->keyPage ; keyPagePtr ; )
      {
        KeyPage* keyPage = getAbsolutePage<KeyPage> ( keyPagePtr );
        stats.referencePage ( "Check Keys", keyPagePtr & PagePtr_Mask, PageType_Key, false );
        __incrementAndRelease ( keyPagePtr, keyPage->nextPage );
      }
    for ( AbsolutePagePtr nsPagePtr = getSB()->namespacePage ; nsPagePtr ; )
      {
        NamespacePage* nsPage = getAbsolutePage<NamespacePage> ( nsPagePtr );
        stats.referencePage ( "Check Namespaces", nsPagePtr & PagePtr_Mask, PageType_Key, false );
        __incrementAndRelease ( nsPagePtr, nsPage->nextPage );
      }
  }

  void PersistentStore::checkFreePageHeader (  AllocationStats& stats )
  {
    FreePageHeader* fpHeader = getFreePageHeader ();
    Log_Check ( "FreePageHeader :\n" );
    Log_Check ( "\tfirstFreePageList=%llx\n"
	        "\townFreePageList=%llx\n"
	        "\tcurrentFreedPageList=%llx\n"
	        "\tpreAllocatedPageList=%llx\n",
	        fpHeader->firstFreePageList,
	        fpHeader->ownFreePageList,
	        fpHeader->currentFreedPageList,
	        fpHeader->preAllocatedPageList );
   
    if ( fpHeader->currentFreedPageList )
      {
        stats.referencePage ( "CurrentFreedPageList", fpHeader->currentFreedPageList, PageType_FreePageList, false );
        PageList* pageList = getAbsolutePage<PageList> ( fpHeader->currentFreedPageList );
        for ( __ui64 index = 0 ; index < pageList->number ; index++ )
      	  {
      	    // Log_Check ( "\t\tcurrentFreed : Page[%llx]=%llx\n", index, pageList->pages[index] );
      	    stats.referencePage ( "CurrentFreedPageList-Content", pageList->pages[index], PageType_FreePage, false );
      	  }
        releasePage ( fpHeader->currentFreedPageList );
      }
    
    if ( fpHeader->ownFreePageList )
      {
        stats.referencePage ( "ownFreePageList", fpHeader->ownFreePageList, PageType_FreePageList, false );
        PageList* pageList = getAbsolutePage<PageList> ( fpHeader->ownFreePageList  );
        for ( __ui64 index = 0 ; index < pageList->number ; index++ )
      	  {
      	    // Log_Check ( "\t\townFreePageList : Page[%llx]=%llx\n", index, pageList->pages[index] );
      	    stats.referencePage ( "ownFreePageList-Content", pageList->pages[index], PageType_FreePage, false );
      	  }
        releasePage ( fpHeader->ownFreePageList );
      }

//  for ( pageList = getAbsolutePage<PageList> ( fpHeader->firstFreePageList ) ;
//    pageList ; pageList = getAbsolutePage<PageList> ( pageList->nextPage ) )
    for ( AbsolutePagePtr pageListPtr = fpHeader->firstFreePageList ; pageListPtr ; )
      {
        PageList* pageList = getAbsolutePage<PageList> ( pageListPtr );
        Log_Check ( "\tAt freePageList=%llx, next=%llx, number=%llx\n",
            pageListPtr,
            pageList->nextPage, pageList->number );
        if ( pageList->number != pageList->maxNumber )
          {
            Warn ( "FreePageList at %llx : Non full page ! (%llu/%llu)\n",
              pageListPtr, pageList->number, pageList->maxNumber );
          }
        stats.referencePage ( "FreePageList", pageListPtr, PageType_FreePageList, false );
        for ( __ui64 index = 0 ; index < pageList->number ; index++ )
      	  {
      	    stats.referencePage ( "FreePageList-Content", pageList->pages[index], PageType_FreePage, false );
      	  }
        __incrementAndRelease ( pageListPtr, pageList->nextPage );
      }
      
    for ( AbsolutePagePtr pageListPtr = fpHeader->atticPageList ; pageListPtr ; )
      {
        PageList* pageList = getAbsolutePage<PageList> ( pageListPtr );
        Log_Check ( "\tAt atticPageList=%llx, next=%llx, number=%llx\n",
            pageListPtr,
            pageList->nextPage, pageList->number );
        stats.referencePage ( "atticPageList", pageListPtr, PageType_FreePageList, false );
        for ( __ui64 index = 0 ; index < pageList->number ; index++ )
      	  {
      	    stats.referencePage ( "atticPageList-Content", pageList->pages[index], PageType_FreePage, false );
      	  }
        __incrementAndRelease ( pageListPtr, pageList->nextPage );
      }      
    Log_Check ( "\tTotal free Page Lists : %llu, free Pages : %llu\n", 
	        stats.pages[PageType_FreePageList],
	        stats.pages[PageType_FreePage] );
  }

  void PersistentStore::putPagesInAttic ( AllocationStats::AbsolutePagePtrList& unsetPageList )
  {
    FreePageHeader* fpHeader = getFreePageHeader ();
    AbsolutePagePtr atticPageListPtr = NullPage;
    PageList* atticPageList = NULL;
    for ( AbsolutePagePtr atticPageListPtr = fpHeader->atticPageList ; atticPageListPtr ; )
      {
        atticPageList = getAbsolutePage<PageList> ( atticPageListPtr );
        if ( atticPageList->number < atticPageList->maxNumber )
          break;
        __incrementAndRelease ( atticPageListPtr, atticPageList->nextPage );
      }
    if ( atticPageListPtr == NullPage )
      {
        Log_Check ( "Creating new atticPageList...\n" );
        atticPageListPtr = getFreePagePtr();
        Log_Check ( "Creating new atticPageList at %llx\n", atticPageListPtr );
        atticPageList = getAbsolutePage<PageList> ( atticPageListPtr );
        alterPage ( atticPageList );
        memset ( atticPageList, 0, PageSize );
        protectPage ( atticPageList );
        syncPage ( atticPageList );
        alterPage ( fpHeader );
        fpHeader->atticPageList = atticPageListPtr;
        protectPage ( fpHeader );
        syncPage ( fpHeader );
      }

    for ( AllocationStats::AbsolutePagePtrList::iterator iter = unsetPageList.begin ();
      iter != unsetPageList.end() ; iter++ )
      {
        AbsolutePagePtr absPagePtr = *iter;
        Log_Check ( "\tPutting %llx in attic.\n", absPagePtr );
        if ( atticPageList->number == atticPageList->maxNumber )
          {
            Fatal ( "NotImplemented : while putting in attic, first attic page list is full.\n" );
          }
        alterPage ( atticPageList );
        atticPageList->pages[atticPageList->number++] = absPagePtr;
        protectPage ( atticPageList );
        syncPage ( atticPageList );
#if 0 // COUNT_FREEPAGES
        lockSB ();
        getSB()->nbFreePages ++;
        unlockSB ();
#endif
      }
  
  }  
  
  void PersistentStore::checkBranch ( AbsolutePagePtr branchPagePtr, BranchPage* branchPage, AllocationStats& _stats )
  {
    Log_Branch ( "Branch page=0x%llx (%p)\n", branchPagePtr, branchPage );
    Log_Branch ( "\tbranchId=%llx, name='%s', forkedFrom=%llx:%llx, lastRevision=%llx\n",
	   branchPage->branchId, branchPage->name, _brid(branchPage->forkedFrom),
	   branchPage->lastRevisionPage );
    AllocationStats stats(_stats);
    stats.referencePage ( "BranchPage", branchPagePtr, PageType_Branch, false );
    stats.initBranchPageTable ();

    for ( AbsolutePagePtr revPagePtr = branchPage->lastRevisionPage ; revPagePtr ; )
      {
        RevisionPage* revPage = getAbsolutePage<RevisionPage> ( revPagePtr );
        checkRevision ( revPagePtr, revPage, stats );
        __incrementAndRelease ( revPagePtr, revPage->lastRevisionPage );
#ifdef __XEM_PERSISTENTSTORE_HAS_PAGEREFERENCECTXT        
        Log_Rev ( "my pageReferenceCtxtNb=%llu -> %llu MBytes\n",
            stats.pageReferenceCtxtNb, (stats.pageReferenceCtxtNb * sizeof(AllocationStats::PageReferenceCtxt)) >> 20 );
#endif // __XEM_PERSISTENTSTORE_HAS_PAGEREFERENCECTXT        
      }
    Log_Branch ( "BranchStats for %llx (at %llx)\n", branchPage->branchId, branchPagePtr );
    Log_Stats ( Log_Branch, stats );

    // sleep ( 1000 );
    
    for ( AbsolutePagePtr revPagePtr = branchPage->lastRevisionPage ; revPagePtr ; )
      {
        RevisionPage* revPage = getAbsolutePage<RevisionPage> ( revPagePtr );
        Log_Branch ( "Checking contents for rev=%llx:%llx\n", _brid(revPage->branchRevId) );
        {
          PersistentDocument* pDoc = getPersistentBranchManager().instanciateTemporaryPersistentDocument ( revPagePtr );
          pDoc->getPersistentDocumentAllocator().checkRelativePages ( stats );
          pDoc->getPersistentDocumentAllocator().checkContents ( );
          delete ( pDoc );
          pDoc = NULL;
        }
        __incrementAndRelease ( revPagePtr, revPage->lastRevisionPage );
      }
    
  }

  void PersistentStore::checkRevision ( AbsolutePagePtr revPagePtr, RevisionPage* revPage, AllocationStats& _stats )
  // Note : this can be the checkRevision() code also...
  {
    AllocationStats stats(_stats);
    stats.referencePage ( "RevisionPage", revPage->branchRevId, NullPage, revPagePtr, PageType_Revision, false );
    Log_Rev ( "Revision page=%llx\n", revPagePtr );
    Log_Rev ( "\tbrid=%llx:%llx, lastRevPage=%llx\n",
	      _brid(revPage->branchRevId), revPage->lastRevisionPage );
    Log_Rev ( "\townedPages=%llu (0x%llx), writable=%llu\n", 
	      revPage->ownedPages, revPage->ownedPages, revPage->documentAllocationHeader.writable );

#if 0
    Log_Rev ( "\trootElement=%llx, elements=%llu (0x%llx)\n",
	      revPage->documentHead.rootElementPtr, revPage->documentHead.elements, revPage->documentHead.elements );
#endif

    Log_Rev ( "\tIndirection page=%llx, level=%u, nextRelativePagePtr=%llx :\n",
	      revPage->indirection.firstPage, revPage->indirection.level, 
	      revPage->documentAllocationHeader.nextRelativePagePtr );
    if ( 1 )
      {
        PersistentDocument* pDoc = getPersistentBranchManager().instanciateTemporaryPersistentDocument ( revPagePtr );
        if ( pDoc )
          {
            pDoc->getPersistentDocumentAllocator().checkPages ( stats );
            // pDoc->checkContents ( stats );
            delete ( pDoc );
            pDoc = NULL;
          }
        else
          {
            Bug ( "." );
          }
      }
#ifdef __XEM_PERSISTENTSTORE_HAS_PAGEREFERENCECTXT        
    Log_Rev ( "my pageReferenceCtxtNb=%llu -> %llu MBytes\n",
        stats.pageReferenceCtxtNb, (stats.pageReferenceCtxtNb * sizeof(AllocationStats::PageReferenceCtxt)) >> 20 );
#endif // __XEM_PERSISTENTSTORE_HAS_PAGEREFERENCECTXT        
    
    // Log_Rev ( "pageTableSize=%llu (%llu MBytes)\n", 
    //  stats.pageTableSize, (stats.pageTableSize*sizeof(AllocationStats::PageStats)) >> 20 );

    Log_Stats ( Log_Rev, stats );
    __ui64 totalOwned = 0;
    for ( __ui64 type = 0 ; type < PageType_Mask ; type++ )
      if ( revPage->ownedTypedPages[type] )
        {
	        Log_Rev ( "\tType %s (0x%llx), owned=%llu\n",
		          __getTypeName(type), type, revPage->ownedTypedPages[type] );
	        totalOwned += revPage->ownedTypedPages[type];
        }
    if ( revPage->ownedPages != totalOwned )
      {
        Warn ( "Wrong number of owned pages : revPage %llx:%llx has %llu owned pages, counted %llu\n",
	       _brid(revPage->branchRevId), revPage->ownedPages, totalOwned );
      }
    if ( stats.getTotalPages() != revPage->ownedPages )
      {
        Warn ( "Wrong ownedPages number : revPage %llx:%llx has %llu, but counted %llu.\n",
	       _brid(revPage->branchRevId),
	        revPage->ownedPages, stats.getTotalPages() );
      }
    // checkFreeList ( revPage, &(revPage->freeListHeader) );
  }

#if 0
  void PersistentStore::checkFreeList ( RevisionPage* revPage, FreeListHeader* freeListHeader )
  {
    Warn ( "Yet. To re-implement using a Context\n" );
    Log_FL ( "Checking freeList at %llx\n", getSegmentPtr ( revPage, freeListHeader ) );
    for ( int level = 0 ; level < FreeListHeader::levelNumber ; level ++ )
      Log_FL ( "\tfirstFree[%d]=%llx\n", level, freeListHeader->firstFree[level] );
    __ui64 totalFreeSegments = 0, totalFreeBytes = 0;
    for ( int level = 0 ; level < FreeListHeader::levelNumber ; level ++ )
      {
        FreeSegment* last = NULL;
        FreeSegment* freeSeg = getSegment<FreeSegment,Read> 
	  ( revPage, freeListHeader->firstFree[level] );
        //      AssertBug ( !freeSeg || ( freeSeg->last == NullPtr ), "Invalid last for a head freeSegment.\n" );
        for (  ; freeSeg ; freeSeg = getSegment<FreeSegment,Read> ( revPage, freeSeg->next ) )
	  {
	    Log_FL ( "\tLevel=%d, freeSeg=%llx, last=%llx, next=%llx, size=%llx, dummy=%x\n",
		     level, getSegmentPtr(revPage, freeSeg), 
		     freeSeg->last, freeSeg->next, freeSeg->size, freeSeg->dummy );
	    AssertBug ( freeSeg->dummy == FreeSegment_dummy, "Invalid dummy.\n" );
	    int freeSegLevel;
	    computeFreeLevel ( freeSegLevel, freeSeg->size );
	    AssertBug ( freeSegLevel == level, "Invalid level\n" );
	    if ( getSegmentPtr(revPage,last) || freeSeg->last )
	      AssertBug ( freeSeg->last == getSegmentPtr(revPage, last),
			  "Invalid last : last=%llx, freeSeg->last=%llx\n",
			  getSegmentPtr(revPage, last), 
			  freeSeg->last );
	    totalFreeSegments++;
	    totalFreeBytes += freeSeg->size;
	    last = freeSeg;

	  }
      }  
    Log_FL ( "Total of FreeListHeader=%llx : free Segments=%llu (0x%llx), bytes=%llu (0x%llx)\n",
	     getSegmentPtr ( revPage, freeListHeader ),
	     totalFreeSegments, totalFreeSegments, totalFreeBytes, totalFreeBytes );
  }
#endif
};

