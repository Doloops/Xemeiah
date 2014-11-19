#if 0

#include <Xemeiah/xemprocessor/xemprocessor.h>
#include <Xemeiah/xprocessor/xprocessor.h>
#include <Xemeiah/kern/store.h>

#include <Xemeiah/webserver/nodeflow-http.h>

#include <Xemeiah/webserver/webserver.h>

#include <Xemeiah/trace.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_WSI Debug

namespace Xem
{

  static void wsi_forAllKeys ( void* arg, LocalKeyId localKeyId, const char* keyName )
  {
    XProcessor& xproc = *((XProcessor*)arg); NodeFlow& nodeFlow = xproc.getNodeFlow ();
  
    Log_WSI ( "For all keys : %x -> '%s'\n", localKeyId, keyName );
    nodeFlow.newElement ( __builtinKey(xemwsi_key) );
    String sId;
    stringPrintf ( sId, "0x%x", localKeyId );
    nodeFlow.newAttribute ( __builtinKey(xemwsi_id), sId );
    
    String value (keyName);
    nodeFlow.newAttribute ( __builtinKey(xemwsi_value), value );
    nodeFlow.elementEnd ( __builtinKey(xemwsi_key) );
  }
  
  static void wsi_forAllNamespaces ( void* arg, NamespaceId nsId, const char* url )
  {
    XProcessor& xproc = *((XProcessor*)arg); NodeFlow& nodeFlow = xproc.getNodeFlow ();
    
    Log_WSI ( "For all namespaces : %x -> '%s'\n", nsId, url );

    nodeFlow.newElement ( __builtinKey(xemwsi_namespace) );
    String sId;
    stringPrintf ( sId, "0x%x", nsId );
    nodeFlow.newAttribute ( __builtinKey(xemwsi_id), sId );
    
    String value (url);
    nodeFlow.newAttribute ( __builtinKey(xemwsi_value), value );
    nodeFlow.elementEnd ( __builtinKey(xemwsi_namespace) );
  }
  

  static void xemwsiInstruction_bootstrap ( __XProcHandlerArgs__ )
  {
    Warn ( "Processing bootstrap.\n" );
  
    NodeFlow& nodeFlow = xproc.getNodeFlow ();

    nodeFlow.newElement ( __builtinKey(xemwsi_bootstrap) );
    nodeFlow.setNamespacePrefix ( __builtinKey(xmlns_xemwsi), xproc.getKeyCache().builtinKeys.namespace_xemwsi(), false );
    
    nodeFlow.newElement ( __builtinKey(xemwsi_keys) );
    
    xproc.getKeyCache().forAllKeys ( &wsi_forAllKeys, (void*) &xproc );
    
    nodeFlow.elementEnd ( __builtinKey(xemwsi_keys) );

    nodeFlow.newElement ( __builtinKey(xemwsi_namespaces) );

    xproc.getKeyCache().forAllNamespaces ( &wsi_forAllNamespaces, (void*) &xproc );

    nodeFlow.elementEnd ( __builtinKey(xemwsi_namespaces) );
        
    nodeFlow.elementEnd ( __builtinKey(xemwsi_bootstrap) );    
  }

  void xemwsiInstruction_serializeRevisionHead ( __XProcHandlerArgs__ )
  {
    XPath docXPath ( item, __builtinKey(xemwsi_document), false );
    ElementRef docElement = docXPath.evalElement ( xproc, currentNode );
    if ( ! docElement ) 
      {
        throwException ( Exception, "Empty element ?" );
      }
    NodeFlowHTTP& nodeFlow = NodeFlowHTTP::asNodeFlowHTTP ( xproc.getNodeFlow() );

    PersistentDocument& pDoc = dynamic_cast<PersistentDocument&> ( docElement.getDocument() );
  
    String mimeType = "application/xem-data-revision-head";
    nodeFlow.serializeData ( mimeType, pDoc.getRevisionPage(), sizeof(RevisionPage) );
//        pDoc.getTotalDocumentSize() < pDoc.getAreaSize() ? pDoc.getTotalDocumentSize() : pDoc.getAreaSize() );
  }

  void xemwsiInstruction_serializeRevisionPages ( __XProcHandlerArgs__ )
  {
    XPath docXPath ( item, __builtinKey(xemwsi_document), false );
    ElementRef docElement = docXPath.evalElement ( xproc, currentNode );
    if ( ! docElement ) 
      {
        throwException ( Exception, "Empty element ?" );
      }
    NodeFlowHTTP& nodeFlow = NodeFlowHTTP::asNodeFlowHTTP ( xproc.getNodeFlow() );

    PersistentDocument& pDoc = dynamic_cast<PersistentDocument&> ( docElement.getDocument() );
  
    
    String areaIdxStr = item.getEvaledAttr ( xproc, currentNode, __builtinKey(xemwsi_area) );
    String lengthStr = item.getEvaledAttr ( xproc, currentNode, __builtinKey(xemwsi_length) );

    Warn ( "[WSI] areaIdx='%s', length='%s'\n", areaIdxStr.c_str(), lengthStr.c_str() );
        
    __ui64 areaIdx = strtoull ( areaIdxStr.c_str(), NULL, 16 );
    __ui64 length = strtoull ( lengthStr.c_str(), NULL, 16 );
        
    Warn ( "[WSI] brId=[%llx:%llx] areaIdx=%llx, length=%llx, AreaSize=%llx\n",
      _brid ( pDoc.getBranchRevId() ), areaIdx, length, pDoc.AreaSize );
        
    if ( length != pDoc.AreaSize )
      {
        throwException ( Exception, "Invalid area size ! \n"  );
      }   
     
    void* area = pDoc.getArea ( areaIdx );
    if ( ! area )
      {
        throwException ( Exception, "Could not get area idx=%llx\n", areaIdx );
      }      

    String mimeType = "application/xem-data-revision-page";
    nodeFlow.serializeData ( mimeType, area, length );
//        pDoc.getTotalDocumentSize() < pDoc.getAreaSize() ? pDoc.getTotalDocumentSize() : pDoc.getAreaSize() );
  }


  void WebServer::installWSIHandlers ( Store& store, XProcessorHandlerMap& xprocHandlerMap )
  {
    // NamespaceId xemwsiNSId = store.getKeyCache().builtinKeys.namespace_xemwsi();

#define __setHandler(__key,__func) \
  xprocHandlerMap.setHandler( store.getKeyCache().builtinKeys.__key(), &__func )    
//    xprocHandlerMap.setHandler( store.getKeyCache().getKeyId ( xemwsiNSId, __key, true ), &__func )    

    __setHandler ( xemwsi_bootstrap, xemwsiInstruction_bootstrap );
    __setHandler ( xemwsi_serialize_revision_head, xemwsiInstruction_serializeRevisionHead );
    __setHandler ( xemwsi_serialize_revision_pages, xemwsiInstruction_serializeRevisionPages );

#undef __setHandler

#define __setFunction(__key,__func) \
    xprocHandlerMap.getDefaultXPathExternalFunctionMap().set ( store.getKeyCache().getKeyId ( xemwsiNSId, __key, true ), &__func ) 

    // __setFunction ( "get-session", xemwsFunc_getSession );
    
#undef __setFunction

  }

};
#endif
