#ifndef __XEM_DOM_ELEMENTREF_CHILDITERATOR_H
#define __XEM_DOM_ELEMENTREF_CHILDITERATOR_H

#include <Xemeiah/dom/elementref.h>

namespace Xem
{
  /** 
   * Optimized child iterator for element ref
   */
  class ChildIterator : public ElementRef
  {
  protected:
    /**
     * Forbidden constructor
     */
    INLINE ChildIterator ( const ElementRef& father )
    : ElementRef ( father )
    {
      Bug ( "ChildIterator shall not be called with a const father ?" );
    }


    /**
     * FORBIDDEN ChildIterator constructor : SHALL NOT BE CALLED
     * Use an explicit cast to ElementRef instead
     * @param father the element to iterate upon
     */
    ChildIterator ( const ChildIterator& father );

  public:
    /**
     * ChildIterator constructor
     * @param father the element to iterate upon
     */
    INLINE ChildIterator ( ElementRef& father )
    : ElementRef ( father.getChild() )
    {

    }

    /**
     * Jump to the next child
     */
    INLINE ChildIterator& operator++(int u)
    {
      ElementSegment* me = getMe<Read> ();
      setElementPtr ( me->youngerPtr );
      return *this;
    }
  
  };
};

#endif // #ifndef __XEM_DOM_ELEMENTREF_CHILDITERATOR_H
