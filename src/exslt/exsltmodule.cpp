#include <Xemeiah/xsl/xslprocessor.h>
#include <Xemeiah/exslt/exsltmodule.h>
#include <Xemeiah/xpath/xpathparser.h>
#include <Xemeiah/xprocessor/xprocessorlib.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_EXSLT Debug

namespace Xem
{
#include <Xemeiah/kern/builtin_keys_prolog_inst.h>
#include <Xemeiah/exslt/builtin-keys/exslt>
#include <Xemeiah/kern/builtin_keys_postlog.h>

  // __XProcessorLib_DECLARE_LIB_INTERNAL(EXSLT,"exslt");
  __XProcessorLib_DECLARE_LIB(EXSLT,"exslt");
  __XProcessorLib_REGISTER_MODULE ( EXSLT, EXSLTCommonModuleForge );
  __XProcessorLib_REGISTER_MODULE ( EXSLT, EXSLTDynamicModuleForge );
  __XProcessorLib_REGISTER_MODULE ( EXSLT, EXSLTSetsModuleForge );

  EXSLTCommonModuleForge::EXSLTCommonModuleForge ( Store& store )
  : XProcessorModuleForge ( store ), exslt_common(store.getKeyCache())
  {
  
  }

  EXSLTCommonModuleForge::~EXSLTCommonModuleForge ( )
  {
  
  }

  void EXSLTCommonModuleForge::instanciateModule ( XProcessor& xprocessor )
  {
    XProcessorModule* module = new EXSLTCommonModule ( xprocessor, *this ); 
    xprocessor.registerModule ( module );    
  }
  
  void EXSLTCommonModuleForge::install ()
  {
    registerHandler(exslt_common.document(), &EXSLTCommonModule::instructionDocument);
    registerFunction(exslt_common.node_set(), &EXSLTCommonModule::functionNodeSet);
  }
  
  EXSLTCommonModule::EXSLTCommonModule ( XProcessor& xproc, EXSLTCommonModuleForge& moduleForge )
  : XProcessorModule ( xproc, moduleForge ), exslt_common(moduleForge.exslt_common)
  {
  
  }
  
  EXSLTCommonModule::~EXSLTCommonModule () {}
  
  void EXSLTCommonModule::install ()
  {
  }
  
  void EXSLTCommonModule::functionNodeSet ( __XProcFunctionArgs__ )
  {
    if ( args.size() != 1 )
      {
        throwException ( Exception, "Invalid number of arguments for node-set()\n" );
      }
    if ( args[0]->size() == 0 ) return;
    Log_EXSLT ( "[EXSLT:NODESET] : uri=%s\n", args[0]->front().toNode().getDocument().getDocumentURI().c_str() );
    
    if ( args[0]->front().toNode().getDocument().getDocumentURI().size() == 0 )
      {
        /*
         * Awfull hack for nodeset()
         */
        ElementRef rootElement = args[0]->front().toNode().getDocument().getRootElement();
        result.pushBack ( rootElement );
        return;
      }
    args[0]->copyTo ( result );
  }

  void EXSLTCommonModule::instructionDocument ( __XProcHandlerArgs__ )
  {
    // NamespaceId xslNsId = getKeyCache().getBuiltinKeys().xsl.ns();
    // XSLProcessor* xslProcessor = dynamic_cast<XSLProcessor*> ( getXProcessor().getModule ( xslNsId ) );
    XSLProcessor& xslProcessor = XSLProcessor::getMe(getXProcessor());
    xslProcessor.xslInstructionResultDocument ( item );
  }

  EXSLTDynamicModuleForge::EXSLTDynamicModuleForge ( Store& store )
  : XProcessorModuleForge ( store ), exslt_dynamic(store.getKeyCache())
  {
  
  }

  EXSLTDynamicModuleForge::~EXSLTDynamicModuleForge ( )
  {
  
  }

  void EXSLTDynamicModuleForge::instanciateModule ( XProcessor& xprocessor )
  {
    XProcessorModule* module = new EXSLTDynamicModule ( xprocessor, *this ); 
    xprocessor.registerModule ( module );    
  }
  
  void EXSLTDynamicModuleForge::install ()
  {
    registerFunction(exslt_dynamic.evaluate(), &EXSLTDynamicModule::functionEvaluate);
  }
  
  EXSLTDynamicModule::EXSLTDynamicModule ( XProcessor& xproc, EXSLTDynamicModuleForge& moduleForge )
  : XProcessorModule ( xproc, moduleForge ), exslt_dynamic(moduleForge.exslt_dynamic)
  {
  
  }
  
  EXSLTDynamicModule::~EXSLTDynamicModule () 
  {
	for ( DynamicXPathExpressionMap::iterator iter = dynamicXPathExpressionMap.begin () ;
		iter != dynamicXPathExpressionMap.end() ; iter++ )
	  {
		delete ( iter->second );
	  }
  }
  
  void EXSLTDynamicModule::install ()
  {

  }
  
  void EXSLTDynamicModule::functionEvaluate ( __XProcFunctionArgs__ )
  {
    if ( args.size() != 1 )
      {
        throwException ( Exception, "Invalid number of arguments for evaluate()\n" );
      }
    for ( NodeSet::iterator iter(*(args[0])) ; iter; iter++ )
      {
        switch ( iter->getItemType() )
        {
        case Item::Type_Attribute:
          {
            AttributeRef attrRef = iter->toAttribute ();
            ElementRef eltRef = attrRef.getElement ();
            XPath xpath ( getXProcessor(), eltRef, attrRef );
            xpath.eval ( result );
          }
          break;
        case Item::Type_String:
        case Item::Type_Element:
          {
            String expression = iter->toString ();
            DynamicXPathExpressionMap::iterator iter = dynamicXPathExpressionMap.find ( expression );
            if ( iter != dynamicXPathExpressionMap.end() )
              {
                iter->second->eval ( result );
              }
            else
              {
                Log_EXSLT ( "Compiling dynamic expression '%s'\n", expression.c_str() );
                NamespaceAlias nsAlias ( getKeyCache() );
                XPathParser* parser = new XPathParser ( getKeyCache(), nsAlias, expression );
                XPath* xpath = new XPath ( getXProcessor(), parser, true );
                String expr = stringFromAllocedStr ( strdup(expression.c_str()) );
                dynamicXPathExpressionMap[expr] = xpath;
                
                xpath->eval ( result );
              }
          }
          break;
        case Item::Type_Integer:
          result.pushBack ( iter->toInteger() );
          break;         
        default:
          throwException ( Exception, "Not implemented : ItemType=%x, value=%s\n", iter->getItemType(), iter->toString().c_str() );
        }
      }
  }
  
  
  EXSLTSetsModuleForge::EXSLTSetsModuleForge ( Store& store )
  : XProcessorModuleForge ( store ), exslt_sets(store.getKeyCache()) { }

  EXSLTSetsModuleForge::~EXSLTSetsModuleForge ( ) { }

  void EXSLTSetsModuleForge::instanciateModule ( XProcessor& xprocessor )
  {
    XProcessorModule* module = new EXSLTSetsModule ( xprocessor, *this ); 
    xprocessor.registerModule ( module );    
  }
  
  void EXSLTSetsModuleForge::install () {}
  
  EXSLTSetsModule::EXSLTSetsModule ( XProcessor& xproc, EXSLTSetsModuleForge& moduleForge )
  : XProcessorModule ( xproc, moduleForge ), exslt_sets(moduleForge.exslt_sets) {}
  
  EXSLTSetsModule::~EXSLTSetsModule () {}
  
  void EXSLTSetsModule::install ()
  {

  }

  void EXSLTSetsModule::functionLeading ( __XProcFunctionArgs__ )
  {
    if ( args.size() != 2 )
      {
        throwException ( Exception, "Invalid number of arguments for leading()\n" );
      }
    NotImplemented ( "exslt set function : leading\n" );
  }

  void EXSLTSetsModule::functionTrailing ( __XProcFunctionArgs__ )
  {
    Bug ( "." );
    if ( args.size() != 2 )
      {
        throwException ( Exception, "Invalid number of arguments for trailing()\n" );
      }
    NotImplemented ( "exslt set function : trailing\n" );
  }
  
};
