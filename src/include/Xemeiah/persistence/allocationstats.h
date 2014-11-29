#ifndef __XEM_PERSISTENCE_ALLOCATIONSTATS_H
#define __XEM_PERSISTENCE_ALLOCATIONSTATS_H

#include <Xemeiah/persistence/format/superblock.h>
#include <Xemeiah/persistence/format/revision.h>
#include <Xemeiah/persistence/format/pages.h>

#include <Xemeiah/persistence/persistentstore.h>

namespace Xem
{
    class AllocationStats
    {
    protected:
    public:
#ifdef __XEM_PERSISTENTSTORE_HAS_PAGEREFERENCECTXT
        struct PageReferenceCtxt
        {
            const char* ctxt;
            BranchRevId brId;
            bool stolen;
            PageType pageType;
            RelativePagePtr relPagePtr;
            PageReferenceCtxt* previous;
        };
#endif // __XEM_PERSISTENTSTORE_HAS_PAGEREFERENCECTXT
        struct PageStats
        {
            AbsolutePagePtr absPagePtr;
            RelativePagePtr relPagePtr;
#ifdef __XEM_PERSISTENTSTORE_HAS_PAGEREFERENCECTXT
            PageReferenceCtxt* pageReferenceCtxt;
#endif // __XEM_PERSISTENTSTORE_HAS_PAGEREFERENCECTXT
        };

        struct RelativePageRevInfos
        {
            BranchRevId brId;
            bool stolen;
            AbsolutePagePtr absPagePtr;
        };

        class RelativePageInfos
        {
        public:
            RelativePageInfos ()
            {
            }
            ~RelativePageInfos ();
            std::list<RelativePageRevInfos> revInfos;

            RelativePageInfos&
            operator= (const RelativePageInfos& other)
            {
                revInfos = other.revInfos;
                return *this;
            }
        };

        std::list<String>* errors;
    public:
        typedef std::map<RelativePagePtr, RelativePageInfos> BranchPageTable;

    private:
        AllocationStats* root;
        AllocationStats* father;

        BranchPageTable* branchPageTable;
        bool stolenBranchPageTable;

        PageStats* pageTable;
        __ui64 pageTableSize;
        __ui64 pageReferenceCtxtNb;
        bool ownsPageTable;
    public:
        AllocationStats ();
        AllocationStats (AllocationStats& father, bool stealBranchPageTable = false);
        ~AllocationStats ();

        void
        initPageTable (__ui64 noMansLand);

        BranchPageTable*
        getBranchPageTable ();

        __ui64 pages[PageType_Mask];
        __ui64 stolenPages[PageType_Mask];

        void
        appendPageReference (PageStats* pStats, const char* ctxt, BranchRevId brId, RelativePagePtr relPagePtr,
                             PageType pageType, bool stolen);

        void
        dumpPageReferences (PageStats* pStats);

        void
        referencePage (const char* ctxt, AbsolutePagePtr absPagePtr, PageType pageType, bool stolen);

        void
        referencePage (const char* ctxt, BranchRevId brId, RelativePagePtr relPagePtr, AbsolutePagePtr absPagePtr,
                       PageType pageType, bool stolen);

        __ui64
        getTotalPages () const;

        __ui64
        getTotalStolenPages () const;

        __ui64 elements;

        void
        checkUnsetPages (AbsolutePagePtrList& unsetPages);

        void
        addError (const char* fmt, ...);
    };
}
#endif // __XEM_PERSISTENCE_ALLOCATIONSTATS_H
