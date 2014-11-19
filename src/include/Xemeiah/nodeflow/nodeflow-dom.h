#ifndef __XEM_NODEFLOW_DOM_H
#define __XEM_NODEFLOW_DOM_H

#include <Xemeiah/nodeflow/nodeflow.h>
#include <Xemeiah/dom/noderef.h>
#include <Xemeiah/dom/elementref.h>

#include <list>

namespace Xem
{
  class ElementRef;
  /**
   * NodeFlowDom : append an existing base ElementRef with NodeFlow events.
   */
  class NodeFlowDom : public NodeFlow
  {
  protected:
    /**
     * Base element we have been instanciating against
     */
    ElementRef baseElement;
  
    /**
     * Current element we are operating on
     */
    ElementRef currentElement;

    /**
     * allowAttributes means we are in a situation that fits for creating an attribute (ie inside of an element)
     */
    bool allowAttributes;
  
    /**
     * Forbid attributes creation outside of where they can be created naturally
     */
    bool forbidAttributeCreationOutsideOfElements;
    
    /**
     * KeyCache accessor
     */
    KeyCache& getKeyCache ();

    /**
     * Anticipated namespaces declared at an element for its children
     */
    typedef struct
    {
      KeyId nsDeclaration;
      NamespaceId nsId;
    } AnticipatedNamespace;
    
    /**
     * Type : List of anticipated namespaces
     */
    typedef std::list<AnticipatedNamespace> AnticipatedNamespaces;
    
    /**
     * List of anticipated namespaces
     */
    AnticipatedNamespaces anticipatedNamespaces;

    /**
     * Serialize DocType if I have one
     */
    virtual void serializeDocType ( KeyId rootKeyId );

    /**
     * Trigger DOM events or not ?
     */
    bool mustTriggerEvents () const { return true; }
  public:
    /**
     * Constructor : NodeFlowDom must be based at an ElementRef for NodeFlow events handling.
     */
    NodeFlowDom ( XProcessor& xproc, const ElementRef& elementRef );
    virtual ~NodeFlowDom ();

    /*
     * Namespace Stuff
     */
    void setNamespacePrefix ( KeyId keyId, NamespaceId nsId, bool anticipated );
    void removeAnticipatedNamespacePrefix ( KeyId keyId );
    void removeAnticipatedNamespaceId ( NamespaceId nsId );
    
    NamespaceId getDefaultNamespaceId ();
    LocalKeyId getNamespacePrefix ( NamespaceId namespaceId );
    NamespaceId getNamespaceIdFromPrefix ( LocalKeyId prefix, bool recursive );

    virtual bool isSafeAttributePrefix ( LocalKeyId prefixId, NamespaceId nsId );
  
    virtual void newElement ( KeyId keyId, bool forceDefaultNamespace );
    void newElement ( KeyId keyId ) { newElement ( keyId, false) ;}
    
    virtual void newAttribute ( KeyId keyId, const String& value );
    virtual void elementEnd ( KeyId keyId );
    virtual void appendText ( const String& text, bool disableOutputEscaping  );

    virtual void newComment ( const String& comment );
    virtual void newPI ( const String& name, const String& contents );

    ElementRef getCurrentElement();
    
    static NodeFlowDom& asNodeFlowDom ( NodeFlow& nodeFlow )
    { return dynamic_cast<NodeFlowDom&> ( nodeFlow ); }
  };
};

#endif // __XEM_NODEFLOW_DOM_H

