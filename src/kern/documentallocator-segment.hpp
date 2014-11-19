#define Log_DA_SegmentHPP Debug
#define Log_DA_SegmentHPP_APW Debug

namespace Xem
{

  __INLINE SegmentPage* DocumentAllocator::__getSegmentPage ( void* seg )
  {
    return (SegmentPage*) ((__ui64)seg & PagePtr_Mask);
  }

  __INLINE void DocumentAllocator::__authorizeWrite ( SegmentPtr segPtr, __ui64 _size )
  {
#if PARANOID
    AssertBug ( _size <= SegmentSizeMax, "Segment size exceeds Area boundaries ! segPtr=0x%llx, size=0x%llx\n",
      segPtr, _size );
    AssertBug ( segPtr, "Null segpointer provided !\n" );
    AssertBug ( segPtr < getDocumentAllocationHeader().nextRelativePagePtr, "Seg pointer out of bounds : '%llx'\n", segPtr );
#endif
    if ( __assertDocumentAlwaysWritable ) return;
    AssertBug ( isWritable(), "Document not writable !\n" );

    __ui64 size = alignSize ( _size );
    Log_DA_SegmentHPP_APW ( "AuthorizeWrite on segPtr=0x%llx, size=0x%llx, aligned=0x%llx\n", segPtr, _size, size );
    RelativePagePtr relPagePtr = segPtr & PagePtr_Mask;
    size += segPtr & SegmentPtr_Mask;
    while ( true )
      {
        authorizePageWrite ( relPagePtr );

        if ( size <= PageSize ) break;
        size -= PageSize;
        relPagePtr += PageSize;
      }
#if 0 // Stupid extra check ?
    size = alignSize ( _size );
    void* seg = getSegment<void,Read> ( segPtr, size );
    alter ( seg, size );
    protect ( seg, size );
#endif
  }

  __INLINE void* DocumentAllocator::__getSegment_Read ( SegmentPtr segPtr, __ui64 size )
  {
#if PARANOID
    AssertBug ( segPtr, "Null segpointer provided !\n" );
    AssertBug ( documentAllocationHeader, "Null documentAllocationHeader !\n" );
    if ( segPtr >= getDocumentAllocationHeader().nextRelativePagePtr )
      {
        checkContents();
      }
    AssertBug ( segPtr < getDocumentAllocationHeader().nextRelativePagePtr, "Seg pointer out of bounds : '%llx'\n", segPtr );
#endif
    __ui64 areaIdx = segPtr >> InAreaBits;
#if PARANOID
    // AssertBug ( areaIdx < areasAlloced, "AreaIdx %llx out of range (max %llx)\n", areaIdx, areasAlloced );
    AssertBug ( size <= AreaSize, "Segment size exceeds Area boundaries ! segPtr=0x%llx, size=0x%llx\n",
      segPtr, size );
    AssertBug ( (segPtr % AreaSize) + size <= AreaSize, "Segment exceeds Area boundaries ! segPtr=0x%llx, size=0x%llx\n",
      segPtr, size );
#endif

#if 0
    lockMutex_Map();
    AssertBug ( areaIdx < areasAlloced, "AreaIdx %llx out of range (max %llx)\n", areaIdx, areasAlloced );
    if( ! areas[areaIdx] )
      {
        mapArea ( areaIdx );
      }
    unlockMutex_Map();
#else
    if( areaIdx >= areasAlloced || ! areas[areaIdx] )
      {
        mapArea ( areaIdx );
      }
#endif
#if PARANOID
        AssertBug ( areas[areaIdx], "Could not map area %llx !\n", areaIdx );
#endif
    return (void*) ( (__ui64) areas[areaIdx] + ( segPtr & AreaPageMask ) );
  }

  template<typename T, PageCredentials how> 
  __INLINE T* DocumentAllocator::getSegment ( SegmentPtr segPtr, __ui64 size )
  {
    void* seg = __getSegment_Read ( segPtr, size );
    if ( how == Write && !__assertDocumentAlwaysWritable ) __authorizeWrite ( segPtr, size );
    return (T*) seg;
  }
};
