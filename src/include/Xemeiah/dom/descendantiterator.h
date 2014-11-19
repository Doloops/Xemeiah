/*
 * descendantiterator.h
 *
 *  Created on: 22 déc. 2009
 *      Author: francois
 */

#ifndef __XEM_DOM_DESCENDANTITERATOR_H
#define __XEM_DOM_DESCENDANTITERATOR_H

#include <Xemeiah/dom/elementref.h>

namespace Xem
{
  /**
   * Optimized descendant (ie //node()) iterator for element ref
   */
  class DescendantIterator : public ElementRef
  {
  protected:
    /**
     * The father we instanciated against
     */
    ElementRef father;

    /**
     * Forbidden constructor
     */
    INLINE DescendantIterator ( const ElementRef& _father )
    : ElementRef ( _father ), father(_father)
    {
      Bug ( "DescendantIterator shall not be called with a const father ?" );
    }
  public:
    /**
     * DescendantIterator constructor
     * @param father the element to iterate upon
     */
    INLINE DescendantIterator ( ElementRef& _father )
    : ElementRef ( _father.getChild() ), father(_father)
    {

    }

    /**
     * Jump to the next child
     */
    INLINE DescendantIterator& operator++(int u)
    {
      ElementSegment* me = getMe<Read> ();
      if ( __doesElementRefHasAttributesAndChildren ( me->flags )
          && me->attributesAndChildren.childPtr )
        {
          setElementPtr ( me->attributesAndChildren.childPtr );
          return *this;
        }
      while ( ! me->youngerPtr )
        {
          if ( me->fatherPtr == father.getElementPtr() )
            {
              setElementPtr ( 0 );
              return *this;
            }
          setElementPtr ( me->fatherPtr );
          me = getMe<Read> ();
        }
      setElementPtr ( me->youngerPtr );
      return *this;
    }
  };
};

#endif /* __XEM_DOM_DESCENDANTITERATOR_H */
