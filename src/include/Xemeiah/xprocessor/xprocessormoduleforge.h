#ifndef __XEM_XPROCESSOR_XPROCESSORMODULEFORGE_H
#define __XEM_XPROCESSOR_XPROCESSORMODULEFORGE_H

#include <Xemeiah/xprocessor/xprocessor.h>
#include <Xemeiah/xprocessor/xprocessormodule.h>

namespace Xem
{
#define __BUILTIN_NAMESPACE_CLASS(__namespaceId) __BUILTIN__##__namespaceId##_CLASS
#define __BUILTIN_NAMESPACE_INSTANCE(__namespaceId) __namespaceId
#define __BUILTIN_NAMESPACE __BUILTIN_NAMESPACE_INSTANCE
#define __DEFINE_BUILTIN_NAMESPACE(__namespaceId) __BUILTIN_NAMESPACE_CLASS(__namespaceId) __namespaceId;
#define __BUILD_BUILTIN_NAMESPACE

  class Store;

  /**
   * API for XProcessorModuleForge derivation : XProcessorModuleForge is responsible for XProcessorModule instanciation.
   * Each XProcessorModuleForge implements a given NamespaceId, and shall be unique wrt this NamespaceId
   */
  class XProcessorModuleForge
  {
    friend class XProcessorLibs; //< Only the lib loader modules are allowed to delete us
  protected:
    /**
     * Reference to our Store
     */
    Store& store;

    /**
     * Protected constructor : this class is virtual, and shall not be constructed directly
     */
    XProcessorModuleForge ( Store& _store )
    : store(_store)
    {}

    /**
     * Virtual protected destructor
     */
    virtual ~XProcessorModuleForge () {}

    /**
     * Shared
     */
    XProcessorModule::LocalHandlerMap localHandlerMap;

    /**
     * Mapping for the local functions
     */
    XProcessorModule::LocalFunctionMap localFunctionMap;

    /**
     * Mapping for the local DomEvent handlers
     */
    XProcessorModule::LocalDomEventMap localDomEventMap;

    /**
     *
     */
    void registerHandler ( KeyId keyId, XProcessorModule::XProcessorHandler handler, bool checkNamespace=true );

    template<typename T>
    void registerHandler ( KeyId keyId, void (T::*typedHandler) ( __XProcHandlerArgs__ ), bool checkNamespace=true )
    { registerHandler(keyId, (XProcessorModule::XProcessorHandler) typedHandler, checkNamespace ); }

    /**
     *
     */
    void registerFunction ( KeyId keyId, XProcessorModule::XProcessorFunction function );

    template<typename T>
    void registerFunction ( KeyId keyId, void (T::*typedFunction) ( __XProcFunctionArgs__ ) )
    { registerFunction(keyId, (XProcessorModule::XProcessorFunction) typedFunction ); }

    /**
     *
     */
    void registerDomEventHandler ( KeyId keyId, XProcessorModule::DomEventHandler handler );

    template<typename T>
    void registerDomEventHandler ( KeyId keyId, void (T::*typedDomEventHandler) ( DomEventType domEventType, ElementRef& domEventElement, NodeRef& nodeRef ) )
    { registerDomEventHandler(keyId, (XProcessorModule::DomEventHandler) typedDomEventHandler ); }

    /**
     * Register this module as capable of handling event triggering
     */
    void registerAsEventTriggerHandler ();
  public:
    /**
     * Store accessor
     */
    Store& getStore() const { return store; }

    /**
     * Get the NamespaceId assigned to this ModuleForge
     */
    virtual NamespaceId getModuleNamespaceId () = 0;

    /**
     * Install this XProcessorModuleForge in Store
     */
    virtual void install () = 0;
    
    /**
     * Instanciate this Module in XProcessor, creating a new XProcessorModule in this XProcessor
     */
    virtual void instanciateModule ( XProcessor& xprocessor ) = 0;

    /**
     *
     */
    XProcessorModule::XProcessorHandler getHandler ( LocalKeyId localKeyId );

    /**
     *
     */
    XProcessorModule::XProcessorFunction getFunction ( LocalKeyId localKeyId );

    /**
     *
     */
    XProcessorModule::DomEventHandler getDomEventHandler ( LocalKeyId localKeyId );

    /**
     * Convenience function : register a XPath attribute
     */
    void registerXPathAttribute ( Document& doc, KeyId keyId );

    /**
     * Convenience function : register a QName attribute
     */
    void registerQNameAttribute ( Document& doc, KeyId keyId );

    /**
     * Convenience function : register a QName attribute
     */
    void registerQNameListAttribute ( Document& doc, KeyId keyId, KeyId targetKeyId );

    /**
     * Convenience function : register a QName attribute
     */
    void registerNamespaceListAttribute ( Document& doc, KeyId keyId, KeyId targetKeyId );

    /**
     * Register default events to a document
     * @param doc the Document to register default Dom Events
     */
    virtual void registerEvents ( Document& doc ) = 0;

    /**
     * Dump Forge extensions
     */
    void dumpExtensions ();
  };
};

#endif //  __XEM_XPROCESSOR_XPROCESSORMODULEFORGE_H
