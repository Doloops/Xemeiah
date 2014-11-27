#include <Xemeiah/persistence/persistentstore.h>
#include <Xemeiah/trace.h>

#include <sys/mman.h>
#include <errno.h>

#define Log_PSLockSuperBlock Debug

namespace Xem
{
    /*
     * SuperBlock locking capabilities
     */
    __INLINE void
    PersistentStore::lockSB ()
    {
        Log_PSLockSuperBlock ( "Locking SB\n" );
        superblockMutex.lock();
#ifdef XEM_MEM_PROTECT_SYS
        if ( mprotect ( superBlock, PageSize, PROT_READ|PROT_WRITE ) == -1 )
        {   Bug ( "Could not lock sb=%p ! err=%d:%s\n", superBlock, errno, strerror(errno) );}
#endif
    }

    __INLINE void
    PersistentStore::lockSB_Read ()
    {
        superblockMutex.lock();
    }

    __INLINE void
    PersistentStore::unlockSB ()
    {
        Log_PSLockSuperBlock ( "UNLocking SB\n" );
#ifdef XEM_MEM_PROTECT_SYS
        if ( mprotect ( superBlock, PageSize, PROT_READ ) == -1 )
        {   Bug ( "Could not lock sb !\n" );}
#endif
        superblockMutex.unlock();
    }

    __INLINE void
    PersistentStore::unlockSB_Read ()
    {
        superblockMutex.unlock();
    }

    __INLINE SuperBlock*
    PersistentStore::getSB ()
    {
        return superBlock;
    }

    __INLINE AbsolutePageRef<FreePageHeader>
    PersistentStore::getFreePageHeader ()
    {
        return getAbsolutePage<FreePageHeader>(getSB()->freePageHeader);
    }

    __INLINE AbsolutePagePtr
    PersistentStore::getFreePageList ()
    {
        return getFreePageList(true);
    }
}
