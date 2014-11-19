#ifndef __XEM_XPROCESSOR_XPROCESSORMODULE_H
#define __XEM_XPROCESSOR_XPROCESSORMODULE_H

#include <Xemeiah/kern/format/core_types.h>
#include <Xemeiah/dom/string.h>
#include <Xemeiah/xpath/xpath.h>


#include <map>
#include <vector>

// #define __XEM_XPROCESSORMODULE_HAS_LOCALMAPS //< Option : XProcessorModule has local maps

namespace Xem
{
  class Store;
  class KeyCache;
  class String;
  class NodeSet;
  class ElementRef;
  class Document;
  class NodeRef;
  class NodeFlow;
  class XPath;
  class XPathDecimalFormat;
  class XProcessor;
  class XProcessorModuleForge;
  class ServiceManager;
  
#define __XProcHandlerArgs__ ElementRef& item
#define __XProcFunctionArgs__ XPath& callingXPath, KeyId functionKeyId, NodeSet& result, \
    NodeRef& node, XPath::FunctionArguments& args, bool isElementFunction
  /**
   * XProcessor Implementation Module : instanciated per XProcessor instance
   */
  class XProcessorModule
  {
    friend class XProcessorModuleForge;
  public:
    /**
     * Type : Function for XProcessor handlers (called from XProcessor)
     */
    typedef void (XProcessorModule::*XProcessorHandler) ( ElementRef& item );

    /**
     * Type : Function for XProcessor functions (called from XPath)
     */
    typedef void (XProcessorModule::*XProcessorFunction) ( __XProcFunctionArgs__ );

    /**
     * Type : FUnction for DomEventHandler functions (called from Dom Events)
     */
    typedef void (XProcessorModule::*DomEventHandler) ( DomEventType domEventType, ElementRef& domEventElement, NodeRef& nodeRef );

    /**
     * Type : Function for XProcessor::getBaseURI() implementation
     */
    typedef String (XProcessorModule::*BaseURIHook) ();

    /**
     * Type : Function for XProcessor::getDefaultDocument() implementation
     */
    typedef ElementRef (XProcessorModule::*GetDefaultDocumentHook) ();

    /**
     * Type : Function for XProcessor::getXPathDecimalFormat() implementation
     */
    typedef XPathDecimalFormat (XProcessorModule::*GetXPathDecimalFormatHook) ( KeyId decimalFormatId );

    /**
     * Type : Function for XProcessor::mustSkipWhitespace() implementation
     */
    typedef bool (XProcessorModule::*MustSkipWhitespaceHook) ( KeyId keyId );

    /**
     * Type : Function for XProcessor::getXPathKeyExpressions() implementation
     */
    typedef void (XProcessorModule::*GetXPathKeyExpressionsHook) ( KeyId keyId, XPath& matchXPath, XPath& useXPath );

    /**
     * Reference to the XProcessor we have been instanciated against
     */
    XProcessor& xprocessor;

    /**
     * Reference to the XProcessorModuleForge which has instanciated us
     */
    XProcessorModuleForge& moduleForge;

  protected:
    /**
     * Type : Mapping for the Handlers implemented in this Module
     */
    typedef std::map<LocalKeyId,XProcessorHandler> LocalHandlerMap;

    /**
     * Type : Mapping for the Functions implemented in this Module
     */
    typedef std::map<LocalKeyId,XProcessorFunction> LocalFunctionMap;
    
    /**
     * Type : Mapping for the Functions implemented in this Module
     */
    typedef std::map<LocalKeyId,DomEventHandler> LocalDomEventMap;

    /**
     * Default Handler for this Namespace
     */
    XProcessorHandler defaultNSHandler;

#ifdef __XEM_XPROCESSORMODULE_HAS_LOCALMAPS
    /**
     * Mapping for the local handlers
     */
    LocalHandlerMap localHandlerMap;

    /**
     * Mapping for the local functions
     */
    LocalFunctionMap localFunctionMap;

    /**
     * Mapping for the local DomEvent handlers
     */
    LocalDomEventMap localDomEventMap;
#endif
    
    /** 
     * Protected constructor
     */    
    XProcessorModule ( XProcessor& _xprocessor, XProcessorModuleForge& moduleForge );
  public:
    /**
     * XProcessorModule destructor
     */
    virtual ~XProcessorModule() {}
    
    /**
     * Install this XProcessorModule : bind it to the XProcessor
     */
    virtual void install () = 0;
    
    /**
     * XProcessor accessor
     */
    XProcessor& getXProcessor()
    const { return xprocessor; }

    /**
     * XProcessorModuleForge default accessor
     */
    XProcessorModuleForge& getXProcessorModuleForge () const { return moduleForge; }

    /**
     * XProcessorModuleForge typed accessor
     */
    template<typename ModuleForgeClass>
    ModuleForgeClass& getTypedModuleForge() const { return dynamic_cast<ModuleForgeClass&> (moduleForge); }
    
    /**
     * Current NodeFlow accessor
     */
    NodeFlow& getNodeFlow();

    /**
     *
     */
    KeyCache& getKeyCache();

    /**
     *
     */
    NodeRef& getCurrentNode();

    /**
     * Stub access to our ServiceManager
     */
    ServiceManager& getServiceManager();

#ifdef __XEM_XPROCESSORMODULE_HAS_LOCALMAPS
    /**
     *
     */
    void registerHandler ( KeyId keyId, XProcessorHandler handler, bool checkNamespace=true );

    template<typename T>
    void registerHandler ( KeyId keyId, void (T::*typedHandler) ( __XProcHandlerArgs__ ), bool checkNamespace=true )
    { registerHandler(keyId, (XProcessorHandler) typedHandler, checkNamespace ); }

    /**
     *
     */
    void registerFunction ( KeyId keyId, XProcessorFunction function );

    template<typename T>
    void registerFunction ( KeyId keyId, void (T::*typedFunction) ( __XProcFunctionArgs__ ) )
    { registerFunction(keyId, (XProcessorFunction) typedFunction ); }

    /**
     *
     */
    void registerDomEventHandler ( KeyId keyId, DomEventHandler handler );

    template<typename T>
    void registerDomEventHandler ( KeyId keyId, void (T::*typedDomEventHandler) ( DomEventType domEventType, ElementRef& domEventElement, NodeRef& nodeRef ) )
    { registerDomEventHandler(keyId, (DomEventHandler) typedDomEventHandler ); }
#endif

    /**
     *
     */
    XProcessorHandler getHandler ( LocalKeyId localKeyId );

    /**
     *
     */
    XProcessorFunction getFunction ( LocalKeyId localKeyId );
    
    /**
     *
     */
    DomEventHandler getDomEventHandler ( LocalKeyId localKeyId );

    /**
     *
     */
    XProcessorHandler getDefaultNSHandler () { return defaultNSHandler; }
    
    /**
     *
     */
    virtual XProcessorHandler getDefaultHandler () { return NULL; }

    /**
     * Get a XProcessor Module Property
     * @param nodeSet the NodeSet to put the property in (can be any type, multi-valued, ...)
     * @param ptyId the QName Id of the property
     */
    virtual void getXProcessorModuleProperty ( NodeSet& nodeSet, KeyId ptyId );

    /**
     * Trigger an event
     */
    virtual void triggerEvent(KeyId eventId, KeyIdList& arguments);
  };
};

#endif // __XEM_XPROCESSOR_XPROCESSORMODULE_H
