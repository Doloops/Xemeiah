#ifdef __XEM_PROVIDE_INLINE

#include <Xemeiah/kern/qnamemap.h>
#include <Xemeiah/kern/store.h>
#include <Xemeiah/kern/document.h>
#include <Xemeiah/dom/documentmeta.h>

#ifdef __XEM_USE_INLINE
#define __INLINE inline
#include "../dom/elementref.hpp"
#include "../dom/attributeref.hpp"
#endif

#ifndef __XEM_USE_INLINE
#define __XEM_USE_INLINE
#endif
#undef __INLINE
#define __INLINE

#include "mutex.hpp"
#include "utf8.hpp"
#include "qnamemap.hpp"
#include "keycache.hpp"
#include "document.hpp"
#include "document-protect.hpp"
#include "documentallocator.hpp"
#include "documentallocator-protect.hpp"
#include "documentallocator-segment.hpp"
#include "documentallocator-fixedsize.hpp"
// #include "env.hpp"

namespace Xem
{
  void Document::__foo__ ()
  {
#ifdef __XEM_DOCUMENT_STUBS_DOCUMENTALLOCATOR
    getElement<Read> ( 0 );
    getElement<Write> ( 0 );
    getSegment<void,Read> ( 0, 0 );
    getSegment<void,Write> ( 0, 0 );
    getSegment<char,Read> ( 0, 0 );
    getSegment<char,Write> ( 0, 0 );
    getSegment<unsigned int,Read> ( 0, 0 );
    getSegment<unsigned int,Write> ( 0, 0 );
    getSegment<ElementSegment,Read> ( 0 );
    getSegment<ElementSegment,Write> ( 0 );
    getSegment<AttributeSegment,Read> ( 0 );
    getSegment<AttributeSegment,Write> ( 0 );
    getSegment<JournalItem,Read> ( 0 );
    getSegment<JournalItem,Write> ( 0 );
    getSegment<SKMapList,Read> ( 0 );        
    getSegment<SKMapList,Write> ( 0 );        
    getSegment<SKMapItem,Read> ( 0, 0 );        
    getSegment<SKMapItem,Write> ( 0, 0 );        
#endif
  }
  
  void DocumentAllocator::__foo__ ()
  {
    getSegment<DocumentHead,Read> ( 0, 0 );
    getSegment<DocumentHead,Write> ( 0, 0 );
    getSegment<FreeSegment,Read> ( 0, 0 );
    getSegment<FreeSegment,Write> ( 0, 0 );
    getSegment<void,Read> ( 0, 0 );
    getSegment<void,Write> ( 0, 0 );
    getSegment<char,Read> ( 0, 0 );
    getSegment<char,Write> ( 0, 0 );
    getSegment<unsigned int,Read> ( 0, 0 );
    getSegment<unsigned int,Write> ( 0, 0 );
    getSegment<__ui64,Read> ( 0, 0 );
    getSegment<__ui64,Write> ( 0, 0 );
    getSegment<ElementSegment,Read> ( 0 );
    getSegment<ElementSegment,Write> ( 0 );
    getSegment<AttributeSegment,Read> ( 0 );
    getSegment<AttributeSegment,Write> ( 0 );
    getSegment<JournalItem,Read> ( 0 );
    getSegment<JournalItem,Write> ( 0 );
    getSegment<SKMapList,Read> ( 0 );        
    getSegment<SKMapList,Write> ( 0 );        
    getSegment<SKMapItem,Read> ( 0, 0 );        
    getSegment<SKMapItem,Write> ( 0, 0 );        
    getSegment<SKMapHeader,Read> ( 0, 0 );
    getSegment<SKMapHeader,Write> ( 0, 0 );
    getSegment<BlobHeader,Read> ( 0, 0 );
    getSegment<BlobHeader,Write> ( 0, 0 );
  }
};

#endif
