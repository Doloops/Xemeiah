#include <Xemeiah/webserver/querydocument.h>
#include <Xemeiah/kern/store.h>

#include <Xemeiah/parser/parser.h>

#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/dom/attributeref.h>

#include <Xemeiah/xemprocessor/xemprocessor.h>
#include <Xemeiah/webserver/webserver.h>
#include <Xemeiah/webserver/webservermodule.h>

#include <Xemeiah/auto-inline.hpp>


#define Log_QDUrl Debug
#define Log_QBlob Debug

namespace Xem
{
  QueryDocument::QueryDocument ( Store& store, DocumentAllocator& documentAllocator, WebServerModuleForge& _webServerModuleForge, Socket _sock ) 
  : VolatileDocument ( store, documentAllocator ), webServerModuleForge(_webServerModuleForge),
    sock(_sock), reader(sock),
    queryElement(*this), xem_web(webServerModuleForge.xem_web)
  {
    reader.setEncoding(Encoding_ISO_8859_1);
#if 0
    contentLength = 0;
    buffIdx = 0; bytesRead = 0;
    hasBinaryContents_ = false;
#endif

    createRootElement ();

    ElementRef queryDocumentRoot = getRootElement();
    queryElement = createElement ( queryDocumentRoot, xem_web.query() );
    queryDocumentRoot.insertChild ( queryElement );
    
#if 0
    queryElement.addNamespaceAlias ( __builtin.xem->defaultPrefix(), keyCache.builtinKeys.xem->ns() );
    queryElement.addNamespaceAlias ( __builtin.xem_role->defaultPrefix(), keyCache.builtinKeys.xem_role->ns() );
    queryElement.addNamespaceAlias ( xem_web.defaultPrefix(), keyCache.builtinKeys.xem_web->ns() );
#endif
    queryElement.addNamespaceAlias ( "xem", "http://www.xemeiah.org/ns/xem" );
    queryElement.addNamespaceAlias ( "xem-role", "http://www.xemeiah.org/ns/xem-role" );
    queryElement.addNamespaceAlias ( xem_web.defaultPrefix(), xem_web.ns() );
  }

  QueryDocument::~QueryDocument ()
  {
#if 0
    if ( hasBinaryContents() )
      {
        Warn ( "Not implemented : still has binary contents !\n" );
      }
#endif
  }

  ElementRef QueryDocument::addTextualElement ( ElementRef& from, KeyId keyId, const String& contents )
  {
    ElementRef newElement = createElement ( from, keyId );
    from.appendLastChild ( newElement );
    ElementRef textElement = createTextNode ( newElement, contents.c_str() );
    newElement.appendLastChild ( textElement );
    return textElement;
  }
};

