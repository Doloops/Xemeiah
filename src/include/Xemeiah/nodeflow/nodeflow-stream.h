#ifndef __XEM_NODEFLOW_STREAM_H
#define __XEM_NODEFLOW_STREAM_H

#include <Xemeiah/dom/string.h>
#include <Xemeiah/nodeflow/nodeflow.h>
#include <Xemeiah/kern/keycache.h>
#include <Xemeiah/kern/namespacealias.h>
#include <Xemeiah/io/bufferedwriter.h>

#include <stdio.h>

#include <list>
#include <map>

namespace Xem
{
  /**
   * NodeFlowStream : transform a NodeFlow event flow to a stream representation.
   * The stream creation is based on an output buffer mechanism, which may be sent to a socket or file
   * if the setFD() functions are used.
   * If no output target is provided, the NodeFlowStream will keep all serialization in its internal buffer, which is
   * accessible through getContents(), offering the possibility to serialize to a String object.
   */
  class NodeFlowStream : public NodeFlow
  {
  protected:
    /**
     * Static protectionMap mechanism
     */
    const char** protectionMap;
    
    /**
     * The binded KeyCache, which is mandatory for LocalKeyId and NamespaceId serialization.
     */
    KeyCache& keyCache;
    
    /**
     * NamespaceAlias binding, which is mandatory for full KeyId serialization.
     * Be carefull that standard constructor does not define a default NamespaceAlias,
     * so setNamespaceAlias() must be called before any non-text event occurs.
     * (This may although change in the near future, for robustness).
     */
    NamespaceAlias namespaceAlias;

    /**
     * Buffered writer
     */
    BufferedWriter* writer;

    /**
     * Output writer : write a string (unprotected)
     */
    inline void doAddStr ( const char* str );
    
    /**
     * Output writer : write a char (unprotected)
     */
    inline void doAddChar ( char c );
    
    /**
     * Is output mode explicitly set ?
     */
    bool isOutputFormatExplicit;

    /**
     * Has already written the prolog or not
     */
    bool hasQualifiedRootElement;

    /**
     * Has written the prolog
     */
    bool hasWrittenXMLDeclaration;

    /**
     * Set to true at the beginning of writing, false after having written the very first element
     */
    bool atRootElement;

    /**
     * Force to keep all text
     */
    bool forceKeepText;

    /**
     * Current depth of indentation. If currentIndent == 0, that means we did not write any element, and we should skip text.
     */
    int currentIndentation;

    /**
     * State marker : last stuff we have written is text, so we don't have to indent or return to line when closing element.
     */
    bool lastWasText;
    
    /**
     * KeyId of the Element being written, available between newElement() and closeElementSection()
     */
    KeyId currentMarkupKeyId;

    /**
     * KeyId of the Element we are in
     */
    KeyId currentElementKeyId;

    /**
     * Anticipated namespace aliases prematurely calls NamespaceAlias.push(), 
     * such that the namespace aliases are only valid for the element to come.
     */
    bool hasAnticipatedNamespaceAliases;
    
    /**
     * Type : Out-of-element namespace declarations.
     */
    typedef std::map<KeyId,NamespaceId> AnticipatedNamespaceAliases;
    
    /**
     * Out-of-element namespace declarations.
     */
    AnticipatedNamespaceAliases anticipatedNamespaceAliases;

    /**
     * Early recording of the attribute list to write.
     */
    class AttributeToWrite
    {
    public:
      KeyId keyId;
      const char* name;
      char* value;

      AttributeToWrite ( KeyId keyId, char* _value );
      ~AttributeToWrite ();
      
    };
    
    /**
     * Type : sorted list of attributes
     */
    typedef std::list<AttributeToWrite*> AttributesList;
    
    /**
     * List of attributes to write
     */
    AttributesList attributesList;

    /**
     * Type : map of attributes to write
     */
    typedef std::map<KeyId,AttributeToWrite*> AttributesMap;

    /**
     * Map of attributes to write
     */
    AttributesMap attributesMap;
    
    /**
     * Write prolog
     */
    void writeXMLDeclaration ();

    /**
     * Add a new attribute to write
     */
    void addAttributeToWrite ( KeyId keyId, const char* value );

    /**
     * Delete an attribute to write (because it has been written)
     */
    void deleteAttributeToWrite ( AttributeToWrite* attr );

    /**
     * Delete all attributesToWrite
     */
    void deleteAttributesToWrite ();

    /**
     * Attribute Sorter Class
     */
    class AttributeSorter
    {
      public:
        AttributeSorter() {} ~AttributeSorter() {}
        
        bool operator() ( AttributeToWrite* left, AttributeToWrite* right )
        { return (strcmp ( left->name, right->name ) < 0 ); }
    };

    /**
     * Find attributes names before sorting
     */
    void setAttributesNames ();

    /**
     * Sort attributes according to their name (i.e. normalize)
     */
    void sortAttributes ( );

    /**
     * Write the element head and the attributes part of an Element serialization
     * @param elementClose directly close the element with a short format
     * @return true if an element has been closed, false otherwise
     */
    bool closeElementSection ( bool elementClose = false );
    
    /**
     * Serialize text
     */
    void serializeText ( const char* text, bool protectLTGT, bool protectQuote, bool protectAmp );

  public:
    /**
     * Constructor. 
     * @param keyCache the KeyCache to bind to.
     */
    NodeFlowStream ( XProcessor& xproc, BufferedWriter* writer = new BufferedWriter() );

    /**
     * NodeFlowStream destructor.
     * If setFile() was used to bind to a file, the buffer will be flushed, and the fidles will be closed.
     */
    virtual ~NodeFlowStream ();

    /**
     * get the NamespaceAlias defined for this NodeFlow
     * @return my namespaceAlias.
     */
    NamespaceAlias& getNamespaceAlias() { return namespaceAlias; }

    void setNamespacePrefix ( KeyId keyId, NamespaceId nsId, bool anticipated );


    NamespaceId getDefaultNamespaceId ()
    { return getNamespaceAlias().getDefaultNamespaceId (); }
    
    LocalKeyId getNamespacePrefix ( NamespaceId namespaceId )
    { return getNamespaceAlias().getNamespacePrefix ( namespaceId ); }

    NamespaceId getNamespaceIdFromPrefix ( LocalKeyId prefix, bool recursive )
    { return getNamespaceAlias().getNamespaceIdFromPrefix ( prefix ); }

    // void setOutputFormat ( const String& formatType, const String& encoding, bool indent, bool standalone, bool omitXMLDeclaration );
   
    void setForceKeepText ( bool b ) { forceKeepText = b; }

    virtual void newElement ( KeyId keyId, bool forceDefaultNamespace );
    virtual void newAttribute ( KeyId keyId, const String& value );
    virtual void elementEnd ( KeyId keyId );
    virtual void appendText ( const String& text, bool disableOutputEscaping  );
    
    virtual void newComment ( const String& comment );
    
    virtual void newPI ( const String& name, const String& contents );
    
    ElementRef getCurrentElement();
    
    /**
     * Get the serialized contents (the one that has not been flushed by flush())
     */
    const char* getContents();

    /**
     * Get serialized contents size
     */
    __ui64 getContentsSize();
    
    /**
     * Flush buffer contents to output target. If none defined by a subclass, just enlarge buffer.
     */
    virtual void flush();
  };
};


#endif // __XEM_NODEFLOW_STREAM_H
