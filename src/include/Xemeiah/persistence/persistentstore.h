#ifndef __XEM_PERSISTENCE_PERSISTENTSTORE_H
#define __XEM_PERSISTENCE_PERSISTENTSTORE_H

#include <Xemeiah/kern/store.h>
#include <Xemeiah/kern/mutex.h>

#include <Xemeiah/persistence/format/superblock.h>
#include <Xemeiah/persistence/format/revision.h>
#include <Xemeiah/persistence/format/pages.h>

#include <Xemeiah/persistence/absolutepageref.h>

#include <list>

// #define __XEM_PERSISTENTDOCUMENT_CHECK_DUMP_ALL_PAGES //< Option : Dump all chunk at PersistentDocument::checkContents()
#define __XEM_PERSISTENCE_CHECK_BUILD_BRANCHPAGETABLE //< Option : build branchPageTable

namespace Xem
{
    extern const char* PersistencePageTypeName[];

    class PersistentDocument;
    class BranchManager;
    class PersistentBranchManager;
    class PersistentStore;
    class PageInfoIterator;
    class PersistentDocumentAllocator;
    class AllocationStats;

    XemStdException(PersistenceException);
    XemStdException(PersistenceCheckContentException);
    XemStdException(PersistenceBranchLastRevisionAlreadyWritable);

    typedef std::list<AbsolutePagePtr> AbsolutePagePtrList;
    typedef std::list<BranchId> BranchesList;
    typedef std::map<RevisionId, BranchesList> BranchesForkFromRevision;

    class BranchHierarchy
    {
    public:
        BranchHierarchy ()
        {
            branchPagePtr = NullPage;
            forkedFrom.branchId = 0;
            forkedFrom.revisionId = 0;
        }
        AbsolutePagePtr branchPagePtr;
        BranchRevId forkedFrom;
        BranchesForkFromRevision branchesForkedFromRevision;
    };

    typedef std::map<BranchId, BranchHierarchy> BranchesHierarchy;

    class PersistentStore : public Store
    {
        friend class PersistentDocument;
        friend class PersistentDocumentAllocator;
        friend class PersistentBranchManager;

    protected:
        /**
         * Internal constructor (for derivates)
         */
        void
        initPersistentStore ();

        /**
         * PersistentBranchManager
         */
        PersistentBranchManager* branchManager;

        /**
         * The FD corresponding to the Store file
         */
        int fd;

        /**
         * Read-only flag for the whole Store.
         */
        bool readOnly;

        /**
         * Total file length of the Store file.
         */
        __ui64 fileLength;

        /**
         * Determines if we can extend file after fileLength (only for regular files)
         */
        bool fileLengthIsAHardLimit;

        /**
         * SuperBlock pointer
         */
        SuperBlock* superBlock;

        /**
         * Store file partial mapping mechanism
         * The mapping is quite similar to the Document one for Areas
         * This mapping is based on the *absolute* page pointer.
         */
        class ChunkInfo
        {
        public:
            void* page;
            __ui64 refCount;

            static const __ui64 PageChunk_Bits = 4; // 5;
            static const __ui64 PageChunk_Index_Bits = (PageChunk_Bits + InPageBits);
            static const __ui64 PageChunk_Size = (PageSize << PageChunk_Bits);
            static const __ui64 PageChunk_Mask = (PageChunk_Size) - 1;
            static const __ui64 PageChunk_ChunkMask = ~((PageChunk_Size) - 1);

            ChunkInfo ()
            {
                page = NULL, refCount = 0;
            }
            ~ChunkInfo ()
            {
            }
        };

        /**
         * The ChunkMap, alias the list of pages allocated
         */
        typedef std::map<AbsolutePagePtr, ChunkInfo> ChunkMap;
        ChunkMap chunkMap;

        Mutex chunkMapMutex;

        /**
         *  Map a page, and give a pointer to it.
         */
        void*
        mapPage (AbsolutePagePtr absPagePtr);

    public:
        /**
         * getAbsolutePage(), getAbsolutePagePtr()maps an AbsolutePagePtr to a in-mem T* page.
         * alterPage, protectPage, syncPage,
         * alterSegment, protectSegment and syncSegment are defined as macros
         */
        void*
        __getAbsolutePage (AbsolutePagePtr pagePtr);

        /**
         * Release a page (give the opportunity to unmap it)
         * This is just a hint, may not crash if the page is not in the page cache
         */
        void
        __releasePage (AbsolutePagePtr absPagePtr);

    protected:
        /*
         * Locking stuff
         */
        Mutex superblockMutex;
        Mutex freePageHeaderMutex;

#ifdef XEM_MEM_PROTECT_TABLE
        unsigned char *mem_pages_table;
        __ui64 mem_pages_table_size;
        static const __ui64 mem_pages_table_max_refCount = 128;
#endif

        /**
         * Maps an area of the Store file.
         * @param offset the offset in the file
         * @param length the length of the area
         * @return the in-mem mmapped area, NULL upon failure.
         */
        void*
        mapArea (__ui64 offset, __ui64 length);

        /**
         * Unmaps an area from the Store file
         */
        void
        unmapArea (void* area, __ui64 length);

        /**
         * Low-level open
         * @param filename the filename of the database
         * @param mayCreate set to true to create filename (needs to be formatted, so needs to be called by format())
         * @return true upon success, false otherwise
         */
        void
        openFile (const char* filename, bool mayCreate);

        /**
         * Lowl-level close
         * @return true on success, false on failure
         */
        void
        closeFile ();

        /**
         * Extends the store file to the expectedFileLength, called under lockSB()
         * @param expectedFileLength the desired file length
         * @return true upon success, false otherwise, but way Bug() or Fatal() on error.
         */
        bool
        extendFile (__ui64 expectedFileLength);

        void
        checkFormat ();

        int
        __getFD () const
        {
            return fd;
        }

        INLINE
        SuperBlock*
        getSB ();

        INLINE
        AbsolutePageRef<FreePageHeader>
        getFreePageHeader ();

        /*
         * SuperBlock locking mechanism
         * lockSB() locks write, lockSB_Read() locks read only.
         */
        INLINE
        void
        lockSB ();INLINE
        void
        lockSB_Read ();INLINE
        void
        unlockSB ();INLINE
        void
        unlockSB_Read ();

        /**
         * Key persistence : loadKeysFromStore
         * Loads all keys stored in the store file.
         *
         */
        void
        loadKeysFromStore ();

        LocalKeyId
        addKeyInStore (const char* keyName);
        NamespaceId
        addNamespaceInStore (const char* namespaceURL);

        template<typename T>
            INLINE AbsolutePageRef<T>
            getAbsolutePage (AbsolutePagePtr pagePtr)
            {
                AbsolutePageRef<T> pageRef(this, pagePtr);
                return pageRef;
            }

        void
        __alterPage (void* page);
        void
        __protectPage (void* page);

        template<typename T>
            INLINE void
            alterPage (T* page)
            {
                __alterPage((void*) page);
            }
        template<typename T>
            INLINE void
            protectPage (T* page)
            {
                __protectPage((void*) page);
            }

        void
        __syncPage (void*& page, bool sync);
        template<typename T>
            INLINE void
            syncPage (T*& page, bool sync)
            {
                __syncPage((void*&) page, sync);
            }
        template<typename T>
            INLINE void
            syncPage (T* page)
            {
                syncPage<T>(page, false);
            }

        /*
         * getFreePage(AbsolutePagePtr) is strictly for non-revision pages
         * Use getFreePage(RevisionPage*,AbsolutePagePtr) when in Document
         */
        AbsolutePagePtr
        getFreePageList (bool mustLock);
        AbsolutePagePtr
        getFreePageList ();

        bool
        putFreePageListPtr (AbsolutePagePtr freePageListPtr);

        AbsolutePagePtr
        getFreePagePtr ();

        bool
        freePage (AbsolutePagePtr pagePtr);
        bool
        freePages (AbsolutePagePtr* pageList, __ui64 number);
        bool
        freePageList (AbsolutePagePtr freePageListPtr);

#ifdef __XEM_COUNT_FREEPAGES
        void
        decrementFreePagesCount (__ui64 count);
#endif // __XEM_COUNT_FREEPAGES

        /**
         * Reserve element ids
         */
        bool
        reserveElementIds (ElementId& nextId, ElementId& lastId);

    public:
        /**
         * Default Constructor
         */
        PersistentStore ();

        /**
         * Default Destructor
         */
        ~PersistentStore ();

        /**
         * Convert myself to PersisentStore
         */
        PersistentStore&
        asPersistentStore ()
        {
            return *this;
        }

        /**
         * Access to the BranchManager
         */
        BranchManager&
        getBranchManager ();

        /**
         * Convert my branchManager to PersistentBranchManager
         */
        PersistentBranchManager&
        getPersistentBranchManager ();

        /**
         * Formats file with default branch flags
         */
        void
        format (const char* filename);

        /**
         * Drop all revisions of all branches that are marked writable (uncommitted).
         */
        bool
        dropUncommittedRevisions ();

        /**
         * Set the PersistentStore readonly (must be done before open() and is incompatible with format() )
         * @param readOnly set to true to set the PersistentStore readonly
         * @return true upon success, false otherwise
         */
        bool
        setReadOnly (bool readOnly);

        /**
         * @return true if the document is readonly
         */
        bool
        isReadOnly () const;

        /**
         * Open an existing file
         */
        void
        open (const char* filename);

        /**
         * Closes the current file
         */
        void
        close ();

        /**
         * Try to shrink a bit of memory
         */
        virtual void
        housewife ();

        /**
         * Get the current size of the page map
         */
        __ui64
        getChunkMapSize () const
        {
            return (__ui64) chunkMap.size();}

    /**
     * Allocation information
     */
    bool isFileLengthIsAHardLimit() const
    {   return fileLengthIsAHardLimit;}

    __ui64 getFileLength() const
    {   return fileLength;}

    /*
     * ***************************************************************
     * Check functions, sorted from finest to most global structures.
     * ***************************************************************
     *
     * checkKeys() is implemented in keys.cpp, the others in check.cpp
     */
    void checkElement ( RevisionPage* revPage, ElementSegment* element );
    // void checkFreeList ( RevisionPage* revisionPage, FreeListHeader* freeListHeader );

protected:
    void buildBranchesHierarchy ( BranchesHierarchy& branchesHierarchy);
public:
    void checkAllContents ();
    void checkKeys ( AllocationStats& stats );
    void checkFreePageHeader ( AllocationStats& stats );
    void putPagesInAttic ( AbsolutePagePtrList& atticPageList );
    void checkBranch ( AbsolutePagePtr branchPagePtr, BranchesHierarchy& hierarchy, AllocationStats& stats );
    void checkRevision ( AbsolutePagePtr revisionPagePtr, RevisionPage* revisionPage, PersistentDocument* document, AllocationStats& stats );

    /**
     * Main Check Function for Storage
     */
    enum CheckFlag
    {
        Check_Internals,
        Check_Clean,
        Check_AllContents
    };
    bool check ( CheckFlag flag );

    /**
     * Testing stuff
     */
    bool isFreePageCacheFull ();
};
}

#endif //  __XEM_PERSISTENCE_PERSISTENTSTORE_H

