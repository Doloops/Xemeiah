#ifndef __XEM_NODEFLOW_H
#define __XEM_NODEFLOW_H

#include <Xemeiah/kern/format/core_types.h>
#include <Xemeiah/kern/keycache.h>

#define __XEM_NODEFLOW_HAS_CDATA_SECTION_ELEMENTS

#ifdef __XEM_NODEFLOW_HAS_CDATA_SECTION_ELEMENTS
#include <map>
#endif

namespace Xem
{
  class Env;
  class XProcessor;
  class ElementRef;
  class XPath;

  /**
   * NodeFlow is a simple interface for Tree creation, with a SAX-type event-based API. 
   * It may be used for Serialization (stream generation of an XML) or direct DOM creation.
   */
  class NodeFlow
  {
    friend class XProcessor; //< for setPreviousNodeFlow()
  protected:
    /**
     * Reference to the XProcessor we have been instanciated for
     */
    XProcessor& xprocessor;

    /**
     * A quick reference to our KeyCache
     */
    KeyCache& keyCache;

    /**
     * The previous NodeFlow defined in Env, each nodeflow being responsible of restoring previous nodeflow in Env after destruction.
     */
    NodeFlow* previousNodeFlow;
    
    /**
     * Set whether we are bound to our XProcessor or not
     */
    bool isBoundToXProcessor;
    
    /**
     * Bind this NodeFlow to our XProcessor
     */
    void setPreviousNodeFlow ( NodeFlow* previousNodeFlow );

    /**
     * Access to our keyCache
     */
    INLINE KeyCache& getKeyCache () const { return keyCache; }
    
    /**
     * Protected (partial) constructor for NodeFlow
     */
    NodeFlow ( XProcessor& xproc );

    /**
     * Is output format explicitly set by setOutputFormat() ?
     */
    bool isOutputFormatExplicit;

    /**
     * Output Methods
     */
    enum OutputMethod
    {
      OutputMethod_XML,
      OutputMethod_HTML,
      OutputMethod_Text,
    };

    /**
     * Current method as set by setOutputFormat() (default is xml)
     */
    OutputMethod outputMethod;

    /**
     * Set encoding
     */
    String encoding;

    /**
     * Set indent
     */
    bool indent;

    /**
     * Set standalone
     */
    bool standalone;

    /**
     * Set omitXMLDeclaration
     */
    bool omitXMLDeclaration;

    /**
     * DocTypes set
     */
    String docTypePublic, docTypeSystem;
    
#ifdef __XEM_NODEFLOW_HAS_CDATA_SECTION_ELEMENTS    
    std::map<KeyId,bool> cdataSectionElements;
#endif
  public:
  
    /**
     * Destructor
     */
    virtual ~NodeFlow ();

    /**
     * Access to our XProcessor
     */
    inline XProcessor& getXProcessor() const {  return xprocessor; }
  
    /**
     * Set output format
     * @param method the declared output method : 'xml', 'html', 'text'.
     * @param encoding the declared encoding
     * @param indent set to true for indentation
     * @param standalone useless option, only adds a standalone="true" in the xml declaration
     * @param omitXMLDeclaration set to true to avoid writing the <?xml ... ?> xml declaration.
     */
    virtual void setOutputFormat ( const String& method, const String& encoding, bool indent, bool standalone, bool omitXMLDeclaration );
    
    /**
     * Requalify output
     */
    virtual void requalifyOutput ( KeyId rootKeyId );

    /**
     * Set output doctype
     * @param docTypePublic the PUBLIC doctype (or empty string for none)
     * @param docTypeSystem the SYSTEM doctype (or empty string for none)
     */
    virtual void setDocType ( const String& _docTypePublic, const String& _docTypeSystem )
    { docTypePublic = _docTypePublic; docTypeSystem = _docTypeSystem; }

    
    /**
     * Event : new namespace alias attribute.
     * @param keyId The keyId of the namespace mapping, must take the form 'xmlns:(namespace)' or 'xmlns'.
     * @param nsId the mapped NamespaceId.
     * @param anticipated true if the namespacePrefix is provided before the element, false if it is provided after.
     */
    virtual void setNamespacePrefix ( KeyId keyId, NamespaceId nsId, bool anticipated ) = 0;
    
    /**
     * Remove an existing namespace prefix aliasing
     */
    virtual void removeAnticipatedNamespacePrefix ( KeyId keyId ) {}
    
    /**
     *
     */
    virtual void removeAnticipatedNamespaceId ( NamespaceId nsId ) {}

    /**
     * Get the active namespace prefixing
     */
    virtual LocalKeyId getNamespacePrefix ( NamespaceId namespaceId ) = 0;

    /**
     * Get the active namespace binding to the prefix
     */
    virtual NamespaceId getNamespaceIdFromPrefix ( LocalKeyId prefix, bool recursive ) = 0;

    /**
     * Get the active namespace binding to the prefix
     */
    virtual NamespaceId getNamespaceIdFromPrefix ( LocalKeyId prefix )
    { return getNamespaceIdFromPrefix(prefix, true); }

    /**
     * Check that a given prefix is safe for an attribute namespace serialization
     * @param prefixId a given prefix
     * @param nsId the namespace to map to
     * @return true if the attribute is safe (may be added)
     */
    virtual bool isSafeAttributePrefix ( LocalKeyId prefixId, NamespaceId nsId )
    { return false; }

    /**
     * Get the active default namespace prefixing
     */
    virtual NamespaceId getDefaultNamespaceId () = 0;


    /**
     * Verify that the given namespace has a valid prefixing
     * If no prefix has been declared, a fake one (on the nsXX format) will be created
     */
    virtual void verifyNamespacePrefix ( NamespaceId nsId ); 
     
    /**
     * Event : new Element. This element must be a real element(), 
     * i.e. not a comment(), text(), pi() or xemint:root.
     * @param keyId the fully-qualified KeyId of this element.
     * @param forceDefaultNamespace set to true to set a xmlns='namespace' attribute. (deprecated).
     *   forceDefaultNamespace is only valid for the element created, and sub-elements will have a zeroed xmlns set.
     */
    virtual void newElement ( KeyId keyId, bool forceDefaultNamespace ) = 0;

    /**
     * Event : new Element. This element must be a real element(), 
     * i.e. not a comment(), text(), pi() or xemint:root.
     * @param keyId the fully-qualified KeyId of this element.
     */
    void newElement ( KeyId keyId ) { newElement ( keyId, false ); }
    
    /**
     * Event : new Attribute.
     * @param keyId the fully-qualified KeyId of this attribute.
     * @param value the value of the attribute.
     */
    virtual void newAttribute ( KeyId keyId, const String& value ) = 0;

    /**
     * Event : Append a text to the last created element.
     * @param text the text to append.
     * @param disableOutputEscaping set to true to disable output escaping.
     */
    virtual void appendText ( const String& text, bool disableOutputEscaping ) = 0;

    /**
     * Event : end of Element. Must match the keyId provided by the 
     * corresponding call to newElement().
     * @param keyId the fully-qualified KeyId of this element.
     */
    virtual void elementEnd ( KeyId keyId ) = 0;    

    /**
     * Event : new Comment.
     * @param comment the comment to set.
     */
    virtual void newComment ( const String& comment ) = 0;
    
    /**
     * Event : new Processing Instruction
     * @param name the name of the Processing instruction
     * @param contents its contents
     */
    virtual void newPI ( const String& name, const String& contents ) = 0;

    /**
     * Restrict output to text-only (for example, NodeFlowTextContents restricts to text-only nodes).
     * @return true if the caller shall restrict himself to text-only contents.
     */
    bool isRestrictToText () { return outputMethod == OutputMethod_Text; }

    /**
     * Get the last ElementRef we have been creating.
     * Only valid if the target implementation of NodeFlow is a DOM creation.
     * Invalid if the target implementation of NodeFlow is a Stream creation.
     * @return the last ElementRef created.
     */
    virtual ElementRef getCurrentElement() = 0;
    
    /**
     * Process XPath for a given nodeset, and serialize it (called by xsl:sequence, for example)
     */
    virtual void processSequence ( XPath& xpath );

#ifdef __XEM_NODEFLOW_HAS_CDATA_SECTION_ELEMENTS    
    /**
     * Register a CData section element
     */
    void setCDataSectionElement ( KeyId keyId );
    
    /**
     * Check if this keyId refers to a CData section element
     */
    bool isCDataSectionElement ( KeyId keyId );
#endif // __XEM_NODEFLOW_HAS_CDATA_SECTION_ELEMENTS    

  };
};


#endif // __XEM_NODEFLOW_H
