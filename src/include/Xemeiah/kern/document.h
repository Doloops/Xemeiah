#ifndef __XEM_KERN_DOCUMENT_H
#define __XEM_KERN_DOCUMENT_H

#include <Xemeiah/kern/store.h>
#include <Xemeiah/kern/mutex.h>
#include <Xemeiah/kern/documentallocator.h>
#include <Xemeiah/kern/transactionaldocument.h>
#include <Xemeiah/dom/string.h>
#include <map>

#define __XEM_DOCUMENT_HAS_REFCOUNT
// #define __XEM_DOCUMENT_HAS_REFCOUNT_LOG
// #define __XEM_DOCUMENT_HAS_DOCUMENTTAG // Option : document has a document tag

namespace Xem
{
  class NodeRef;
  class ElementRef;
  class ElementMultiMapRef;
  class DocumentMeta;
  class XProcessor;
  class XPath; // For the indexing stuff
  class XPathParser; 
  class NodeSet;


  /**
   * Define the RoleId as a non-namespace QName
   */
  typedef LocalKeyId RoleId;
  
  /**
   * Document class. 
   * Provides access to a specific revision, whether Read-only or Read-Write.
   * Documents can be set volatile (VolatileDocument), such that they will not be stored on disk, and all data
   * created will be lost when the document will be deleted.
   */
  class Document : public TransactionalDocument
  {
    friend class Store;
    friend class PersistentStore;
    friend class DocumentRef; // Temporary, and ugly
    friend class NodeRef;
    friend class ElementRef;
    friend class AttributeRef;
    friend class SKMapRef;
    friend class BlobRef; //< Needs the ability to allocate data for blob pieces
    friend class SKMultiMapRef;
    friend class SubDocument;
  protected:
    /**
     * Reference to the Store.
     */
    Store& store;

    /**
     * Reference to our DocumentAllocator
     */
    DocumentAllocator& documentAllocator;

    /**
     * Reference to a DocumentAllocator for garbage collection
     */
    DocumentAllocator* boundDocumentAllocator;

    /**
     * Do Reference a DocumentAllocator for garbage collection
     */
    void bindDocumentAllocator ( DocumentAllocator* allocator );

    /**
     * DocumentAllocator Accessor
     */
    INLINE DocumentAllocator& getDocumentAllocator() const { return documentAllocator; }
  
#ifdef __XEM_DOCUMENT_HAS_DOCUMENTTAG
    /**
     * Document name.
     */
    String documentTag;
#endif

    /**
     * Segment pointer to the DocumentHead
     */
    SegmentPtr documentHeadPtr;

    /**
     * Pointer to the Root Element of this Document.
     */
    ElementPtr rootElementPtr;

    /**
     * Unparsed Entities Map class
     */
    typedef std::map<String,String> UnparsedEntitiesMap;

    /**
     * Pointer to unparsed entities map
     */
    UnparsedEntitiesMap* unparsedEntitiesMap;

    /**
     * Returns the DocumentHead of this document.
     */
    INLINE DocumentHead& getDocumentHead();

    /**
     * Default constructor to initialze stuff
     * @param store the Store to bind to.
     */
    Document ( Store& store, DocumentAllocator& allocator );

    /**
     * ElementId index boolean. If set to true, each created Element will be
     * indexed using a SKMapRef placed as an attribute of the root element.
     * isIndexed is computed like this : 
     * - at revision creation time, only non-volatile documents are indexed (i.e. isIndexed = ! isVolatile).
     * - when a document is openned on an already existing revision, the root is scanned for an attribute called xem:index 
     *   of type AtttributeType_SKMap. If the attribute exists, isIndexed is set to true.
     *
     * (Nota : the root element itself is indexed).
     * \todo cache the SKMap reference in the Document, 
     * to avoid the lookup for each element creation.
     */
    bool isIndexed;

    /**
     * Callback function called in createRootElement() to determine if the document is indexed using ElementId
     * @return true if the document shall be indexed
     */
    virtual bool mayIndex() = 0;

    /**
     * Dummy function forcing template instanciation
     */
    void __foo__ ();

    /**
     * Document reference counts
     */
    __ui64 refCount;
    
    /**
     * RefCount Mutex
     */
    Mutex refCountMutex;
  public:
    /**
     * Lock refCount
     */
    void lockRefCount();

    /**
     * Unlock refCount
     */
    void unlockRefCount();

    /**
     * Assert that the refCount is locked
     */
    void assertRefCountLocked();

    /**
     * Increment document refcounting
     */
    void incrementRefCount ();
    void incrementRefCountLockLess ();

    /**
     * Decrement document refcounting
     */
    void decrementRefCount ();
    void decrementRefCountLockLess ();
    
    /**
     * Return reference counting
     */
    bool isReferenced ();
    
    /**
     * Return number (for information)
     */
    __ui64 getRefCount () const { return refCount; }
  protected:

    /**
     * DocumentHead alter
     */
#ifdef XEM_MEM_PROTECT_SYS
    INLINE void alterDocumentHead ();
    INLINE void protectDocumentHead ();
#else
    INLINE void alterDocumentHead () {}
    INLINE void protectDocumentHead () {}
#endif


    /*
     * DOM Initiation facilities
     */
    
    /**
     * Root Element Creation
     * @return false upon failure (unused).
     */
    bool createRootElement ();

    /**
     * Provides a free ElementId to be used in a new element creation.
     * Each Document has its own ElementId cache, filled by Store::reserveElementIds().
     * @return a free ElementId.
     * @see Store::reserveElementIds()
     */
    ElementId getFreeElementId ();

    /**
     * Indexes an element ptr from its ElementId
     * The document must be set isIndexed
     * @see isIndexed
     */
    void indexElementById ( ElementRef& eltRef );

    /**
     * Removes the index
     */
    void unIndexElementById ( ElementRef& eltRef );

    /**
     * Document Role, as a KeyId
     */
    RoleId roleId;

    /**
     * release Document resources (must be called with refCount mutex on)
     */
    virtual void releaseDocumentResources () = 0;

    /**
     * Document destructor.
     * A Bug() will be called if the document still has a non-zero refCount.
     */
    virtual ~Document();
  public:
    /**
     * Returns a reference to the Store.
     */
    INLINE Store& getStore() const { return store; } 

    /**
     * Returns a reference to the Store's KeyCache
     */
    INLINE KeyCache& getKeyCache() { return getStore().getKeyCache(); }

    /**
     * @return true if the document is set Write, false if set Read
     */
    INLINE bool isWritable ();

#ifdef __XEM_DOCUMENT_HAS_DOCUMENTTAG
    /**
     * Gets the document tag;
     */
    INLINE const String& getDocumentTag() { return documentTag; }

    /**
     * Set the document tag
     */
    INLINE void setDocumentTag ( const String& str ) { documentTag = stringFromAllocedStr(strdup(str.c_str())); }
#else
    /**
     * Gets the document tag
     */
    virtual String getDocumentTag();
#endif

    /**
     * @return the document URI
     */
    virtual String getDocumentURI();
    
    /**
     * @return the document base URI
     */
    virtual String getDocumentBaseURI();

    /**
     * Sets document's URI
     */
    virtual void setDocumentURI ( const String& uri );

    /**
     * @return the root element of the revision.
     * \todo think about a way to cleanly implement chrooted environments.
     */
    ElementRef getRootElement();

    /**
     * Element creation : creates a orphan (ie to link) element
     * @param fromElement an element to compute memory-affinity from
     * @param keyId the keyId of the element to create
     * @return the orphan Element
     */
    ElementRef createElement ( ElementRef& fromElement, KeyId keyId );

    /**
     * Element creation : creates a orphan (ie to link) element
     * @param fromElement an element to compute memory-affinity from
     * @param keyId the keyId of the element to create
     * @param elementId the elementId of the element to create
     * @return the orphan Element
     */
    ElementRef createElement ( ElementRef& fromElement, KeyId keyId, ElementId elementId );

  protected:
    /**
     * Textual Node creation : creates a orphan (ie to link) text(), pi() or comment()
     * @param fromElement an element to compute memory-affinity from
     * @param keyId the keyId of the element to create
     * @param elementId the elementId of the element to create (provide elementId==0 to assign a new one)
     * @return the orphan Element
     */
    ElementRef createTextualNode ( ElementRef& fromElement, KeyId keyId, ElementId elementId );

  public:
    /**
     * Create a text node
     * @param fromElement an element to compute memory-affinity from
     * @param text the textual contents
     * @return the orphan Element
     */
    ElementRef createTextNode ( ElementRef& fromElement, const char* text );

    /**
     * Create a text node
     * @param fromElement an element to compute memory-affinity from
     * @param text the textual contents
     * @return the orphan Element
     */
    ElementRef createTextNode ( ElementRef& fromElement, const String& text );

    /**
     * Create a comment node
     * @param fromElement an element to compute memory-affinity from
     * @param comment the comment
     * @return the orphan Element
     */
    ElementRef createCommentNode ( ElementRef& fromElement, const char* comment );

    /**
     * Create a PI node
     * @param fromElement an element to compute memory-affinity from
     * @param piName the name of the PI
     * @param piContents the contents of the PI
     * @return the orphan Element
     */
    ElementRef createPINode ( ElementRef& fromElement, const char* piName, const char* piContents );
    
    /**
     * Returns an ElementRef from a elementId
     * The document must be set indexed
     * @param elementId the elementId to find
     * @return the ElementRef corresponding to the ElementId provided.
     * @see isIndexed
     */
    ElementRef getElementById ( ElementId elementId );

    /**
     * Returns an ElementRef from a const char* elementId
     * @param strElementId the string containing the elementId.
     * @return the ElementRef corresponding to the ElementId provided.
     * @see getElementId()
     */
    virtual ElementRef getElementById ( const char* strElementId );

    /**
     * Returns an ElementRef from a String elementId
     * @param strElementId the string containing the elementId.
     * @return the ElementRef corresponding to the ElementId provided.
     * @see getElementId()
     */
    ElementRef getElementById ( const String& strElementId );

    /**
     * Returns the DocumentMeta associated with this document
     */
    DocumentMeta getDocumentMeta ();

  protected: // Protected Meta helpers
    /**
     * Get the Meta-Element root for this document
     * @param create set to true to create the Meta-Element if it does not exist
     * @return the meta-element, or a zero ElementRef if not exist and create=false
     */
    ElementRef getMetaElement ( bool create = false );

    /**
     * Process a new Dom Event
     */
    INLINE void processDomEvent ( XProcessor& xproc, DomEventType eventType, NodeRef& nodeRef );
  public:

    /**
     * Returns a SKMultiMapRef for element indexing stuff (ex: xsl:key())
     * \todo Shall migrate to the metaIndexer mechanism
     * @param keyNameId the name of the mapping key 
     * @return the corresponding ElementMultiMapRef
     */
    ElementMultiMapRef getKeyMapping ( KeyId keyNameId );

    /**
     * Returns a newly created SKMultiMapRef for element indexing.
     * @param keyNameId the name of the mapping key
     * @param config the SKMapConfig.
     * @return the newly created ElementMultiMapRef
     */
    ElementMultiMapRef createKeyMapping ( KeyId keyNameId, SKMapConfig& config );

    /**
     * Returns a newly created SKMultiMapRef for element indexing.
     * @param keyNameId the name of the mapping key
     * @return the newly created ElementMultiMapRef
     */
    ElementMultiMapRef createKeyMapping ( KeyId keyNameId );

    /**
     * Do some cleaning, several stuff.
     * This may hurt the pointers, so do it when you are sure no in-mem ptr is active.
     */
    virtual void housewife ();

    /**
     * Get document RoleId
     */
    INLINE RoleId getRoleId();
    
    /**
     * Set document RoleId
     */
    INLINE void setRoleId ( RoleId roleId );

    /**
     * Get document role
     */
    INLINE String getRole();
    
    /**
     * Set document role
     */ 
    INLINE void setRole ( const String& _role );

    /**
     * Get the flags used to open this document
     */
    virtual DocumentOpeningFlags getDocumentOpeningFlags()
    { Bug ( "no getDocumentOpeningFlags.\n" ); return DocumentOpeningFlags_Invalid; }

    /**
     * Set a new unparsed entity
     */
    virtual void setUnparsedEntity ( const String& entityName, const String& entityValue );

    /**
     *
     */
    virtual String getUnparsedEntity ( const String& entityName );

    /**
     * Check document correctness (steal stuff from Persistence)
     */ 
    virtual bool checkContents() { return true; }
    
    /**
     * Document statistics
     */
    class Stats
    {
    public:
      Stats();
      ~Stats();
      __ui64 numberOfXPathParsed;
      __ui64 numberOfXPathInstanciated;
      void showStats();
    };

    /**
     * Document statistics instance
     */
    Stats stats;
    
    /**
     * Misc stats : get number of areas alloced
     */
    __ui64 getNumberOfAreasAlloced () const { return getDocumentAllocator().getNumberOfAreasAlloced(); }
    
    /**
     * Misc stats : get number of areas mapped
     */
    __ui64 getNumberOfAreasMapped () const { return getDocumentAllocator().getNumberOfAreasMapped(); }
    
    /**
     * Misc stats : get area size
     */
    __ui64 getAreaSize () const { return getDocumentAllocator().getAreaSize(); }
    
    /**
     * Get total in-memory size for this document
     */
    __ui64 getTotalDocumentSize () const { return getDocumentAllocator().getTotalDocumentSize(); }
  };
};

#endif // __XEM_KERN_DOCUMENT_H

