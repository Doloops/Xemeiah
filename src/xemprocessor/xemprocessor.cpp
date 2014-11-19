#include <Xemeiah/xemprocessor/xemprocessor.h>
#include <Xemeiah/xemprocessor/xemmoduleforge.h>
#include <Xemeiah/xprocessor/xprocessorlib.h>

#include <Xemeiah/auto-inline.hpp>

namespace Xem
{

#include <Xemeiah/kern/builtin_keys_prolog_inst.h>
#include <Xemeiah/xemprocessor/builtin-keys.h>
#include <Xemeiah/kern/builtin_keys_postlog.h>

  __XProcessorLib_DECLARE_LIB (XemProcessor, "xemprocessor" );
  __XProcessorLib_REGISTER_DEPENDENCY (XemProcessor, "xsl" );
  __XProcessorLib_REGISTER_MODULE ( XemProcessor, XemModuleForge );

  NamespaceId XemModuleForge::getModuleNamespaceId ( )
  {
    return xem.ns();
  }

  void XemModuleForge::instanciateModule ( XProcessor& xprocessor )
  {
    XProcessorModule* module = new XemProcessor ( xprocessor, *this ); 
    xprocessor.registerModule ( module );
  }

  XemProcessor::XemProcessor ( XProcessor& xproc, XemModuleForge& moduleForge ) 
  : XProcessorModule ( xproc, moduleForge ), xem(moduleForge.xem), xem_role(moduleForge.xem_role), xem_event(moduleForge.xem_event)
  {
    
  }

  XemProcessor& XemProcessor::getMe ( XProcessor& xproc )
  {
    return dynamic_cast<XemProcessor&> ( *xproc.getModule ( "http://www.xemeiah.org/ns/xem", true ) );
  }

  void XemProcessor::install ( )
  {
    // getXProcessor().installModule(getKeyCache().getBuiltinKeys().xsl.ns());
    getXProcessor().installModule("http://www.w3.org/1999/XSL/Transform");

    XProcessor::XProcessorFunction defaultFunctionHook (this, (XProcessorFunction) &XemProcessor::xemFunctionDefaultFunction );
    getXProcessor().registerDefaultFunction ( defaultFunctionHook );

    XProcessor::XProcessorHandler defaultHandler ( this, (XProcessorHandler) &XemProcessor::xemInstructionDefault );
    getXProcessor().registerDefaultHandler ( defaultHandler );

  }

  void XemModuleForge::install ()
  {
    // Install common
    registerHandler ( xem.set_meta_indexer(), &XemProcessor::xemInstructionSetMetaIndexer );
    registerHandler ( xem.housewife(), &XemProcessor::xemInstructionHousewife );

    registerHandler ( xem.exception(), &XemProcessor::xemInstructionException );
    registerHandler ( xem.catch_exceptions(), &XemProcessor::xemInstructionCatchExceptions );

    // Install import
    registerHandler ( xem.import_document(), &XemProcessor::xemInstructionImportDocument );
    registerHandler ( xem.import_folder(), &XemProcessor::xemInstructionImportFolder );
    registerHandler ( xem.import_zip(), &XemProcessor::xemInstructionImportZip );

    // Install Objects
    registerHandler ( xem.procedure(), &XemProcessor::xemInstructionProcedure );
    registerHandler ( xem.set_current_code_scope(), &XemProcessor::xemInstructionSetCurrentCodeScope );
    registerHandler ( xem.process(), &XemProcessor::xemInstructionProcess );
    registerHandler ( xem.instance(), &XemProcessor::xemInstructionInstance );
    registerHandler ( xem.param(), &XemProcessor::xemInstructionParam );
    registerHandler ( xem.call(), &XemProcessor::xemInstructionCall );
    registerHandler ( xem.method(), &XemProcessor::xemInstructionMethod );
    registerHandler ( xem.function(), &XemProcessor::xemInstructionFunction );

    // Manually trigger an event
    registerHandler ( xem.trigger_event(), &XemProcessor::xemInstruction_triggerEvent );


    // External module loader
    registerHandler ( xem.load_external_module(), &XemProcessor::xemInstructionLoadExternalModule );

    // Event-based dynamic view
    registerHandler ( xem.xem_view(), &XemProcessor::xemInstructionView );

    /**
     * Misc functions
     */
    registerFunction ( xem.get_version(), &XemProcessor::xemFunctionGetVersion );
    registerFunction ( xem.rand(), &XemProcessor::xemFunctionRand );
    registerFunction ( xem.get_current_time(), &XemProcessor::xemFunctionGetCurrentTime );
    registerFunction ( xem.variable(), &XemProcessor::xemFunctionVariable );

    /**
     * Role-based
     */
    registerHandler ( xem.open_document(), &XemProcessor::xemInstructionOpenDocument );
    registerFunction ( xem.transmittable(), &XemProcessor::xemFunctionTransmittable );
    registerFunction ( xem.get_node(), &XemProcessor::xemFunctionGetNode );

    registerFunction ( xem.object_definition(), &XemProcessor::xemFunctionGetObjectDefinition );
    registerFunction ( xem.qname_id(), &XemProcessor::xemFunctionGetQNameId );

    // registerFunction ( xem.home(), &XemProcessor::xemFunctionGetHome );

    /*
     * Register default DomEvents for functions
     */
    registerDomEventHandler(xem.object(), &XemProcessor::domEventObject );
    registerDomEventHandler(xem.method(), &XemProcessor::domEventMethod );
    registerDomEventHandler(xem.function(), &XemProcessor::domEventFunction );

    /**
     * Register default DomEvents for meta-indexer and xem-view
     */
    registerDomEventHandler(xem.meta_indexer_trigger(), &XemProcessor::domMetaIndexerTrigger );
    registerDomEventHandler(xem.xem_view_trigger(), &XemProcessor::domXemViewTrigger );

  }

  void XemModuleForge::registerEvents ( Document& doc )
  {
    registerXPathAttribute(doc, xem.select());
    registerXPathAttribute(doc, xem.root());
    registerXPathAttribute(doc, xem.this_());
    registerQNameAttribute(doc, xem.name());
    registerQNameAttribute(doc, xem.method());

    doc.getDocumentMeta().getDomEvents().registerEvent(DomEventMask_Element, xem.object(), xem.object() );
    doc.getDocumentMeta().getDomEvents().registerEvent(DomEventMask_Element, xem.method(), xem.method() );
    doc.getDocumentMeta().getDomEvents().registerEvent(DomEventMask_Element, xem.function(), xem.function() );
  }


};
