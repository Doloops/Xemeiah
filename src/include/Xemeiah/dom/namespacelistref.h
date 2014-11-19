/*
 * qnamelist.h
 *
 *  Created on: 9 nov. 2009
 *      Author: francois
 */

#ifndef __XEM_DOM_NAMESPACELIST_H
#define __XEM_DOM_NAMESPACELIST_H

#include <Xemeiah/dom/integermapref.h>
#include <Xemeiah/dom/elementref.h>

namespace Xem
{
  /**
   * QName List (dirty hack as an IntegerMapRef)
   */
  class NamespaceListRef : public IntegerMapRef
  {
  public:
    NamespaceListRef ( const AttributeRef& attributeRef )
    : IntegerMapRef(attributeRef) {}

    ~NamespaceListRef () {}

    static NamespaceListRef createNamespaceListRef(ElementRef& elementRef, KeyId keyId )
    {
      return elementRef.addSKMap(keyId,SKMapType_NamespaceList);
    }

    static NamespaceListRef findNamespaceListRef(ElementRef& elementRef, KeyId keyId )
    {
      return elementRef.findAttr(keyId,AttributeType_NamespaceList);
    }

    class iterator : public SKMapRef::iterator
    {
    public:
      iterator(NamespaceListRef& NamespaceListRef)
      : SKMapRef::iterator(NamespaceListRef) {}

      ~iterator () {}

      NamespaceId getNamespaceId()
      {
        return (NamespaceId) getHash();
      }
    };

    void put ( NamespaceId nsId )
    {
      IntegerMapRef::put ( (Integer) nsId, (Integer) nsId );
    }

    bool has ( NamespaceId nsId )
    {
      return ( IntegerMapRef::get((Integer) nsId) != (Integer)0 );
    }

    bool empty ()
    {
      if ( getAttributePtr() == NullPtr ) return true;
      return getHead() == NULL;
    }
  };

};

#endif // __XEM_DOM_NAMESPACELISTREF_H
