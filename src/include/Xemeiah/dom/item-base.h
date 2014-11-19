#ifndef __XEM_DOM_ITEM_BASE_H
#define __XEM_DOM_ITEM_BASE_H

#include <Xemeiah/dom/item.h>
#include <Xemeiah/kern/format/core_types.h>

namespace Xem
{
  /**
   * Implementation-dependant template for Item (handles at least Number, Integer and bool types).
   */
  template<typename T>
  class ItemImpl : public Item
  {
   protected:
    T contents;
   public:
    INLINE ItemImpl ( const T& _contents ) { contents = _contents; }
    INLINE ~ItemImpl () {}

    ItemType getItemType() const;

    String toString ( );
    bool toBool ();
    Integer toInteger ();
    Number toNumber ();

    void clearContents () {}
  };

};

#endif // __XEM_DOM_ITEM_BASE_H
