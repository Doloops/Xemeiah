#include <Xemeiah/kern/store.h>
#include <Xemeiah/kern/document.h>
#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/dom/attributeref.h>
#include <Xemeiah/dom/documentmeta.h>

#include <Xemeiah/auto-inline.hpp>

/*
 * \file Event handling of elements.
  */
 
#define Log_ElementEvent Debug

namespace Xem
{
  void ElementRef::eventElement ( XProcessor& xproc, DomEventType domEventType )
  {
    getDocument().processDomEvent ( xproc, domEventType, *this );
  }

  void ElementRef::eventAttribute ( XProcessor& xproc, DomEventType domEventType, AttributeRef& attrRef )
  {
    Log_ElementEvent ( "Event newAttribute for %x : %x\n", getKeyId(), attrRef.getKeyId() );
    AssertBug ( *this == attrRef.getElement(), "Invalid attribute, wrong element !\n" );

    getDocument().processDomEvent ( xproc, domEventType, attrRef );
#if 0
    if ( attrRef.isAVT() )
      { 
        /**
         * @todo Optimize this, we don't need to parse non-AVT strings
         */
        XPathParser xpathParser ( *this, attrRef, true );
        AttributeRef xpathAttr = xpathParser.saveToStore ( *this, attrRef.getKeyId() );
      }
#endif
#if 0
    if ( domEventType == DomEventType_CreateAttribute )
      {
        getDocument().appendJournal(*this,JournalOperation_UpdateAttribute,*this,attrRef.getKeyId());
      }
#endif
  }
};
