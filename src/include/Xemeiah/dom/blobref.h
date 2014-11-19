#ifndef __XEM_DOM_BLOBREF_H
#define __XEM_DOM_BLOBREF_H

#include <Xemeiah/dom/skmapref.h>
#include <Xemeiah/kern/format/blob.h>

namespace Xem
{
  /**
   * BlobRef is a Binary-Large-Object attribute, based on the SKMapRef storing
   */
  class BlobRef : public SKMapRef
  {
  protected:

    /**
     * When there is no piece, append a new one
     */ 
    __ui64 appendPiece ( void** buffPtr, __ui64 count, __ui64 offset, bool delaySetSize );
    
    /**
     * madviseWillNeed on a window
     */
    void madviseWillNeed ( void* window, __ui64 windowSize );

    /**
     * set Size
     */
    void setSize ( __ui64 size );
    
    /**
     * Check update
     */
    void checkUpdateJournal ();
  public:
    /**
     *
     */
    BlobRef ( Document& doc ) : SKMapRef(doc)
    {}

    /**
     * ElementMapRef instanciator from an AttributeRef
     */
    BlobRef ( const AttributeRef& attrRef );  
    ~BlobRef ();

    /**
     * Get Piece allocation Profile
     */
    AllocationProfile getPieceAllocationProfile ();    

    /**
     * Set Piece allocation Profile
     */
    void setPieceAllocationProfile ( AllocationProfile allocProfile );    

    /**
     * Get total size of current blob
     */
    __ui64 getSize ();

    /**
     * Get a piece of a blob
     */
    __ui64 getPiece ( void** buffPtr, __ui64 count, __ui64 offset );

    /**
     * Read a piece of blob
     * @param buff the buffer to write to
     * @param count the maximum number of bytes to write (size of buff)
     * @offset where to start to
     * @return the number of bytes read (until the end of the piece)
     */
    __ui64 readPiece ( char* buff, __ui64 count, __ui64 offset );

    /**
     * Allow a part of a piece to be written
     * @param buff the pointer to a buffer we will write to
     * @param count the maximum size we would write to (in case of allocation)
     * @param offset the offset of the buffer we can write to
     * @return the number of bytes we can write in this piece, which shall not be more than count (but may be less)
     */
    __ui64 allowWritePiece ( void** buffPtr, __ui64 count, __ui64 offset, bool delaySetSize = false );
    
    /**
     * Protect a piece allowed by allowWritePiece ()
     * @param buff the pointer provided by allowWritePiece()
     * @param pieceSize the size of the piece provided by allowWritePiece()
     */
    void protectWritePiece ( void* buff, __ui64 pieceSize );

    /**
     * Write a piece of blob
     * @param buff the buffer to read from to
     * @param count the total number of bytes to read from (size of buff)
     * @offset where to start to
     * @return the number of bytes written
     */
    __ui64 writePiece ( const void* buff, __ui64 count, __ui64 offset );

    /**
     * Read from a file
     */
    void readFromFile ( const String& filePath );

    /**
     * Read from a file
     */
    void readFromFile ( const String& filePath, void (*windowEvent) (BlobRef& blob, void* arg), void* arg );

    /**
     * Read from a file
     */
    void saveToFile ( const String& filePath );

    /**
     * Modify blob size
     */
    void truncateSize ( __ui64 newSize );

    /**
     * Copy a blob
     */
    void copyFrom ( BlobRef& sourceBlob );

    /**
     * Update a blob from another blob
     */
    void updateBlobFrom ( BlobRef& sourceBlob )
    { copyFrom ( sourceBlob ); }    
    
    /**
     * Set mime-type
     */
    void setMimeType ( const String& mimeType );

    /**
     * Get mime-type
     */
    String getMimeType ();

    /**
     *
     */
    void inflate ( BlobRef& targetBlob, const String& method = "gzip" );

    /**
     * check
     */    
    void check ();
    
    /**
     * deleteAttribute : delete all contents !
     */
    virtual void deleteAttribute ();
  };
};

#endif // __XEM_DOM_BLOBREF_H

