#include <Xemeiah/xupdate/xupdateprocessor.h>

#include <Xemeiah/dom/nodeset.h>
#include <Xemeiah/xpath/xpath.h>

#include <Xemeiah/kern/exception.h>
#include <Xemeiah/xsl/xslprocessor.h>
#include <Xemeiah/nodeflow/nodeflow-dom.h>
#include <Xemeiah/kern/format/journal.h>

#include <Xemeiah/xprocessor/xprocessorlib.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_XUpdate Debug

namespace Xem
{
  __XProcessorLib_DECLARE_LIB ( XUpdateProcessor, "xupdate" );
  __XProcessorLib_REGISTER_MODULE ( XUpdateProcessor, XUpdateModuleForge );

#include <Xemeiah/kern/builtin_keys_prolog_inst.h>
#include <Xemeiah/xupdate/builtin-keys/xupdate>
#include <Xemeiah/kern/builtin_keys_postlog.h>

#define throwXUpdateException(...) throwException ( XUpdateException, __VA_ARGS__ )

  XUpdateModuleForge::XUpdateModuleForge ( Store& store ) 
  : XProcessorModuleForge ( store ), xupdate(store.getKeyCache())
  {}

  XUpdateProcessor::XUpdateProcessor ( XProcessor& xproc, XUpdateModuleForge& moduleForge ) 
  : XProcessorModule ( xproc, moduleForge ), xupdate(moduleForge.xupdate)
  {
  
  }


  void XUpdateProcessor::xupdateSetDocumentWritable ( ElementRef& item, Document& document )
  {
    Log_XUpdate ( "[XUP] Checking that document [%llx:%llx] is writable\n", _brid(document.getBranchRevId()) );
    /*
     * If the document is writable, there is no problem at all
     */
    if ( document.isWritable() && document.isLockedWrite() ) return;
    document.grantWrite ( getXProcessor() );
  }

  void XUpdateProcessor::xupdateSetDocumentWritable ( ElementRef& item, NodeRef& nodeRef )
  { 
    xupdateSetDocumentWritable ( item, nodeRef.getDocument() ); 
  }


  void XUpdateProcessor::xupdateInstructionModifications ( __XProcHandlerArgs__ )
  {
    getXProcessor().processChildren ( item );
  }

  void XUpdateProcessor::xupdateInstructionAppend ( __XProcHandlerArgs__ )
  {
    XPath xpath ( getXProcessor(), item, xupdate.select() );
    ElementRef baseElement = xpath.evalElement ( );
    
    
    if ( ! baseElement )
      {
        throwXUpdateException ( "Empty xpath result, could not process xupdate:append with xpath='%s'\n",
          item.getAttr ( xupdate.select() ).c_str() );
      }

    Log_XUpdate ( "XUpdate : baseElement='%s' (role=%s)\n", baseElement.generateVersatileXPath().c_str(), baseElement.getDocument().getRole().c_str() );

    xupdateSetDocumentWritable ( item, baseElement );
    
    NodeFlowDom nodeFlowDom ( getXProcessor(), baseElement );
    getXProcessor().setNodeFlow ( nodeFlowDom );

    getXProcessor().processChildren ( item );
  }

  ElementRef XUpdateProcessor::xupdateProcessElement ( ElementRef& elementInstruction, ElementRef& baseElement, JournalOperation journalOperation )
  {
    if ( journalOperation != JournalOperation_InsertBefore
      && journalOperation != JournalOperation_InsertAfter )
      {
        Bug ( "Invalid operation %d\n", journalOperation );
      }
    KeyId keyId = elementInstruction.getAttrAsKeyId ( getXProcessor(), xupdate.name() );
    ElementRef baseParent = baseElement.getFather ();
    ElementRef newChild = baseElement.getDocument().createElement ( baseParent, keyId );
    
    if ( journalOperation == JournalOperation_InsertBefore ) 
      baseElement.insertBefore ( newChild );
    else
      baseElement.insertAfter ( newChild );
    
    NodeFlowDom nodeFlowDom ( getXProcessor(), newChild );
    getXProcessor().setNodeFlow ( nodeFlowDom );
    
    getXProcessor().processChildren ( elementInstruction );

    return newChild;  
  }

  void XUpdateProcessor::xupdateInstructionInsertBeforeAfter ( __XProcHandlerArgs__ )
  {
    JournalOperation journalOperation = JournalOperation_NoOp;
    if ( item.getKeyId() == xupdate.insert_before() )
      journalOperation = JournalOperation_InsertBefore;
    else if ( item.getKeyId() == xupdate.insert_after() )
      journalOperation = JournalOperation_InsertAfter;
    else
      {
        Bug ( "Invalid op : '%s'\n", item.getKey().c_str() );
      }
    
    XPath xpath ( getXProcessor(), item, xupdate.select() );
    ElementRef baseElement = xpath.evalElement ( );
    if ( ! baseElement )
      {
        throwXUpdateException ( "Empty xpath result, could not process %s with xpath='%s'\n",
          item.getKey().c_str(), item.getAttr ( xupdate.select() ).c_str() );
      }
    xupdateSetDocumentWritable ( item, baseElement );
    for ( ElementRef childItem = item.getChild() ; childItem ; childItem = childItem.getYounger() )
      {
        if ( childItem.getKeyId() == xupdate.element() )
          {
            xupdateProcessElement ( childItem, baseElement, journalOperation );
            continue;
          }
        throwException ( XUpdateException, "Invalid child of xupdate:insert-before : '%s'\n",
          childItem.getKey().c_str() );
      }
  }

  void XUpdateProcessor::xupdateInstructionRename ( __XProcHandlerArgs__ )
  {
    XPath xpath ( getXProcessor(), item, xupdate.select() );
    ElementRef node = xpath.evalElement ( );
    if ( ! node )
      {
        throwXUpdateException ( "Could not select node !\n" );
      }
    xupdateSetDocumentWritable ( item, node );
#if 0
    String newName = XSLProcessor::evalChildrenAsString ( xproc, item, currentNode );
    KeyId keyId = xproc.getKeyCache().getKeyId ( xproc.getNamespaceAlias(),
        newName.c_str(), true );
    if ( ! keyId )
      {
        throwXUpdateException ( "Invalid key : '%s'\n", newName.c_str() );
      }
    node.rename ( keyId );
#endif
    NotImplemented ( "Node renaming !\n" );
  }

  void XUpdateProcessor::xupdateInstructionRemove ( __XProcHandlerArgs__ )
  {
    XPath xpath ( getXProcessor(), item, xupdate.select() );
    NodeSet nodes;
    xpath.eval ( nodes );    
    for ( NodeSet::iterator iter(nodes) ; iter ; iter++ )
      {
        if ( iter->isElement() )
          {
            ElementRef baseElement = iter->toElement();
            if ( ! baseElement )
              {
                throwXUpdateException ( "Could not select node !\n" );
              }

            xupdateSetDocumentWritable ( item, baseElement );
            baseElement.deleteElement ( getXProcessor() );
          }
        else if ( iter->isAttribute() )
          {
            AttributeRef baseAttribute = iter->toAttribute();
            if ( ! baseAttribute )
              {
                throwXUpdateException ( "Could not select node !\n" );
              }

            ElementRef baseElement = baseAttribute.getElement();
            xupdateSetDocumentWritable ( item, baseElement );
            baseElement.deleteAttr(baseAttribute.getKeyId());
          }
        else
          {
            throwXUpdateException ( "Invalid type for xupdate:remove\n" );
          }
      }
  }


  void XUpdateProcessor::xupdateInstructionUpdate ( __XProcHandlerArgs__ )
  {
    XPath xpath ( getXProcessor(), item, xupdate.select() );

    NodeSet result;
    xpath.eval ( result );
    if ( result.size() == 0 )
      {
        throwXUpdateException ( "XUpdate : empty result in update from item=%s", item.generateVersatileXPath().c_str() );  
#if 0
        Warn ( "[XUPDATE] : Empty result !\n" );
        String resStr = item.getAttr ( xupdate.select) );
        Log_XUpdate ( "Res : %s\n", resStr.c_str() );

        XPath xpath2 ( xproc.getKeyCache(), resStr, true );
        String res = xpath2.evalString ( xproc, currentNode );
        if ( ! res.size() )
          {
            throwXUpdateException ( "XUpdate selection is empty !\n" );
          }

        const char* g = res.c_str();
        if ( g[0] == '@' )
          {
            if ( ! currentNode.isElement() )
              {
                throwXUpdateException ( "XUpdate selection is not an element !\n" );
              }
            // Try to build up an attribute with this !
            KeyId keyId = xproc.getKeyCache().getKeyId ( xproc.getNamespaceAlias(),
                &(g[1]) , false );
            if ( ! keyId )
              {
                throwXUpdateException ( "XUpdate selection has an invalid attribute name : '%s'\n", &(g[1]) );
              }
            String contents 
              = XSLProcessor::evalChildrenAsString ( xproc, item, currentNode );
            currentNode.toElement().addAttr ( keyId, contents );
          }
        else
          {
            throwXUpdateException ( "XUpdate selection has an invalid XPath : '%s'\n",
                           g );
          }
#endif
      }
    for ( NodeSet::iterator iter(result) ; iter ; iter++ )
      {
        if ( ! iter->isNode() )
          {
            throwXUpdateException ( "XUpdate selection is not a Node : '%s'\n",
                           iter->toString().c_str() );
          }
        xupdateSetDocumentWritable ( item, iter->toNode() );
        if ( iter->isAttribute() )
          {
            String contents = getXProcessor().evalChildrenAsString ( item );
            AttributeRef attrRef = iter->toAttribute ();
            Log_XUpdate ( "[XUPDATE] update attribute '%s', contents '%s'\n",
              attrRef.getKey().c_str(), contents.c_str() );
            attrRef.setString ( contents );
          }
        else if ( iter->isElement() )
          {
            ElementRef elt = iter->toElement();
            if ( elt.isText() )
              {
                String contents = getXProcessor().evalChildrenAsString ( item );
                elt.setText ( contents );           
              }
            else if ( elt.getChild() && elt.getChild().isText() && ! elt.getChild().getYounger() )
              {
                // Special case, where element only contains a text element.
                elt = elt.getChild ();
                String contents = getXProcessor().evalChildrenAsString ( item );
                elt.setText ( contents );
              }
            else
              {
                NodeFlowDom nodeFlowDom ( getXProcessor(), elt );
                getXProcessor().setNodeFlow ( nodeFlowDom );
                getXProcessor().processChildren ( item );
              }
          }
      }
  }

  void XUpdateProcessor::xupdateInstructionElement ( __XProcHandlerArgs__ )
  {
    KeyId keyId = 0;
    try
      {
        keyId = item.getAttrAsKeyId ( getXProcessor(), xupdate.name() );
      }
    catch ( Exception* e )
      {
        delete ( e );
        keyId = 0;
      }
    if ( keyId == 0 )
      {
        String key = item.getEvaledAttr ( getXProcessor(), xupdate.name() );
        try
          {
            ElementRef currentModifiedElement = getNodeFlow().getCurrentElement();
            keyId = getXProcessor().getKeyCache().getKeyIdWithElement ( currentModifiedElement, key);
          }
        catch ( Exception* e )
          {
            detailException ( e, "xupdate:element : Could not parse name '%s'.\n", key.c_str() );
            throw ( e );
          }
      }
    getNodeFlow().newElement ( keyId, false );
    
    try
      {
        getXProcessor().processChildren ( item );
      }
    catch ( Exception * e )
      {
        getNodeFlow().elementEnd ( keyId );
        throw ( e );
      }
    getNodeFlow().elementEnd ( keyId );
  }

  void XUpdateProcessor::xupdateInstructionAttribute ( __XProcHandlerArgs__ )
  {
    Log_XUpdate ( "Called xupdate:attribute from '%s'\n", item.generateVersatileXPath().c_str() );
    getXProcessor().evalAttributeContent ( item, xupdate.name() );
  }

  void XUpdateProcessor::xupdateInstructionNotHandled ( __XProcHandlerArgs__ )
  {
    throwXUpdateException ( "XUpdate instruction not handled : %s\n", item.getKey().c_str() );
  }

  void XUpdateProcessor::install ()
  {
    defaultNSHandler = (XProcessorHandler) (&XUpdateProcessor::xupdateInstructionNotHandled);
  }

  void XUpdateModuleForge::install ()
  {
    Log_XUpdate ( "Installing XUPdateProcessor !\n" );
    
    registerHandler ( xupdate.modifications(), &XUpdateProcessor::xupdateInstructionModifications );
    registerHandler ( xupdate.append(), &XUpdateProcessor::xupdateInstructionAppend );
    registerHandler ( xupdate.insert_before(), &XUpdateProcessor::xupdateInstructionInsertBeforeAfter );
    registerHandler ( xupdate.insert_after(), &XUpdateProcessor::xupdateInstructionInsertBeforeAfter );
    registerHandler ( xupdate.rename(), &XUpdateProcessor::xupdateInstructionRename );
    registerHandler ( xupdate.remove(), &XUpdateProcessor::xupdateInstructionRemove );
    registerHandler ( xupdate.update(), &XUpdateProcessor::xupdateInstructionUpdate );
    registerHandler ( xupdate.element(), &XUpdateProcessor::xupdateInstructionElement );
    registerHandler ( xupdate.attribute(), &XUpdateProcessor::xupdateInstructionAttribute );
  }


  void XUpdateModuleForge::registerEvents(Document& doc)
  {
    registerXPathAttribute(doc, xupdate.select());
  }

};

