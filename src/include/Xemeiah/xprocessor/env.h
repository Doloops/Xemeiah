#ifndef __XEM_XPROCESSOR_ENV_H
#define __XEM_XPROCESSOR_ENV_H

#include <Xemeiah/kern/format/core_types.h>

#include <Xemeiah/kern/poolallocator.h>

#include <Xemeiah/kern/namespacealias.h>
#include <Xemeiah/dom/nodeset.h>

#include <map>
#include <list>
#include <string>

/*
 * Environment Variables and Documents
 */
 
 
namespace Xem
{
  XemStdException ( VariableNotFoundException );
  XemStdException ( EvalContextException );
  
  class DocumentAllocator;
  class XProcessorModule;
  class XProcessorException;
  // class XProcessorHandlerMap;

#define __XEM_ENV_ENVENTRY_HAS_POOLALLOCATOR //< Pool Allocator

  /**
   * Env (for Environment) is responsible for storing all contextual information while processing.
   * These contextual pieces of information include :
   * -# Variable bindings
   * -# Parsed documents
   * -# Current evaluation nodeset
   * -# Target NodeFlow, the target to which the processor outputs generated nodes and text.
   * For variable storing, Env uses a mapping stack, with automatic garbage collecting :
   * -# Env::pushEnv() and Env::popEnv() defines sections between which variable declarations occur.
   * -# A variable declaration can overwrite a preceding declaration set in a lower level of the stack.
   * -# In this case, the original variable content will be restored at the end of the stack level where the new variable binding was defined.
   *
   * Example : (EnvId refers to the stack level of the Env class).
   * - EnvId=1 : pushEnv() : sets EnvId=2.
   * - EnvId=2 : setVariable('A','contents of A') : assigns to variable A the value 'contents of A'.
   * - EnvId=2 : pushEnv() : sets EnvId=3.
   * - EnvId=3 : setVariable('A','different contents for A') : overwrite variable A with the value 'different contents for A'.
   * - EnvId=3 : getVariable('A') : returns 'different contents for A'
   * - EnvId=3 : popEnv() : sets back EnvId=2, and restore previous variable binding for A, as there was a new declaration at EnvId=3.
   * - EnvId=2 : getVariable('A') : returns 'contents of A'.
   * - EnvId=2 : popEnv() : sets back EnvId=1, and delete the binding for variable A. A no longer exist as a variable in Env.
   * - EnvId=1 : getVariable('A') : throw exception VariableNotFoundException.
   */  
  class Env
  {
    friend class NodeFlow;

  public:
    /**
     * EnvId defines the level of Env's declaration stack.
     */
    typedef unsigned long EnvId;
  protected:
    /**
     * Binding to the Store.
     */
    Store& store;

    /**
     * Points to the Env that forked us, or NULL if the Env is not forked.
     */
    Env* fatherEnv;

    /**
     * Mapping from document names to document contents.
     */
    typedef std::map<String,Document*> DocumentMap;

    /**
     * The mapping for active documents, as asked by getDocument() (XPath document() function, for example)
     * If a new Context has been created to store the document context, then it is freed when the NodeSet is freed. 
     */
    DocumentMap documentMap;

    /**
     * Current Env settings
     */
    class EnvSettings
    {
    public:
      EnvSettings(); ~EnvSettings();
      EnvId documentAllocatorMaximumAge;
    };

    /**
     * Reference to our own Settings (must be set by XProcessor)
     */
    EnvSettings& envSettings;

    /**
     * A variable binding for a given stack level.
     */
    class EnvItem : public NodeSet
    {
    public:
      /**
       * Simple constructor
       */
      EnvItem() 
      { nextInEntry = NULL; previousItem = NULL; }
      
      /** 
       * Simple destructor
       */
      ~EnvItem() {}
      
      KeyId keyId; /**< the KeyId of the variable binding. See Xem::KeyCache for KeyId bindings */
      EnvId envId; /**< the EnvId where this variable binding was set */
      EnvItem* nextInEntry; /**< Next variable declaration for the same EnvId */
      EnvItem* previousItem; /**< Previous binding of the same variable KeyId */
    };
    
    /**
     * The EnvEntry defines the entry-point for each pushed Env.
     * It is the head of the single-linked-list of EnvItems for this pushed Env
     * (using EnvItem::nextInEntry).
     */
    class EnvEntry
    {
    public:
      /**
       * Simple constructor
       */
      EnvEntry ();
      
      /**
       * Simple destructor
       */
      INLINE ~EnvEntry ();
    
      /**
       * The envId (the current evaluation context level) for this Entry
       */
      EnvId envId;
      
      /**
       * The first item defined for this level
       */
      EnvItem* firstItem;
      
      /**
       * A pointer to the previous Entry
       */
      EnvEntry* lastEntry;
      
#ifdef __XEM_ENV_ENVENTRY_HAS_POOLALLOCATOR
      /**
       * The poolAllocator defined for this level
       */
      PoolAllocator<EnvItem,PageSize> defaultPoolAllocator;
#endif
      /**
       * Class : list of documents assigned for this level
       */
      typedef std::list<Document*> BindedDocuments;
      
      /**
       * List of documents assigned for this level, which will be deleted at popEnv()
       */
      BindedDocuments* bindedDocuments;
      
      /**
       * If this level has a documentAllocator, it is set here, and will be deleted at popEnv()
       */
      DocumentAllocator* documentAllocator;
      
      /**
       * The previous DocumentAllocator, which will be restored at popEnv()
       */
      DocumentAllocator* previousDocumentAllocator;
      
      /**
       * Create a new EnvItem for this level
       * @return a new EnvItem
       */
      INLINE EnvItem* newEnvItem ();
      
      /**
       * Release a EnvItem
       * @param envItem the envItem to release
       */
      INLINE void releaseEnvItem ( EnvItem* envItem );
    };
    
    /**
     * The head (last created) Entry for this Env
     */
    EnvEntry* headEntry;
    
    /**
     * The current evaluation level, which must be higher or equal to the headEntry's level
     */
    EnvId currentEnvId;

    /**
     * The itemMap maps the KeyId of the variable to its current
     * single-linked-list of variable values (using EnvItem::previousItem).
     * The first one in the linked-list is supposed to be the good one.
     * \todo replace std::map by a Xem::QNameMap-style linear array.
     */
    typedef std::map<KeyId,EnvItem*> ItemMap;
    
    /**
     * The current itemMap available
     */
    ItemMap itemMap;

    /**
     * Create a new EnvEntry for headEntry
     */
    void createEnvEntry();
    
    /**
     * Release a EnvEntry
     */
    void freeEnvEntry();

    /**
     * Assign a created EnvItem to an existing EnvEntry
     */
    INLINE void assignEnvItem ( EnvEntry* envEntry, EnvItem* envItem );

    /** 
     * Get the entry defined just behind the current one (currentEnvId-1)
     */
    INLINE EnvEntry* getBehindEntry ( );
    
    /**
     * Get the current entry (currentEnvId)
     */
    INLINE EnvEntry* getHeadEntry ( );

    /**
     * The current DocumentAllocator defined
     */
    DocumentAllocator* currentDocumentAllocator;
    
    /**
     * The level at which we have created our DocumentAllocator
     */
    EnvId currentDocumentAllocatorEnvId;

    /**
     * The node with which we have called a xpath expression (ie the current() node)
     */    
    NodeRef* xpathBaseNode;
    
    /**
     * Class : NodeSet iterator stack
     */
    typedef std::list<NodeSet::iterator*> IteratorStack;
    
    /**
     * The Iterator stack which is used for position() and last() evaluation
     */
    IteratorStack iteratorStack;
    
    /**
     * Effective fetching of document
     */    
    ElementRef fetchDocumentRoot ( const String& effectiveURL );    

    /**
     * <p>Env fork mechanism, for multi-threaded usage (prototype ! be carefull !).
     * The idea is to obtain a shadow copy of the current Env variable bindings (and DocumentMap, ...).</p>
     * @param store the main Store to use for this Env.
     * @param fatherEnv the Env to copy variables from.
     */
    Env( Store& store, Env* fatherEnv ) DEPRECATED;
    
    /**
     * Default constructor (non-forked Env)
     * @param store the main Store to use for this Env.
     */
    Env( Store& store, EnvSettings& envSettings );

  public:
    
    /**
     * Env destructor
     */
    ~Env();

    /**
     * get the binded store.
     * @return the Store with which Env has been constructed.
     */
    inline Store& getStore() const { return store; }

    /**
     * Get the current base URI (implemented in XProcessor)
     */
    virtual String getBaseURI () = 0;
    
    /**
     * Push the Env's declaration stack.
     */
    INLINE void pushEnv () __FORCE_INLINE;
    
    /**
     * Pop the Env's declaration stack.
     * All variables set after the last corresponding call to pushEnv() will be destroyed.
     */
    INLINE void popEnv () __FORCE_INLINE;

    /**
     * Get the current EnvId, ie the current level of Env's declaration stack.
     */
    INLINE EnvId getCurrentEnvId() const { return currentEnvId; }

    /**
     * @return the current() node for XPath evaluation
     **/
    INLINE NodeRef& getCurrentNode();

    /**
     * Push the current NodeSet iterator, providing the Env with information on current nodeset (position() and last()).
     * Note that iterator pushing is only allowed in NodeSet::iterator constructor, with the constructor NodeSet::iterator ( NodeSet&, Env& ).
     * @param iterator the current NodeSet iterator to push.
     */
    INLINE void pushIterator ( NodeSet::iterator& iterator );
    
    /**
     * Pop the current NodeSet iterator.
     * @param iterator the current NodeSet iterator to pop. Must be equal to the iterator we pushed in the corresponding call to pushIterator().
     */
    INLINE void popIterator ( NodeSet::iterator& iterator );

    /**
     * Disables the current iterator : a NULL iterator will be inserted
     * This forces getPosition() callers to find another way to compute position, for example by computing the 
     * current Node position in document.
     */
    void disableIterator ();
    
    /**
     * Restore current iterator : after a call to disableIterator(), restore the previous iterator set.
     * 
     */
    void restoreIterator ();

    /**
     * Defines wether Env has a current running iterator.
     * @return false if no iterator is set. getPosition() and getLast() will abort badly if it is not the case.
     */
    INLINE bool hasIterator ();

    /**
     * Get current iterator
     */
    NodeSet::iterator& getCurrentIterator ();

    /**
     * Get current nodeSet
     */
    NodeSet& getCurrentNodeSet ();

    /**
     * Get the current node's XPath position() in the current nodeset.
     * @return the current XPath position()
     */
    Integer getPosition();

    /**
     * Get the current node's XPath last() in the current nodeset.
     * @return the current XPath last()
     */
    Integer getLast();

    /**
     * Get the Store's KeyCache.
     * @return the Store's KeyCache
     */
    INLINE KeyCache& getKeyCache() const { return store.getKeyCache(); }
    
    /**
     * Set the operating base node for XPath evaluation.
     */
    INLINE void setXPathBaseNode ();
    
    /**
     * Get the current base node for XPath evaluation
     * @return the base node, as reported by XPath's current() function.
     */
    INLINE NodeRef& getXPathBaseNode();

    /**
     * Assigns a value to a variable designed with a KeyId.
     * Stackable Env allows to replace an already existing value of a variable,
     * previous content will be restored when the corresponding popEnv() function will be called.
     * @param keyId the KeyId of the variable
     * @param behind set variable behind or not
     * @return the NodeSet assigned for this keyId. Note that this NodeSet will be delete()ed at the corresponding call to popEnv().
     */
    INLINE NodeSet* setVariable ( KeyId keyId, bool behind = false );    

    /**
     * Allocates a new variable binding 
     * @param keyId the keyId of the variable
     * @param behind set behind or not
     */
    INLINE NodeSet* allocateVariable ( KeyId keyId, bool behind = false );    
    
    /**
     * Assign a variable binding
     * @param keyId the keyId of the variable
     * @param nodeSet a NodeSet as provided by allocateVariable ()
     * @param behind set behind or not
     */
    INLINE void assignVariable ( KeyId keyId, NodeSet* nodeSet, bool behind = false );

    /**
     * set a single ElementRef as variable.
     * @param keyId the keyId of the variable
     * @param nodeRef the NodeRefRef to put as content of the variable assignation
     * @param behind set behind or not
     */
    INLINE void setVariableNode ( KeyId keyId, NodeRef& nodeRef, bool behind = false );
  

    INLINE void setElement ( KeyId keyId, ElementRef& elementRef, bool behind = false )
    { setVariableNode(keyId, elementRef, behind ); }

    INLINE void setElement ( KeyId keyId, const ElementRef& elementRef_, bool behind = false )
    { ElementRef elementRef = elementRef_; setVariableNode(keyId, elementRef, behind ); }

    INLINE void setAttribute ( KeyId keyId, AttributeRef& attributeRef, bool behind = false )
    { setVariableNode(keyId, attributeRef, behind ); }

    /**
     * Sets a variable with textual content.
     * @param keyId the keyId of the variable.
     * @param value the textual content of the variable (will be copied).
     */
    void setString ( KeyId keyId, const char* value );

    /**
     * Sets a variable with textual content.
     * @param keyId the keyId of the variable.
     * @param value the textual content of the variable (will be copied).
     */
    void setString ( KeyId keyId, const String& value );

    /**
     * Checks that Env has variable defined by its KeyId.
     * @param keyId the KeyId of the variable
     * @return true if there is a binding, false otherwise.
     */
    INLINE bool hasVariable ( KeyId keyId );

    /**
     * Gets a variable content.
     * @param keyId the KeyId of the variable
     * @return the valid NodeSet. Will always return a valid NodeSet, and raise an Exception if there was no variable binding.
     * \note NodeSet* pointer is only valid until the next call to the corresponding popEnv().
     */
    INLINE NodeSet* getVariable ( KeyId keyId );

    /**
     * Gets a variable content as a String object.
     * @param keyId the KeyId of the variable.
     * @return the variable's content, as a String object.
     */
    String getString ( KeyId keyId );

    /**
     * Get the current DocumentAllocator to use
     * @param behind set if we want the immediate one or the one just behind
     */
    DocumentAllocator& getCurrentDocumentAllocator( bool behind );

    /**
     * Bind a document to the current evaluation context ; this document will be released at the next popEnv() call
     * @param the document to bind to this context
     * @param bindBehind if true, bind to the currentEnvId, otherwise bind it to currentEnvId-1
     */
    void bindDocument ( Document* document, bool bindBehind );

    /**
     * Creates a volatile document, and bind it to the XProcessor
     * @param bindBehind if true, bind to the currentEnvId, otherwise bind it to currentEnvId-1
     */
    ElementRef createVolatileDocument ( bool behind );

    /**
     * Bind a document to the current evaluation context ; this document will be released at the next popEnv() call
     * @param the document to bind to this context
     * @param bindBehind if true, bind to the currentEnvId, otherwise bind it to currentEnvId-1
     */
    void bindDocument ( Document& document, bool bindBehind )
    { bindDocument ( &document, bindBehind ); }

    /**
     * Set a document in the document map
     * @param url the URL to set the document to
     * @param document the document to associate to this URL
     */
    void setDocument ( const String& url, Document& document );
    
    /**
     * Sets a document binding.
     * @param url the url of the document, not prefixed by the default lookup directory (we shall, maybe...).
     * @param documentRoot the root element of the document. 
     */
    void setDocument ( const String& url, ElementRef& documentRoot ) DEPRECATED;

    /**
     * Gets a document's root Element.
     * @param url the url of the document.
     * @param baseURI the optional baseURI to use ; can be NULL, in that case the current XSL stylesheet is used (if any).
     * @return the root Element of the document.
     */ 
    ElementRef getDocumentRoot ( const String &url, const String* baseURI );
    
    /**
     * Gets a document's root Element.
     * @param url the url of the document.
     * @return the root Element of the document.
     */ 
    ElementRef getDocumentRoot ( const String &url )
    { return getDocumentRoot ( url, NULL ); }
    
    /**
     * Frees memory from a in-memory parsed Document
     * @param url the url of the document, not prefixed by the default lookup directory.
     */
    void releaseDocument ( const String& url );

    /**
     * Register default events
     */
    virtual void registerEvents ( Document& doc ) = 0;

    /**
     * Forks the current Env
     * @return a forked Env, inheritating all variable bindings currently set.
     */
    Env* forkEnv ();

    /**
     * Exception handling : detail the exception raised with information on the current Env's state.
     * @param xpe the Exception to detail
     * @param fullDump be really verbose about it.
     */
    void dumpEnv ( Exception* xpe, bool fullDump );

    /**
     * Exception handling : detail the exception raised with information on the current Env's state.
     * @param xpe the Exception to detail
     * Don't be verbose...
     */
    void dumpEnv ( Exception* xpe ) { dumpEnv ( xpe, false ); }

    /**
     * (debug) Dump information on the current Env's state.
     */
    void dumpEnv ();
  };
  
};

#endif // __XEM_XPROCESSOR_ENV_H

