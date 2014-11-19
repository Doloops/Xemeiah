#ifdef __XEM_PROVIDE_INLINE

#include <Xemeiah/kern/store.h>
#include <Xemeiah/kern/document.h>
#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/dom/domeventmask.h>
#include <Xemeiah/dom/attributeref.h>
#include <Xemeiah/dom/skmapref.h>
#include <Xemeiah/dom/blobref.h>
#include <Xemeiah/xpath/xpath.h>

#ifdef __XEM_USE_INLINE
#define __INLINE inline
#include "../kern/utf8.hpp"
#include "../kern/qnamemap.hpp"
#include "../kern/keycache.hpp"
#include "../kern/document.hpp"
#include "../kern/document-protect.hpp"
#include "../kern/documentallocator.hpp"
#include "../kern/documentallocator-protect.hpp"
#include "../kern/documentallocator-segment.hpp"
#include "../kern/documentallocator-fixedsize.hpp"
#undef __INLINE
#endif

#define __INLINE
#include "domeventmask.hpp"
#include "noderef.hpp"
#include "elementref.hpp"
#include "attributeref.hpp"
#include "nodeset.hpp"
#include "skmapref.hpp"
#include "skmultimapref.hpp"

namespace Xem
{
  void ElementRef::__foo__ ()
  {
    getMe<Read> ();
    getMe<Write> ();
  }

  void AttributeRef::__foo__ ()
  {
    getMe<Read> ();
    getMe<Write> ();

    getData<void,Read> ();
    getData<void,Write> ();
    
    getData<char,Read> ();
    getData<char,Write> ();

    getData<SKMapHeader,Read> ();
    getData<SKMapHeader,Write> ();
    getData<BlobHeader,Read> ();
    getData<BlobHeader,Write> ();
    
    getData<XPathSegment,Read> ();
    getData<XPathSegment,Write> ();

    getData<NamespaceId,Read> ();
    getData<NamespaceId,Write> ();
    
    getData<Integer,Read> ();
    getData<Integer,Write> ();    
  }

  void SKMapRef::__foo__ ()
  {
    getItem<Read> ( NullPtr );
    getItem<Write> ( NullPtr );
  
  }
  
  void SKMultiMapRef::multi_iterator::__foo__ ()
  {
    getCurrentList<Read> ();
    getCurrentList<Write> ();
  }
};
#endif // __XEM_PROVIDE_INLINE

