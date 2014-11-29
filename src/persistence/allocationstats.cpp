#include <Xemeiah/persistence/persistentstore.h>
#include <Xemeiah/persistence/allocationstats.h>
#include <Xemeiah/trace.h>

#include <Xemeiah/auto-inline.hpp>
#include <Xemeiah/persistence/auto-inline.hpp>

#include <stdarg.h>

#define CheckError(...) { \
      fprintf ( stderr, "[CHECK][ERROR]" __VA_ARGS__ ); \
      addError (__VA_ARGS__); }
#define Log_CheckStats Log

namespace Xem
{

    AllocationStats::AllocationStats ()
    {
        root = this;
        father = NULL;
        errors = new std::list<String>();
        pageTable = NULL;
        pageTableSize = 0;
        pageReferenceCtxtNb = 0;
        memset(pages, 0, sizeof(pages));
        memset(stolenPages, 0, sizeof(stolenPages));
        elements = 0;
        branchPageTable = new BranchPageTable();
        stolenBranchPageTable = false;
        ownsPageTable = false;
    }

    AllocationStats::AllocationStats (AllocationStats& _father, bool stealBranchPageTable)
    {
        root = _father.root;
        father = &_father;
        errors = NULL;
        pageReferenceCtxtNb = 0;
        pageTable = father->pageTable;
        pageTableSize = father->pageTableSize;
        memset(pages, 0, sizeof(pages));
        memset(stolenPages, 0, sizeof(stolenPages));
        elements = 0;

        AssertBug(father->branchPageTable, "Null branchPageTable from father !");
        if (stealBranchPageTable)
        {
            branchPageTable = father->branchPageTable;
        }
        else
        {
            /**
             * Create a copy of the whole branchBageTable
             */
            branchPageTable = new BranchPageTable();
            *branchPageTable = *(father->branchPageTable);
        }
        stolenBranchPageTable = stealBranchPageTable;
        ownsPageTable = false;
    }

    AllocationStats::~AllocationStats ()
    {
        if (father)
        {
            for (__ui64 type = 0; type < PageType_Mask; type++)
            {
                father->pages[type] += pages[type];
                father->stolenPages[type] += stolenPages[type];
                father->elements += elements;
            }
            father->pageReferenceCtxtNb += pageReferenceCtxtNb;
        }
        else if (ownsPageTable)
        {
            AssertBug(pageTable, "No Page Table set !\n");
            free(pageTable);
        }
        if (!stolenBranchPageTable && branchPageTable)
        {
            delete (branchPageTable);
        }
    }

    AllocationStats::RelativePageInfos::~RelativePageInfos ()
    {
    }

    void
    AllocationStats::initPageTable (__ui64 noMansLand)
    {
        pageTableSize = (noMansLand >> InPageBits);
        Log_CheckStats ( "Initing %llu pages for PageTable check (%llu bytes).\n",
                pageTableSize, sizeof(PageType) * pageTableSize );
        pageTable = (PageStats*) malloc(sizeof(PageStats) * pageTableSize);
        memset(pageTable, 0, sizeof(PageStats) * pageTableSize);
        ownsPageTable = true;
    }

    AllocationStats::BranchPageTable*
    AllocationStats::getBranchPageTable ()
    {
        AssertBug(branchPageTable != NULL, "Null branchPageTable here !");
        return branchPageTable;
    }

    void
    AllocationStats::checkUnsetPages (AbsolutePagePtrList& unsetPages)
    {
        AssertBug(pageTable, "pageTable not initied !\n");
        __ui64 nbPages = 2, nbUnref = 0, nbStolen = 0;
        for (__ui64 idx = 2; idx < pageTableSize; idx++)
        {
            PageStats* pStats = &(pageTable[idx]);

            if (!pStats->absPagePtr)
            {
                CheckError("Page %llx not referenced !\n", idx << InPageBits);
                nbUnref++;
                unsetPages.push_back(idx << InPageBits);
            }
            else if (__isStolen(pStats->absPagePtr))
            {
                CheckError("Page %llx stolen but not set !\n", idx << InPageBits);
                nbStolen++;
            }
            else
            {
                nbPages++;
                //	  Log_CheckStats ( "Page %llx, type %llx\n", idx << InPageBits, pageTable[idx] );
            }

        }
        Log_CheckStats ( "PageTable : found %llu pages.\n", nbPages );
        if (nbStolen)
        {
            CheckError("PageTable : %llu pages not set !\n", nbStolen);
        }
        if (nbUnref)
        {
            CheckError("PageTable : %llu pages not set !\n", nbUnref);
        }
    }

    void
    AllocationStats::appendPageReference (PageStats* pStats, const char* ctxt, BranchRevId brId,
                                          RelativePagePtr relPagePtr, PageType pageType, bool stolen)
    {
#ifdef __XEM_PERSISTENTSTORE_HAS_PAGEREFERENCECTXT   
        PageReferenceCtxt* prCtxt = new PageReferenceCtxt();
        prCtxt->previous = pStats->pageReferenceCtxt;
        pStats->pageReferenceCtxt = prCtxt;
        prCtxt->ctxt = ctxt;
        prCtxt->brId = brId;
        prCtxt->stolen = stolen;
        prCtxt->pageType = pageType;
        prCtxt->relPagePtr = relPagePtr;
        pageReferenceCtxtNb ++;
#endif // __XEM_PERSISTENTSTORE_HAS_PAGEREFERENCECTXT   

    }

    void
    AllocationStats::dumpPageReferences (PageStats* pStats)
    {
#ifdef __XEM_PERSISTENTSTORE_HAS_PAGEREFERENCECTXT   
        if ( ! pStats->pageReferenceCtxt )
        {
            Error ( "No pageReferenceCtxt defined for pStats=%p\n", pStats );
        }
        for ( PageReferenceCtxt* prCtxt = pStats->pageReferenceCtxt;
                prCtxt; prCtxt = prCtxt->previous )
        {
            CheckError ( "\tContext '%s' : brId=%llx:%llx, relPagePtr=%llx, pageType=%llx, stolen=%s\n",
                    prCtxt->ctxt, _brid ( prCtxt->brId ), prCtxt->relPagePtr, prCtxt->pageType, prCtxt->stolen ? "true" : "false" );

        }
#endif // __XEM_PERSISTENTSTORE_HAS_PAGEREFERENCECTXT      
    }

#define CheckRefError(...) do { \
  CheckError ( "While referencing page ctxt='%s', brId=%llx:%llx, relPagePtr=%llx, absPagePtr=%llx, pageType=%llx, stolen=%s\n", \
    ctxt, _brid(brId), relPagePtr, absPagePtr, pageType, stolen ? "true" : "false" ); \
  if ( pStats ) { dumpPageReferences ( pStats ); } else { CheckError ( "No pageStats defined.\n" ); } \
  CheckError ( "-->" __VA_ARGS__ ); } while ( 0)

    void
    AllocationStats::referencePage (const char* ctxt, AbsolutePagePtr absPagePtr, PageType pageType, bool stolen)
    {
        static const BranchRevId branchRevId_NULL =
            { 0, 0 };
        return referencePage(ctxt, branchRevId_NULL, NullPage, absPagePtr, pageType, stolen);
    }

    void
    AllocationStats::referencePage (const char* ctxt, BranchRevId brId, RelativePagePtr relPagePtr,
                                    AbsolutePagePtr absPagePtr, PageType pageType, bool stolen)
    {
        PageStats* pStats = NULL;
        if ((absPagePtr & PagePtr_Mask) != absPagePtr)
        {
            CheckRefError("Invalid non-aligned page : %llx\n", absPagePtr);
        }
        if ((absPagePtr & PagePtr_Mask) == NullPage)
        {
            CheckRefError("Null absPagePtr.\n");
            return;
        }
        if (pageType == 0)
        {
            CheckRefError("Null pageType.\n");
            return;
        }
        if (pageTable)
        {
            __ui64 pageIndex = absPagePtr >> InPageBits;
            if (pageIndex >= pageTableSize)
            {
                CheckRefError("Page abs=%llx has index=%llx beyong table size=%llx\n", absPagePtr, pageIndex,
                              pageTableSize);
                return;
            }
#ifdef __XEM_PERSISTENCE_CHECK_BUILD_BRANCHPAGETABLE
            if (relPagePtr && pageType == PageType_Segment)
            {
                BranchPageTable* brTable = getBranchPageTable();
                BranchPageTable::iterator iter = brTable->find(relPagePtr);
                if (iter == brTable->end())
                {
                    // brTable->insert(BranchPageTable::value_type(relPagePtr, new RelativePageInfos()));
                    (*brTable)[relPagePtr].revInfos.size();
                    iter = brTable->find(relPagePtr);
                }
                RelativePageInfos& rpi = iter->second;
                bool dupl = false;
                for (std::list<RelativePageRevInfos>::iterator rpviter = rpi.revInfos.begin();
                        rpviter != rpi.revInfos.end(); rpviter++)
                {
                    RelativePageRevInfos& rpvi = *rpviter;
                    if (bridcmp(rpvi.brId, brId ) == 0)
                    {
                        CheckRefError(
                                "Invalid duplicate relativePage declaration for rel=%llx, abs=%llx, brId=%llx:%llx (already has abs=%llx)\n",
                                relPagePtr, absPagePtr, _brid(brId), rpvi.absPagePtr);
                        dupl = true;
                        Bug(".");
                    }
                }
                if (!dupl)
                {
                    RelativePageRevInfos rpvi; // = new RelativePageRevInfos();
                    rpvi.brId = brId;
                    rpvi.stolen = stolen;
                    rpvi.absPagePtr = absPagePtr;
                    rpi.revInfos.push_back(rpvi);
                    Log_CheckStats ( "[RPVI=%p] brId=%llx:%llx, relPagePtr=%llx, stolen=%d, abs=%llx\n",
                            &rpvi, _brid(brId), relPagePtr, stolen, absPagePtr );
                }
            }
#endif // __XEM_PERSISTENCE_CHECK_BUILD_BRANCHPAGETABLE
            pStats = &(pageTable[pageIndex]);
            if (stolen)
            {
                if (!pStats->absPagePtr)
                {
                    pStats->absPagePtr = pageType | PageFlags_Stolen;
                    pStats->relPagePtr = relPagePtr;
                }
                else
                {
                    if (pageType != __getPageType(pStats->absPagePtr))
                    {
                        CheckRefError(
                                "Diverging page type ! absPagePtr=%llx, already set pageType=%llx, now saying pageType=%llx " "(rel=0x%llx, saying rel=0x%llx)\n",
                                absPagePtr, __getPageType(pStats->absPagePtr), pageType, pStats->relPagePtr,
                                relPagePtr);
                    }
                    if (pStats->relPagePtr != relPagePtr)
                    {
                        CheckRefError(
                                "Diverging relativePagePtr ! absPagePtr=%llx, already set relPagePtr=%llx, now saying relPagePtr=%llx " "(type=%llx, saying type=%llx)\n",
                                absPagePtr, pStats->relPagePtr, relPagePtr, __getPageType(pStats->absPagePtr),
                                pageType);
                    }
                }
            }
            else
            {
                if (__isStolen(pStats->absPagePtr))
                {
                    if (pageType != __getPageType(pStats->absPagePtr))
                    {
                        CheckRefError(
                                "Diverging page type ! absPagePtr=%llx, already set pageType=%llx, now saying pageType=%llx\n",
                                absPagePtr, __getPageType(pStats->absPagePtr), pageType);
                    }
                    if (pStats->relPagePtr != relPagePtr)
                    {
                        CheckRefError(
                                "Diverging relativePagePtr ! absPagePtr=%llx, already set relPagePtr=%llx, now saying relPagePtr=%llx\n",
                                absPagePtr, pStats->relPagePtr, relPagePtr);
                    }
                }
                else
                {
                    if (pStats->absPagePtr != 0)
                    {
                        CheckRefError("Page abs=%llx already set in table with value=%llx, relPagePtr=%llx\n",
                                      absPagePtr, pStats->absPagePtr, pStats->relPagePtr);
                        Bug(".");
                    }
                }
#if 0
                if ( absPagePtr == 0x1a000 || absPagePtr == 0x1c000 )
                {
                    CheckRefError ( "WAS HERE.\n");
                }
#endif
                pStats->absPagePtr = pageType;
                pStats->relPagePtr = relPagePtr;
            }
        }
        /*
         Log_CheckStats ( "Reference page=%llx, pageType=%llx, stolen=%s\n",
         absPagePtr, pageType, stolen ? "yes" : "no" );
         */
        if (stolen)
        {
            stolenPages[pageType]++;
        }
        else
        {
            pages[pageType]++;
        }
        if (pStats)
        {
            appendPageReference(pStats, ctxt, brId, relPagePtr, pageType, stolen);

            if (pageType == PageType_Segment)
            {
                // dumpPageReferences ( pStats );
            }
        }
        return;
    }
#if 0
    bool AllocationStats::refPage ( AbsolutePagePtr absPagePtr )
    {
        return refPage ( absPagePtr & PagePtr_Mask,
                __getPageType ( absPagePtr ),
                __isStolen ( absPagePtr ) );
    }
#endif

    Xem::__ui64
    AllocationStats::getTotalPages () const
    {
        __ui64 total = 0;
        for (__ui64 type = 0; type < PageType_Mask; type++)
            total += pages[type];
        return total;
    }

    Xem::__ui64
    AllocationStats::getTotalStolenPages () const
    {
        __ui64 total = 0;
        for (__ui64 type = 0; type < PageType_Mask; type++)
            total += stolenPages[type];
        return total;
    }

    void
    AllocationStats::addError (const char* fmt, ...)
    {
        std::list<String>* errors = root->errors;

        va_list a_list;
        va_start(a_list, fmt);
        String error = StringFormatVA(fmt, a_list);
        va_end(a_list);
        errors->push_back(error);
    }
}
