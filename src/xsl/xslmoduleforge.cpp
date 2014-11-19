#include <Xemeiah/xsl/xslmoduleforge.h>
#include <Xemeiah/xsl/defaultpimodule.h>
#include <Xemeiah/xprocessor/xprocessorlib.h>
#include <Xemeiah/xsl/xslprocessor.h>
#include <Xemeiah/xpath/xpathparser.h>
#include <Xemeiah/dom/documentmeta.h>

#include <Xemeiah/auto-inline.hpp>

#undef __builtin
#define __builtin store.getKeyCache().getBuiltinKeys()

#define Log_XSL Debug

namespace Xem
{
  __XProcessorLib_DECLARE_LIB ( XSL, "xsl" );
  __XProcessorLib_REGISTER_MODULE ( XSL, XSLModuleForge );
  __XProcessorLib_REGISTER_MODULE ( XSL, DefaultPIModuleForge );
  
#include <Xemeiah/kern/builtin_keys_prolog_inst.h>
#include <Xemeiah/xsl/builtin-keys/xsl>
#include <Xemeiah/xsl/builtin-keys/xslimpl>
#include <Xemeiah/kern/builtin_keys_postlog.h>

  XSLModuleForge::XSLModuleForge ( Store& store )
  : XProcessorModuleForge ( store ), xsl(store.getKeyCache()),xslimpl(store.getKeyCache())
  {
    xpathDefaultSort = NULL;
  }
  
  XSLModuleForge::~XSLModuleForge ()
  {
    Log_XSL ( "DELETE XSLModuleForge !\n" );
    if ( xpathDefaultSort ) delete ( xpathDefaultSort );
  }

  NamespaceId XSLModuleForge::getModuleNamespaceId ( )
  {
    return xsl.ns();
  }

  void XSLModuleForge::install ()
  {
    Log_XSL ( "Installing XSLModuleForge for '%s' (ns=%x, at %p)\n",
       store.getKeyCache().getNamespaceURL(getModuleNamespaceId()), getModuleNamespaceId(), this );  

    NamespaceAlias nsAlias ( store.getKeyCache() );
    xpathDefaultSort = new XPathParser ( store.getKeyCache(), nsAlias, ".", false );

    /**
     * Toplevel operator
     */
    registerHandler ( xsl.stylesheet(), &XSLProcessor::xslInstructionStylesheet  );
    registerHandler ( xsl.output(), &XSLProcessor::xslInstructionOutput );

    /**
     * Template-based operations
     */
    registerHandler ( xsl.template_(), &XSLProcessor::xslInstructionTemplate  );

    registerHandler ( xsl.apply_templates(), &XSLProcessor::xslInstructionApplyTemplates  );
    registerHandler ( xsl.apply_imports(), &XSLProcessor::xslInstructionApplyImports  );
    registerHandler ( xsl.call_template(), &XSLProcessor::xslInstructionCallTemplate );

    registerHandler ( xsl.param(), &XSLProcessor::xslInstructionNoAction  );
    registerHandler ( xsl.with_param(), &XSLProcessor::xslInstructionNoAction  );

    /**
     * Loops and conditions
     */
    registerHandler ( xsl.for_each(), &XSLProcessor::xslInstructionForEach  );
    registerHandler ( xsl.if_(), &XSLProcessor::xslInstructionIf  );
    registerHandler ( xsl.choose(), &XSLProcessor::xslInstructionChoose );
    registerHandler ( xsl.sort(), &XSLProcessor::xslInstructionNoAction  );

    /**
     * Variables
     */
    registerHandler ( xsl.variable(), &XSLProcessor::xslInstructionVariable  );

    /**
     * Output values(), &XSLProcessor::elements(), &XSLProcessor::...
     */
    registerHandler ( xsl.value_of(), &XSLProcessor::xslInstructionValueOf  );
    registerHandler ( xsl.copy(), &XSLProcessor::xslInstructionCopy  );
    registerHandler ( xsl.copy_of(), &XSLProcessor::xslInstructionCopyOf  );
    registerHandler ( xsl.element(), &XSLProcessor::xslInstructionElement );
    registerHandler ( xsl.attribute(), &XSLProcessor::xslInstructionAttribute  );
    registerHandler ( xsl.attribute_set(), &XSLProcessor::xslInstructionAttributeSet  );
    registerHandler ( xsl.number(), &XSLProcessor::xslInstructionNumber );
    registerHandler ( xsl.text(), &XSLProcessor::xslInstructionText );
    registerHandler ( xsl.processing_instruction(), &XSLProcessor::xslInstructionProcessingInstruction );


    /**
     * Messages and Comments
     */
    registerHandler ( xsl.message(), &XSLProcessor::xslInstructionMessage  );
    registerHandler ( xsl.comment(), &XSLProcessor::xslInstructionComment  );

    /**
     * XSLT 2.0 ResultDocument
     */
    registerHandler ( xsl.result_document(), &XSLProcessor::xslInstructionResultDocument  );
    registerHandler ( xsl.sequence(), &XSLProcessor::xslInstructionSequence  );
    registerHandler ( xsl.fallback(), &XSLProcessor::xslInstructionFallback );

  }

  void XSLModuleForge::instanciateModule ( XProcessor& xprocessor )
  {
    XProcessorModule* module = new XSLProcessor ( xprocessor, *this ); 
    xprocessor.registerModule ( module );
  }

  void XSLModuleForge::registerEvents ( Document& doc )
  {
    doc.getDocumentMeta().getDomEvents().registerEvent(DomEventType_CreateElement,
            xsl.stylesheet(),
            xslimpl.domevent_prepare_stylesheet());

    registerXPathAttribute ( doc, xsl.match() );
    registerXPathAttribute ( doc, xsl.select() );
    registerXPathAttribute ( doc, xsl.test() );
    registerXPathAttribute ( doc, xsl.use() );
    registerQNameAttribute ( doc, xsl.name() );
    registerQNameAttribute ( doc, xsl.mode() );


    registerNamespaceListAttribute ( doc, xsl.exclude_result_prefixes(),
        getStore().getKeyCache().getBuiltinKeys().xemint.exclude_result_prefixes() );
    registerNamespaceListAttribute ( doc, xsl.extension_element_prefixes(),
        getStore().getKeyCache().getBuiltinKeys().xemint.extension_element_prefixes() );
  }

};

