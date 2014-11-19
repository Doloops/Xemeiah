#ifndef __XEM_DOM_NODEREF_H
#define __XEM_DOM_NODEREF_H

#include <Xemeiah/kern/document.h>
#include <Xemeiah/kern/exception.h>

#include <Xemeiah/dom/item.h>

#define __XEM_DOM_NODEREF_OPERATOR_BOOL_HACK
#define __XEM_DOM_NODEREF_GETKEYID_HACK

// #define __XEM_DOM_NODEREF_CHROOT_USING_XEMINT_ROOT


namespace Xem
{
  class ElementRef;
  class AttributeRef;
  class NamespaceAlias;

  XemStdException ( DOMException );
#define throwDOMException(...) throwException ( DOMException, __VA_ARGS__ )

  /**
   * NodeRef : A node (Element or Attribute) reference.
   * At creation time, each NodeRef is associated to a Context,
   * and they may not change contexts over time.
   */
  class NodeRef : public Item
  {
  protected:
    /**
     * The reference to our strongly-linked document
     */
    Document& document;
    
    /**
     * The pointer to our in-memory location
     */
    SegmentPtr nodePtr;
    
    /**
     * Internal constructor (only ElementRef and AttributeRef are allowed to instanciate use)
     */
    NodeRef ( Document& document );

  public:
    /**
     * Virtual destructor
     */
    virtual ~NodeRef ();

    /**
     * Our document accessor
     */
    Document& getDocument() const { return document; } 

    /**
     * Our document allocator accessor
     */
    DocumentAllocator& getDocumentAllocator() const { return document.getDocumentAllocator(); } 

    
    
    /**
     * Stub to access our Store
     */
    INLINE Store& getStore() const { return document.store; }
    
    /**
     * Stub to access our KeyCache
     */
    INLINE KeyCache& getKeyCache() const { return getStore().getKeyCache(); }

    /**
     * Root document element accessor
     * @return the root Element of the document we are bound to (may not return a zero ElementRef)
     */
    ElementRef getRootElement ();

    /**
     * Root document element comparator
     * @return true if I am the root element of the document, false otherwise
     */
    INLINE bool isRootElement ();

    virtual ItemType getItemType() const = 0;
    virtual bool isElement() const = 0;
    virtual bool isAttribute() const = 0;
    virtual ElementRef& toElement() = 0;
    virtual AttributeRef& toAttribute() = 0;

    bool isNode() const { return true; }
    NodeRef& toNode() { return *this; }

    virtual ElementRef getElement() = 0;

    virtual NodeRef* copy() = 0;

    
#ifdef LOG
    void log();
#else
    void log() {}
#endif

    virtual String generateId() = 0;
    virtual String generateVersatileXPath () = 0; 

    virtual String toString() = 0;

    virtual String getKey() = 0;

#ifdef __XEM_DOM_NODEREF_GETKEYID_HACK
    INLINE KeyId getKeyId();
#else    
    virtual KeyId getKeyId() = 0;
#endif // __XEM_DOM_NODEREF_GETKEYID_HACK

    virtual NamespaceId getNamespaceId() = 0;

    virtual LocalKeyId getNamespacePrefix ( NamespaceId nsId, bool recursive ) = 0;

#ifdef __XEM_DOM_NODEREF_OPERATOR_BOOL_HACK  
    INLINE operator bool() const;    
#else
    virtual operator bool() const = 0;
#endif    
    
    virtual bool isBeforeInDocumentOrder ( NodeRef& eRef ) = 0;

    INLINE bool operator== ( NodeRef& nodeRef );
    
    const char* __getKey ();
    
    /**
     * Check if a string is a valid ElementId (as provided by generateId())
     * @param eltId the string ElementId to check
     * @return true if the eltId looks like a valid ElementId, false otherwise
     */
    static bool isValidNodeId( const String& nodeId );

    /**
     * Parse a given string format Node identifier : role, ElementId and attributeKeyId
     */
    static bool parseNodeId ( const String& strNodeId, String& role, ElementId& elementId, KeyId& attributeKeyId );
  };
};

#endif // __XEM_DOM_NODEREF_H
