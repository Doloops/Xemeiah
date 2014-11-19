/*
 * xemintmodule.cpp
 *
 *  Created on: 8 janv. 2010
 *      Author: francois
 */

#include <Xemeiah/xprocessor/xemintmodule.h>
#include <Xemeiah/dom/domeventmask.h>
#include <Xemeiah/dom/qnamelistref.h>
#include <Xemeiah/dom/namespacelistref.h>
#include <Xemeiah/xpath/xpath.h>
#include <Xemeiah/xpath/xpathparser.h>
#include <Xemeiah/xprocessor/xprocessorlib.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_XemInt Debug

namespace Xem
{
  __XProcessorLib_REGISTER_MODULE ( XProcessorDefaultModules, XemIntModuleForge );

  void XemIntModule::domEventXPath ( DomEventType domEventType, ElementRef& domEventElement, NodeRef& nodeRef )
  {
#if PARANOID
    AssertBug ( nodeRef.isAttribute(), "NodeRef is not an attribute !!!\n" );
#endif
    AttributeRef attrRef = nodeRef.toAttribute ();

    Log_XemInt ( "[DOMEVENTACTION] Parsing '%s' (type=%d) as XPath.\n", attrRef.getKey().c_str(),
        attrRef.getAttributeType() );
    if ( attrRef.getAttributeType() != AttributeType_String )
      {
        Log_XemInt ( "Skipping non-String type %x\n", attrRef.getAttributeType() );
        return;
      }
    if ( domEventType == DomEventType_CreateAttribute || domEventType == DomEventType_AfterModifyAttribute )
      {
        ElementRef eltRef = attrRef.getElement();
        XPathParser xpathParser ( eltRef, attrRef.getKeyId(), false );
        xpathParser.saveToStore ( eltRef, attrRef.getKeyId() );
      }
    else
      {
        NotImplemented ( "DomEventType %s\n", DomEventMask(domEventType).toString().c_str() );
      }
  }

  void XemIntModule::domEventQName ( DomEventType domEventType, ElementRef& domEventElement, NodeRef& nodeRef )
  {
    AttributeRef attrRef = nodeRef.toAttribute();
    if ( attrRef.getAttributeType() != AttributeType_String )
      {
        Log_XemInt ( "Skipping non-String type %x\n", attrRef.getAttributeType() );
        return;
      }
    if ( domEventType == DomEventType_CreateAttribute || domEventType == DomEventType_AfterModifyAttribute )
      {
        if ( strchr(attrRef.toString().c_str(), '{') ) return;

        ElementRef eltRef = attrRef.getElement();
        Log_XemInt ( "[DOMEVENTACTION] Parse QName on '%s'='%s'\n",
          attrRef.generateVersatileXPath().c_str(), attrRef.toString().c_str() );
        try
          {
            KeyId keyId = getKeyCache().getKeyIdWithElement ( eltRef, attrRef.toString(), false );
            eltRef.addAttrAsKeyId ( attrRef.getKeyId(), keyId );
          }
        catch ( Exception* e )
          {
            Warn ( "Could not parse QName on '%s'='%s' : %s\n",
              attrRef.generateVersatileXPath().c_str(), attrRef.toString().c_str(), e->getMessage().c_str() );
            delete ( e );
          }
      }
    else
      {
        NotImplemented ( "DomEventType %s\n", DomEventMask(domEventType).toString().c_str() );
      }
  }

  void XemIntModule::domEventQNameList ( DomEventType domEventType, ElementRef& domEventElement, NodeRef& nodeRef )
  {
    AttributeRef attrRef = nodeRef.toAttribute();

    if ( attrRef.getAttributeType() != AttributeType_String )
      {
        Log_XemInt ( "Skipping non-String type %x\n", attrRef.getAttributeType() );
        return;
      }
    if ( domEventType == DomEventType_CreateAttribute || domEventType == DomEventType_AfterModifyAttribute )
      {
        /*
         * Brute-force : delete old attr, add new one
         */
        ElementRef eltRef = attrRef.getElement();
        QNameListRef qnameList = eltRef.findAttr(attrRef.getKeyId(), AttributeType_QNameList);
        if ( qnameList )
          eltRef.deleteAttr(attrRef.getKeyId(), AttributeType_QNameList);
        qnameList = eltRef.getAttrAsQNameList(attrRef.getKeyId());
      }
    else
      {
        NotImplemented ( "DomEventType %s\n", DomEventMask(domEventType).toString().c_str() );
      }
  }

  void XemIntModule::domEventNamespaceList ( DomEventType domEventType, ElementRef& domEventElement, NodeRef& nodeRef )
  {
    AttributeRef attrRef = nodeRef.toAttribute();
    if ( attrRef.getAttributeType() != AttributeType_String )
      {
        Log_XemInt ( "Skipping non-String type %x\n", attrRef.getAttributeType() );
        return;
      }
    if ( domEventType == DomEventType_CreateAttribute || domEventType == DomEventType_AfterModifyAttribute )
      {
        Log_XemInt ( "DomEventType %s\n", DomEventMask(domEventType).toString().c_str() );
        Log_XemInt ( "NSList : %s\n", attrRef.toString().c_str() );
        /*
         * Brute-force : delete old attr, add new one
         */
        ElementRef eltRef = attrRef.getElement();
        KeyId targetId = domEventElement.getAttrAsKeyId(getKeyCache().getBuiltinKeys().xemint.target());
        Log_XemInt ( "NSList (1) : %s\n", eltRef.getAttr(attrRef.getKeyId()).c_str() );
        NamespaceListRef nsList = eltRef.findAttr(targetId, AttributeType_NamespaceList);
        if ( nsList )
          eltRef.deleteAttr(targetId, AttributeType_NamespaceList);
        nsList = eltRef.getAttrAsNamespaceList(targetId, attrRef);

        Log_XemInt ( "Built : %s\n", getKeyCache().dumpKey(nsList.getKeyId()).c_str() );
      }
    else if ( domEventType == DomEventType_DeleteAttribute || domEventType == DomEventType_BeforeModifyAttribute )
      {
        ElementRef eltRef = attrRef.getElement();
        KeyId targetId = domEventElement.getAttrAsKeyId(getKeyCache().getBuiltinKeys().xemint.target());
        eltRef.deleteAttr(targetId, AttributeType_NamespaceList);
      }
    else
      {
        NotImplemented ( "DomEventType %s\n", DomEventMask(domEventType).toString().c_str() );
      }
  }

  void XemIntModuleForge::install ()
  {
    registerDomEventHandler(getStore().getKeyCache().getBuiltinKeys().xemint.domevent_xpath_attribute(), &XemIntModule::domEventXPath );
    registerDomEventHandler(getStore().getKeyCache().getBuiltinKeys().xemint.domevent_qname_attribute(), &XemIntModule::domEventQName );

    registerDomEventHandler(getStore().getKeyCache().getBuiltinKeys().xemint.domevent_qnamelist_attribute(), &XemIntModule::domEventQNameList );
    registerDomEventHandler(getStore().getKeyCache().getBuiltinKeys().xemint.domevent_namespacelist_attribute(), &XemIntModule::domEventNamespaceList );

  }
};


