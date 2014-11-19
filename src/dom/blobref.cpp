#include <Xemeiah/dom/blobref.h>
#include <Xemeiah/dom/attributeref.h>

#include <Xemeiah/kern/format/journal.h>

#include <Xemeiah/auto-inline.hpp>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
// #include <zlib.h> // Immature : zlib inflate/deflate for blobs

#define __XEM_BLOB_READPIECE_MADVISE

#ifdef __XEM_BLOB_READPIECE_MADVISE
#include <sys/mman.h>
#endif

#define Log_Blob Debug
#define Warn_Blob Warn

// #define __XEM_DOM_BLOBREF_CHECK

namespace Xem
{
  BlobRef ElementRef::addBlob ( KeyId blobNameId )
  {
    SKMapConfig config;
    SKMapRef::initDefaultSKMapConfig ( config );
    BlobRef blobRef = addSKMap ( blobNameId, config, SKMapType_Blob );
    blobRef.setPieceAllocationProfile ( 15 );    
    return blobRef;
  }
  
  BlobRef::BlobRef ( const AttributeRef& attrRef )
  : SKMapRef ( attrRef ) 
  {
    checkSKMapType ( SKMapType_Blob );
    Log_Blob ( "Instanciated blob at 0x%llx, size=0x%llx\n", nodePtr, nodePtr ? getSize() : -1 );
#ifdef __XEM_DOM_BLOBREF_CHECK
    if ( nodePtr ) check ();
#endif
  }

  BlobRef::~BlobRef ()
  {
#ifdef __XEM_DOM_BLOBREF_CHECK
    check ();
#endif  
  }

  AllocationProfile BlobRef::getPieceAllocationProfile ()
  {
    BlobHeader* blobHeader = getData<BlobHeader,Read> ();
    return blobHeader->pieceAllocationProfile;
  }
  
  void BlobRef::setPieceAllocationProfile ( AllocationProfile allocProfile )
  {
    BlobHeader* blobHeader = getData<BlobHeader,Write> ();
    alterData ();
    blobHeader->pieceAllocationProfile = allocProfile;
    protectData ();
  }
    
  __ui64 BlobRef::getSize ()
  {
    BlobHeader* blobHeader = getData<BlobHeader,Read> ();
    return blobHeader->totalSize;
  }

  void BlobRef::setSize ( __ui64 size )
  {
    BlobHeader* blobHeader = getData<BlobHeader,Write> ();

    alterData ();
    blobHeader->totalSize = size;
    protectData ();
  }

  void BlobRef::checkUpdateJournal ()
  {
    Warn ( "To reimplement !\n" );
#if 0
    BlobHeader* blobHeader = getData<BlobHeader,Read> ();
    if ( bridcmp ( getDocument().getBranchRevId(), blobHeader->lastUpdatedBranchRevId ) )
      {
        blobHeader = getData<BlobHeader,Write> ();
        
        Log_Blob ( "(blob0x%llx) checkUpdate [%llx:%llx]->[%llx:%llx]\n", nodePtr, _brid(blobHeader->lastUpdatedBranchRevId),
            _brid(getDocument().getBranchRevId()) );
        
        alterData ();
        blobHeader->lastUpdatedBranchRevId = getDocument().getBranchRevId();
        protectData ();

        AssertBug ( getDocument().isWritable(), "Document is not writable !\n" );
        ElementRef baseElement = getElement();
        getDocument().appendJournal ( baseElement, JournalOperation_UpdateBlob, baseElement, getKeyId() );
      }
#endif
  }

  __ui64 BlobRef::getPiece ( void** buffPtr, __ui64 count, __ui64 offset )
  {
    AssertBug ( nodePtr, "Could not get piece on an empty BlobRef !\n" );
    // if ( offset % sizeof(FreeSegment) ) Bug ( "Offset not aligned to FreeSegmentSize !\n" );
    
    *buffPtr = NULL;
    
    if ( getSize() == 0 ) return 0;
    if ( offset >= getSize() ) return 0;

    iterator iter(*this);
    iter.findClosest ( offset );
    if ( ! iter )
      {
        Warn_Blob ( "iter is null !\n" );
        return 0;
      }
    /**
     * Checking the piece start
     */
    __ui64 pieceStart = iter.getHash ();
    AssertBug (  pieceStart <=  offset, 
          "Provided a piece with an offset=0x%llx, whereas called with offset=0x%llx\n",
          pieceStart,  offset );
    __ui64 readOffset = offset - pieceStart; // readOffset is the size between the beginning of the piece and the offset asked

    __ui64 nextPieceStart;
    if ( ! iter.getNextHash(nextPieceStart) ) 
      {
        nextPieceStart = getSize();
        Bug ( "That shall not happen !\n" );
      }
    
    __ui64 pieceLength = nextPieceStart - pieceStart;

    
    /**
     * Now checking the piece offset
     */
    __ui64 pieceOffset = iter.getValue();
    if ( ! pieceOffset )
      {
        Warn_Blob ( "iter has null value !\n" );
        return 0;
      }

    AssertBug ( readOffset < pieceLength, "Invalid situation.\n" );
    __ui64 readLength = pieceLength - readOffset;

    void* piece = getDocumentAllocator().getSegment<void,Read> ( pieceOffset + readOffset, readLength );
    *buffPtr = piece; 

    Log_Blob ( "READ-BLOB Provided piece at %p (pieceOffset=0x%llx, readOffset=0x%llx, readLength=0x%llx, "
        "pieceLength=0x%llx, count=0x%llx)\n", 
        *buffPtr, pieceOffset, readOffset, readLength, pieceLength, count );
    return readLength;
  }
  
  void BlobRef::madviseWillNeed ( void* window, __ui64 windowSize )
  {
#ifdef __XEM_BLOB_READPIECE_MADVISE
    __ui64 alignedWindow = ((__ui64)window & PagePtr_Mask);
    __ui64 alignedEnd = ((__ui64) window + (__ui64) windowSize);
    if ( alignedEnd & ~PagePtr_Mask )
      {
        alignedEnd = (alignedEnd & PagePtr_Mask) + PageSize;
      }
    __ui64 alignedSize = alignedEnd - alignedWindow;

    if ( madvise ( (void*)alignedWindow, alignedSize, MADV_WILLNEED ) == -1 )
      {
        Warn ( "Could not madvise(%p,%llx,MADV_WILLNEED) (src %p:%llx) : error=%d:%s\n",
            (void*) alignedWindow, alignedSize, window, windowSize, errno, strerror(errno) );
        Warn ( "Could not madvise : at document [%llx:%llx], role=%s, blob=%s\n",
            _brid(getDocument().getBranchRevId()), getDocument().getRole().c_str(),
            generateVersatileXPath().c_str() );
      }
    else
      {
        Log_Blob ( "Ok for madvise(%p,%llx,MADV_WILLNEED) (src %p:%llx).\n",
            (void*) alignedWindow, alignedSize, window, windowSize );
      }
#endif // __XEM_BLOB_READPIECE_MADVISE
  }

  __ui64 BlobRef::readPiece ( char* buff, __ui64 count, __ui64 offset )
  {
#if 0
    Bug ( "BOGUS ! DONT USE !!!!\n" );
    void* window;
    __ui64 windowSize = getPiece ( &window, count, offset );
    if ( ! windowSize ) return 0;
    memcpy ( buff, window, windowSize );
    return windowSize;
#endif
    __ui64 remains = count;
    __ui64 uoffset = 0;

    if ( offset >= getSize() )
      {
        Warn ( "Trying to read beyond blob end... Exiting right now.\n" );
        // Bug ( "." );
        return 0;
      }
    if ( offset + remains > getSize() )
      {
        remains = getSize() - offset;
        Warn ( "Would try and read more than blob size (%llu), offset=%llu reducing from %llu (end %llu) to %llu (end %llu)\n",
            getSize(),
            offset,
            count, count + offset,
            remains, remains + offset );
        // Bug ( "." );
      }

    while ( remains )
      {
        void* window;
        AssertBug ( offset < getSize(), "Starting beyond blob !\n" );

        __ui64 windowSize = getPiece ( &window, remains, offset );
        if ( !windowSize )
          {
            Warn ( "Could not read !, windowSize=%ld, offset=0x%llx, remains=0x%llx\n",
                (long) windowSize, offset, remains );
            // AssertBug ( remains + offset == getSize(), "Read beyond blob ?\n" );
            AssertBug ( offset >= getSize(), "Starting beyond blob !\n" );
            break;
          }
        madviseWillNeed ( window, windowSize );

        if ( windowSize > remains ) windowSize = remains;

        memcpy ( &(buff[uoffset]), window, windowSize );
        __ui64 written = windowSize;
        offset += written;
        uoffset += written;
        remains -= written;
      }
    return uoffset;
  }  

  __ui64 BlobRef::appendPiece ( void** buffPtr, __ui64 count, __ui64 offset, bool delaySetSize )
  {
    __ui64 alignedCount = getDocumentAllocator().alignSize ( count + (offset % sizeof(FreeSegment)) );
    __ui64 alignedOffset = offset - (offset % sizeof(FreeSegment));

    SegmentPtr segPtr = getDocumentAllocator().getFreeSegmentPtr ( alignedCount, getPieceAllocationProfile() );
    Log_Blob ( "Created a new segment at 0x%llx, offset=0x%llx, alignedOffset=0x%llx, count=0x%llx, alignedCount=0x%llx\n", 
        segPtr, offset, alignedOffset, count, alignedCount );

    iterator lastIter(*this);
    lastIter.findClosest ( alignedOffset );

    if ( !getSize() )
      {
        Log_Blob ( "BLOB INIT : insert [0x%llx]=0x%llx\n", alignedOffset, segPtr );
        AssertBug ( !lastIter, "lastIter not empty, but blob has zero size !\n" );
        AssertBug ( alignedOffset == 0 && offset == 0, "Not zero aligned-offsets !\n" );
        lastIter.insert ( alignedOffset, segPtr );
      }
    else
      {
        AssertBug ( lastIter, "lastIter does not exist !\n" );
        Log_Blob ( "lastIter hash=0x%llx, value=0x%llx\n", lastIter.getHash(), lastIter.getValue() );
        AssertBug ( lastIter.getValue() == 0, "Invalid non-zero lastIter !\n" );
        lastIter.setValue ( segPtr );
      }

#if 0
    if ( lastIter && lastIter.getHash() == alignedOffset ) 
      {
        AssertBug ( lastIter.getValue() == 0, "Invalid non-zero value.\n" );
        lastIter.setValue ( segPtr );
      }
    else 
      {
        // if ( offset % sizeof(FreeSegment) ) Bug ( "Offset not aligned to FreeSegment !\n" );
        lastIter.insert ( alignedOffset, segPtr );
        // iter.findClosest ( offset );
      }
#endif
    /**
     * Mark the next piece
     */
    __ui64 nextPieceStart = alignedOffset + alignedCount;
    iterator iterNext(*this);
    iterNext.findClosest ( nextPieceStart );
    iterNext.insert ( nextPieceStart, 0 );
    
    Log_Blob ( "(blob0x%llx) Marked aligned : segment from 0x%llx must have next=[0x%llx] with 0x0 offset\n", 
        nodePtr, alignedOffset, nextPieceStart );
    
    /*
     * Shall update size now
     */
    // AssertBug ( blobHeader->totalSize < size, "Invalid append ! (SHRINK NOT IMPLEMENTED YET)\n" );
     
    if (! delaySetSize)
      setSize ( offset+count );
    
    Log_Blob ( "SegPtr=0x%llx, corrected=0x%llx\n", segPtr, segPtr + (offset - alignedOffset) );
    char* piece = getDocumentAllocator().getSegment<char,Write> ( segPtr, alignedCount );
    // getDocument().alter ( piece, alignedCount );
    
    *buffPtr = &(piece[offset - alignedOffset]);
    Log_Blob ( "WRITE-BLOB APPEND Provided piece at %p (buff=%p)  (segPtr=0x%llx, alignedCount=0x%llx), "
        "for offset=0x%llx, count=0x%llx), ajustment=0x%llx\n", 
        piece, *buffPtr, segPtr, alignedCount, offset, count, offset - alignedOffset );
       
    getDocumentAllocator().alter ( *buffPtr, count );
    return count;
  }
    
  __ui64 BlobRef::allowWritePiece ( void** buffPtr, __ui64 count, __ui64 offset, bool delaySetSize )
  {
    AssertBug ( count, "Invalid count=0\n" );
    AssertBug ( nodePtr, "Could not get piece on an empty BlobRef !\n" );

    if ( ! getDocument().isWritable() )
      {
        Bug ( "Document not writable !\n" );
        throwException ( Exception, "Document not writable !\n" );
      }
    if ( ! getDocument().isLockedWrite() )
      {
        Bug ( "Document not locked write !\n" );
        throwException ( Exception, "Document not locked write !\n" );
      }

    /**
     * Check that we are not out of bounds
     */
    if ( count > SegmentSizeMax ) count = SegmentSizeMax;

    /**
     * align size
     */

    __ui64 alignedOffset = offset - (offset % sizeof(FreeSegment));
    
#if PARANOID
    AssertBug ( alignedOffset <= offset, "invalid align.\n" );
    __ui64 alignedCount = getDocumentAllocator().alignSize ( count + (offset % sizeof(FreeSegment)) );
    AssertBug ( alignedCount >= count, "invalid align.\n" );
    AssertBug ( offset + count <= alignedOffset + alignedCount, "Invalid segment.\n" );

    Log_Blob ( "(blob%llx) At write : getSize()=0x%llx, offset=0x%llx, alignedOffset=0x%llx, count=0x%llx, alignedCount=0x%llx\n",
       nodePtr, getSize(), offset, alignedOffset, count, alignedCount );
#endif
    
    if ( offset > getSize() )
      {
        NotImplemented ( "Writing blob with holes !\n" );
      }
    
    /**
     * Find the first piece that could match
     */
    if ( getSize() == 0 && offset == 0 )
      {
        /**
         * simply append this
         */
        checkUpdateJournal ();
        __ui64 toWrite = appendPiece ( buffPtr, count, offset, delaySetSize );
#ifdef __XEM_DOM_BLOBREF_CHECK
        Log_Blob ( "** Checking after INIT :\n" );
        check ();
#endif // __XEM_DOM_BLOBREF_CHECK
        return toWrite;
      }

    iterator iter(*this);
    iter.findClosest ( alignedOffset );
    
    if ( ! iter )
      {
        Bug ( "iter is null ! offset=0x%llx, getSize()=0x%llx\n",  offset, getSize() );
      }
    if ( ! iter.getValue() )
      {
        if ( offset == getSize() )
          {
            /*
             * We can append freely here.
             */
            if ( offset != alignedOffset )
              {
                NotImplemented ( "Not-aligned offsets.\n" );
              }
            checkUpdateJournal ();
            __ui64 toWrite = appendPiece ( buffPtr, count, offset, delaySetSize );
#ifdef __XEM_DOM_BLOBREF_CHECK            
            Log_Blob ( "** Checking after APPEND :\n" );
            check ();
#endif // __XEM_DOM_BLOBREF_CHECK
            return toWrite;
          }
        NotImplemented ( "HOLE ???\n" );
      }

    __ui64 pieceStart = iter.getHash ();
    __ui64 pieceOffset = iter.getValue();

    /**
     * Ok, now we have an iterator, which shall fit to the piece
     */
    AssertBug (  pieceStart <=  offset, "Provided a piece with an offset=0x%llx, whereas called with offset=0x%llx\n",
      pieceStart,  offset );
    __ui64 readOffset = alignedOffset - pieceStart; // readOffset is the size between the beginning of the piece and the offset asked

    __ui64 nextPieceStart;
    if ( ! iter.getNextHash(nextPieceStart) ) nextPieceStart = getSize();
    
    AssertBug ( pieceStart < nextPieceStart, "Invalid starts !\n" );
    
    __ui64 pieceLength = nextPieceStart - pieceStart;

    Log_Blob ( "Found piece start=0x%llx, next=0x%llx, length=0x%llx, pieceOffset=0x%llx\n", 
        pieceStart, nextPieceStart, pieceLength, pieceOffset );

    AssertBug ( readOffset < pieceLength, "Invalid situation.\n" );
    __ui64 writeLength = pieceLength - readOffset;

    
    if ( ! pieceOffset )
      {
        Log_Blob ( "pieceOffset is null : that means a hole. pieceStart=0x%llx, next=0x%llx\n",
            pieceStart, nextPieceStart );
        
        Bug ( "." );
      }

    checkUpdateJournal ();

    char* piece = getDocumentAllocator().getSegment<char,Write> ( pieceOffset + readOffset, writeLength );
    *buffPtr = &(piece[offset - alignedOffset]);

    Log_Blob ( "WRITE-BLOB EXIST Provided piece at %p, buff=%p (segPtr=0x%llx, pieceOffset=0x%llx,readOffset=0x%llx)\n", 
        piece, *buffPtr, pieceOffset + readOffset, pieceOffset, readOffset );
        
    __ui64 finalCount = writeLength-(offset - alignedOffset);
    
    Log_Blob ( "[ALTER] : alter at buffPtr=%p, finalCount=0x%llx\n", *buffPtr, finalCount );
    
    Log_Blob ( "WRITE-BLOB EXIST minor adjustment = 0x%llx, count=0x%llx\n",
        offset - alignedOffset, finalCount );

    __ui64 writeEndsAt = alignedOffset + writeLength;
    if ( writeEndsAt > offset + count )
      {
        Warn ( "Restricting ?\n" );
        writeEndsAt = offset + count;
      }
    if ( ! delaySetSize && writeEndsAt > getSize() )
      {
        setSize ( writeEndsAt );
      }

#ifdef __XEM_DOM_BLOBREF_CHECK
    Log_Blob ( "** Checking after EXIST :\n" );
    check ();
#endif //__XEM_DOM_BLOBREF_CHECK

    getDocumentAllocator().alter ( *buffPtr, finalCount );
    return finalCount;
  }
  
  void BlobRef::protectWritePiece ( void* buff, __ui64 pieceSize )
  {
    getDocumentAllocator().protect ( buff, pieceSize );
  }

  __ui64 BlobRef::writePiece ( const void* buff, __ui64 count, __ui64 offset )
  {
    __ui64 writtenCount = 0;
    __ui64 remains = count;
    
    while ( remains )
      {
        void* piece;
        __ui64 pieceAskedSize = remains;
        // if ( pieceAskedSize < 64 ) pieceAskedSize = 64;
        __ui64 pieceSize = allowWritePiece ( &piece, pieceAskedSize, offset + writtenCount );
        if ( pieceSize < 0 )
          {
            Warn_Blob ( "Could not write ! That shall never happen.\n" );
            return writtenCount;
          }
        void* window = (void*) &(((char*)buff)[writtenCount]);
        __ui64 windowSize = pieceSize < remains ? pieceSize : remains;
        Log_Blob ( "memcpy : piece=%p, window=%p, windowSize=%llx\n", piece, window, windowSize );
        memcpy ( piece, window, windowSize );
        protectWritePiece ( piece, pieceSize );
        
        remains -= windowSize;
        writtenCount += windowSize;
      }
    return count;
  }

  void BlobRef::truncateSize ( __ui64 newSize )
  {
    Warn ( "BlobRef::truncateSize() : shall implement this !\n" );
    setSize ( newSize );  
  }
      
  void BlobRef::readFromFile ( const String& filePath )
  {
    readFromFile(filePath, NULL, NULL);
  }

  void BlobRef::readFromFile ( const String& filePath, void (*windowEvent) (BlobRef& blob, void* arg), void* arg )
  {
    int fd = open ( filePath.c_str(), O_RDONLY );
    if ( fd == - 1 ) throwException ( Exception, "Could not open file '%s'\n", filePath.c_str() );
    
    struct stat st;
    if ( fstat(fd,&st) ) 
      {
        ::close ( fd );
        throwException ( Exception, "Could not stat file '%s'\n", filePath.c_str() );
      }
    
    __ui64 fileSize = st.st_size;
    Log_Blob ( "File size of '%s' is 0x%llx\n", filePath.c_str(), fileSize );
    
    __ui64 remains = fileSize;
    __ui64 offset = 0;
    void* window;
    bool delaySetSize = true;
    
    while ( remains )
      {
        if ( offset && windowEvent )
          {
            (*windowEvent) ( *this, arg );
          }

        __ui64 windowSize = allowWritePiece ( &window, remains, offset, delaySetSize );
        AssertBug ( windowSize, "Invalid zero window size !\n" );
        
        ssize_t written = pread ( fd, window, windowSize, offset );
        protectWritePiece ( window, windowSize );

        if ( delaySetSize && getSize() < ( offset + windowSize ) )
          {
            Log_Blob ( "Blob '%s' : setSize %llx -> %llx\n", generateVersatileXPath().c_str(),
                getSize(), offset + windowSize );
            setSize ( offset + windowSize );
          }

        if ( written == -1 )
          {
            ::close ( fd );
            throwException ( Exception, "Could not read from file '%s', at offset=0x%llx, windowSize=0x%llx, err=%d:%s\n", 
                filePath.c_str(), offset, windowSize, errno, strerror(errno) );
          }
        if ( (__ui64) written != windowSize )
          {
            ::close ( fd );
            throwException ( Exception, "Could not read from file '%s', at offset=0x%llx, windowSize=0x%llx, written=0x%llx\n", 
                filePath.c_str(), offset, windowSize, (__ui64) written );
          }
        offset += written;
        remains -= written;
      }
    ::close ( fd );
  }
  
  void BlobRef::saveToFile ( const String& filePath )
  {
    int fd = ::open ( filePath.c_str(), O_RDWR|O_CREAT|O_TRUNC, 0644 );
    if ( fd == - 1 ) throwException ( Exception, "Could not open file '%s'\n", filePath.c_str() );

    __ui64 remains = getSize();
    __ui64 offset = 0;

    while ( remains )
      {
        void* window;
        __ui64 windowSize = getPiece(&window, SegmentSizeMax, offset );

        Log_Blob ( "remains=%llx, offset=%llx, windowSize=%llx\n", remains, offset, windowSize );

        if ( windowSize == 0 )
          {
            throwException ( Exception, "Invalid zero windowSize !\n" );
          }
        if  ( windowSize > remains )
          {
            windowSize = remains;
          }

        ssize_t res = pwrite ( fd, window, windowSize, offset );
        if ( res <= 0 )
          {
            ::close(fd);
            throwException ( Exception, "Could not pwrite : err=%d:%s\n", errno, strerror(errno));
          }
        AssertBug ( (__ui64) res == windowSize, "Could not fully write blob !\n" );
        remains -= windowSize;
        offset += windowSize;
      }
    ::close ( fd );
  }

  void BlobRef::copyFrom ( BlobRef& sourceBlob )
  {
    if ( sourceBlob.getSize() < getSize() )
      {
        NotImplemented ( "update means reduce in size !\n" );
      }
    if ( sourceBlob.getSize() == 0 ) return;
    
    __ui64 remains = sourceBlob.getSize();
    __ui64 offset = 0;
    while ( remains )
       {
         Log_Blob ( "At offset=0x%llx, remains=0x%llx\n", offset, remains );
         void* sourceWindow;
         __ui64 sourceWindowSize = sourceBlob.getPiece ( &sourceWindow, remains, offset );
         AssertBug ( sourceWindowSize, "Null sourceWindowSize\n" );
       
         void* targetWindow;
         __ui64 targetWindowSize = allowWritePiece ( &targetWindow, remains, offset );
         AssertBug ( targetWindowSize, "Null targetWindowSize\n" );
         
         __ui64 readBytes = sourceWindowSize < targetWindowSize ? sourceWindowSize : targetWindowSize;
         
         memcpy ( targetWindow, sourceWindow, readBytes );
         
         protectWritePiece ( targetWindow, targetWindowSize );
         
         if ( readBytes > remains ) readBytes = remains;
         remains -= readBytes;
         offset += readBytes;
       }
     Log_Blob ( "[BLOB-COPY] Copied %llu -> %llu bytes\n", sourceBlob.getSize(), getSize() );
  }

  void BlobRef::setMimeType ( const String& mimeType )
  {
    if ( mimeType.size() >= 256 )
      {
        throwException(Exception, "Mime type too long : '%s'\n", mimeType.c_str() );
      }
    try
      {
        char mimeTypeS[256], *mimeSubTypeS;
        strncpy ( mimeTypeS, mimeType.c_str(), 256 );
        mimeSubTypeS = strchr ( mimeTypeS, '/' );
        if ( ! mimeSubTypeS )
          {
            throwException ( Exception, "Invalid mime format !\n" );
          }
        *mimeSubTypeS = '\0'; mimeSubTypeS++;

        for ( char* c = mimeSubTypeS ; *c ; c++ )
          if ( *c == '+' ) *c = 'p';

        LocalKeyId mimeTypeId = getKeyCache().getKeyId(0, mimeTypeS,true);
        LocalKeyId mimeSubTypeId = getKeyCache().getKeyId(0, mimeSubTypeS,true);

        Log_Blob ( "Set mime type (%s,%s) => (%x,%x)\n", mimeTypeS, mimeSubTypeS, mimeTypeId, mimeSubTypeId );

        BlobHeader* blobHeader = getData<BlobHeader,Write> ();
        alterData();
        blobHeader->mimeTypeId = mimeTypeId;
        blobHeader->mimeSubTypeId = mimeSubTypeId;
        protectData();
      }
    catch ( Exception* e )
      {
        detailException(e, "Could not set mime type '%s' on blob '%s'\n", mimeType.c_str(), generateVersatileXPath().c_str() );
        throw;
      }
  }

  String BlobRef::getMimeType ()
  {
    BlobHeader* blobHeader = getData<BlobHeader,Read> ();
    if ( blobHeader->mimeTypeId && blobHeader->mimeSubTypeId )
      {
        String mimeType;
        stringPrintf(mimeType,"%s/%s",
            getKeyCache().getLocalKey(blobHeader->mimeTypeId),
            getKeyCache().getLocalKey(blobHeader->mimeSubTypeId) );
        return mimeType;
      }
    return "";
  }

#if 0 // IMMATURE
  void BlobRef::inflate ( BlobRef& targetBlob, const String& method )
  {
    z_stream myStream;
    gz_header gzHeader;

    memset(&myStream,0,sizeof(myStream));

    saveToFile("/tmp/proxy.blob.gz");

    Bug ( "." );
    // int windowBits = 15;
    // unsigned char* window = (unsigned char*) malloc ( 1 << windowBits );

//    int res = inflateBackInit(&myStream, windowBits, window );
    int res = ::inflateInit(&myStream);
    if ( res != Z_OK )
      {
        throwException ( Exception, "Could not init infalteBackInit : err=%d\n", res );
      }

    __ui64 inOffset = 0;
    __ui64 inRemains = getSize();

    bool header = false;

    while ( true )
      {
        Info ( "inOffset=%llx, inRemains=%llx\n", inOffset, inRemains );
        if ( myStream.avail_in == 0 && inRemains )
          {
            void* window;
            __ui64 windowSize = inRemains < SegmentSizeMax ? inRemains : SegmentSizeMax;
            myStream.avail_in = getPiece(&window,windowSize,inOffset);
            inOffset += myStream.avail_in;
            myStream.next_in = (Bytef*) window;

            Info ( "next_in=%p, avail_in=%d\n", myStream.next_in, myStream.avail_in );

            if ( header )
              {
                res = ::inflateGetHeader(&myStream,&gzHeader);
                if ( res != Z_OK )
                  {
                    throwException ( Exception, "Could not inflateGetHeader : err=%d\n", res );
                  }
                header = false;
              }
          }
        if ( myStream.avail_out == 0 )
          {
            myStream.avail_out = SegmentSizeMax;
            myStream.next_out = (Bytef*) malloc(myStream.avail_out);
            Info ( "next_out=%p, avail_out=%d\n", myStream.next_out, myStream.avail_out );
          }
        res = ::inflate ( &myStream, Z_BLOCK );
        if ( res != Z_OK )
          {
            throwException ( Exception, "Could not inflate : err=%d\n", res );
          }
      }
    // inflateInit(&myStream);
  }
#endif

  void BlobRef::check ()
  {
    __ui64 blobSize = getSize();
    Log_Blob ( "----------------------- CHECK (blob0x%llx) size=0x%llx ------------------------------\n",
        nodePtr, blobSize );
    bool foundEnd = false;
    for ( iterator iter(*this) ; iter ; iter++ )
      {
        __ui64 pieceStart = iter.getHash();
        __ui64 nextPieceStart = 0, pieceLength = 0;
        if ( iter.getNextHash(nextPieceStart))
            pieceLength = nextPieceStart - pieceStart;
        __ui64 pieceOffset = iter.getValue();
        if ( pieceStart < blobSize )
          {
            AssertBug ( pieceOffset, "No offset declared !\n" );
          }
        else
          {
            AssertBug ( !foundEnd, "Already found end !\n" );
            foundEnd = true;
            AssertBug ( !pieceOffset, "Piece after end has an offset !\n" );
          }
        Log_Blob ( "(blob0x%llx) P [0x%llx:0x%llx]->[0x%llx:0x%llx]->%p l=0x%llx, o=0x%llx, bS=0x%llx [end=0x%llx]\n",
            nodePtr, 
            pieceStart, nextPieceStart, 
            pieceOffset, pieceOffset + pieceLength,
            pieceOffset ? getDocumentAllocator().getSegment<void,Read> ( pieceOffset, 8 ) : NULL,
            pieceLength, pieceOffset, blobSize,
            pieceOffset + pieceLength ? *(getDocumentAllocator().getSegment<__ui64,Read> ( pieceOffset + pieceLength, sizeof(__ui64) ) ) : 0
            );
      } //     char* piece = getDocument().getSegment<char,Write> ( pieceOffset + readOffset, writeLength );

    Log_Blob ( "----------------------- END CHECK (blob0x%llx) size=0x%llx ------------------------------\n",
        nodePtr, blobSize );
  }

  void BlobRef::deleteAttribute ()
  {
    SegmentPtr itemPtr = getHeader()->head;
    while ( itemPtr )
      {
        SKMapItem* item = getDocumentAllocator().getSegment<SKMapItem,Read> ( itemPtr, getItemSize((__ui32)0) );
        SegmentPtr nextItemPtr = item->next[0];

        Info ( "At item=%llx, next=%llx\n", itemPtr, nextItemPtr );

        SKMapHash pieceStart = item->hash;
        SKMapHash pieceEnd = 0;
        if ( nextItemPtr )
          {
            pieceEnd = getDocumentAllocator().getSegment<SKMapItem,Read> ( nextItemPtr, getItemSize((__ui32)0) )->hash;
          }
        SKMapValue offset = item->value;
        if ( offset )
          {
            AssertBug ( pieceEnd, "No piece end !\n" );
            __ui64 pieceSize = pieceEnd - pieceStart;
            Info ( "\tDeleting from offset=%llx, size=%llx\n", offset, pieceSize );
            getDocumentAllocator().freeSegment ( offset, pieceSize );
          }
        getDocumentAllocator().freeSegment ( itemPtr, getItemSize(item) );
    
        itemPtr = nextItemPtr;
      }
    deleteAttributeSegment ();
  }
};
