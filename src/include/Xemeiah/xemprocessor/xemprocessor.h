#ifndef __XEM_XEMPROCESSOR_H
#define __XEM_XEMPROCESSOR_H

#include <Xemeiah/kern/keycache.h>
#include <Xemeiah/dom/documentmeta.h>
#include <Xemeiah/xemprocessor/xemmoduleforge.h>
#include <Xemeiah/xprocessor/xprocessormodule.h>
#include <Xemeiah/xprocessor/xprocessor.h>

#include <Xemeiah/kern/exception.h>

namespace Xem
{
  class Store;
  class Session;
  class XProcessorHandlerMap;
  class ElementMapRef;
  class XPath;
  class MetaIndexer;
  class XemView;

  XemStdException ( XemProcessorException );
  XemStdException ( XemEventException );
#define throwXemProcessorException(...) throwException ( XemProcessorException, __VA_ARGS__ )
#define throwXemEventException(...) throwException ( XemEventException, __VA_ARGS__ )

  /**
   * XemProcessor implements the 'xem' namespace elements, which provide various extensions to XSL and XUpdate.
   */
  class XemProcessor : public XProcessorModule
  {
    friend class XemObjectModule;
    friend class XemModuleForge;
  protected:
    // MetaIndexer module
    // MetaIndexer addMetaIndexer ( Document& doc, KeyId keyId, XPathParser& matchXPath, XPathParser& useXPath, XPathParser& scopeXPath );
    MetaIndexer addMetaIndexer ( Document& doc, ElementRef& declaration );

    MetaIndexer getMetaIndexer ( Document& doc, KeyId keyId );
    void xemInstructionSetMetaIndexer ( __XProcHandlerArgs__ );
    void domMetaIndexerSort ( ElementRef elt, XPath& orderBy );
    void domMetaIndexerTrigger ( DomEventType domEventType, ElementRef& domEventElement, NodeRef& nodeRef );

    // View module
    XemView addXemView ( ElementRef& declaration, ElementRef& base );
    void xemInstructionView ( __XProcHandlerArgs__ );
    void domXemViewTrigger ( DomEventType domEventType, ElementRef& domEventElement, NodeRef& nodeRef );
    void xemViewDoInsert ( ElementRef& base, ElementRef target, ElementRef modelFather, KeyId lookupId, KeyId lookupNameId );
    void xemViewInsert ( ElementRef& base, const ElementRef& source, ElementRef& xemView, KeyId lookupId, KeyId lookupNameId );
    void xemViewRemove ( ElementRef& base, const ElementRef& source );

    // Common
    void xemInstructionDefault ( __XProcHandlerArgs__ );

    void xemInstructionHousewife ( __XProcHandlerArgs__ );
    void xemInstructionException ( __XProcHandlerArgs__ );
    void xemInstructionCatchExceptions ( __XProcHandlerArgs__ );
    void xemInstructionNotHandled ( __XProcHandlerArgs__ );
    void xemFunctionGetHome ( __XProcFunctionArgs__ );
    
    // Import Stuff
    void xemInstructionImportDocument ( __XProcHandlerArgs__ );
    void xemInstructionImportFolder ( __XProcHandlerArgs__ );
    void xemInstructionImportZip ( __XProcHandlerArgs__ );
    
    // Object-Oriented XML Programming
    void domEventObject ( DomEventType domEventType, ElementRef& domEventElement, NodeRef& nodeRef );
    void domEventMethod ( DomEventType domEventType, ElementRef& domEventElement, NodeRef& nodeRef );
    void domEventFunction ( DomEventType domEventType, ElementRef& domEventElement, NodeRef& nodeRef );

    void xemInstructionSetCurrentCodeScope ( __XProcHandlerArgs__ ); //< Assign the current code scope
    void xemInstructionProcedure ( __XProcHandlerArgs__ ); // Simple xem:procedure executer
    void xemInstructionMethod ( __XProcHandlerArgs__ );
    void xemInstructionFunction ( __XProcHandlerArgs__ );
    void xemInstructionProcess ( __XProcHandlerArgs__ );
    void xemInstructionInstance ( __XProcHandlerArgs__ );
    void xemInstructionCall ( __XProcHandlerArgs__ );
    void xemInstructionParam ( __XProcHandlerArgs__ );
    void xemInstruction_triggerEvent ( __XProcHandlerArgs__ );
    

    // XPath Functions
    void xemFunctionGetVersion ( __XProcFunctionArgs__ );
    void xemFunctionRand ( __XProcFunctionArgs__ );    
    void xemFunctionGetCurrentTime ( __XProcFunctionArgs__ );
    void xemFunctionVariable ( __XProcFunctionArgs__ );

    // Role-based Functions
    void xemInstructionOpenDocument ( __XProcHandlerArgs__ );
    void xemFunctionTransmittable ( __XProcFunctionArgs__ );
    void xemFunctionGetNode ( __XProcFunctionArgs__ );
    
    // Role-based helper functions
    ElementRef resolveElementWithRole ( const String& nodeIdStr, KeyId& attributeKeyId );
  
    // Object-Base default function
    void xemFunctionGetQNameId ( __XProcFunctionArgs__ );
    void xemFunctionGetObjectDefinition ( __XProcFunctionArgs__ );
    void xemFunctionDefaultFunction ( __XProcFunctionArgs__ );

    // External module library loading
    void xemInstructionLoadExternalModule ( __XProcHandlerArgs__ );


    XemModuleForge& getXemModuleForge() const { return getTypedModuleForge<XemModuleForge>(); }
    
    void doInstanciate ( ElementRef& caller, ElementRef& constructorMethod, ElementRef& instance );
    
    static SKMapHash hashMethodId ( KeyId classId, KeyId methodId );

  public:
    XemProcessor ( XProcessor& xproc, XemModuleForge& moduleForge );
    ~XemProcessor () {}

    __BUILTIN_NAMESPACE_CLASS(xem) &xem;
    __BUILTIN_NAMESPACE_CLASS(xem_role) &xem_role;
    __BUILTIN_NAMESPACE_CLASS(xem_event) &xem_event;

    static XemProcessor& getMe ( XProcessor& xproc );
    
    /**
     * set current execution code scope
     */
    void setCurrentCodeScope ( ElementRef& codeScope, bool behind = false );
    ElementRef getCurrentCodeScope ();
    
    bool hasCurrentCodeScope ();

    /*
     * Call Method helpers
     */
    ElementRef getObjectDefinition ( KeyId classId );
    ElementRef getObjectConstructor ( KeyId classId );
    ElementRef findMethod ( KeyId classId, LocalKeyId methodId );
    ElementRef findFunction ( ElementMapRef& functionMap, KeyId classId, LocalKeyId functionId );
    
    KeyId getElementClass ( ElementRef& element );
    String getElementClassName ( ElementRef& element );


    void resolveNodeAndPushToNodeSet ( NodeSet& result, const String& nodeIdStr );

    void callMethod ( ElementRef& caller, ElementRef& thisElement, ElementRef& method, bool setThis );

    void callMethod ( ElementRef& caller, ElementRef& thisElement, KeyId classId, LocalKeyId methodId, bool setThis );

    void callMethod ( ElementRef& thisElement, LocalKeyId methodId );

    void callMethod ( ElementRef& thisElement, const String& methodName );

    void callMethod ( __XProcHandlerArgs__ );

    void controlMethodArguments ( KeyId classId, KeyId methodId, KeyIdList& arguments );
    void controlMethodArguments ( ElementRef& thisElement, KeyId methodId, KeyIdList& arguments );

    void callElementConstructor ( ElementRef& thisElement );

    ElementRef getThisFromCaller ( ElementRef& caller );

    /**
     * Install handlers
     */
    void install ();
  };
  
};
  
#endif // __XEM_XEMPROCESSOR_H

