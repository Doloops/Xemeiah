#ifndef __XEM_DOM_ITEM_H
#define __XEM_DOM_ITEM_H

#include <Xemeiah/kern/format/core_types.h>

#include <Xemeiah/trace.h>

namespace Xem
{
  class ElementRef;
  class AttributeRef;
  class NodeRef;
  class String;

  static const Integer IntegerInfinity = 0xffffffff;

#define NodeRefNull (*((NodeRef*)NULL))
#define ElementRefNull (*((ElementRef*)NULL))
#define AttributeRefNull (*((AttributeRef*)NULL))
  
  /**
   * The Item class represents items of a NodeSet collection.
   * Each Item can be :
   * - An Element
   * - An Attribute
   * - A boolean value
   * - An integer value
   * - A number value
   * - A string value
   */
  class Item
  {
   public:
    /**
     * Item Type of the current item.
     */
    enum ItemType
      {
        Type_Null = 0,
        Type_Element = 1,
        Type_Attribute = 2,
        Type_Bool = 3,
        Type_Integer = 4,
        Type_Number = 5,
        Type_String = 6
      };
    virtual ~Item() {}

    virtual ItemType getItemType() const = 0;
    
    virtual bool isElement() const { return false; };
    virtual bool isAttribute () const { return false; };
    virtual bool isNode() const { return false; };

    virtual ElementRef& toElement () { Bug ( "Not an element.\n" ); return ElementRefNull; }
    virtual AttributeRef& toAttribute () { Bug ( "Not an attribute.\n" ); return AttributeRefNull; }
    virtual NodeRef& toNode() { Bug ( "Not a node.\n" ); return NodeRefNull; }

    virtual String toString ( ) = 0;
    virtual bool toBool ();
    virtual Integer toInteger ();
    virtual Number toNumber ();

    virtual bool isNaN ();
    
    // Function Alias
    Integer toInt () { return toInteger(); }
    
    virtual void clearContents () {}
    
    static Number roundNumber ( Number arg );
  };
};


#endif // __XEM_DOM_ITEM_H
