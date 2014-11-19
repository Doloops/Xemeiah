#ifndef __XEM_KERN_STORE_H
#define __XEM_KERN_STORE_H

#include <Xemeiah/kern/format/core_types.h>
#include <Xemeiah/kern/format/document_head.h>
#include <Xemeiah/kern/keycache.h>
#include <Xemeiah/kern/mutex.h>
#include <Xemeiah/trace.h>

#include <map>
#include <list>
#include <string>

/*
 * Memory protection configuration :
 * XEM_MEM_PROTECT enables the memory protection
 * (if symbol is not defined, no protection is done at all).
 * XEM_MEM_PROTECT_SYS uses mprotect(2) to protect pages.
 * If XEM_MEM_PROTECT_SYS is defined, memory is RDONLY by default.
 * if not, memory is RDWR.
 * XEM_MEM_PROTECT_TABLE builds a table to  refCount the alteredPages.
 * maximum refCount is defined as mem_pages_table_max_refCount.
 * Nota : uses (unsigned char), so max=255.
 * XEM_MEM_PROTECT_ASYNC_ON_PROTECT tells to call msync ( .. , ASYNC )
 * when calling protectPage ().
 */
#ifndef XEM_MEM_PROTECT
#undef XEM_MEM_PROTECT_SYS
#undef XEM_MEM_PROTECT_TABLE
#undef XEM_MEM_PROTECT_ASYNC_ON_PROTECT
#endif


// #define __XEM_STORE_VOLATILEAREAS_USE_MMAP //< Option : use mmap() to allocate large memory chunks


namespace Xem
{
  class KeyCache;
  class Document;

  class DocumentAllocator;
  class VolatileDocument;
  class VolatileDocumentAllocator;
  class PersistentStore;
  class BranchManager;
  class ServiceManager;
  class XProcessorLibs;
  
  /**
   * Main Factory class for most of Xemeiah's objects, Store handles Key caching with KeyCache, Document creation and mappings, ...
   */
  class Store
  {
    friend class KeyCache;
    friend class Document;
    friend class VolatileDocumentAllocator;
    friend class NetDocument;
    friend class QueryDocument;
    friend class XProcessorModule;
  protected:
    /**
     * Accessor to myself
     */
    INLINE Store& getStore() { return (Store&) *this; }
    
    /**
     * Keys Handling
     *
     * Local key Cache is Store agnostic, except for Key persistence
     */
    KeyCache keyCache;

    /**
     * Key persistence : add a key in the store.
     * @param keyName the local part of the key
     * @return a new LocalKeyId corresponding to the key
     */
    virtual LocalKeyId addKeyInStore ( const char* keyName ) = 0;

    /**
     * Namespace persistence : add a namespace to the store.
     * @param namespaceURL the URL of the namespace
     * @return a new NamespaceId for it.
     */
    virtual NamespaceId addNamespaceInStore ( const char* namespaceURL ) = 0;

    /**
     * Element Indexing
     * Free elementIds are reserved per-revision to reduce access to SuperBlock
     */ 
    virtual bool reserveElementIds ( ElementId& nextId, ElementId& lastId ) = 0;

#ifdef __XEM_STORE_VOLATILEAREAS_USE_MMAP
    /**
     * File Descriptor providing volatileDocument maps.
     */
    int volatileAreasFD;
#endif
    
    /**
     * List of mmap()ed memory regions for use with volatile documents
     */
    std::list<void*> volatileAreas;

    /** 
     * Lock to protect volatileDocumentAreas
     */
    Mutex volatileAreasMutex;
    
    /**
     * Provide a temporary memory region, mapped from /dev/zero
     * @return a ready-to-use memory region, sized Document::AreaSize.
     */
    virtual void* getVolatileArea ();
    
    /**
     * Release a temporary memory region, unmmap() it or reycle it for future use.
     * @param area a memory region as provided by getVolatileDocumentArea().
     */
    virtual void releaseVolatileArea ( void* area );

    /**
     * Default constructor
     */
    Store ();
    
    /**
     * Single stub for KeyCache::getBuiltinKeys()
     */
    bool buildKeyCacheBuiltinKeys (); 

    /**
     * List of stored modules
     */
    XProcessorLibs* xprocessorLibs;

    /**
     * Service Manager
     */
    ServiceManager* serviceManager;

#if 0
    /**
     * DomEventMap
     */
    DomEventMap domEventMap;
#endif

  public:
    
    /**
     * Default destructor
     */
    virtual ~Store ();

    /*
     * Key Handling
     * @return the Store's KeyCache.
     */
    INLINE KeyCache& getKeyCache() { return keyCache; }

    /*
     * get the BranchManager
     */
    virtual BranchManager& getBranchManager() = 0;

    /**
     * Create a volatile context.
     * @return an empty Document with only a root element defined.
     */
    VolatileDocument* createVolatileDocument ();

    /**
     * Create a volatile context.
     * @return an empty Document with only a root element defined.
     */
    VolatileDocument* createVolatileDocument ( DocumentAllocator& allocator );

    /**
     * Release a document
     */
    void releaseDocument ( Document* doc );

    /**
     * XProcessorLibs Accessor
     */
    XProcessorLibs& getXProcessorLibs ();

    /**
     * ServiceManager Accessor
     */
    ServiceManager& getServiceManager ();
    
#if 0
    /**
     * DomEventMap Accessor
     */
    DomEventMap& getDomEventMap ();
#endif

    /**
     * Stats contains various runtime usage statistics for the Store.
     */
    class Stats
    {
    public:
      Stats();
      ~Stats();
      __ui64 numberOfXPathParsed;
      __ui64 numberOfXPathInstanciated;      
      __ui64 numberOfStaticXPathParsed;
      __ui64 numberOfVolatileAreasProvided;
      __ui64 numberOfVolatileAreasCreated;
      __ui64 numberOfVolatileAreasDeleted;

      void showStats ();
    };

    /**
     * Stats instance
     */
    Stats stats;

    /**
     * Try to cleanup Store
     */
    virtual void housewife ();
  };
};


#endif // __XEM_KERN_STORE_H

