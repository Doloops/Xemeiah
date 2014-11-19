#ifndef __XEM_PARSER_SAXHANDLER_DOM_H
#define __XEM_PARSER_SAXHANDLER_DOM_H

#include <Xemeiah/parser/saxhandler.h>
#include <Xemeiah/kern/namespacealias.h>
#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/dom/attributeref.h>

#include <list>

namespace Xem
{
  /**
   * Parsing : Event Handler (SAX-type), creating nodes on Document Object Model.
   *
   * The following rules and constraints apply on EventHandlerDom :
   * -# In charge of checking that XML is well-formed (openned and closed markups match, ..).
   * -# In charge of resolving markup names to KeyIds
   * -# In charge of handling namespace declaration,s including default namespace declarations (xmlns attribute).
   * -# Create elements as soon as possible. It is preferable to create a new element and rename it afterwards.
   * -# Element and attribute KeyId can be changed if a xmlns:(namespace) or xmlns attribute is provided.
   * -# No element or attribute KeyId is stable between eventElement() and eventAttributeEnd().
   * -# Be carefull, attributes are not unique wrt their local name only. This may hurt when we can not resolve the prefix yet.
   * -# ElementRef controls attribute unicity (wrt KeyId and AttributeType), but silently replaces existing attributes.
   */
  class XProcessor;
  class SAXHandlerDom : public SAXHandler
  {
  protected:
    /**
     * Our calling XProcessor
     */
    XProcessor& xproc;

    /**
     * Accessor to the KeyCache
     */
    inline KeyCache& getKeyCache();
    
    /**
     * Our root and current elements
     */    
    ElementRef rootElement, currentElement;
    
    /**
     * The flags for keeping text or not
     */
    enum KeepTextMode
      {
        KeepTextMode_All,
        KeepTextMode_XSL,
        KeepTextMode_None      
      };
      
    /**
     * Indicates the text mode to keep.
     * \todo reshape this. We may use the XPath (ElementRef::mustSkipWhitespace(Env& env)) stuff to check
     */
    KeepTextMode keepTextMode;

    /**
     * The QName KeyId of a xsl:text element, used if keepTextMode == xslTextKeyId
     */
    KeyId xslTextKeyId;

    /**
     * Defines wether or not keep text at root element (only valid for partial parsing)
     */
    bool keepTextAtRootElement;

    /**
     * The NamespaceAlias to use while parsing.
     * The namespaceAlias is built using the xmlns:(namespace) and xmlns attributes.
     */
    NamespaceAlias namespaceAlias;
    
    /**
     * The prefixId of the element, as declared by the parser.
     * The value is set in eventElement(), and clear in eventAttributeEnd().
     */
    LocalKeyId elementPrefixId;
    
    /**
     * Indicates that element namespace lookup was deferred in eventElement().
     */
    bool elementNamespaceWasDeferred;
    
    /**
     * List of parsed attributes, to rename attributes when xmlns:* attributes arise
     */  
    struct ParsedAttribute
    {
      /**
       * The prefix of the attribute, as sent by the parser
       */
      LocalKeyId prefixId;

      /**
       * The local part of the attribute, as sent by the parser
       */
      LocalKeyId localKeyId;

      /**
       * A pointer to the attribute created
       */
      AttributePtr attrPtr;
    };

    /**
     * Array of parsed attributes
     */
    ParsedAttribute* parsedAttributes;
    
    /**
     * Number of parsed attributes created
     */
    __ui32 parsedAttributes_alloced;
    
    /**
     * Total number of parsed attributes alloced
     */
    __ui32 parsedAttributes_number;
    
    /**
     * Get a parsed attribute from the ParsedAttribute array
     */
    ParsedAttribute* getNewParsedAttribute();
    
    /**
     * Put current DOM position in exception
     * @param e the exception to dump DOM position to
     */
    void dumpContext ( Exception* e );
  public:
    /**
     * SAXHandlerDom constructor
     * @param xproc the XProcessor to use for DomEvent triggering
     * @param rootElement the root element to add nodes to
     */
    SAXHandlerDom ( XProcessor& xproc, ElementRef& rootElement );

    /**
     * Set option : keep text mode
     * @param mode the keep text mode : 'all' to keep all text, 'xsl' to restrict text to XSL, 'normal' for any XML normal file
     */
    bool setKeepTextMode ( const String& mode );

    /**
     * Set option : keep non-space text at root element
     */
    bool setKeepTextAtRootElement ( bool value );

    /**
     * SAX Handler destructor
     */
    ~SAXHandlerDom ();

    /*
     * SAX Event callbacks, see SAXHandler for further information on these
     */
    virtual void eventElement ( const char* ns, const char *name );
    virtual void eventAttr ( const char* ns, const char *name, const char *value );
    virtual void eventAttrEnd ();
    virtual void eventElementEnd ( const char* ns, const char *name );
    virtual void eventText ( const char *text );
    virtual void eventComment ( const char *comment );
    virtual void eventProcessingInstruction ( const char * name, const char* content );
    virtual void eventEntity ( const char* entityName, const char* entityValue );
    virtual void eventNDataEntity ( const char* entityName, const char* ndata );
    virtual void eventDoctypeMarkupDecl ( const char* markupName, const char* value );
    virtual void parsingFinished ();
  };
};

#endif // __XEM_PARSER_SAXHANDLER_DOM_H
