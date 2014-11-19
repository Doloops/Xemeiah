#include <Xemeiah/kern/document.h>
#include <Xemeiah/dom/noderef.h>
#include <Xemeiah/trace.h>

#include <sys/mman.h>
#include <errno.h>

#define Log_PROT Debug
#define Log_PROT_DAH Debug

// #define __XEM_KERN_DOCUMENTALLOCATOR_CHECKPAGEISALTERABLE

namespace Xem
{
  /*
   * Segment Protection
   */
#ifdef XEM_MEM_PROTECT_SYS
  __INLINE void DocumentAllocator::__alter ( void* seg, __ui64 size )
  { 
    AssertBug ( size <= SegmentSizeMax, "Size out of range !\n" );
    Log_PROT ( "[ALTER %p/%llx]\n", seg, size );
    SegmentPage* segPage = __getSegmentPage ( seg );
    size += (__ui64) seg - (__ui64) segPage;
    while ( true )
      {
        Log_PROT ( "Alter segment page at %p\n", segPage );
        alterSegmentPage  ( segPage );    
        if ( size <= PageSize ) break;
        size -= PageSize;
        segPage = (SegmentPage*) ((__ui64) segPage + (__ui64) PageSize);
      }
  }

  __INLINE void DocumentAllocator::__protect ( void* seg, __ui64 size )
  { 
    AssertBug ( size <= SegmentSizeMax, "Size out of range !\n" );

    Log_PROT ( "[PROTECT %p/%llx]\n", seg, size );
    SegmentPage* segPage = __getSegmentPage ( seg );

    size += (__ui64) seg - (__ui64) segPage;
    while ( true )
      {
        Log_PROT ( "Protect segment page at %p\n", segPage );
        protectSegmentPage  ( segPage );    
        if ( size <= PageSize ) break;
        size -= PageSize;
        segPage = (SegmentPage*) ((__ui64) segPage + (__ui64) PageSize);
      }
  }

  __INLINE void DocumentAllocator::alterSegmentPage ( void* page )
    {
#ifdef __XEM_KERN_DOCUMENTALLOCATOR_CHECKPAGEISALTERABLE
      checkPageIsAlterable ( page );
#endif
      if ( mprotect ( page, PageSize, PROT_READ | PROT_WRITE ) == -1 )
        {
          Bug ( "Could not alter page %p : %d:%s\n", page, errno, strerror(errno) );
        }
      Log_MemProtect ( "Altered page=%p\n", page );
    }

  __INLINE void DocumentAllocator::protectSegmentPage ( void* page )
    {
#ifdef __XEM_KERN_DOCUMENTALLOCATOR_CHECKPAGEISALTERABLE
      checkPageIsAlterable ( page );
#endif
      if ( mprotect ( page, PageSize, PROT_READ ) == -1 )
        {
          Bug ( "Could not protect page %p : %d:%s\n", page, errno, strerror(errno) );
        }
      Log_MemProtect ( "Protected page=%p\n", page );
    }
    
  __INLINE void DocumentAllocator::alterDocumentAllocationHeader ()
  {
    void* page = documentAllocationHeader;
    if ( (__ui64)documentAllocationHeader & (PageSize-1) )
      {
        page = (void*) ( (__ui64)documentAllocationHeader & ~(PageSize-1) );
        Log_PROT_DAH ( "[ALTER-DAH] Not aligned dAH=%p, page=%p !!!\n", documentAllocationHeader, page );
      }
    if ( mprotect ( page, PageSize, PROT_READ | PROT_WRITE ) == -1 )
      {
        Fatal ( "Could not alter page %p : %d:%s\n", documentAllocationHeader, errno, strerror(errno) );
      }
  }
  
  __INLINE void DocumentAllocator::protectDocumentAllocationHeader () 
  {
    void* page = documentAllocationHeader;
    if ( (__ui64)documentAllocationHeader & (PageSize-1) )
      {
        page = (void*) ( (__ui64)documentAllocationHeader & ~(PageSize-1) );
        Log_PROT_DAH ( "[PROTECT-DAH] Not aligned dAH=%p, page=%p !!!\n", documentAllocationHeader, page );
      }
    if ( mprotect ( page, PageSize, PROT_READ ) == -1 )
      {
        Fatal ( "Could not alter page %p : %d:%s\n", documentAllocationHeader, errno, strerror(errno) );
      }
  }

  __INLINE void DocumentAllocator::alterFreeSegmentsLevelHeader ( AllocationProfile allocProfile )
  {
    return alterDocumentAllocationHeader();
  }

  __INLINE void DocumentAllocator::protectFreeSegmentsLevelHeader ( AllocationProfile allocProfile )
  {
    return protectDocumentAllocationHeader();
  }

#endif
 
};
