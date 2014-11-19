#if 0 // DEPRECATED

#include <Xemeiah/kern/subdocument.h>
#include <Xemeiah/dom/elementref.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_SubDoc Log

namespace Xem
{
  SubDocument::SubDocument ( ElementRef& foreignRoot )
  : Document ( foreignRoot.getDocument() ),
    fatherDocument ( foreignRoot.getDocument() )
  {
    Log_SubDoc ( "Created new SubDoc at %p (from doc %p) store=%p <- %p\n"
      "rev %p <- %p\n"
      "areasAlloced %p <- %p\n"
      "areasMapped %p <- %p\n"
      "areas %p <- %p\n"
#ifdef __XEM_DOCUMENT_HAS_ISWRITABLEPAGECACHE      
      "isWritablePageCacheSz %p <- %p\n"
      "isWritablePageCache %p <- %p\n"
#endif // __XEM_DOCUMENT_HAS_ISWRITABLEPAGECACHE
    , this, &(foreignRoot.getDocument() ),
    &(getStore()), &(foreignRoot.getDocument().getStore()),
    getRev(), foreignRoot.getDocument().getRev(),
    &areasAlloced, &foreignRoot.getDocument().areasAlloced,
    &areasMapped, &foreignRoot.getDocument().areasMapped,
    &areas, &foreignRoot.getDocument().areas
#ifdef __XEM_DOCUMENT_HAS_ISWRITABLEPAGECACHE      
    , &isWritablePageCacheSz, &foreignRoot.getDocument().isWritablePageCacheSz,
    &isWritablePageCache, &foreignRoot.getDocument().isWritablePageCache 
#endif // __XEM_DOCUMENT_HAS_ISWRITABLEPAGECACHE
    );
    
    rootElementPtr = foreignRoot.getElementPtr ();
    // mustUnmapAreasAtDestruction = false;
    
    if ( foreignRoot == foreignRoot.getDocument().getRootElement() )
      getDocumentTag() = foreignRoot.getDocument().getDocumentTag();
    else
      getDocumentTag() = foreignRoot.generateVersatileXPath();
  }
  
  SubDocument::~SubDocument ()
  {
    Log_SubDoc ( "Deleting subDoc at '%p'\n", this );
  }
};
#endif
