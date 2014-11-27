#include <Xemeiah/persistence/persistentstore.h>
#include <Xemeiah/persistence/persistentdocument.h>
#include <Xemeiah/persistence/persistentbranchmanager.h>
#include <Xemeiah/kern/servicemanager.h>
#include <Xemeiah/trace.h>
#include <Xemeiah/log-time.h>

#include <Xemeiah/auto-inline.hpp>
#include <Xemeiah/persistence/auto-inline.hpp>

#include <sys/mman.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <list>

#define Log_Store Debug

namespace Xem
{
    const char* PersistencePageTypeName[] =
        { "Unknown", // 0x00
                "FreePageList", // 0x01
                "FreePage", // 0x02
                "Unknown", // 0x03
                "Segment", // 0x04
                "Unknown", // 0x05
                "Unknown", // 0x06
                "Unknown", // 0x07
                "Unknown", // 0x08
                "Unknown", // 0x09
                "Key", // 0x0a
                "Branch", // 0x0b
                "Revision", // 0x0c
                "PageInfo", // 0x0d
                "Indirection", // 0x0e
                "Unknown", // 0x0e
            };

    void
    PersistentStore::initPersistentStore ()
    {
        readOnly = false;
        superBlock = NULL;
        fd = -1;

        branchManager = NULL;
    }

    PersistentStore::PersistentStore () :
            chunkMapMutex("ChunkMap"), superblockMutex("SuperBlock"), freePageHeaderMutex("FreePageHeader")
    {
        initPersistentStore();
    }

    PersistentStore::~PersistentStore ()
    {
        if (fd != -1)
        {
            Warn("PersistentStore : deleting while file is still openned !\n");
        }
        delete (branchManager);
    }

    PersistentBranchManager&
    PersistentStore::getPersistentBranchManager ()
    {
        return *branchManager;
    }

    BranchManager&
    PersistentStore::getBranchManager ()
    {
        return getPersistentBranchManager();
    }

    bool
    PersistentStore::reserveElementIds (ElementId& nextId, ElementId& lastId)
    {
        lockSB();
        nextId = getSB()->nextElementId;
        getSB()->nextElementId += 256;
        lastId = getSB()->nextElementId;
        unlockSB();
        return true;
    }

    bool
    PersistentStore::isFreePageCacheFull ()
    {
#ifdef XEM_FREEPAGE_CACHE
        Log_Store ( "FreePageCache at %lx / %llx\n",
                freePageCache.size() , maxFreePageCache );
        return ( freePageCache.size() >= maxFreePageCache - PageSize );
#endif
        return true;
    }

    bool
    PersistentStore::setReadOnly (bool __readOnly)
    {
        AssertBug(superBlock == NULL, "setting read-only with an openned file !\n");
        readOnly = __readOnly;
        return readOnly;
    }

    bool
    PersistentStore::isReadOnly () const
    {
        return readOnly;
    }

    bool
    PersistentStore::openFile (const char* _filename, bool mayCreate)
    {
        static const size_t minimumFileSize = PageSize * 8;
        AssertBug(fd == -1, "Store already openned !\n");
        char filename[4096];
        strcpy(filename, _filename);
        char buffer[4096];
        while (readlink(filename, buffer, 4096) != -1)
        {
            Log_Store ( "Filename is a readlink.\n" );
            strcpy(filename, buffer);
        }
        Log_Store ( "Real file name is '%s'. readOnly=%d\n", filename, readOnly );

        int flags = (readOnly ? O_RDONLY : O_RDWR) | (mayCreate ? O_CREAT : 0);
        // O_NOATIME |
        // | O_RDONLY
        // | O_DIRECT
        mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;
        fd = ::open(filename, flags, mode);
        if (fd == -1)
        {
            Error("Could not open file '%s'. Error %d:%s\n", filename, errno, strerror(errno));
            return false;
        }
        struct stat fileStat;
        if (fstat(fd, &fileStat) == -1)
        {
            Error("Could not stat(2) file '%s, Error %d:%s\n", filename, errno, strerror(errno));
            ::close(fd);
            return false;
        }
        if (S_ISREG(fileStat.st_mode))
        {
            fileLength = (__ui64) fileStat.st_size;
            fileLengthIsAHardLimit = false;
        }
        else if ( S_ISBLK(fileStat.st_mode) )
        {
            off_t endOffset = lseek ( fd, 0, SEEK_END );
            Info ( "File '%s' is a block device (size=%llu, blksize=%llu, blocks=%llu, st_dev=%lu). lseek() gives size=%llu (%llu MBytes)\n",
                    filename,
                    (__ui64) fileStat.st_size,
                    (__ui64) fileStat.st_blksize,
                    (__ui64) fileStat.st_blocks,
                    (unsigned long) fileStat.st_rdev,
                    (__ui64) endOffset, ((__ui64) endOffset) >> 20 );
            fileLength = (__ui64) endOffset;
            if ( fileLength % PageSize )
            {
                // Correct fileLength to be a multiple of PageSize
                fileLength -= (fileLength % PageSize);
            }
            fileLengthIsAHardLimit = true;
        }
        else
        {
            Fatal ( "Not implemented : file type mode %d\n", fileStat.st_mode );
        }
        if (fileLength < minimumFileSize && mayCreate)
        {
            Info("File '%s' is empty or too narrow, initializing with empty contents.\n", filename);
            fileLength = minimumFileSize;
            char buffer[PageSize];
            memset(buffer, 0, PageSize);
            for (off_t offset = 0; offset < (off_t) fileLength; offset += PageSize)
            {
                ssize_t result = pwrite(fd, buffer, PageSize, offset);
                if (result != PageSize)
                {
                    Fatal("Could not write preliminary page of data.\n");
                }
            }
        }
        Info("fileLength of '%s' is %llu (0x%llx) bytes, %llu (0x%llx) pages.\n", filename, (__ui64) fileLength,
             (__ui64) fileLength, (__ui64) fileLength / PageSize, (__ui64) fileLength / PageSize);

        AssertBug(fileLength % PageSize == 0, "File length is not a multiple of pageSize !\n");
        AssertBug(fileLength >= minimumFileSize, "File too narrow !\n");
        AssertBug(fileLength % PageSize == 0, "fileLength=%llx is not a multiple of PageSize=%llx !\n",
                  (__ui64) fileLength, PageSize);
        if (readOnly)
        {
            Warn("Openning readOnly !\n");
        }

        superBlock = (SuperBlock*) mapPage(0);
        if (!getSB())
        {
            Error("Could not access superblock for file '%s'\n", filename);
            ::close(fd);
            fd = -1;
            return false;
        }
        Log_Store ( "Openned file '%s', superBlock=%p\n", filename, superBlock );
        Log_Store ( "\tnoMansLand at=%llx, fileLength=%llx\n", getSB()->noMansLand, fileLength );
#if 0
        __ui64 totalFree = ( fileLength - getSB()->noMansLand ) + ( getSB()->nbFreePages * PageSize );
        __ui64 totalUsed = fileLength - totalFree;
        __ui64 totalUsedPct = (100 * totalUsed) / fileLength;
        Info ( "\tTotal used=%llu MBytes (%llu%%), free=%llu MBytes\n",
                totalUsed >> 20, totalUsedPct, totalFree >> 20 );
#endif
#ifdef XEM_MEM_PROTECT_TABLE
        mem_pages_table_size = fileLength / PageSize;
        Log_Store ( "Allocing %llu(0x%llx) bytes for mem page table\n",
                mem_pages_table_size, mem_pages_table_size );
        mem_pages_table = (unsigned char*) malloc ( mem_pages_table_size );
        AssertBug ( mem_pages_table_size, "Could not alloc mem pages\n" );
        Log_Store ( "Alloced %llu(0x%llx) bytes for mem page table=%p\n",
                mem_pages_table_size, mem_pages_table_size, mem_pages_table );

#endif
        return true;
    }

    bool
    PersistentStore::extendFile (__ui64 expectedFileLength)
    {
        AssertBug(fileLength % PageSize == 0, "File length is not a multiple of pageSize !\n");
        AssertBug(expectedFileLength % PageSize == 0, "File length is not a multiple of pageSize !\n");

        if (expectedFileLength <= fileLength)
            return true;

        if (fileLengthIsAHardLimit)
        {
            Fatal("Could not extend file to %lu MBytes, because fileLength=%lu MBytes is a hard limit.\n",
                  (unsigned long ) expectedFileLength >> 20, (unsigned long ) fileLength >> 20);
        }
        int res = ftruncate(fd, expectedFileLength);
        if (res == -1)
        {
            Fatal("Could not extend file from %llu Mb to %llu Mb using ftruncate(fd=%d, length=%llu) : err=%d:%s\n",
                  fileLength >> 20, expectedFileLength >> 20, fd, (__ui64) expectedFileLength, errno, strerror(errno));
            return false;
        }
        Log("Extended file : from 0x%llx (%llu Mb) to 0x%llx (%llu Mb)\n", fileLength, fileLength >> 20,
            expectedFileLength, expectedFileLength >> 20);
        fileLength = expectedFileLength;
        return true;
    }

    bool
    PersistentStore::closeFile ()
    {
        Info("Closing Store File...\n");
        NTime begin, end;

        begin = getntime();
        __ui64 totalPages = 0;

        /*
         * Syncing superBlock
         */
        if (msync(superBlock, PageSize, MS_SYNC) == -1)
        {
            Bug("Could not sync SuperBlock. Error %d:%s !\n", errno, strerror(errno));
        }

        for (ChunkMap::iterator iter = chunkMap.begin(); iter != chunkMap.end(); iter++)
        {
            void* page = iter->second.page;
            Log_Store ( "Unmapping chunk page [chunk:%llx] = %p\n", iter->first, page );
            AssertBug(page, "Null page for [chunk:%llx]pa !\n", iter->first);
#if 0
            if ( msync ( page, PageSize, MS_SYNC ) == -1 )
            {   Bug ( "Could not sync whole file. Error %d:%s !\n", errno, strerror(errno) );}
#endif
            if (munmap(page, ChunkInfo::PageChunk_Size) == -1)
            {
                Fatal("Could not unmap file. Error %d:%s\n", errno, strerror(errno));
                return false;
            }
            totalPages++;
        }
        chunkMap.clear();

        superBlock = NULL;
        totalPages++;

        end = getntime();
        Info("At store closeFile() : unmapped %llu store page chunks\n", totalPages);
        WarnTime("Msync/Munmap Store File took : ", begin, end);
        begin = getntime();
        if (::close(fd))
        {
            Bug("Could not close file. Error %d:%s\n", errno, strerror(errno));
            return false;
        }
        fd = -1;
        end = getntime();
        WarnTime("Closed Store File took : ", begin, end);
        return true;
    }

    bool
    PersistentStore::format (const char* filename)
    {
        Info("Formatting Store File '%s' ...\n", filename);
        if (!openFile(filename, true))
            return false;
        SuperBlock * sb = getSB();
        AssertBug(sb, "Could not get sb !\n");
        /*
         * Preliminary SuperBlock formatter.
         *
         * lockSB() and unlockSB() may no be accessible
         * so do the mprotect() stuff by myself in case of XEM_MEM_PROTECT_SYS
         */
#ifdef XEM_MEM_PROTECT_SYS
        mprotect ( sb, sizeof(SuperBlock), PROT_READ|PROT_WRITE );
#endif
        //  lockSB ();
        memset(sb, 0, PageSize);
        strncpy(sb->magic, XEM_SB_MAGIC, sb->magic_length);
        strncpy(sb->version, XEM_SB_VERSION, sb->version_length);

        sb->pageSize = PageSize;

        sb->lastBranch = NullPage;
        sb->nextBranchId = 1;
        sb->keyPage = NullPage;
        sb->namespacePage = NullPage;
        /*
         * FreeListHeader is at offset PageSize (1st page after superblock)
         */
        sb->freePageHeader = PageSize * 1;
        /*
         * Two pages are set by default : SuperBlock and FreeListHeader
         * so noMansLand is at page 2.
         */
        sb->noMansLand = PageSize * 2;

        sb->nextElementId = 1;
#ifdef __XEM_COUNT_FREEPAGES
        sb->nbFreePages = 0;
#endif

#ifdef XEM_MEM_PROTECT_SYS
        mprotect ( sb, sizeof(SuperBlock), PROT_READ );
#endif

        /*
         * Preliminary work is done, so protect functions are accessible.
         * Create the freePageHeader now.
         */
        FreePageHeader* freePageHeader = getAbsolutePage<FreePageHeader>(sb->freePageHeader);
        alterPage(freePageHeader);
        memset(freePageHeader, 0, PageSize);
        protectPage(freePageHeader);

        Info("Format ok, loading keys...\n");

        /*
         * Create the default keys and the local key cache now.
         */
        if (!loadKeysFromStore())
        {
            Bug("Could not create default keys.\n");
        }

        Info("Starting BranchManager...\n");

        /*
         * Create the default Main Branch.
         */
        AssertBug(branchManager == NULL, "Already have a branch manager !\n");
        branchManager = new PersistentBranchManager(*this);

        return true;
    }

    bool
    PersistentStore::open (const char* filename)
    {
        Info("Openning Store File '%s'\n", filename);
        if (!openFile(filename, false))
        {
            return false;
        }
        if (!checkFormat())
        {
            closeFile();
            return false;
        }
#if 0
        if ( ! readOnly )
        {
            if ( ! dropUncommittedRevisions() )
            {   closeFile(); return false;}
        }
#endif
        Log_Store ( "Successfully openned '%s'\n", filename );

        if (!loadKeysFromStore())
        {
            Error("Could not load keys.\n");
            closeFile();
            return false;
        }
        AssertBug(branchManager == NULL, "Already have a branch manager !\n");
        branchManager = new PersistentBranchManager(*this);
        Info("Openned file '%s' : noMansLand at=0x%llx, nbFreePages=0x%llx, fileLength=%lu MBytes\n", filename,
             getSB()->noMansLand, getSB()->nbFreePages, (unsigned long ) fileLength >> 20);

        if (!check(Check_Clean))
        {
            Error("Could not check clean file.\n");
            closeFile();
            return false;
        }

        return true;
    }

    bool
    PersistentStore::close ()
    {
#if 0
        Info("Stopping services...\n");
        getServiceManager().stopServiceManager();
        Info("Waiting services termination...\n");
        getServiceManager().waitTermination();
        Info("All services stopped.\n");
#endif

        delete (branchManager);
        branchManager = NULL;
        closeFile();

        Info("PersistentStore=%p : closed !\n", this);
        return true;
    }

    void
    PersistentStore::housewife ()
    {
        Info("Housewife : Store has %llu MBytes (%llu chunks) in ChunkMap\n",
             (getChunkMapSize() * ChunkInfo::PageChunk_Size) >> 20, (__ui64) getChunkMapSize());
        Store::housewife();

    }
}
;

