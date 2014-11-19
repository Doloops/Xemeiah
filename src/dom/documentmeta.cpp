/*
 * documentmeta.cpp
 *
 *  Created on: 9 nov. 2009
 *      Author: francois
 */

#include <Xemeiah/dom/documentmeta.h>
#include <Xemeiah/dom/childiterator.h>
#include <Xemeiah/dom/attributeref.h>
#include <Xemeiah/dom/elementmapref.h>
#include <Xemeiah/xprocessor/xprocessor.h>

#include <Xemeiah/auto-inline.hpp>

#define __xemint getKeyCache().getBuiltinKeys().xemint

namespace Xem
{
  DocumentMeta::DocumentMeta ( const ElementRef& element )
  : ElementRef(element)
  {
    AssertBug ( *this, "Element not defined !\n" );
    AssertBug ( getKeyId() == __xemint.document_meta(), "Element is not a document meta !" );

  }

  DomEvents DocumentMeta::getDomEvents ()
  {
    for ( ChildIterator child(*this) ; child ; child++ )
      {
        if ( child.getKeyId() == __xemint.dom_events() )
          return child;
      }
    ElementRef events(getDocument());
    if ( getDocument().isLockedWrite() )
      {
        events = getDocument().createElement(*this,__xemint.dom_events());
        insertChild(events);
      }
    return events;
  }
};
