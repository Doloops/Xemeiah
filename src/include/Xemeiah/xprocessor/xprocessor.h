#ifndef __XEM_XPROCESSOR_H
#define __XEM_XPROCESSOR_H

#include <Xemeiah/kern/store.h>
#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/nodeflow/nodeflow.h>
#include <Xemeiah/xprocessor/env.h>
#include <Xemeiah/xprocessor/xprocessormodule.h>

#include <map>
#include <list>


namespace Xem
{
  /**
   * Standard XProcessor Exception
   */
  XemStdException ( XProcessorException );
  XemStdException ( RuntimeException );

  class XPathDecimalFormat;

  /**
   * Set UserId
   */
  typedef Integer UserId;
  
#define throwXProcessorException(...) throwException ( XProcessorException, __VA_ARGS__ )

/**
 * Access to the builtin keys from the XProcessor context
 */
#define __builtin getXProcessor().getKeyCache().getBuiltinKeys()

  /**
   * XProcessor processes XSLT, XUpdate, and generally each XML-based language by navigating in Element nodes
   */
  class XProcessor : public Env
  {
    friend class NodeFlow; //< for restorePreviousNodeFlow()

  public:
    /**
     * XProcessor hook for a XProcessorModule-defined handler, function or misc
     */
    template<typename HookClass>
    class XProcessorHook
    {
    public:
      /**
       * The XProcessorModule holding this hook
       */
      XProcessorModule* module;

      /**
       * The corresponding hook function
       */
      HookClass hook;
      
      /**
       * Default constructor
       */
      XProcessorHook() { module = NULL; hook = NULL; }

      /**
       * Full Constructor
       * @param _module the XProcessorModule holding this hook
       * @param _hook the hook
       */
      XProcessorHook ( XProcessorModule* _module, HookClass _hook )
      { module = _module; hook = _hook; }

      /**
       * Checks that the hook is correctly set !
       */
      operator bool ()
      {
        return (module && hook);
      }

      /**
       * XProcessorHook destructor
       */
      ~XProcessorHook() {}
    };
    
    /**
     * Type : hook for a XProcessor Element handler 
     */
    typedef XProcessorHook<XProcessorModule::XProcessorHandler> XProcessorHandler;
    
    /**
     * Type : hook for a XProcessor XPath Function handler
     */
    typedef XProcessorHook<XProcessorModule::XProcessorFunction> XProcessorFunction;

    /**
     * Type : hook for a XProcessor DomEventHandler handler
     */
    typedef XProcessorHook<XProcessorModule::DomEventHandler> DomEventHandler;

    /**
     * XProcessor settings
     */
    class Settings : public EnvSettings
    {
    public:
      Settings ();
      ~Settings ();
      __ui64 xpathChildLookupEnabled;
      __ui64 xpathChildLookupThreshold;
      bool autoInstallModules;
    };
  protected:
    /**
     * Class : Map of all XProcessorModule instanciated for this XProcessor
     */
    typedef std::map<NamespaceId,XProcessorModule*> ModuleMap;

    /**
     * Map of all XProcessorModule instanciated for this XProcessor
     */
    ModuleMap moduleMap;

    /**
     * Default XProcessor handler
     */
    XProcessorHandler defaultHandler;

    /**
     * Default XProcessor function
     */
    XProcessorFunction defaultFunction;

    /**
     * Hook used for getBaseURI() function
     */
    XProcessorHook<XProcessorModule::BaseURIHook> baseURIHook;

    /**
     * Hook used for getDefaultDocument() function
     */
    XProcessorHook<XProcessorModule::GetDefaultDocumentHook> getDefaultDocumentHook;

    /**
     * Hook used for getXPathDecimalFormat() function
     */
    XProcessorHook<XProcessorModule::GetXPathDecimalFormatHook> getXPathDecimalFormatHook;

    /**
     * Hook used for mustSkipWhitespace() function
     */
    XProcessorHook<XProcessorModule::MustSkipWhitespaceHook> mustSkipWhitespaceHook;

    /**
     * Hook used for getXPathKeyExpressions() function
     */
    XProcessorHook<XProcessorModule::GetXPathKeyExpressionsHook> getXPathKeyExpressionsHook;

    /**
     * The maximum authorized stack level in XProcessor execution.
     * This forbids general recursion problems.
     */
    Env::EnvId maxLevel;

    /**
     * Current NodeFlow target for this Env.
     */
    NodeFlow* currentNodeFlow;
    
    /**
     * Restores previous NodeFlow.
     * This is called directly from within the current NodeFlow's destructor.
     * @param previous the previous NodeFlow set (as recorded by the current NodeFlow when it was pushed using setNodeFlow()).
     */
    void restorePreviousNodeFlow ( NodeFlow* previous ) { currentNodeFlow = previous; }
    
    /**
     * Current XProcessor settings
     */
    Settings settings;

    /**
     * UserId running this XProcessor
     */
    UserId userId;
  public:

    /**
     * XProcessor constructor.
     * @param store the Store to bind to.
     * @param xprocHandlerMap the XProcessorHandlerMap to use for processing nodes.
     */
    XProcessor ( Store& store );
    
    /**
     * XProcessor destructor
     */
    virtual ~XProcessor ();

    /**
     * Stub to acces the KeyCache
     * @return the Store KeyCache.
     */
    KeyCache& getKeyCache() const { return getStore().getKeyCache(); }

    /**
     * Access to our settings
     */
    const Settings& getSettings() const { return settings; }

    /**
     * Access to our settings for modification
     */
    Settings& setSettings() { return settings; }

    /**
     * Check if XProcessor has a NodeFlow set
     */
    INLINE bool hasNodeFlow ();

    /**
     * Get the currently binded NodeFlow
     * @return Current NodeFlow. May raise an Exception if no NodeFlow is binded.
     */
    INLINE NodeFlow& getNodeFlow();

    /**
     * Set the current NodeFlow to operate on.
     * Note that NodeFlow unbinding is achieved in NodeFlow destructor.
     * @param nodeFlow the current NodeFlow.
     */    
    void setNodeFlow ( NodeFlow& nodeFlow );

    /**
     * Register a new module (called by XProcessorModuleForge::instanciateModule() )
     */
    void registerModule ( NamespaceId moduleNSId, XProcessorModule* xprocessorModule );

    /**
     * Register a new module (called by XProcessorModuleForge::instanciateModule() )
     */
    void registerModule ( XProcessorModule* xprocessorModule );

    /**
     * Install a module configured in the store
     * @param moduleNSId the Namespace Id of the module to instanciate
     * @param throwException throw an exception if the module can not be found, do nothing otherwise
     */
    void installModule ( NamespaceId moduleNSId, bool throwException = true );

    /**
     * Install a module configured in the store
     */
    void installModule ( const char* moduleName );
    
    /**
     * Install a module configured in the store
     */
    inline void installModule ( const String& moduleName ) { installModule ( moduleName.c_str() ); }

    /**
     * Uninstall a configured module
     */
    void uninstallModule ( NamespaceId moduleNSId );
    
    /**
     * Install all modules
     */
    void installAllModules ();

    /**
     * Load a XProcessorModule library
     * @param libName the library name
     * @param installModules install library's modules in XProcessor
     */
    void loadLibrary ( const String& libName, bool installModules = true );

    /**
     * Get a module from its NamespaceId
     */
    INLINE XProcessorModule* getModule ( NamespaceId moduleNSId, bool mayAutoInstall = false );

    /**
     * Get a module from its Namespace URL
     */
    XProcessorModule* getModule ( const char* moduleName, bool mayAutoInstall = false );

    /**
     * Get a module from its Namespace URL (alias)
     */
    XProcessorModule* getModule ( const String& moduleName, bool mayAutoInstall = false )
    { return getModule ( moduleName.c_str(), mayAutoInstall ); }

    /**
     * Register the default Handler
     */
    void registerDefaultHandler ( XProcessorHandler xprocessorHandler ) { defaultHandler = xprocessorHandler; }
    
    /**
     * Register the default Function
     */
    void registerDefaultFunction ( XProcessorFunction xprocessorFunction ) { defaultFunction = xprocessorFunction; }
    
    /**
     * Register the getBaseURI() hook
     */
    void registerBaseURIHook ( XProcessorHook<XProcessorModule::BaseURIHook> hook ) { baseURIHook = hook; }
    
    /**
     * Register the getDefaultDocument() hook
     */
    void registerGetDefaultDocumentHook ( XProcessorHook<XProcessorModule::GetDefaultDocumentHook> hook ) { getDefaultDocumentHook = hook; }

    /**
     * Register the getXPathDecimalFormat() hook
     */
    void registerGetXPathDecimalFormatHook ( XProcessorHook<XProcessorModule::GetXPathDecimalFormatHook> hook ) { getXPathDecimalFormatHook = hook; }

    /**
     * Register the mustSkipWhitespace() hook
     */
    void registerMustSkipWhitespaceHook ( XProcessorHook<XProcessorModule::MustSkipWhitespaceHook> hook ) { mustSkipWhitespaceHook = hook; }

    /**
     * Register the getXPathKeyExpressions() hook
     */
    void registerGetXPathKeyExpressionsHook ( XProcessorHook<XProcessorModule::GetXPathKeyExpressionsHook> hook ) { getXPathKeyExpressionsHook = hook; }

    /**
     * Get an XProcessorHandler 
     */
    INLINE XProcessorHandler getXProcessorHandler ( KeyId keyId );

    /**
     * Get an XProcessorFunction
     */    
    INLINE XProcessorFunction getXProcessorFunction ( KeyId keyId );

    /**
     * Get a DomEventHandler
     */
    INLINE DomEventHandler getDomEventHandler ( KeyId keyId );

    /**
     * Getting maximum level
     * @return the maximum number of Env stack levels allowed.
     */
    EnvId getMaxLevel () const { return maxLevel; }
    
    /**
     * Set the maximum level (depth) of processing.
     * Estimate 2 levels per each XSL element processed.
     * @param maxEnvId the maximum number of Env stack levels allowed.
     */
    void setMaxLevel ( EnvId maxEnvId );

    /**
     * Get XProcessorModule properties
     * @param nodeSet the NodeSet to put the property in
     * @param ptyId the QName Id of the property
     */
    void getXProcessorModuleProperty ( NodeSet& nodeSet, KeyId ptyId );
    
    /**
     * Register all DomEvents into the Document
     */
    void registerEvents ( Document& doc );

    /**
     * Process a source Element
     * @param processingItem the item to process
     */
    void process ( ElementRef& processingItem ) __FORCE_INLINE;

    /**
     * Process all children of a source Element
     * @param currentNode the contextual node
     */
    void processChildren ( ElementRef& processingItem ) __FORCE_INLINE;

    /**
     * Process a source text node Element
     * @param processingItem the item to process
     */
    void processTextNode ( ElementRef& processingItem );

    /**
     * External class call : Process an element with a given KeyId (which may be different)
     */
    void processElement ( ElementRef& processingItem, KeyId handlerId );

    /**
     * Eval children as a string
     */
    String evalChildrenAsString ( ElementRef& item );

    /**
     * Eval attribute content
     */
    void evalAttributeContent ( ElementRef& item, KeyId attrNameKeyId, KeyId namespaceAttrKeyId = 0, KeyId attributeValueKeyId = 0 );

    /**
     * Get current baseURI
     */
    String getBaseURI ();

    /**
     * Get Default Document
     */
    ElementRef getDefaultDocument (); 
     
    /**
     * Get Decimal Format
     */
    XPathDecimalFormat getXPathDecimalFormat ( KeyId decimalFormatId );
     
    /**
     * Check if we must skip that whitespace
     */
    bool mustSkipWhitespace ( KeyId keyId );

    /**
     * Get the XPath expressions implementing XPath key() function
     */
    void getXPathKeyExpressions ( KeyId keyId, XPath& matchXPath, XPath& useXPath );

    /**
     * Generic processing of a variable or param definition element
     * @param item The variable element (can be xsl:variable, xsl:param, xsl:with-param, xem:param, ...)
     * @param nameKeyId the keyId of the 'name' attribute of item
     * @param selectKeyId the keyId of the 'select' attribute of item
     * @param setBehind defines wether the variable must be declared at current Env stack (false) or at the level before (true).
     */
    void processInstructionVariable ( ElementRef& item, KeyId nameKeyId, KeyId selectKeyId, bool setBehind = false );

    /**
     * Fills a thrown Exception with details about current Execution.
     * @param xpe the Exception to fill
     * @param actionItem the current processed item
     * @param currentNode the current context node
     * @param processOrProcessChlidren determines whether handleException() was called from process() or processChildren()
     */
    void handleException ( Exception* xpe, ElementRef& actionItem, bool processOrProcessChildren );

    /**
     *
     */
    String getProcedureAlias ( const String& aliasFile, const String& filePath );

    /**
     * Run an XML-based procedure
     * @param filePath the absolute path to the procedure to parse and run
     */
    void runProcedure ( const String& filePath );

    /**
     * Call the event trigger hook (if registered)
     */
    void triggerEvent ( KeyId eventId, KeyIdList& attributes );

    /**
     * Dump Extensions
     */
    void dumpExtensions ();

    /**
     * Set userId
     */
    void setUserId ( UserId userId_ ) { userId = userId_; }

    /**
     * Get userId
     */
    UserId getUserId ( ) { return userId; }
  };
};

#endif // __XEM_XPROCESSOR_H
