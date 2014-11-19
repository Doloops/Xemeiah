#include <Xemeiah/xemprocessor/xemprocessor.h>

#include <Xemeiah/kern/subdocument.h>
#include <Xemeiah/dom/documentmeta.h>
#include <Xemeiah/xprocessor/xprocessor.h>
#include <Xemeiah/xpath/xpath.h>
#include <Xemeiah/xpath/xpathparser.h>
#include <Xemeiah/xsl/xslprocessor.h>
#include <Xemeiah/xupdate/xupdateprocessor.h>
#include <Xemeiah/nodeflow/nodeflow-stream.h>
#include <Xemeiah/xprocessor/xprocessorlibs.h>


#include <Xemeiah/version.h>

#include <Xemeiah/auto-inline.hpp>

#include <time.h>

#define Log_XemCommon Debug
#define Log_XemCommon_LoadLib Debug

namespace Xem
{
#if 0
  static void xemInstructionSetDocument ( __XProcHandlerArgs__ )
  {
    if ( ! item.hasAttr(xem.href() ) )
      {
	      throwXemProcessorException ( "Instruction xem:set-document has no attribute xem:href\n" );
      }
    if ( ! item.hasAttr(xem.select() ) ) 
      {
        throwXemProcessorException ( "Instruction xem:set-document has no attribute xem:select\n" );
      }
    String href = item.getEvaledAttr ( xproc, currentNode, xem.href() );
    XPath selectXPath ( item, xem.select() );
    NodeSet document;
    selectXPath.eval ( xproc, document, currentNode );
    if ( ! document.isScalar() )
      {
        throwXemProcessorException ( "Instruction xem:set-document : xem:select '%s' returns a non-scalar result !\n",
            item.getAttr (xem.select() ).c_str() );
      }
    if ( ! document.front().isElement() )
      {
        throwXemProcessorException ( "Instruction xem:set-document : xem:select '%s' returns a non-element result !\n",
            item.getAttr (xem.select() ).c_str() );
      }
    ElementRef rootDocument = document.front().toElement();
    xproc.setDocument ( href, rootDocument );
  }

  static void xemInstructionReleaseDocument ( __XProcHandlerArgs__ )
  {
    return;
    
    String documentName = item.getEvaledAttr ( xproc, currentNode, xem.href() );
    xproc.releaseDocument ( documentName );
    
    Log_XemCommon ( "[xem:release-document] Releasing document '%s'\n", documentName.c_str() );
  }
#endif

  void XemProcessor::xemInstructionDefault ( __XProcHandlerArgs__ )
  {
    if ( hasCurrentCodeScope() && item.getNamespaceId() )
      {
        ElementRef codeScope = getCurrentCodeScope();
        ElementMapRef constructorMap = codeScope.findAttr(xem.constructor_map(),
            AttributeType_SKMap);

        KeyId classId = getElementClass(item); // item.getKeyId()item.getKeyId()
        ElementRef constructor = constructorMap.get(classId);
        if ( constructor )
          {
            Log_XemCommon ( "Found constructor at %s\n", constructor.generateVersatileXPath().c_str() );
            xemInstructionInstance(item);
            return;
          }
      }
    Log_XemCommon ( "Called default instruction for %s (%x)\n", item.getKey().c_str(), item.getKeyId() );
    XSLProcessor& xslProcessor = XSLProcessor::getMe(getXProcessor());
    xslProcessor.xslInstructionDefault(item);
  }

  void XemProcessor::xemInstructionHousewife ( __XProcHandlerArgs__ )
  {
    item.getDocument().housewife ();
    getCurrentNode().getDocument().housewife ();
  }

  void XemProcessor::xemInstructionException ( __XProcHandlerArgs__ )
  {
    String message = getXProcessor().evalChildrenAsString ( item );
    throwException ( RuntimeException, "%s", message.c_str() );
  }

  void XemProcessor::xemInstructionCatchExceptions ( __XProcHandlerArgs__ )
  {
    for ( ElementRef tryChild = item.getChild() ; tryChild ; tryChild = tryChild.getYounger() )
      {
      	if ( tryChild.getKeyId() != xem.try_() )
          continue;
        try
          {
            getXProcessor().processChildren ( tryChild );
          }
        catch ( Exception* e )
          {
            Warn ( "Got exception '%s'\n", e->getMessage().c_str() );
            
            for ( ElementRef catchChild = item.getChild() ; catchChild ; catchChild = catchChild.getYounger() )
              {
                if ( catchChild.getKeyId() == xem.catch_() )
                  {
                    getXProcessor().setString ( xem.exception(), e->getMessage() );
                    getXProcessor().processChildren ( catchChild );
                  }
              }
            delete ( e );
          }
        break;
      }
  }

      
  void XemProcessor::xemInstructionNotHandled ( __XProcHandlerArgs__ )
  {
    throwException ( RuntimeException, "Xem instruction not handled : %s\n", item.getKey().c_str() );
  }
  
  void XemProcessor::xemFunctionGetVersion ( __XProcFunctionArgs__ )
  {
    String version = __XEM_VERSION;
    result.setSingleton ( version );
  }
  
  void XemProcessor::xemFunctionRand ( __XProcFunctionArgs__ )
  {
    result.setSingleton ( (Integer) rand() );
  }

  void XemProcessor::xemFunctionVariable ( __XProcFunctionArgs__ )
  {
    if ( args.size() != 1 )
      {
        throwXemProcessorException("Invalid number of arguments for xem:variable() !");
      }
    NodeSet* ns = args[0];
    if ( ns->size() != 1 )
      {
        throwXemProcessorException("Invalid number of nodes for first argument of xem:variable() !");
      }
    if ( ! ns->front().isAttribute() )
      {
        throwXemProcessorException("Argument of xem:variable() is not an attribute !");
      }
    AttributeRef attrRef = ns->front().toAttribute();
    KeyId keyId = attrRef.getElement().getAttrAsKeyId(getXProcessor(),attrRef.getKeyId());
    Log_XemCommon ( "Selected attr=%x\n", keyId );
    NodeSet* variable = getXProcessor().getVariable(keyId);
    variable->copyTo(result);
  }

  void XemProcessor::xemFunctionGetCurrentTime ( __XProcFunctionArgs__ )
  {
    Bug ( "NOT IMPLEMENTED.\n" );
    // result.setSingleton ( XemFSModule::timeToRFC1123Time ( time(NULL) ) );
  }

  void XemProcessor::xemInstructionLoadExternalModule ( __XProcHandlerArgs__ )
  {
    String libName = item.getEvaledAttr ( getXProcessor(), xem.name() );
    
    Log_XemCommon_LoadLib ( "Trying to load library : '%s'\n", libName.c_str() );

    try
      {
      	getXProcessor().loadLibrary ( libName );
      }
    catch ( Exception* e )
      {
        detailException ( e, "Could not load library : '%s'\n", libName.c_str() );
        throw ( e );
      }
    Log_XemCommon_LoadLib ( "Library '%s' loaded !\n", libName.c_str() );
  }

};

