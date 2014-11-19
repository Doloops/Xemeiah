/*
 * xem-metaindexer.cpp
 *
 *  Created on: 9 nov. 2009
 *      Author: francois
 */

#include <Xemeiah/xemprocessor/xemprocessor.h>
#include <Xemeiah/xemprocessor/xem-metaindexer.h>
#include <Xemeiah/dom/documentmeta.h>
#include <Xemeiah/dom/childiterator.h>
#include <Xemeiah/dom/attributeref.h>
#include <Xemeiah/dom/elementmapref.h>
#include <Xemeiah/dom/qnamelistref.h>
#include <Xemeiah/xprocessor/xprocessor.h>
#include <Xemeiah/xpath/xpathparser.h>
#include <Xemeiah/kern/format/journal.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_DMI Debug
#define Warn_DMI Log

namespace Xem
{
  MetaIndexer::MetaIndexer( const ElementRef& element, XemProcessor& _xemProcessor )
  : ElementRef(element),xemProcessor(_xemProcessor),xem(xemProcessor.xem)
  {
    if ( *this )
      {
        AssertBug ( getKeyId() == getKeyCache().getBuiltinKeys().xemint.dom_event(), "Element is not a dom-event !\n" );
        AssertBug ( getAttrAsKeyId(xem.class_()) == xem.meta_indexer(), "Wrong class !\n" );
      }
  }


  void MetaIndexer::eval ( XProcessor& xproc, NodeSet& result, ElementRef& baseElement, NodeSet& args )
  {
    XPath useXPath ( xproc, *this, xem.use() );
    KeyId nameId = getAttrAsKeyId ( xem.name() );

    ElementMultiMapRef mapRef = baseElement.findAttr ( nameId, AttributeType_SKMap );
    if ( ! mapRef )
      {
        Log_DMI ( "Element '%s' : could not fetch SKMap ! No contents ?\n",
            baseElement.generateVersatileXPath().c_str() );
        return;
      }
    XPath::evalFunctionKeyGet ( mapRef, useXPath, result, args );

  }

  void MetaIndexer::build ( XProcessor& xproc )
  {
    XPath matchXPath(xproc, *this, getKeyCache().getBuiltinKeys().xemint.match());
    XPath useXPath(xproc, *this, xem.use());
    XPath scopeXPath(xproc, *this, xem.scope());

    ElementRef rootElement = getDocument().getRootElement ();

    ElementMultiMapRef nullMap(getDocument());
    KeyId nameId = getAttrAsKeyId(xem.name());
    matchXPath.evalFunctionKeyBuild ( rootElement, nullMap, matchXPath, useXPath, scopeXPath, nameId );
  }

  ElementMultiMapRef MetaIndexer::getScope ( XProcessor& xproc, NodeRef& base, bool create )
  {
    XPath scopeXPath ( xproc, *this, xem.scope() );
    KeyId mapId = getAttrAsKeyId (xem.name());

    NodeSet scopeNodeSet;
    scopeXPath.eval ( scopeNodeSet, base );

    if ( scopeNodeSet.size() == 1 && scopeNodeSet.front().isElement() )
      {
        ElementRef scopeElement = scopeNodeSet.toElement();
        ElementMultiMapRef currentMap = scopeElement.findAttr ( mapId, AttributeType_SKMap );
        if ( ! currentMap )
          {
            if ( create )
              {
                Log_DMI ( "New skMap at '%s'\n", scopeElement.generateVersatileXPath().c_str() );
                currentMap = scopeElement.addSKMap ( mapId, SKMapType_ElementMultiMap );
              }
          }
        Log_DMI ( "Inserting '%s'\n", base.generateVersatileXPath().c_str() );
        return currentMap;
      }
    Log_DMI ( "Silently ignoring '%s' : scope did not eval to an Element.\n",
        base.generateVersatileXPath().c_str() );
    return AttributeRef ( getDocument() );
  }

//  MetaIndexer XemProcessor::addMetaIndexer ( Document& doc, KeyId keyId,
//      XPathParser& matchXPathParser, XPathParser& useXPathParser, XPathParser& scopeXPathParser )
  MetaIndexer XemProcessor::addMetaIndexer ( Document& doc, ElementRef& declaration )
  {
    KeyId keyId = declaration.getAttrAsKeyId ( xem.name() );
    if ( declaration.getDefaultNamespaceId() )
      {
        NotImplemented ( "Not supported : meta indexer with a default namespace !\n" );
      }

    /*
     * Check that XPath attributes parse well
     */

    String matchExpression = declaration.getAttr(xem.match());
    String useExpression = declaration.getAttr(xem.use());
    String scopeExpression = declaration.getAttr(xem.scope());

    DocumentMeta docMeta = doc.getDocumentMeta();
    doc.grantWrite(getXProcessor());
#if 0
    DomEvent domEvent = docMeta.getDomEvents().registerEvent(
        useXPath,
        xem.meta_indexer_trigger());
#endif
    DomEvent domEvent = docMeta.getDomEvents().createDomEvent();
    domEvent.setEventMask(DomEventMask_Element|DomEventMask_Attribute);

    domEvent.copyNamespaceAliases(declaration);
    domEvent.setHandlerId(xem.meta_indexer_trigger());

    domEvent.setMatchXPath(getXProcessor(), useExpression);
    domEvent.addAttr(xem.use(), useExpression);
    domEvent.addAttr(xem.scope(), scopeExpression);

    domEvent.addNamespaceAlias(xem.defaultPrefix(), xem.ns());
    domEvent.addAttrAsQName ( xem.name(), keyId );
    domEvent.addAttrAsQName ( xem.class_(), xem.meta_indexer() );

    if ( declaration.hasAttr(xem.orderby()))
      {
        String orderByExpression = declaration.getAttr(xem.orderby());
        domEvent.addAttr(xem.orderby(),orderByExpression);
      }

#if 0
    matchXPathParser.saveToStore ( domEvent, xem.match() );
    domEvent.getDocument().appendJournal(domEvent, JournalOperation_UpdateAttribute, domEvent, xem.match());
    useXPathParser.saveToStore ( domEvent, xem.use() );
    domEvent.getDocument().appendJournal(domEvent, JournalOperation_UpdateAttribute, domEvent, xem.use());
    scopeXPathParser.saveToStore ( domEvent, xem.scope() );
    domEvent.getDocument().appendJournal(domEvent, JournalOperation_UpdateAttribute, domEvent, xem.scope());
#endif
    Log_DMI ( "----------- Building EventMap ....\n" );
    docMeta.getDomEvents().buildEventMap();
    return MetaIndexer(domEvent,*this);
  }

  void XemProcessor::xemInstructionSetMetaIndexer ( __XProcHandlerArgs__ )
  {
    XPath selectXPath ( getXProcessor(), item, xem.select() );
    ElementRef rootElement = selectXPath.evalElement ();
    Document& doc = rootElement.getDocument ();

#if 0
    KeyId keyId = item.getAttrAsKeyId ( xem.name() );

    XPathParser matchXPath ( item, xem.match() );
    XPathParser useXPath ( item, xem.use() );
    XPathParser scopeXPath ( item, xem.scope() );

    MetaIndexer metaIndexer = addMetaIndexer(doc, keyId, matchXPath, useXPath, scopeXPath );
#endif
    MetaIndexer metaIndexer = addMetaIndexer(doc, item );

    metaIndexer.build ( getXProcessor() );

    Log_DMI ( "Created metaIndexer at %s (elt=%llx)(doc=%p)\n",
        metaIndexer.generateVersatileXPath().c_str(),
        metaIndexer.getElementId(),
        &(metaIndexer.getDocument()) );
#if 0 // PARANOID
    KeyId classId = metaIndexer.getAttrAsKeyId ( xem.class_() );
    AssertBug ( classId == xem.meta_indexer(), "Wrong class !\n" );

    KeyId nameId = metaIndexer.getAttrAsKeyId ( xem.name() );
    AssertBug ( nameId == keyId, "Wrong name !\n" );
#endif
  }

  void XemProcessor::domMetaIndexerSort ( ElementRef elt, XPath& orderBy )
  {
    String myKey = orderBy.evalString(elt);
    Log_DMI ( "Sort using key value : '%s'\n", myKey.c_str() );
    if ( ! elt.getYounger() )
      {
        if ( ! elt.getElder() )
          {
            Log_DMI ( "Lonely child : no need to sort.\n" );
            return;
          }
        String elderKey = orderBy.evalString(elt.getElder());
        int res = myKey.compareTo(elderKey);
        Log_DMI ( "Elder key : '%s', res=%d\n", elderKey.c_str(), res );
        if ( res >= 0 )
          return;
      }
    ElementRef father = elt.getFather();
    elt.unlinkElementFromFather();
    for ( ChildIterator child(father) ; child ; child++ )
      {
        String childKey = orderBy.evalString(child);
        int res = myKey.compareTo(childKey);
        Log_DMI ( "Compare '%s' with '%s' => %d\n", myKey.c_str(), childKey.c_str(), res );
        if ( res < 0 )
          {
            Log_DMI ( "Insert here !\n" );
            child.insertBefore(elt);
            return;
          }
      }
    Bug ( "This is fatal : we shall not be here.\n" );
  }

  void XemProcessor::domMetaIndexerTrigger ( DomEventType domEventType, ElementRef& domEventElement, NodeRef& nodeRef )
  {
    MetaIndexer metaIndexer(domEventElement,*this);

    AssertBug ( metaIndexer, "DomEvent is not a metaIndexer !\n" );

    if ( domEventType == DomEventType_CreateAttribute
      || domEventType == DomEventType_AfterModifyAttribute )
      {
        ElementRef eltRef = nodeRef.getElement();

        XPath useXPath ( getXProcessor(), metaIndexer, xem.use() );

        ElementMultiMapRef currentMap = metaIndexer.getScope ( getXProcessor(), eltRef, true );
        if ( ! currentMap )
          return;
        currentMap.insert ( eltRef, useXPath );

        if ( metaIndexer.hasAttr(xem.orderby()))
          {
            Log_DMI ( "Sorting using expression %s\n", metaIndexer.getAttr(xem.orderby()).c_str() );
            XPath orderBy ( getXProcessor(), metaIndexer, xem.orderby() );
            domMetaIndexerSort ( eltRef, orderBy );
          }
      }
    else if ( domEventType == DomEventType_DeleteAttribute
           || domEventType == DomEventType_BeforeModifyAttribute )
      {
        ElementRef eltRef = nodeRef.getElement();

        XPath useXPath ( getXProcessor(), metaIndexer, xem.use() );

        ElementMultiMapRef currentMap = metaIndexer.getScope ( getXProcessor(), eltRef, true );
        if ( ! currentMap )
          {
            Bug ( "." );
            return;
          }
        currentMap.remove ( eltRef, useXPath );
      }
    else
      {
        Warn ( "NotImplemented : DomEventType='%s' on node='%s', domEvent='%s'\n",
            DomEventMask(domEventType).toString().c_str(),
            nodeRef.generateVersatileXPath().c_str(),
            domEventElement.generateVersatileXPath().c_str() );
      }
  }

  MetaIndexer XemProcessor::getMetaIndexer ( Document& doc, KeyId keyId )
  {
    if ( KeyCache::getNamespaceId(keyId) == 0 )
      {
        Log_DMI ( "Skipping non-namespaced meta-indexer %s (%x)\n", getKeyCache().dumpKey(keyId).c_str(), keyId );
        return MetaIndexer(ElementRef(doc),*this);
      }
    DomEvents events = doc.getDocumentMeta().getDomEvents();
    Log_DMI ( "Fetching Indexer from events : %s\n", events.generateVersatileXPath().c_str() );
    for ( ChildIterator child (events) ; child ; child ++ )
      {
        if ( ! child.findAttr(xem.class_(),AttributeType_KeyId)
            && ! child.findAttr(xem.class_(),AttributeType_String) )

          {
            Log_DMI ( "No class for %s\n", child.generateVersatileXPath().c_str() );
            continue;
          }
        Log_DMI ( "Class : %s\n", getKeyCache().dumpKey(child.getAttrAsKeyId(xem.class_())).c_str() );
        if ( child.getAttrAsKeyId(xem.class_()) != xem.meta_indexer() )
          continue;
        if ( child.getAttrAsKeyId(xem.name()) != keyId )
          continue;
        return MetaIndexer(child,*this);
      }
    Warn_DMI ( "Did not find meta indexer %s (%x)\n", getKeyCache().dumpKey(keyId).c_str(), keyId );
    return MetaIndexer(ElementRef(doc),*this);
  }

};
