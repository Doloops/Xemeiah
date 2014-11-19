#ifndef __XEM_KERN_FORMAT_CORE_TYPES_H
#error Shall include <Xemeiah/kern/format/core_types.h> first !
#endif

#ifndef __XEM_KERN_FORMAT_DOM_H
#define __XEM_KERN_FORMAT_DOM_H
namespace Xem
{
  /**
   * In the ElementSegment, the ElementFlag is stored in the same 64 bit word
   * as the KeyId. So ElementFlag must remain a __ui32 value.
   * ElementFlag is a 32-bit value, splitted like this :
   * 0x ff ff ff ff
   * 31-15 : Unused
   * 15-8 : Flags
   * - 15 : policy Element : this Element has policy information we shall compute (when altering one of its descendants)
   * - 14 : callback Element : this Element has callback information, we shall compute as well (/!\). 
   * - 13 : isWhitespace : the Element is a text() node (isText() is true), and contains only spaces in its contents.
   * - 12 : hasNamespaceAliases : the Element is an element(), and contains AttributeType_NamespaceAlias attributes.
   * - 01 : HasAttributesAndChildren
   */
  typedef __ui32 ElementFlag;
  static const ElementFlag ElementFlag_HasAttributesAndChildren = 0x01;
  static const ElementFlag ElementFlag_HasTextualContents       = 0x02;
  static const ElementFlag ElementFlag_DisableOutputEscaping    = 0x04;
  static const ElementFlag ElementFlag_WrapCData                = 0x08;
  static const ElementFlag ElementFlag_HasNamespaceAlias        = 0x10;

  /**
   * Element in-store format.
   * This structure is never exposed as an API,
   * but it is accessible through ElementRef.
   * \todo study the opportunity to reduce ElementSegment size : do not store ElementId (shall be a on-demand attribute), but may hurt when Context is RO.
   * \todo study the opportunity to reduce ElementSegment size for text(), processing-instruction() and comment() nodes (what for ?)
   * -# do not store descending ptrs and attribute ptr
   * -# store the text size (8 bytes), 
   * -# use the remaining 16 bytes to store beginning of element
   * -# for processing-instruction() nodes, store the PI name as a LocalKeyId (16bytes) in the ElementFlag
   */
  struct ElementAttributesAndChildren
  {
    /* Descending pointers */
    ElementPtr childPtr;       // 8
    ElementPtr lastChildPtr;   // 16

    /* Attribute pointer */
    AttributePtr attrPtr;      // 24  
  }; // 3*8 = 24bytes
  
  struct ElementTextualContents
  {
    static const __ui64 shortFormatSize = 16; // sizeof(ElementAttributesAndChildren) - sizeof(DomTextSize);
    union 
    {
      SegmentPtr contentsPtr;          // 8
      char contents[shortFormatSize];  // 16  
    };
    DomTextSize size;                  // 4
  };
   
  struct ElementSegment
  {
    KeyId keyId;             //  4
    ElementFlag flags;       //  8
    ElementId id;            // 16

    /* Surrounding pointers */
    ElementPtr fatherPtr;    // 24
    ElementPtr elderPtr;     // 32
    ElementPtr youngerPtr;   // 40

    union 
    {
      ElementAttributesAndChildren attributesAndChildren;
      ElementTextualContents textualContents;
    };

  }; // 4+4 + 4 * 8 + max(24,?) = 64bytes


  /**
   * AttributeFlag contains both AttributeType_ and AttributeFlag_ information.
   */
  typedef __ui32 AttributeFlag;

   /**
   * Attribute unicity is true for (KeyId,AttributeType) couples.
   * This allows alternate formats (multiple AttributeTypes) 
   * for a given KeyId.
   */
  typedef AttributeFlag AttributeType;
  static const AttributeType AttributeType_String         = 0x01;
  static const AttributeType AttributeType_NamespaceAlias = 0x02;
  static const AttributeType AttributeType_Number         = 0x03;
  static const AttributeType AttributeType_Integer        = 0x04;
  static const AttributeType AttributeType_BaseMask       = 0x0f;
  
  static const AttributeType AttributeType_SKMap          = 0x10;
  static const AttributeType AttributeType_XPath          = 0x20;
  static const AttributeType AttributeType_KeyId          = 0x30;

  static const AttributeType AttributeType_Mask           = 0xff;

  static const AttributeFlag AttributeFlag_IsWhitespace   = 0x0100;
  static const AttributeFlag AttributeFlag_IsAVT          = 0x0200;
    
  static const AttributeFlag AttributeFlag_PropertiesMask = 0xff00;

#define __getAttributeType(__flag ) ( __flag & AttributeType_Mask )
#define __isAttributeBaseType(__flag) ( __flag & AttributeType_BaseMask )
  
#define AttributeType_Blob          AttributeType_SKMap   //< Alias for Blob
#define AttributeType_QNameList     AttributeType_SKMap   //< Alias for QNameList
#define AttributeType_NamespaceList AttributeType_SKMap   //< Alias for NamespaceList
#define AttributeType_ElementId     AttributeType_Integer //< Alias for ElementId

  /**
   * Attribute in-store format.
   * \note This structure is never exposed as an API, but its data is accessible through AttributeRef and its derivates.
   * Attribute constraints and concepts
   * - Attribute lookup must be fast, esp. when finding a particular KeyId.
   * - Attributes are never a lot : we can assert that there will never be more 
   *   than 10 attributes in an element. This may not lock the design up to 10 
   *   attributes, but allows us not to implement a complex mapping mechanism.
   * - Attributes must be fast to create, alter or delete.
   * - Attribute alteration may imply a deletion and re-creation, when the alloced
   *   size is smaller than what the new content requires.
   * - Attributes have alternate (speed-up) formats, derived from the base format.
   * - The base formats are one of the standard xs: formats : 
   *    - String, 
   *    - Integer, 
   *    - Decimal, 
   *    - Bool,
   *    - NamespaceAlias, i.e. 'xmlns:(namespace)' or 'xmlns' attributes.
   * - The non-base formats (complex formats) are :
   *    - Internal pre-parsed XPath,
   *    - SKMapRef and SKMultiMapRef contents.
   *
   * - We only want to expose, throw the DOM API (ElementRef::getFirstAttr(), AttributeRef::getNext()),
   *   the base format attributes. Alternate formats are only accessible through ElementRef::findAttr ( KeyId, AttributeType ).
   * - Make the design take this in consideration.
   */
   struct AttributeSegment
  {
    /**
     * The attribute QName
     */
    KeyId keyId;      // 4

    /**
     * Flags for this attribute (see before)
     * The 16 upper bits of the flag is used for Attribute size, to multiply by sizeof(FreeSegment)
     * The 16 lower bits are for Type and mask
     */
    AttributeFlag flag; // 8

    /**
     * Pointer to the next attribute
     */
    SegmentPtr nextPtr; // 16
  }; // Head size : 4 + 4 + 8 = 16

#define AttributeSegment_SizeUnit 16

#define AttributeSegment_setSize(__attrSegment,__size) \
  __attrSegment->flag = (__attrSegment->flag & (AttributeType_Mask|AttributeFlag_PropertiesMask)) \
     + ((__size / AttributeSegment_SizeUnit + (__size%AttributeSegment_SizeUnit ? 1 : 0 ) ) << 16);

#define AttributeSegment_getSize(__attrSegment) \
  ((__attrSegment->flag >> 16)*AttributeSegment_SizeUnit)
};
#endif //  __XEM_KERN_FORMAT_DOM_H
