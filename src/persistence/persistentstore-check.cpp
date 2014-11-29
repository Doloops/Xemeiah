#include <Xemeiah/persistence/persistentstore.h>
#include <Xemeiah/persistence/persistentbranchmanager.h>
#include <Xemeiah/persistence/persistentdocument.h>
#include <Xemeiah/persistence/allocationstats.h>
#include <Xemeiah/trace.h>

#include <Xemeiah/persistence/format/keys.h>
#include <string.h> // For memset.

#include <list>
#include <map>

#include <Xemeiah/auto-inline.hpp>
#include <Xemeiah/persistence/auto-inline.hpp>

namespace Xem
{

#if 0
#define __incrementAndRelease(__ptr,__member) \
 do { \
  AbsolutePagePtr __tmp = __member; \
  releasePage ( __ptr ); \
  __ptr = __tmp; } while ( 0 )
#endif

#define __printCheck(...) fprintf ( stderr, "[CHECK]" __VA_ARGS__ )

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

    bool
    PersistentStore::check (CheckFlag flag)
    {
        switch (flag)
        {
            case Check_Internals:
            {
                checkFormat();
                Log_Check ( "Check Step 1 : Basic in-mem structures check\n" );
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
                for ( __ui64 idx = 0; idx < mem_pages_table_size; idx++ )
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
                checkKeys(stats);
                break;
            }
            case Check_AllContents:
                Log_Check ( "Check Step 2 : Checking all contents.\n" );
                checkAllContents();
                break;
            case Check_Clean:

                break;
        }
        return true;
    }

    void
    PersistentStore::checkFormat ()
    {
        SuperBlock* sb = getSB();
        Log_Check ( "sb at %p\n", sb );
        if (strncmp(sb->magic, XEM_SB_MAGIC, sb->magic_length) != 0)
        {
            throwException(PersistenceException, "Invalid file magic '%s'\n", sb->magic);
        }
        if (strncmp(sb->version, XEM_SB_VERSION, sb->version_length) != 0)
        {
            throwException(PersistenceException, "Invalid file version '%s'\n", sb->version);
        }
        Log_Check ( "Check : sb->magic : '%s', sb->version : '%s'\n", sb->magic, sb->version );
        // TODO : check SuperBlock sizes
    }

    void
    PersistentStore::buildBranchesHierarchy (BranchesHierarchy& branchesHierarchy)
    {
        SuperBlock* sb = getSB();
        for (AbsolutePagePtr branchPagePtr = sb->lastBranch; branchPagePtr;)
        {
            AbsolutePageRef<BranchPage> branchPageRef = getAbsolutePage<BranchPage>(branchPagePtr);
            BranchId branchId = branchPageRef.getPage()->branchId;
            BranchRevId forkedFrom = branchPageRef.getPage()->forkedFrom;

            branchesHierarchy[branchId].branchPagePtr = branchPagePtr;
            branchesHierarchy[branchId].forkedFrom = forkedFrom;

            if (forkedFrom.branchId)
            {
                branchesHierarchy[forkedFrom.branchId].branchesForkedFromRevision[forkedFrom.revisionId].push_back(
                        branchId);
            }
            branchPagePtr = branchPageRef.getPage()->lastBranch;
        }
    }

    /*
     * Checking All Contents.
     */
    void
    PersistentStore::checkAllContents ()
    {
        SuperBlock* sb = getSB();
        Log_Check ( "SuperBlock : \n" );
        Log_Check ( "\tlastBranch=%llx, keyPage=%llx\n"
                "\tfreePageHeader=%llx, noMansLand=%llx\n",
                sb->lastBranch, sb->keyPage,
                sb->freePageHeader, sb->noMansLand );
        Log_Check ( "\tnextElementId=%llx\n",
                sb->nextElementId );
        AllocationStats allStats;
        allStats.initPageTable(sb->noMansLand);
        checkFreePageHeader(allStats);
        checkKeys(allStats);

        BranchesHierarchy branchesHierarchy;

        buildBranchesHierarchy(branchesHierarchy);

        for (BranchesHierarchy::iterator branchesIter = branchesHierarchy.begin();
                branchesIter != branchesHierarchy.end(); branchesIter++)
        {
            Log("At branch : %llu, forkedFrom=[%llx,%llx], number of forked revisions : %lu\n", branchesIter->first,
                _brid(branchesIter->second.forkedFrom), branchesIter->second.branchesForkedFromRevision.size());
            if (branchesIter->second.forkedFrom.branchId == 0)
            {
                checkBranch(branchesIter->second.branchPagePtr, branchesHierarchy, allStats);
            }
        }

        Log_Check ( "\n" );
        Log_Check ( "AllStats for sb=%p\n", getSB() );
        Log_Stats(Log_Check, allStats);
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
        if (allStats.pages[PageType_FreePageList] + allStats.pages[PageType_FreePage] != getSB()->nbFreePages)
        {
            allStats.addError("Wrong number of free pages : SuperBlock has %llu, stats have %llu\n",
                              getSB()->nbFreePages,
                              allStats.pages[PageType_FreePageList] + allStats.pages[PageType_FreePage]);
        }
#endif
        bool doPutPagesInAttic = false;
        AbsolutePagePtrList unsetPages;
        allStats.checkUnsetPages(unsetPages);
        if (doPutPagesInAttic && unsetPages.size())
        {
            putPagesInAttic(unsetPages);
        }
#if 0 // Clear up freePageHeader - WHAT FOR ????
        FreePageHeader* fpHeader = getFreePageHeader ();
        alterPage ( fpHeader );
        fpHeader->firstFreePageList = NullPage;
        fpHeader->currentFreedPageList = NullPage;
        fpHeader->ownFreePageList = NullPage;
        protectPage ( fpHeader );
#endif
        if ( !allStats.errors->empty() )
        {
            String result;
            for ( std::list<String>::iterator iter = allStats.errors->begin() ; iter != allStats.errors->end() ; iter ++)
            {
                result += *iter;
                result += "\n";
            }
            throwException(PersistenceCheckContentException, "Details : \n%s", result.c_str());
        }
    }

    void
    PersistentStore::checkKeys (AllocationStats& stats)
    {
        for (AbsolutePagePtr keyPagePtr = getSB()->keyPage; keyPagePtr;)
        {
            AbsolutePageRef<KeyPage> keyPageRef = getAbsolutePage<KeyPage>(keyPagePtr);
            stats.referencePage("Check Keys", keyPagePtr & PagePtr_Mask, PageType_Key, false);
            keyPagePtr = keyPageRef.getPage()->nextPage;
        }
        for (AbsolutePagePtr nsPagePtr = getSB()->namespacePage; nsPagePtr;)
        {
            AbsolutePageRef<NamespacePage> nsPageRef = getAbsolutePage<NamespacePage>(nsPagePtr);
            stats.referencePage("Check Namespaces", nsPagePtr & PagePtr_Mask, PageType_Key, false);
            nsPagePtr = nsPageRef.getPage()->nextPage;
        }
    }

    void
    PersistentStore::checkFreePageHeader (AllocationStats& stats)
    {
        AbsolutePageRef<FreePageHeader> fpHeaderRef = getFreePageHeader();
        FreePageHeader* fpHeader = fpHeaderRef.getPage();
        Log_Check ( "FreePageHeader :\n" );
        Log_Check ( "\tfirstFreePageList=%llx\n"
                "\townFreePageList=%llx\n"
                "\tcurrentFreedPageList=%llx\n"
                "\tpreAllocatedPageList=%llx\n",
                fpHeader->firstFreePageList,
                fpHeader->ownFreePageList,
                fpHeader->currentFreedPageList,
                fpHeader->preAllocatedPageList );

        if (fpHeader->currentFreedPageList)
        {
            stats.referencePage("CurrentFreedPageList", fpHeader->currentFreedPageList, PageType_FreePageList, false);
            AbsolutePageRef<PageList> pageListRef = getAbsolutePage<PageList>(fpHeader->currentFreedPageList);
            for (__ui64 index = 0; index < pageListRef.getPage()->number; index++)
            {
                // Log_Check ( "\t\tcurrentFreed : Page[%llx]=%llx\n", index, pageList->pages[index] );
                stats.referencePage("CurrentFreedPageList-Content", pageListRef.getPage()->pages[index],
                PageType_FreePage,
                                    false);
            }
        }

        if (fpHeader->ownFreePageList)
        {
            stats.referencePage("ownFreePageList", fpHeader->ownFreePageList, PageType_FreePageList, false);
            AbsolutePageRef<PageList> pageListRef = getAbsolutePage<PageList>(fpHeader->ownFreePageList);
            for (__ui64 index = 0; index < pageListRef.getPage()->number; index++)
            {
                // Log_Check ( "\t\townFreePageList : Page[%llx]=%llx\n", index, pageList->pages[index] );
                stats.referencePage("ownFreePageList-Content", pageListRef.getPage()->pages[index], PageType_FreePage,
                                    false);
            }
        }

        for (AbsolutePagePtr pageListPtr = fpHeader->firstFreePageList; pageListPtr;)
        {
            AbsolutePageRef<PageList> pageListRef = getAbsolutePage<PageList>(pageListPtr);
            Log_Check ( "\tAt freePageList=%llx, next=%llx, number=%llx\n",
                    pageListPtr,
                    pageListRef.getPage()->nextPage, pageListRef.getPage()->number );
            if (pageListRef.getPage()->number != pageListRef.getPage()->maxNumber)
            {
                Warn("FreePageList at %llx : Non full page ! (%llu/%llu)\n", pageListPtr, pageListRef.getPage()->number,
                     pageListRef.getPage()->maxNumber);
            }
            stats.referencePage("FreePageList", pageListPtr, PageType_FreePageList, false);
            for (__ui64 index = 0; index < pageListRef.getPage()->number; index++)
            {
                stats.referencePage("FreePageList-Content", pageListRef.getPage()->pages[index], PageType_FreePage,
                                    false);
            }
            pageListPtr = pageListRef.getPage()->nextPage;
        }

        for (AbsolutePagePtr pageListPtr = fpHeader->atticPageList; pageListPtr;)
        {
            AbsolutePageRef<PageList> pageListRef = getAbsolutePage<PageList>(pageListPtr);
            Log_Check ( "\tAt atticPageList=%llx, next=%llx, number=%llx\n",
                    pageListPtr,
                    pageListRef.getPage()->nextPage, pageListRef.getPage()->number );
            stats.referencePage("atticPageList", pageListPtr, PageType_FreePageList, false);
            for (__ui64 index = 0; index < pageListRef.getPage()->number; index++)
            {
                stats.referencePage("atticPageList-Content", pageListRef.getPage()->pages[index], PageType_FreePage,
                                    false);
            }
            pageListPtr = pageListRef.getPage()->nextPage;
        }
        Log_Check ( "\tTotal free Page Lists : %llu, free Pages : %llu\n",
                stats.pages[PageType_FreePageList],
                stats.pages[PageType_FreePage] );
    }

    void
    PersistentStore::putPagesInAttic (AbsolutePagePtrList& unsetPageList)
    {
        AbsolutePageRef<FreePageHeader> fpHeaderRef = getFreePageHeader();
        FreePageHeader* fpHeader = fpHeaderRef.getPage();
        AbsolutePagePtr atticPageListPtr = NullPage;
        AbsolutePageRef<PageList> atticPageListRef(*this);
        for (AbsolutePagePtr atticPageListPtr = fpHeader->atticPageList; atticPageListPtr;)
        {
            atticPageListRef = getAbsolutePage<PageList>(atticPageListPtr);
            if (atticPageListRef.getPage()->number < atticPageListRef.getPage()->maxNumber)
            {
                break;
            }
        }
        if (atticPageListPtr == NullPage)
        {
            Log_Check ( "Creating new atticPageList...\n" );
            atticPageListPtr = getFreePagePtr();
            Log_Check ( "Creating new atticPageList at %llx\n", atticPageListPtr );
            atticPageListRef = getAbsolutePage<PageList>(atticPageListPtr);
            alterPage(atticPageListRef.getPage());
            memset(atticPageListRef.getPage(), 0, PageSize);
            protectPage(atticPageListRef.getPage());
            syncPage(atticPageListRef.getPage());
            alterPage(fpHeader);
            fpHeader->atticPageList = atticPageListPtr;
            protectPage(fpHeader);
            syncPage(fpHeader);
        }

        for (AbsolutePagePtrList::iterator iter = unsetPageList.begin(); iter != unsetPageList.end(); iter++)
        {
            AbsolutePagePtr absPagePtr = *iter;
            Log_Check ( "\tPutting %llx in attic.\n", absPagePtr );
            if (atticPageListRef.getPage()->number == atticPageListRef.getPage()->maxNumber)
            {
                Fatal("NotImplemented : while putting in attic, first attic page list is full.\n");
            }
            alterPage(atticPageListRef.getPage());
            atticPageListRef.getPage()->pages[atticPageListRef.getPage()->number++] = absPagePtr;
            protectPage(atticPageListRef.getPage());
            syncPage(atticPageListRef.getPage());
#if 0 // COUNT_FREEPAGES
            lockSB ();
            getSB()->nbFreePages ++;
            unlockSB ();
#endif
        }

    }

    void
    PersistentStore::checkBranch (AbsolutePagePtr branchPagePtr, BranchesHierarchy& hierarchy,
                                  AllocationStats& fatherStats)
    {
        AbsolutePageRef<BranchPage> branchPageRef = getAbsolutePage<BranchPage>(branchPagePtr);
        BranchPage* branchPage = branchPageRef.getPage();

        Log_Branch ( "Branch page=0x%llx (%p)\n", branchPagePtr, branchPage );
        Log_Branch ( "\tbranchId=%llx, name='%s', forkedFrom=%llx:%llx, lastRevision=%llx\n",
                branchPage->branchId, branchPage->name, _brid(branchPage->forkedFrom),
                branchPage->lastRevisionPage );
        AllocationStats stats(fatherStats);
        stats.referencePage("BranchPage", branchPagePtr, PageType_Branch, false);

        Log_Branch ("BranchPageTable %p has %lu records\n", stats.getBranchPageTable(), stats.getBranchPageTable()->size());
        std::list<AbsolutePagePtr> revisionPagePtrList;
        for (AbsolutePagePtr revPagePtr = branchPage->lastRevisionPage; revPagePtr;)
        {
            revisionPagePtrList.push_front(revPagePtr);
            AbsolutePageRef<RevisionPage> revPageRef = getAbsolutePage<RevisionPage>(revPagePtr);
            revPagePtr = revPageRef.getPage()->lastRevisionPage;
#ifdef __XEM_PERSISTENTSTORE_HAS_PAGEREFERENCECTXT        
            Log_Rev ( "my pageReferenceCtxtNb=%llu -> %llu MBytes\n",
                    stats.pageReferenceCtxtNb, (stats.pageReferenceCtxtNb * sizeof(AllocationStats::PageReferenceCtxt)) >> 20 );
#endif // __XEM_PERSISTENTSTORE_HAS_PAGEREFERENCECTXT        
        }
        Log_Branch ( "BranchStats for %llx (at %llx)\n", branchPage->branchId, branchPagePtr );

        for (std::list<AbsolutePagePtr>::iterator revisionPageIterator = revisionPagePtrList.begin();
                revisionPageIterator != revisionPagePtrList.end(); revisionPageIterator++)
        {
            Log_Branch ("Before checkRevision(), BranchPageTable=%p has %lu records\n", stats.getBranchPageTable(), stats.getBranchPageTable()->size());

            AbsolutePageRef<RevisionPage> revPageRef = getAbsolutePage<RevisionPage>(*revisionPageIterator);

            PersistentDocument* pDoc = getPersistentBranchManager().instanciateTemporaryPersistentDocument(
                    revPageRef.getPagePtr());
            AssertBug(pDoc, "Could not create a persistent document at page %llx\n", revPageRef.getPagePtr());

            checkRevision(revPageRef.getPagePtr(), revPageRef.getPage(), pDoc, stats);
            Log_Branch ( "Checking contents for rev=%llx:%llx (branchPageTable=%p has %lu records)\n",
                         _brid(revPageRef.getPage()->branchRevId), stats.getBranchPageTable(), stats.getBranchPageTable()->size() );

            pDoc->getPersistentDocumentAllocator().checkRelativePages(stats);
            pDoc->getPersistentDocumentAllocator().checkContents(stats);
            delete (pDoc);

            RevisionId revisionId = revPageRef.getPage()->branchRevId.revisionId;
            BranchHierarchy& branchHierarchy = hierarchy[branchPage->branchId];
            BranchesForkFromRevision::iterator found = branchHierarchy.branchesForkedFromRevision.find(revisionId);
            if (found != branchHierarchy.branchesForkedFromRevision.end())
            {
                BranchesList& branchesList = found->second;
                for (BranchesList::iterator branchIdIter = branchesList.begin(); branchIdIter != branchesList.end();
                        branchIdIter++)
                {
                    BranchId childBranchId = *branchIdIter;
                    BranchHierarchy& childBranchHierarchy = hierarchy[childBranchId];
                    Log_Branch("==> Checking forked branch %llx from [%llx:%llx]\n", childBranchId, branchPage->branchId, revisionId);
                    checkBranch(childBranchHierarchy.branchPagePtr, hierarchy, stats);
                    Log_Branch("<== Checked  forked branch %llx from [%llx:%llx]\n", childBranchId, branchPage->branchId, revisionId);
                }
            }
            Log_Branch ("Before checkRevision(), BranchPageTable=%p has %lu records\n", stats.getBranchPageTable(), stats.getBranchPageTable()->size());
        }
        Log_Stats(Log_Branch, stats);

    }

    void
    PersistentStore::checkRevision (AbsolutePagePtr revPagePtr, RevisionPage* revPage, PersistentDocument* pDoc,
                                    AllocationStats& _stats)
    {
        AllocationStats stats(_stats, true);
        stats.referencePage("RevisionPage", revPage->branchRevId, NullPage, revPagePtr, PageType_Revision, false);
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

        pDoc->getPersistentDocumentAllocator().checkPages(stats);
#ifdef __XEM_PERSISTENTSTORE_HAS_PAGEREFERENCECTXT        
        Log_Rev ( "my pageReferenceCtxtNb=%llu -> %llu MBytes\n",
                stats.pageReferenceCtxtNb, (stats.pageReferenceCtxtNb * sizeof(AllocationStats::PageReferenceCtxt)) >> 20 );
#endif // __XEM_PERSISTENTSTORE_HAS_PAGEREFERENCECTXT        

        // Log_Rev ( "pageTableSize=%llu (%llu MBytes)\n",
        //  stats.pageTableSize, (stats.pageTableSize*sizeof(AllocationStats::PageStats)) >> 20 );

        Log_Stats(Log_Rev, stats);
        __ui64 totalOwned = 0;
        for (__ui64 type = 0; type < PageType_Mask; type++)
            if (revPage->ownedTypedPages[type])
            {
                Log_Rev ( "\tType %s (0x%llx), owned=%llu\n",
                        __getTypeName(type), type, revPage->ownedTypedPages[type] );
                totalOwned += revPage->ownedTypedPages[type];
            }
        if (revPage->ownedPages != totalOwned)
        {
            stats.addError("Wrong number of owned pages : revPage %llx:%llx has %llu owned pages, counted %llu\n",
                 _brid(revPage->branchRevId), revPage->ownedPages, totalOwned);
        }
        if (stats.getTotalPages() != revPage->ownedPages)
        {
            stats.addError("Wrong ownedPages number : revPage %llx:%llx has %llu, but counted %llu.\n",
                 _brid(revPage->branchRevId), revPage->ownedPages, stats.getTotalPages());
        }
        // checkFreeList ( revPage, &(revPage->freeListHeader) );
    }

#if 0
void PersistentStore::checkFreeList ( RevisionPage* revPage, FreeListHeader* freeListHeader )
{
    Warn ( "Yet. To re-implement using a Context\n" );
    Log_FL ( "Checking freeList at %llx\n", getSegmentPtr ( revPage, freeListHeader ) );
    for ( int level = 0; level < FreeListHeader::levelNumber; level ++ )
    Log_FL ( "\tfirstFree[%d]=%llx\n", level, freeListHeader->firstFree[level] );
    __ui64 totalFreeSegments = 0, totalFreeBytes = 0;
    for ( int level = 0; level < FreeListHeader::levelNumber; level ++ )
    {
        FreeSegment* last = NULL;
        FreeSegment* freeSeg = getSegment<FreeSegment,Read>
        ( revPage, freeListHeader->firstFree[level] );
        //      AssertBug ( !freeSeg || ( freeSeg->last == NullPtr ), "Invalid last for a head freeSegment.\n" );
        for (; freeSeg; freeSeg = getSegment<FreeSegment,Read> ( revPage, freeSeg->next ) )
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
}
