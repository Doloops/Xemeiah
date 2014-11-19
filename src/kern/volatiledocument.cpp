#include <Xemeiah/kern/volatiledocument.h>
#include <Xemeiah/kern/volatiledocumentallocator.h>

#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/dom/skmapref.h>
#include <Xemeiah/trace.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_VolDocument Debug

namespace Xem
{
  VolatileDocument* Store::createVolatileDocument ()
  {
    VolatileDocumentAllocator* volatileDocumentAllocator = new VolatileDocumentAllocator (*this);

    VolatileDocument* document = createVolatileDocument ( *volatileDocumentAllocator );
    
    document->bindDocumentAllocator ( volatileDocumentAllocator );
    
    return document;
  }

  VolatileDocument* Store::createVolatileDocument ( DocumentAllocator& documentAllocator )
  {
    VolatileDocument* document = new VolatileDocument ( *this, documentAllocator );
    
    Log_VolDocument ( "New volatile document at %p\n", document );
    
    document->createRootElement();
    return document;
  }


  VolatileDocument::VolatileDocument ( Store& __s, DocumentAllocator& allocator )
    : TransactionlessDocument(__s, allocator)
  {
    Log_VolDocument ( "Creating Volatile Document at %p...\n", this );
    isIndexed = false;

    refCount = 0;

    /*
     * Allocate a new HeadPtr for the documentHead
     */
    documentHeadPtr = getDocumentAllocator().getFreeSegmentPtr ( sizeof(DocumentHead), 0 );
  
    Log_VolDocument ( "documentHeadPtr = %llx\n", documentHeadPtr );
    /*
     * Now, initialize documentHead
     */
    alterDocumentHead ();

    getDocumentHead().rootElementPtr = getDocumentHead().metaElementPtr = NullPtr;
    
    getDocumentHead().firstReservedElementId = 0;
    getDocumentHead().lastReservedElementId = 0;
    getDocumentHead().elements = 0;

    protectDocumentHead ();
            
    Log_VolDocument ( "Creating Volatile Document... OK, at %p.\n", this );
  }

  bool VolatileDocument::mayIndex ()
  {
    /*
     * TODO Think more about forcing a document to be indexed : may delegate to Store
     */
    return true;
  }

  VolatileDocument::~VolatileDocument ()
  {
    Log_VolDocument ( "[VolatileDocument DESTRUCTOR] this=%p\n", this );
  }

  void VolatileDocument::releaseDocumentResources ()
  {
    Log_VolDocument ( "[VolatileDocument releaseDocumentResources] this=%p, refCount=%llu\n", this, refCount );
  }
};
