#include <sys/mman.h>
#include <errno.h>
#include <string.h>

/*
 * Xem Memory Protection Capability (XMPC)
 */

#define Log_PSProtectHPP Debug

/**
 * TODO : Reimplement this with a strong multithread mechanism
 */
namespace Xem
{

#ifdef XEM_MEM_PROTECT
  __INLINE void PersistentStore::__alterPage ( void* page )
  {
#if PARANOID
    AssertBug ( readOnly == false, "readOnly has been set !\n" );
    AssertBug ( page, "Null page has been given.\n" );
#endif
#ifdef XEM_MEM_PROTECT_TABLE
    AssertBug ( (getAbsolutePagePtr(page)>>InPageBits) < mem_pages_table_size, 
		"XMPC : Out Of Bounds : page=%llx, max=%llx\n",
		getAbsolutePagePtr(page), mem_pages_table_size );
    AssertBug ( mem_pages_table[getAbsolutePagePtr(page)>>InPageBits] <= mem_pages_table_max_refCount, 
		      "XMPC : Overflow for page %llx\n", getAbsolutePagePtr(page) );
    mem_pages_table[getAbsolutePagePtr(page)>>InPageBits] ++;
#endif
#ifdef XEM_MEM_PROTECT_SYS
    if ( mprotect ( page, PageSize, PROT_READ|PROT_WRITE ) == -1 )
      {
        Error ( "Could not alter page %p, error %d:%s\n",
            page, errno,strerror(errno) );

        Bug ( "." );
      }
#endif
  }

  __INLINE void PersistentStore::__protectPage ( void* page )
  {
#ifdef XEM_MEM_PROTECT_TABLE
    AssertBug ( (getAbsolutePagePtr(page)>>InPageBits) < mem_pages_table_size, "XMPC : OOB\n" );
    AssertBug ( mem_pages_table[getAbsolutePagePtr(page)>>InPageBits], 
		    "XMPC : Protect a non-altered page %llx\n", getAbsolutePagePtr(page) );
        mem_pages_table[getAbsolutePagePtr(page)>>InPageBits] --;
#endif
#ifdef XEM_MEM_PROTECT_SYS
    if ( mprotect ( page, PageSize, PROT_READ ) == -1 )
      {
        Bug ( "Could not protect page %p\n", page );
      }
#endif
#if XEM_MEM_PROTECT_ASYNC_ON_PROTECT 
    // AutoSync
    if ( msync ( page, PageSize, MS_ASYNC ) == -1 )
      {
	      Fatal ( "Could not ASync page %p\n", page );
      }
#endif
  }
#else
  __INLINE void PersistentStore::__alterPage ( void* page ) {}
  __INLINE void PersistentStore::__protectPage ( void* page ) {}
#endif

  __INLINE void PersistentStore::__syncPage ( void*& page, bool sync )
  {
#if PARANOID
    // NoMansLand Check...
#endif
#if XEM_SYNC
    Log_PSProtectHPP ( "Sync page %p\n", page );
#if 0
    AssertBug ( (__ui64) page > (__ui64) store_ptr
		&& (__ui64) page < (__ui64) store_ptr + getSB()->noMansLand,
		"Page not in store !\n" );
#endif
    if ( msync ( page, PageSize, sync ? MS_SYNC : MS_ASYNC ) == -1 )
      {
        Fatal ( "Could not sync page %p : Error %d:%s\n", 
	        page, errno, strerror(errno) );
      }
#endif
  }



};

