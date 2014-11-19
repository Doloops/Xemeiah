#ifndef __XEM_DOM_ATTRIBUTE_H
#define __XEM_DOM_ATTRIBUTE_H

#include <Xemeiah/dom/elementref.h>

namespace Xem
{
  /**
   * AttributeRef : reference to an attribute node.
   * 
   * @see NodeRef
   */
  class AttributeRef : public NodeRef
  {
    friend class Document;
    friend class PersistentDocument;
    friend class ElementRef;
    friend class SAXHandlerDom;
    friend class NodeRef;
    friend AttributeRef AttributeRefConstructor ( Document& doc, ElementPtr eltPtr, AttributePtr attrPtr );
  protected:
    /**
     * Internal unused function to force template instanciation.
     */
    void __foo__();
    
    /**
     * The relative pointer to the element that holds this attribute
     */
    ElementPtr parentElementPtr__;

    /**
     * The relative pointer to the attribute node.
     */
    // AttributePtr attributePtr;

    /**
     * returns the AttributeSegment corresponding to the attribute node.
     */
    template<PageCredentials how> INLINE AttributeSegment* getMe();

    INLINE DomTextSize getSize();

    /**
     * Internal constructor (called from ElementRef)
     */
    INLINE AttributeRef ( Document& document, ElementPtr __parentElementPtr, AttributePtr __attributePtr );

  public:
    /**
     * Internal getter for the ptr
     */
    INLINE AttributePtr getAttributePtr() const;

    /**
     * Internal getter for the element ptr
     */
    INLINE ElementPtr getParentElementPtr() const;

  protected:
    /**
     * Internal setter for the ptr
     */
    INLINE void setAttributePtr ( AttributePtr _ptr );

    /**
     * Internal setter for the element ptr
     */
    INLINE void setParentElementPtr ( ElementPtr _ePtr );

    /**
     * Constructor allocating a new attribute
     * @param document the Document used for Attribute creation
     * @param allocProfile the allocation profile to provide to the Document for segment allocation
     * @param keyId the KeyId of the attribute to create.
     * @param attributeType the type of the attribute
     * @param size the size of the attribute.
     */
    AttributeRef ( Document& document, AllocationProfile allocProfile, KeyId keyId, AttributeType attributeType, DomTextSize size );
    
    /**
     * Constructor allocating a new String attribute
     * @param document the Document used for Attribute creation
     * @param allocProfile the allocation profile to provide to the Document for segment allocation
     * @param keyId the KeyId of the attribute to create.
     * @param value the string value of the attribute.
     */
    AttributeRef ( Document& document, AllocationProfile allocProfile, KeyId keyId, const char* value );
    
    /**
     * Update AttributeFlags according to the (updated) value (String only yet)
     */
    void updateAttributeFlags ();
  public:
    /**
     * Zeroed attribute constructor
     * @param document the Document to assign to this attribute (mandatory)
     * \note As the AttributePtr is set to null, evaluation to bool of the instanciated AttributeRef will return false.
     */
    INLINE AttributeRef ( Document& document );

    /**
     * Copy constructor
     */
    INLINE AttributeRef ( const AttributeRef& __a );

    /**
     * Constructor from an ElementRef
     * The resulting attribute is set to the first attribute of the ElementRef.
     */
    INLINE AttributeRef ( ElementRef& __e );

    /**
     * Attribute destructor. Mostly anything to do here.
     */
    INLINE ~AttributeRef ( );

    /**
     * Attribute Setter
     */
    INLINE AttributeRef& operator= ( AttributePtr __ptr );
    
    /**
     * Attribute Comparison
     */
    INLINE bool operator== ( const AttributeRef& __a );
    INLINE bool operator!= ( const AttributeRef& __a )
    { return ( ! ( (*this) == __a ) ); }

    void rename ( KeyId keyId );    

    bool isBeforeInDocumentOrder ( NodeRef& eRef );

    /**
     * Create a copy of myself
     */
    NodeRef* copy () { return new AttributeRef(*this); }

    INLINE AttributeRef& operator= ( const AttributeRef& eRef );
    INLINE operator bool() const;
    
    INLINE bool toBool() { return (bool)(*this); }    

    ItemType getItemType() const { return Item::Type_Attribute; }
    bool isElement() const { return false; }
    bool isAttribute() const { return true; }
    ElementRef& toElement()  { Bug ( "Node is not an element !\n" ); return ElementRefNull; }
    AttributeRef& toAttribute() { return *this; }

    INLINE ElementRef getElement();

    INLINE KeyId getKeyId();
    INLINE NamespaceId getNamespaceId();
    INLINE LocalKeyId getLocalKeyId();
    String getKey();

    INLINE const char* getKey(NamespaceAlias& nsAlias) DEPRECATED;

    /**
     * Builds a String representation of the unique identifier
     * @return this String representation
     */
    String generateId();

    String generateVersatileXPath (); 

    INLINE AttributeType getType();
    INLINE AttributeType getAttributeType() { return getType(); }
    INLINE bool isBaseType();
    INLINE AttributeRef getNext();

    template<typename T, PageCredentials how>
      INLINE T* getData();

    INLINE void alterData ();
    INLINE void protectData ();

    INLINE String toString ();
    INLINE Integer toInteger ();
    INLINE Number toNumber ();
        
    /**
     * When the attribute is a AttributeType_NamespaceAlias, return the namespaceId.
     */
    INLINE NamespaceId getNamespaceAliasId ();

    LocalKeyId getNamespacePrefix ( NamespaceId nsId, bool recursive );

  protected:
    /**
     * Delete attribute segment
     */  
    void deleteAttributeSegment ();
  public:
    /**
     * Delete this Attribute
     */
    virtual void deleteAttribute ();

    /**
     * Flag shortcuts
     */
    INLINE bool isAVT ();
    INLINE bool isWhitespace ();

    /*
     * Modifiers
     */
    void setString ( const char* value );
    void setString ( const String& value );
  };

  inline AttributeRef AttributeRefConstructor ( Document& doc, ElementPtr eltPtr, AttributePtr attrPtr )
  {
    return AttributeRef(doc,eltPtr,attrPtr);
  }

};


#endif // __XEM_DOM_ATTRIBUTE_H
