/*
 * xem-view.cpp
 *
 *  Created on: 6 nov. 2009
 *      Author: francois
 */

#include <Xemeiah/xemprocessor/xemprocessor.h>
#include <Xemeiah/xemprocessor/xem-view.h>
#include <Xemeiah/xpath/xpathparser.h>
#include <Xemeiah/dom/childiterator.h>
#include <Xemeiah/dom/elementmapref.h>
#include <Xemeiah/nodeflow/nodeflow-dom.h>

#include <Xemeiah/dom/descendantiterator.h>
#include <Xemeiah/kern/format/journal.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_XemView Debug
#define Warn_XemView Warn

namespace Xem
{
  /**
   * Simple hook to register created Elements
   */
  class NodeFlowXemView : public NodeFlowDom
  {
    ElementRef base;
    KeyId mapKeyId;
  public:

    NodeFlowXemView ( XProcessor& xproc, const ElementRef& elementRef, KeyId _mapKeyId, const ElementRef& _base )
    : NodeFlowDom(xproc,elementRef), base(_base)
    {
      mapKeyId = _mapKeyId;
    }
    virtual ~NodeFlowXemView () {}

    virtual void newElement ( KeyId keyId, bool forceDefaultNamespace );
  };

  void NodeFlowXemView::newElement ( KeyId keyId, bool forceDefaultNamespace )
  {
    ElementRef lastCurrent = currentElement;
    NodeFlowDom::newElement ( keyId, forceDefaultNamespace );
    AssertBug ( lastCurrent != currentElement, "Did not manage to create an element !\n" );

    ElementRef sourceElement = getXProcessor().getCurrentNode().toElement();
    ElementMultiMapRef multiMapRef = base.findAttr(mapKeyId, AttributeType_SKMap );
    if ( ! multiMapRef )
      {
        multiMapRef = base.addSKMap(mapKeyId, SKMapType_ElementMultiMap );
      }
    multiMapRef.put(sourceElement.getElementId(),currentElement);
  }

  XemView::XemView( const ElementRef& element, XemProcessor& _xemProcessor )
  : ElementRef(element),xemProcessor(_xemProcessor),xem(xemProcessor.xem)
  {
    if ( *this )
      {
        AssertBug ( getKeyId() == getKeyCache().getBuiltinKeys().xemint.dom_event(), "Element is not a dom-event !\n" );
        AssertBug ( getAttrAsKeyId(xem.class_()) == xem.xem_view(), "Wrong class !\n" );
      }
  }

  XemView XemProcessor::addXemView ( ElementRef& declaration, ElementRef& base )
  {
    DocumentMeta docMeta = base.getDocument().getDocumentMeta();
    DomEvent domEvent = docMeta.getDomEvents().createDomEvent();
    domEvent.setEventMask(DomEventMask_Element|DomEventMask_Attribute);

    domEvent.copyNamespaceAliases(declaration);
    domEvent.setHandlerId(xem.xem_view_trigger());
    domEvent.setMatchXPath(getXProcessor(), declaration.getAttr(xem.match()));

    KeyId lookupId = declaration.getAttrAsKeyId(getXProcessor(), xem.lookup());
    KeyId lookupNameId = declaration.getAttrAsKeyId(getXProcessor(), xem.lookup_name());

    if ( ! lookupId || ! lookupNameId )
      {
        throwException ( Exception, "Invalid lookup schemes, "
            "must provide xem:lookup() and xem:lookup-name() attributes, for xem:view at %s\n",
            declaration.generateVersatileXPath().c_str() );
      }

    domEvent.addNamespaceAlias ( xem.defaultPrefix(), xem.ns() );
    domEvent.addAttrAsQName ( xem.class_(), xem.xem_view() );
    domEvent.addAttrAsElementId ( xem.base_view(), base.getElementId() );

    for ( DescendantIterator descendant(declaration) ; descendant ; descendant++ )
      {
        Log_XemView ( "At %s\n", descendant.generateVersatileXPath().c_str() );
        for ( AttributeRef attr = descendant.getFirstAttr() ; attr ; attr = attr.getNext() )
          {
            if ( attr.getAttributeType() == AttributeType_String && attr.isAVT() )
              {
                Log_XemView ( "\tAVT : %s\n", attr.generateVersatileXPath().c_str() );
                XPath attrXPath(getXProcessor(), descendant, attr.getKeyId(), true );
                domEvent.addQNames(attrXPath);
              }
          }
      }
    domEvent.copyContents(getXProcessor(),declaration);

    Log_XemView ( "----------- Building EventMap ....\n" );
    docMeta.getDomEvents().buildEventMap();

    return XemView(domEvent,*this);
  }


  void XemProcessor::xemInstructionView ( __XProcHandlerArgs__ )
  {
    ElementRef baseNode = getXProcessor().getNodeFlow().getCurrentElement();

    Log_XemView ( "Building view for %s\n", baseNode.generateVersatileXPath().c_str() );
    addXemView ( item, baseNode );
  }


  bool XemView::controlScope ( XProcessor& xproc, NodeRef& base )
  {
    XPath scopeXPath ( xproc, *this, xem.scope() );
    ElementRef scope = scopeXPath.evalElement(base);
    Log_XemView ( "scope=%s\n", scope.generateVersatileXPath().c_str() );
    for ( ElementRef father = base.getElement() ; father ; father = father.getFather() )
      {
      Log_XemView ( "at=%s\n", father.generateVersatileXPath().c_str() );
        if ( scope == father )
          return true;
      }
    return false;
  }

  void XemProcessor::xemViewDoInsert ( ElementRef& base, ElementRef target, ElementRef modelFather, KeyId lookupId, KeyId lookupNameId )
  {
    for ( ChildIterator model(modelFather) ; model ; model++ )
      {
        String key = model.getEvaledAttr(getXProcessor(), lookupNameId);
        Log_XemView ( "Got key : '%s' (from %s)\n", key.c_str(), model.getAttr(lookupNameId).c_str() );

        ElementRef targetChild = target.lookup(getXProcessor(), lookupId, key);

        if ( ! targetChild )
          {
            Log_XemView ( "No child, creating...\n");
            NodeFlowXemView nodeFlow(getXProcessor(),target, xem.created_elements_map(), base);
            getXProcessor().setNodeFlow(nodeFlow);
            getXProcessor().process(model);
          }
        else
          {
            ElementMultiMapRef multiMapRef = base.findAttr(xem.created_elements_map(), AttributeType_SKMap );
            multiMapRef.put(getCurrentNode().toElement().getElementId(),targetChild);
            xemViewDoInsert ( base, targetChild, model, lookupId, lookupNameId );
          }
      }
  }

  void XemProcessor::xemViewInsert ( ElementRef& base, const ElementRef& source_, ElementRef& xemView, KeyId lookupId, KeyId lookupNameId )
  {
    ElementRef source = source_;
    NodeSet nodeSet; nodeSet.pushBack(source);
    NodeSet::iterator iter(nodeSet,getXProcessor());

    xemViewDoInsert(base, base, xemView, lookupId, lookupNameId );
  }


  void XemProcessor::xemViewRemove ( ElementRef& base, const ElementRef& source_ )
  {
    ElementRef source = source_;

    ElementMultiMapRef multiMapRef = base.findAttr(xem.created_elements_map(), AttributeType_SKMap );
    if ( ! multiMapRef )
      {
        Log_XemView ( "[XEMVIEWDELETE] Nothing created !\n" );
        return;
      }
    std::list<ElementRef> destroyList;
    for ( ElementMultiMapRef::multi_iterator iter(multiMapRef,source.getElementId()) ;
         iter ; iter++ )
      {
        ElementRef elt = multiMapRef.get(iter);
        destroyList.push_back(elt);
      }
    if ( destroyList.empty() )
      {
        Log_XemView ( "[XEMVIEWDELETE] Nothing created for %s\n", source.generateVersatileXPath().c_str() );
        return;
      }
    multiMapRef.remove(source.getElementId());

    for ( std::list<ElementRef>::iterator iter = destroyList.begin() ; iter != destroyList.end() ; )
      {
        ElementRef& elt = *iter;
        Log_XemView ( "[XEMVIEWDELETE] Removing elt='%s'\n", elt.generateVersatileXPath().c_str() );
        if ( elt.getChild() )
          {
            Log_XemView ( "[XEMVIEWDELETE] Skipping : still has children !\n" );
            iter++;
            continue;
          }
        Log_XemView ( "[XEMVIEWDELETE] Deleting !\n" );
        elt.deleteElement(getXProcessor());
        destroyList.erase(iter);
        /*
         * Restart the iterator from the beginning
         */
        iter = destroyList.begin();
      }
    Log_XemView ( "[XEMVIEWDELETE] After iterating : remains %lu elements in destroyList\n", (unsigned long) destroyList.size() );
  }

  void XemProcessor::domXemViewTrigger ( DomEventType domEventType, ElementRef& domEventElement, NodeRef& nodeRef )
  {
#if PARANOID
    XPath matchXPath(getXProcessor(),domEventElement,getKeyCache().getBuiltinKeys().xemint.match());
    AssertBug (  matchXPath.matches(nodeRef), "XPath does not match nodeRef=%s!\n", nodeRef.generateVersatileXPath().c_str() );
#endif
    XemView xemView(domEventElement,*this);

    Document& currentDoc = nodeRef.getDocument();
    ElementId baseId = xemView.getAttrAsElementId(xem.base_view());
    ElementRef base = currentDoc.getElementById(baseId);

    if ( !base )
      {
        throwException ( Exception, "Could not resolve baseId %llx\n", baseId );
      }

    if ( ! xemView.controlScope(getXProcessor(),nodeRef) )
      {
        Log_XemView ( "currentNode not part of the scopeXPath.\n" );
        return;
      }

    KeyId lookupId = xemView.getAttrAsKeyId(xem.lookup());
    KeyId lookupNameId = xemView.getAttrAsKeyId(xem.lookup_name());
    Log_XemView ( "Event : domEventType=%d, domEventElement=%s, nodeRef=%s, lookup=%s, lookupName=%s\n",
        domEventType, domEventElement.generateVersatileXPath().c_str(),
        nodeRef.generateVersatileXPath().c_str(),
        getKeyCache().dumpKey(lookupId).c_str(),
        getKeyCache().dumpKey(lookupNameId).c_str() );

    ElementRef sourceElement = nodeRef.getElement();

    switch ( domEventType )
    {
      /*
       * These events can reshape the view : remove, then insert
       */
    case DomEventType_CreateElement:
    case DomEventType_CreateAttribute:
    case DomEventType_DeleteAttribute:
      xemViewRemove ( base, sourceElement );
      xemViewInsert ( base, sourceElement, xemView, lookupId, lookupNameId );
      break;

      /*
       * After modify will be called after a BeforeModify : so insert only
       */
    case DomEventType_AfterModifyAttribute:
      xemViewInsert ( base, sourceElement, xemView, lookupId, lookupNameId );
      break;

      /*
       * DeleteElement : only remove. Before : only remove, AfterModify will insert.
       */
    case DomEventType_BeforeModifyAttribute:
    case DomEventType_DeleteElement:
      xemViewRemove ( base, sourceElement );
      break;
    default:
      {
        Warn_XemView ( "Skip event %s : domEventType=%d, domEventElement=%s, nodeRef=%s, lookup=%s, lookupName=%s\n",
            DomEventMask(domEventType).toString().c_str(),
            domEventType, domEventElement.generateVersatileXPath().c_str(),
            nodeRef.generateVersatileXPath().c_str(),
            getKeyCache().dumpKey(lookupId).c_str(),
            getKeyCache().dumpKey(lookupNameId).c_str() );
        break;
      }
    }
  }
};
