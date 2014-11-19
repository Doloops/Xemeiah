#include <Xemeiah/xemprocessor/xemprocessor.h>
#include <Xemeiah/dom/documentmeta.h>

#include <Xemeiah/xprocessor/xprocessor.h>
#include <Xemeiah/xpath/xpath.h>

#include <Xemeiah/kern/branchmanager.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_XemRoleBased Debug

namespace Xem
{
  void XemProcessor::xemInstructionOpenDocument ( __XProcHandlerArgs__ )
  {
    KeyId roleKeyId = item.getAttrAsKeyId(getXProcessor(), xem.role());
    if ( KeyCache::getNamespaceId(roleKeyId) )
      {
        throwException ( Exception, "Invalid namespace role name at '%s'\n", item.generateVersatileXPath().c_str() );
      }
    RoleId roleId = KeyCache::getLocalKeyId(roleKeyId);

    Log_XemRoleBased ( "Role : %x (%s)\n", roleId, getKeyCache().dumpKey(roleId).c_str() );

    if ( !item.hasAttr(xem.branch() ) )
      {
        throwException(Exception, "No attribute 'branch' defined at '%s'\n", item.generateVersatileXPath().c_str() );
      }
    String branchName = item.getEvaledAttr(getXProcessor(), xem.branch() );

    if ( !item.hasAttr(xem.mode() ) )
      {
        throwException(Exception, "No attribute 'mode' defined at '%s'\n", item.generateVersatileXPath().c_str() );
      }
    String openMode = item.getEvaledAttr(getXProcessor(), xem.mode() );

    BranchManager& branchManager = getXProcessor().getStore().getBranchManager();

    Document* document = branchManager.openDocument ( branchName, openMode, roleId );

    if ( ! document )
      {
        throwException ( Exception, "Could not open persistentDocument with branchName='%s'\n", branchName.c_str() );
      }
    document->incrementRefCount();
    if ( openMode == "as-revision" && document->isWritable() && ! document->isLockedWrite() )
      {
        document->lockWrite();
        if ( ! document->isWritable() )
          {
            Warn ( "After BranchManager::openDocument(), locked write, but document is no more writable !\n" );
            /*
             * This document may be corrupted : open it again with a DocumentOpeningFlags_Read
             */
            BranchRevId brId = document->getBranchRevId();
            Store& store = getXProcessor().getStore();
            store.releaseDocument(document);
            document = branchManager.openDocument(brId.branchId, brId.revisionId, DocumentOpeningFlags_Read, roleId );
            document->incrementRefCount();
          }
      }

    if ( document->isLockedWrite() && !document->getDocumentMeta().getDomEvents().getChild() )
      {
        Log_XemRoleBased ( "Setting default Events for [%llx:%llx] (branch %s, openMode=%s)\n",
            _brid(document->getBranchRevId()), branchName.c_str(), openMode.c_str() );
        getXProcessor().registerEvents(*document);
      }

    if ( document->getRoleId() != roleId )
      {
        throwException(Exception, "Invalid role set for document : %s (%x), setting %s (%x)\n",
            document->getRole().c_str(), document->getRoleId(),
            getKeyCache().dumpKey(roleId).c_str(), roleId );
      }

    Log_XemRoleBased ( "Openning role '%s' (%x), branchName '%s' rev=[%llu:%llu]\n",
      getKeyCache().dumpKey(roleId).c_str(), roleId,
      branchName.c_str(),
      _brid(document->getBranchRevId()) );

    getXProcessor().bindDocument ( document, true );

    document->decrementRefCount(); // Because we already incremented refcount.

    KeyId varKeyId = getKeyCache().getKeyId(xem_role.ns(), roleId);
    ElementRef rootElement = document->getRootElement();
    getXProcessor().setElement(varKeyId, rootElement, true);
  }

  ElementRef XemProcessor::resolveElementWithRole ( const String& nodeIdStr, KeyId& attributeKeyId )
  {
    String role;
    ElementId elementId;
    if ( ! NodeRef::parseNodeId ( nodeIdStr, role, elementId, attributeKeyId ) )
      {
        throwException ( Exception, "Invalid NodeId Format '%s'\n", nodeIdStr.c_str() );
      }
    KeyId fullKeyId = getKeyCache().getKeyId ( xem_role.ns(), role.c_str(), true );
    ElementRef rootElement = getXProcessor().getVariable(fullKeyId)->front().toElement();
    ElementRef resolvedElement = rootElement.getDocument().getElementById ( elementId );
    
    if ( ! resolvedElement )
      {
        throwException ( Exception, "Could not resolve element from '%s'\n", nodeIdStr.c_str() );
      }
    return resolvedElement;  
  }

  void XemProcessor::resolveNodeAndPushToNodeSet ( NodeSet& result, const String& nodeIdStr )
  {
    KeyId attributeKeyId = 0;
    ElementRef resolvedElement = resolveElementWithRole ( nodeIdStr, attributeKeyId );
    if ( !resolvedElement )
      {
        throwException ( Exception, "Could not resolve element fro '%s'.\n", nodeIdStr.c_str() );
      }
    if ( attributeKeyId )
      {
        AttributeRef attrRef = resolvedElement.findAttr ( attributeKeyId, AttributeType_String );
        if ( ! attrRef )
          {
            throwException ( Exception, "Could not get attr '%x' in element %s from '%s'\n", attributeKeyId,
                resolvedElement.generateVersatileXPath().c_str(), nodeIdStr.c_str() );
          }
        Log_XemRoleBased ( "Pushing attribute '%s'\n", attrRef.generateVersatileXPath().c_str() );
        result.pushBack ( attrRef );
      }
    else
      {
        Log_XemRoleBased ( "Pushing element '%s'\n", resolvedElement.generateVersatileXPath().c_str() );
        result.pushBack ( resolvedElement );
      }
  }

  
  void XemProcessor::xemFunctionTransmittable ( __XProcFunctionArgs__ )
  {
    NodeSet& res0 = *(args[0]);
    bool separator = false;
    String trans;
    for ( NodeSet::iterator iter0 ( res0 ) ; iter0 ; iter0++ )
    {
    	if ( separator ) trans += ";";
    	Item& item = *iter0;
    	switch ( item.getItemType() )
    	{
    	case Item::Type_Null:
    	  Bug ( "Null Item !\n" );
    	  break;
    	case Item::Type_Element:
    	case Item::Type_Attribute:
    	  trans += item.toNode().generateId ();
    	  break;
    	case Item::Type_Bool:
    	case Item::Type_Integer:
    	case Item::Type_Number:
    	case Item::Type_String:
    	  trans += item.toString();
    	  break;
    	
    	}
    }    
    result.setSingleton ( trans );
    Log_XemRoleBased ( "Transmittable : returned '%s'\n", trans.c_str() );
  }  
  
  void XemProcessor::xemFunctionGetNode ( __XProcFunctionArgs__ )
  {
    String arg0 = args[0]->toString ();
    resolveNodeAndPushToNodeSet ( result, arg0.c_str() );
  }  
};
