/*
 * qnamelist.h
 *
 *  Created on: 9 nov. 2009
 *      Author: francois
 */

#ifndef __XEM_DOM_QNAMELIST_H
#define __XEM_DOM_QNAMELIST_H

#include <Xemeiah/dom/integermapref.h>
#include <Xemeiah/dom/elementref.h>

namespace Xem
{
  /**
   * QName List (dirty hack as an IntegerMapRef)
   */
  class QNameListRef : public IntegerMapRef
  {
  public:
    QNameListRef ( const AttributeRef& attributeRef )
    : IntegerMapRef(attributeRef) {}

    ~QNameListRef () {}

    static QNameListRef createQNameListRef(ElementRef& elementRef, KeyId keyId )
    {
      return elementRef.addSKMap(keyId,SKMapType_QNameList);
    }

    static QNameListRef findQNameListRef(ElementRef& elementRef, KeyId keyId )
    {
      return elementRef.findAttr(keyId,AttributeType_QNameList);
    }

    class iterator : public SKMapRef::iterator
    {
    public:
      iterator(QNameListRef& QNameListRef)
      : SKMapRef::iterator(QNameListRef) {}

      ~iterator () {}

      KeyId getKeyId()
      {
        return (KeyId) getHash();
      }
    };

    void put ( KeyId keyId )
    {
      IntegerMapRef::put ( (Integer) keyId, (Integer) keyId );
    }

    bool has ( KeyId keyId )
    {
      return ( IntegerMapRef::get((Integer) keyId) != (Integer)0 );
    }

    bool empty ()
    {
      if ( getAttributePtr() == NullPtr ) return true;
      return getHead() == NULL;
    }
  };

};

#endif // __XEM_DOM_QNAMELISTREF_H
