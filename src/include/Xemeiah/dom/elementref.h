#ifndef __XEM_DOM_ELEMENT_H
#define __XEM_DOM_ELEMENT_H

#include <Xemeiah/kern/exception.h>
#include <Xemeiah/kern/document.h>
#include <Xemeiah/dom/noderef.h>
#include <Xemeiah/dom/string.h>
#include <Xemeiah/kern/format/dom.h>
#include <Xemeiah/kern/format/domeventtype.h>
#include <Xemeiah/kern/format/skmap.h>

// #define __XEM_ELEMENTREF_CACHE //< Option : put the resolved in-mem pointer in cache

// #define __XEM_DOM_ELEMENTREF_EVENTS_WITHOUT_XPROCESSOR //< Deprecated interface, will die

namespace Xem
{
#define __doesElementRefHasAttributesAndChildren(__flag) \
  ((__flag) & ElementFlag_HasAttributesAndChildren)

#define __checkElementFlag_HasAttributesAndChildren(__flag) \
  AssertBug ( __doesElementRefHasAttributesAndChildren(__flag), "Invalid flag %x\n", __flag )


  class AttributeRef;
  class SKMapRef;
  class SKMultiMapRef;
  class QNameListRef;
  class NamespaceListRef;
  class BlobRef;
  class XPath;
  class NodeFlow;
  class XProcessor;
  class BufferedWriter;

  XemStdException ( AttributeTooLongException );

  /**
   * ElementRef : Reference to an element in the Store.
   *
   * @see NodeRef
   */
  class ElementRef : public NodeRef
  {
    friend class Document;
    friend class DocumentAllocator; // Because we have to allow him to fetch our ElementPtr.
    friend class SubDocument; // We must allow SubDocument to get our ElementPtr.
    friend class PersistentDocument; // Journal applying mechanism needs to access to protected functions
    friend class SKMapRef;
    friend class XPath;
    friend class ElementMapRef;
    friend class ElementMultiMapRef;
    friend class NodeRef; // This is for a stupid hack about operator(bool)
    friend class DescendantIterator; // This is for a stupid hack about operator(bool)
    friend ElementRef ElementRefConstructor ( Document& doc, ElementPtr eltPtr );
  protected:
    /**
     * Internal unused function to force template instanciation.
     */
    void __foo__();

  public:
    /**
     * Relative pointer accessor
     * @return the relative pointer of the element
     */
    INLINE ElementPtr getElementPtr() const;

  protected:
    /**
     * Relative pointer setter
     * @parem the relative pointer of the element
     */
    INLINE void setElementPtr ( ElementPtr ePtr );
   
#ifdef __XEM_ELEMENTREF_CACHE
    ElementSegment* __me_cache;
#endif // __XEM_ELEMENTREF_CACHE

    /**
     * Access to the Element segment
     * @return a writable segment if how=Write, a read-only one otherwise
     */
    template<PageCredentials how> INLINE ElementSegment* getMe() __FORCE_INLINE;
    
    /**
     * getSegment() stub to Document
     * @param ptr the relative address of a segment to fetch
     * @return a T-classed writable segment (if how=Write), a read-only one otherwise
     */
    template<typename T, PageCredentials how> INLINE T* getSegment ( SegmentPtr& ptr )
    { return getDocumentAllocator().getSegment<T,how> ( ptr ); }

    /**
     * Protected constructor from a Document and a relative pointer
     * @param document the document to use
     * @param __ptr the relative pointer of the element
     */
    INLINE ElementRef ( Document& document, SegmentPtr __ptr );
    
    /**
     * Set flag
     */
    INLINE void setFlag ( ElementFlag flag, bool value );
    
    /**
     *
     */
    INLINE bool hasFlag ( ElementFlag flag );
    
    INLINE KeyId __getNS ( KeyId keyId );
  public:
    /*
     * *************************************** CONSTRUCTORS *************************************
     */
    /**
     * Zero-Constructor from a Document
     * The resulting ElementRef has a zero ptr, and evals to false in bool operator()
     */
    INLINE ElementRef ( Document& document );

    /**
     * Copy constructor.
     * The resulting ElementRef has the same Context and ptr as the brother.
     * @param brother the ElementRef to copy.
     */
    INLINE ElementRef ( const ElementRef& brother );

    /**
     * Copy constructor from an ElementRef pointer.
     * \deprecated use ElementRef::ElementRef ( const ElementRef& brother ) instead.
     */
    INLINE ElementRef ( ElementRef* brother );

    /**
     * Constructor from an attribute ; the resulting ElementRef is the element that holds this attribute
     * @param attribute the attribute to construct from
     */
    INLINE ElementRef ( const AttributeRef& attribute );

    /**
     * Constructor from a node.
     * If the node is an Element, the behavior is similar to ElementRef ( const ElementRef& brother ).
     * If the node is an attribute, the behavior is similar to ElementRef ( const AttributeRef& attribute );
     */
    INLINE ElementRef ( NodeRef& node );

    /**
     * ElementRef destructor ; ElementRef destructor has just nothing to do.
     */
    INLINE ~ElementRef();

    ItemType getItemType() const { return Item::Type_Element; }
    bool isElement() const { return true; }
    bool isAttribute() const { return false; }
    ElementRef& toElement() { return *this; }
    AttributeRef& toAttribute() { Bug ( "Node is not an attribute !\n" ); return AttributeRefNull; }

    /**
     * Create a copy of myself
     * @return a instanciated ElementRef as a copy of myself
     */
    NodeRef* copy() { return new ElementRef(*this); }

    /**
     * Return myself as element (virtual function from NodeRef)
     * @return a stack-instanciated copy 
     */
    ElementRef getElement() { return ElementRef(*this); }

    /**
     * Assignment operator.
     * Both ElementRef must have the same Document assigned.
     */
    INLINE ElementRef& operator= ( const ElementRef& eRef );

    /*
     * *************************************** MEMBER ACCESSORS *************************************
     */
    /**
     * Get the AllocationProfile for this node.
     */
    INLINE AllocationProfile getAllocationProfile ();
    
    /**
     * Get the unique Id (ElementId) of the element
     * @return the unique Id of the element
     */
    INLINE ElementId getElementId() { return getMe<Read>()->id; }

    /**
     * Get the current KeyId of the element
     * @return the current KeyId of the element
     */
    INLINE KeyId getKeyId();
    
    /**
     * Get the current NamespaceId of the element (zero if none)
     * @return the current NamespaceId of the element (zero if none)
     */
    INLINE NamespaceId getNamespaceId();
    
    /**
     * Try to build a string representation of the KeyId, using ancestors namespace declarations for ns prefix construction
     * @return a string represention of the current keyId
     */
    INLINE String getKey();
    
    /**
     * Builds a String representation of the unique identifier
     * @return this String representation
     */
    String generateId();

    /**
     * Builds a xpath-style unique identifier for this element, from the root element of the document
     * @return this String representation
     */
    String generateVersatileXPath (); 

    /**
     * Is this element a Text node
     * @return true if the current element is a text node (ElementType=ET_Text, xemint:textnode keyword)
     */
    INLINE bool isText();
    
    /**
     * Is this element a Comment node
     * @return true if the current element is a comment node (ElementType=ET_Comment, xemint:comment keyId)
     */
    INLINE bool isComment();
    
    /**
     * Is this element a Processing Instruction (PI) node
     * @return true if the current element is a PI node (ElementType=ET_PI, the keyId is the PI name)
     */
    INLINE bool isPI();
       
    /**
     * Is this element a regular element (ie not text(), comment() or pi())
     */   
    INLINE bool isRegularElement();
     
    /**
     * Retrieves text contents for isText(), isComment() and isPI() nodes
     * @return the text contents
     */
    INLINE String getText();

    /**
     * Retrieves the PI name for isPI(), PI name is stored as a namespaceless keyId
     * @return the String representation of the PI name
     */
    INLINE String getPIName();
    
    /**
     * Assign contents to this text, comment or PI node
     * param s the contents to set
     */
    INLINE void setText ( const char* contents, DomTextSize textSize );

    /**
     * Assign contents to this text, but with no textSize provided
     */
    INLINE void setText ( const char* contents );    
    
    /**
     * Set disable output escaping flag
     */
    INLINE void setDisableOutputEscaping ( bool escaping = true ); 
    
    /**
     * Get disable output escaping flag
     */
    INLINE bool getDisableOutputEscaping(); 

    /**
     * Set wrap cdata flag
     */
    INLINE void setWrapCData ( bool wrap = true );

    /**
     * Get wrap cdata flag
     */
    INLINE bool getWrapCData ();

    /**
     * Assign contents to this text, comment or PI node (this element shall be isText(), isComment() or isPI())
     * @param s the contents to set
     */
    inline void setText ( const String& s ) { setText ( s.c_str() ); }
    
    /**
     * Append contents to this text, comment or PI node (this element shall be isText(), isComment() or isPI()) 
     * @param s the contents to append
     */
    void appendText ( const String& );

    /**
     * Is this element full of whitespaces
     * @return true if the current element is a text node with whitespaces only
     */
    INLINE bool isWhitespace();
    
    /**
     * Must XPath skip this whitespace-only text node (as a post-parser implementation for xsl:strip-space)
     * @return true if XPath shall skip this element
     */
    bool mustSkipWhitespace ( XProcessor& xproc );

    /**
     * Position retrieving : get current position in parent's child list
     * @return the position of the current node
     */
    Integer getPosition();

    /**
     * Position retrieving : get last position of parent's child list
     * @return the last position of the youngest element in brotherhood
     */
    Integer getLastPosition();

    /**
     * Gets the number of this element's children
     * @return the number of children
     */
    Integer getNumberOfChildren();

    /**
     * String convertor for Elements
     */
    String toString ();

    /*
     * ******************************* COMPARISON OPERATORS ********************************
     */
    /**
     * Comparison operators.
     * If both documents are different, the comparison will return false.
     * If not, comparison is done on the Element pointer.
     * @see ptr
     */
    INLINE bool operator== ( const ElementRef& eRef );

    /**
     * Comparison operator. Uses operator== ().
     */
    INLINE bool operator!= ( const ElementRef& eRef )
    { return ! ( (*this) == eRef ); }
    
    /**
     * Existence operator.
     * @return true if the ElementRef has a valid ptr, false if not.
     */
    INLINE operator bool() const;
     
    INLINE bool toBool() { return (bool)(*this); }
     
    /**
     * Document-order comparison operator
     * @param eRef another node to compare
     * @return true if current node is before eRef in document order, false otherwise.
     */
    bool isBeforeInDocumentOrder ( NodeRef& eRef );

    /*
     * ******************************* GENERIC ATTRIBUTES ********************************
     */
    /**
     * Attribute list accessor
     * @return the first attribute in the attribute list
     */
    AttributeRef getFirstAttr();

    /**
     * Check if we have an attribute named keyId
     * @param keyId the keyId to search
     * @return true if the element has an attribute with this keyId 
     */
    INLINE bool hasAttr ( KeyId keyId );

    /**
     * Check if we have an attribute named keyId, with the same namespace that the element
     * @param keyId the keyId to search
     * @return true if the element has an attribute with this keyId 
     */
    bool hasAttrNS ( KeyId keyId ) { return hasAttr ( __getNS ( keyId ) ); }

    /**
     * @param keyId the keyId to search
     * @param type the attribute type to search
     * @return true if the element has an attribute with this keyId and type
     */
    bool hasAttr ( KeyId keyId, AttributeType type );
    
    /**
     * @param keyId the keyId to search
     * @param type the attribute type to search
     * @return the first attribute with this keyId and type, or a false() AttributeRef if none is found
     */
    INLINE AttributeRef findAttr ( KeyId keyId, AttributeType type = AttributeType_String ) __FORCE_INLINE;

    /**
     * Add an already-allocated AttributeRef to the attributes list of the element
     * @param attr the AttributeRef to add.
     * \note the AttributeRef must not have been added to another ElementRef before....
     */
    INLINE void addAttr ( AttributeRef& attr );
    
    /**
     * Add an attribute to the element.
     * @param keyId the keyId of the element (fully qualified, if no namespace is provided, the attribute has no namespace).
     * @param type the AttributeType_* type (as defined in <Xemeiah/kern/format/core_dom.h>
     * @param size the *PAYLOAD* of the attribute, that is the data associated with it.
     * @return a reference to the newly created attribute.
     */
    AttributeRef addAttr ( KeyId keyId, AttributeType type, DomTextSize size );
    
    /**
     * Add a string attribute
     * @param keyId the fully qualified keyId of this attribute
     * @param value the string value to set
     * @return the newly created attribute
     */
    AttributeRef addAttr ( KeyId keyId, const String& value );

    /**
     * Add a string attribute
     * @param xproc the XProcessor to use to trigger event
     * @param keyId the fully qualified keyId of this attribute
     * @param value the string value to set
     * @return the newly created attribute
     */
    AttributeRef addAttr ( XProcessor& xproc, KeyId keyId, const String& value );

    /**
     * Add a string attribute
     * @param keyId the fully qualified keyId of this attribute
     * @param value the string value to set
     * @return the newly created attribute
     */
    AttributeRef addAttr ( KeyId keyId, const char* value );

    /**
     * Add Attr as a number
     */
    AttributeRef addAttrAsNumber ( KeyId keyId, Number number );

    /**
     * Add Attr as an integer
     */
    AttributeRef addAttrAsInteger ( KeyId keyId, Integer i, AttributeType type = AttributeType_Integer );

    /**
     * Add Attr as ElementId
     */
    AttributeRef addAttrAsElementId ( KeyId keyId, ElementId i );

    /**
     * Get a string attribute
     * @param keyId the keyId to search for
     * @return the String content of the attribute, throws an exception if none is found
     */
    String getAttr ( KeyId keyId );

    /**
     * Get a string attribute, with namespace control of the element/attribute
     * @param keyId the keyId to search for ; attribute or element must have the namespace provided
     * @return the String content of the attribute, throws an exception if none is found
     */
    String getAttrNS ( KeyId keyId ) { return getAttr ( __getNS ( keyId ) ); }

    /**
     * \deprecated or so I think. Typed-attribute interface must be rethought.
     */
    AttributeSegment* getUntypedAttr ( KeyId keyId, AttributeType type ) DEPRECATED;

    /**
     * Get an attribute as Number
     * @param keyId the keyId to search for
     * @return the Number content of the attribute, throws an exception if none is found
     */
    Number getAttrAsNumber ( KeyId keyId );

    /**
     * Get an attribute as Integer
     */
    Integer getAttrAsInteger ( KeyId keyId );

    /**
     * Get an attribute as ElementId
     */
    ElementId getAttrAsElementId ( KeyId keyId );

    /**
     * Get an attribute as Number
     * @param keyId the keyId to search for
     * @return the Number content of the attribute, throws an exception if none is found
     */
    Number getAttrAsNumber ( KeyId keyId, Number fallback ) DEPRECATED;


    /*
     * ******************************* NAMESPACE ATTRIBUTES ********************************
     */

    /**
     * Add a namespace aliasing attribute (xmlns: or xmlns:prefix)
     * @param prefixId the LocalKeyId prefix
     * @param namespaceId the namespace Id to associate
     * @return the created attribute
     */
    AttributeRef addNamespacePrefix ( LocalKeyId prefixId, NamespaceId namespaceId );

    /**
     * Add a namespace aliasing attribute
     * @param keyId the qname of the attribute (xmlns: or xmlns:prefix form)
     * @param namespaceId the namespace Id to associate
     * @return the created attribute
     */
    AttributeRef addNamespaceAlias ( KeyId keyId, NamespaceId namespaceId );

    /**
     * Add a namespace aliasing attribute
     * @param prefix : the prefix to add, indicate null prefix for xmlns default setting
     * @param namespaceURL the namespace URL to associate
     * @return the create attribute
     */
    AttributeRef addNamespaceAlias ( const String& prefix, const String& namespaceURL );

    /**
     * Find a namespace prefix declaration in the DOM.
     * @param keyId the namespace declaration keyId (xmlns:(namespace) or xmlns).
     * @param recursive shall dig the whole ElementRef ancestors for this declaration
     * @return the target NamespaceId of the declaration, 0 if none found.
     */
    NamespaceId getNamespaceAlias ( KeyId keyId, bool recursive = true );

    /**
     * Find a namespace from a prefix declared in the DOM
     */
    NamespaceId getNamespaceIdFromPrefix ( LocalKeyId prefixId, bool recursive = true );

    /**
     * Get the default namespace applicable for this element
     */
    NamespaceId getDefaultNamespaceId ( bool recursive = true );

    /**
     * Find the last prefix defined for a given NamespaceId
     * @param nsId the NamespaceId to find for
     * @param recursive dig the whole ElementRef ancestors for this declaration
     * @return the last prefix for this namespace, 0 if none found.
     */
    LocalKeyId getNamespacePrefix ( NamespaceId nsId, bool recursive = true );

    /**
     * Generate a new Namespace Prefix for this NamespaceId
     * @param nsId the NamespaceId to generate a prefix for
     * @return the prefix generated, or the existing one if we already had one
     */
    LocalKeyId generateNamespacePrefix ( NamespaceId nsId );

    /**
     * Does this element have a namespace alias declaration ?
     * @return true if this element has a namespace declaration, false if not.
     */
    bool hasNamespaceAliases ();

    /**
     * Copy all namespace declarations existing for an element
     */
    void copyNamespaceAliases ( ElementRef& source, bool recursive = true );

    /*
     * ******************************* QNAMES AND NAMESPACES LIST ATTRIBUTES ********************************
     */

    /**
     * Add Attr as KeyId
     */
    AttributeRef addAttrAsKeyId ( KeyId keyId, KeyId valueKeyId );

    /**
     * Add Attr as a QName
     * @param keyId the keyId of the attribute
     * @param qnameId the QName keyId
     * @return the newly created QName
     */
    AttributeRef addAttrAsQName ( KeyId keyId, KeyId qnameId );

    /**
     * Returns an attribute contents, considered as a QName, and parsed as a KeyId.
     * The attribute content must not be an AVT.
     * @param attrKeyId the attribute KeyId to parse.
     * @return the KeyId corresponding to the attribute content, considered as a QName.
     */
    KeyId getAttrAsKeyId ( KeyId attrKeyId );

    /**
     * Returns an attribute contents, considered as a QName, and parsed as a KeyId.
     * The attribute content may be an AVT.
     * @param xproc the XProcessor to use for XPath processing if the attr is an AVT.
     * @param attrKeyId the attribute KeyId to parse.
     * @return the KeyId corresponding to the attribute content, considered as a QName.
     */
    KeyId getAttrAsKeyId ( XProcessor& xproc, KeyId attrKeyId );

    /**
     * Returns an attribute as a list of QName Ids
     * @param attrKeyId the attribute KeyId to parse.
     * @return the list of QNames
     */
    KeyIdList getAttrAsKeyIdList ( KeyId attrKeyId, bool useDefaultNamespace = true );

    /**
     * Returns an attribute as a QNameListRef list of QName Ids
     * @param attrKeyId the attribute Keyid (wether as String or as direct QNameListRef)
     */
    QNameListRef getAttrAsQNameList ( KeyId attrKeyId );

    /**
     * Adds a QName to a QNameListRef
     */
    void addQNameInQNameList ( KeyId attrKeyId, KeyId qnameId );

    /**
     * Returns an attribute as a QNameListRef list of QName Ids
     * @param namespaceListKeyId the attribute KeyId (NamespaceListRef attribute)
     * @param sourceKeyId the attribute KeyId (String attribute)
     * @return the NamespaceListRef built
     */
    NamespaceListRef getAttrAsNamespaceList ( KeyId namespaceListKeyId, AttributeRef& sourceAttr );

    /**
     * Adds a QName to a QNameListRef
     * @param namespaceListKeyId the attribute KeyId (NamespaceListRef attribute)
     * @param sourceKeyId the attribute KeyId (String attribute)
     * @param nsId the NamespaceId to add
     */
    void addNamespaceInNamespaceList ( KeyId namespaceListKeyId, KeyId source, NamespaceId nsId );

    /*
     * ******************************* SKMAP ATTRIBUTES ********************************
     */
  protected:
    /**
     * Add a SKMap (associative Map) to the element
     * @param keyId the key name of the attribute to create
     * @param config the SKMapConfig to use
     * @param mapType the SKMapType to use
     * @return the created attribute
     */
    AttributeRef addSKMap ( KeyId keyId, SKMapConfig& config, SKMapType mapType );

  public:

    /**
     * Add a SKMap (associative Map) to the element
     * @param keyId the key name of the attribute to create
     * @param mapType the SKMapType to use
     * @return the created attribute
     */
    AttributeRef addSKMap ( KeyId keyId, SKMapType mapType );

    /*
     * ******************************* BLOB ATTRIBUTES ********************************
     */

    /**
     * Add a Blob Attribute to the Element (implemented in blobref.cpp)
     * @param blobNameId the name of the Blob attribute
     * @return the instanciated blob
     */
    BlobRef addBlob ( KeyId blobNameId );


    /*
     * ******************************* AVT ATTRIBUTES ********************************
     */

    /**
     * Embedded XPath AVT attribute evaluation, for param="text[{xpath}text]*" forms of attributes
     * @param xproc the XProcessor to use (for variables, functions, ...)
     * @param keyId the attribute keyId to use
     * @return the evaled String.
     */
    String getEvaledAttr ( XProcessor& xproc, KeyId keyId );

    /**
     * Embedded XPath AVT attribute evaluation, with namespace checking
     * @param xproc the XProcessor to use (for variables, functions, ...)
     * @param keyId the attribute keyId to use
     * @return the evaled String.
     */
    inline String getEvaledAttrNS ( XProcessor& xproc, KeyId keyId )
    { return getEvaledAttr ( xproc, __getNS ( keyId ) ); }

    /*
     * ******************************* DELETE ATTRIBUTES ********************************
     */

    /**
     * Deletes an attribute
     * @param keyId the keyId of the attribute to delete
     * @attrType keyId the type of the attribute to delete
     * @return true upon success
     */
    bool deleteAttr ( KeyId keyId, AttributeType attrType );
    
    /**
     * Deletes all attributes with a given keyId
     * @param keyId the keyId of the attributes to delete
     * @return true upon success
     */
    bool deleteAttr ( KeyId keyId );

    /**
     * Delete all attributes
     */
    void deleteAttributes ( XProcessor& xproc );

    /*
     * *************************** ELEMENTS HIERARCHY ACCESSORS *****************************************
     */
    /**
     * Father accessor
     * @return father ElementRef, or a zero ElementRef if orphan or root
     */
    ElementRef getFather();
    
    /**
     * First child accessor
     * @return first child ElementRef, or a zero ElementRef if no child
     */
    INLINE ElementRef getChild();

    /**
     * Last child accessor
     * @return last child ElementRef, or a zero ElementRef if no child
     */
    ElementRef getLastChild();
    
    /**
     * Preceding element accessor
     * @return preceding ElementRef, or a zero ElementRef if no preceding
     */
    ElementRef getElder();
    
    /**
     * Following element accessor
     * @return following ElementRef, or a zero ElementRef if no following
     */
    ElementRef getYounger();

    /*
     * *************************** ELEMENT MODIFIERS *****************************************
     */
    /**
     * Rename the element.
     * This function may not crash, so take care of providing a valid newKeyId.
     * @param newKeyId the new KeyId to use.
     */
    void rename ( KeyId newKeyId );

    /*
     * Insert already-allocated (but supposed-to-be not linked) elements
     */
    void insertBefore ( ElementRef& elementRef );
    void insertAfter ( ElementRef& elementRef, bool updateJournal = true );
    void insertChild ( ElementRef& elementRef, bool updateJournal = true );
    void appendLastChild ( ElementRef& elementRef );
    
    // Function aliases
    void appendChild ( ElementRef& elementRef ) { appendLastChild ( elementRef ); }

    /**
     * Copy an attribute from another Element (and optionaly from another Document)
     */
    AttributeRef copyAttribute ( AttributeRef& attrRef );

    /**
     * Recursive content copy.
     * The whole contents of eRef will be copied (attributes and children) to this node.
     * @param eRef the ElementRef to copy attributes and children from.
     */
    void copyContents ( XProcessor& xproc, ElementRef& eRef, bool onlyBaseAttributes = true );

    /**
     * Deletes this element
     */
    void deleteElement () DEPRECATED;

    /**
     * Detach an element out of his father
     */
    void unlinkElementFromFather ();

    /**
     * Deletes this element, with a calling XProcessor
     */
    void deleteElement ( XProcessor& xproc );

    /**
     * Deletes all element children, with a calling XProcessor
     */
    void deleteChildren ( XProcessor& xproc );


    /*
     * *************************** META-INDEXING *****************************************
     */
    ElementRef lookup ( XProcessor& xproc, KeyId functionId, const String& value );

    /*
     * *************************** NOTIFICATION EVENTS *****************************************
     */

    /**
     * Event : New Attribute from a called XProcessor
     */
    void eventAttribute ( XProcessor& xproc, DomEventType domEventType, AttributeRef& attrRef );

    /**
     * Event : New Element from a called XProcessor
     */
    void eventElement ( XProcessor& xproc, DomEventType domEventType );

#ifdef __XEM_DOM_ELEMENTREF_EVENTS_WITHOUT_XPROCESSOR
    /**
     * Event : New Attribute.
     * @param attrRef the attribute added to this element
     */
    void eventNewAttribute ( AttributeRef& attrRef ) DEPRECATED;
    
    /**
     * Event : New Element (when parsing is finished for this element and all its children) - context-free
     */
    void eventNewElement ( ) DEPRECATED;
#endif // __XEM_DOM_ELEMENTREF_EVENTS_WITHOUT_XPROCESSOR

    /*
     * *************************** SERIALIZATION *****************************************
     */
    /**
     * Serialize to a FILE*
     */
    void serialize ( FILE* fp );

    /**
     * Serialize ElementRef to a NodeFlow
     * @nodeFlow the NodeFlow to serialize to.
     */
    void serialize ( NodeFlow& nodeFlow );

    /**
     * Serialize Namespace Aliases
     * @nodeFlow the NodeFlow to serialize to.
     */
    void serializeNamespaceAliases ( NodeFlow& nodeFlow, bool anticipated = true );

    /**
     * Really ensure that the given namespace is available and properly declared in the nodeFlow
     * If it is not declared in the NodeFlow, a new namespace alias will be serialized, declaring this nsId.
     * @param nodeFlow the NodeFlow to serialize to.
     * @param nsId the NamespaceId we have to control.
     * @param anticipated Defines if the optional alias to create is anticipated (for element) or not (for attributes)
     * @param prefixId the prefered prefixId to use if we have to create an Id
     */
    void ensureNamespaceDeclaration ( NodeFlow& nodeFlow, NamespaceId nsId, bool anticipated, LocalKeyId prefixId = 0 );

    /*
     * *************************** DEPRECATED toXML() SERIALIZATION *****************************************
     */

    /**
     * The flags for element serialization.
     */
    typedef int ToXMLFlags;
    /**
     * Write the <?xml version="1.0" encoding="UTF-8" ?> stuff at head.
     * Default version is 1.0, encoding is UTF-8 if non provided
     */
    static const ToXMLFlags Flag_XMLHeader = 0x01;
    /**
     * Do not write to the socket, just compute the number of bytes
     * that should be written.
     */
    static const ToXMLFlags Flag_NoWrite = 0x02;
    /**
     * Only write children, do not write the calling ElementRef.
     */
    static const ToXMLFlags Flag_ChildrenOnly = 0x04;
	
    /**
     * If set, will add an attribute called elementId on each serialized element node,
     * which contains the elementId of the element.
     */
    static const ToXMLFlags Flag_ShowElementId = 0x08;
    
    /**
     * Do not indent the resulting XML (minimize space).
     */
    static const ToXMLFlags Flag_NoIndent = 0x10;

    /**
     * Write XHTML Doctype
     */
    static const ToXMLFlags Flag_XHTMLDocType = 0x20;

	  /**
	   * Sort : namespaces and attributes are serialized sorted by their name.
	   */
	  static const ToXMLFlags Flag_SortAttributesAndNamespaces = 0x40;

    /**
     * Output - HTML : don't protect special markups
     */
	  static const ToXMLFlags Flag_Output_HTML = 0x80;

    /**
     * Write text : don't protect text
     */
	  static const ToXMLFlags Flag_UnprotectText = 0x100;

    /**
     * Write XML : put the (stupid) standalone marker
     */
	  static const ToXMLFlags Flag_Standalone  = 0x200;
	  
    /**
     * Serialization functions (writing to XML).
     * @param fd the descriptor/socket to write to.
     * @param flags the flags to use for writing.
     * @param encoding
     * @return the number of bytes written.
     */
    void toXML ( BufferedWriter& writer, ToXMLFlags flags, const String& encoding );
    void toXML ( int fd, ToXMLFlags flags );
    void toXML ( FILE* fp, ToXMLFlags flags );
    void toXML ( BufferedWriter& writer );
  };

  inline ElementRef ElementRefConstructor ( Document& doc, ElementPtr eltPtr )
  {
    return ElementRef(doc,eltPtr);
  }
};

#endif // __XEM_DOM_ELEMENT_H

