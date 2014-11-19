#include <Xemeiah/webserver/webservermodule.h>
#include <Xemeiah/webserver/webserver.h>
#include <Xemeiah/webserver/querydocument.h>
#include <Xemeiah/xemprocessor/xemprocessor.h>
#include <Xemeiah/xemprocessor/xemservicemodule.h>
#include <Xemeiah/kern/servicemanager.h>
#include <Xemeiah/xpath/xpath.h>
#include <Xemeiah/xpath/xpathparser.h>
#include <Xemeiah/webserver/nodeflow-http.h>
#include <Xemeiah/dom/blobref.h>
#include <Xemeiah/kern/volatiledocumentallocator.h>
#include <Xemeiah/xprocessor/xprocessorlib.h>

#include <Xemeiah/auto-inline.hpp>

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netdb.h>

#define Log_WebServerModule Debug

namespace Xem
{
  __XProcessorLib_DECLARE_LIB(WebServer,"webserver");
  __XProcessorLib_REGISTER_MODULE( WebServer, WebServerModuleForge );

  WebServerModule::WebServerModule ( XProcessor& xproc, WebServerModuleForge& moduleForge ) 
  : XProcessorModule ( xproc, moduleForge ), xem_web(moduleForge.xem_web), xem_role(moduleForge.xem_role)
  {
  }
  
  WebServerModule::~WebServerModule ()
  {
  }
    
  void WebServerModule::webInstructionService ( __XProcHandlerArgs__ )
  {
    WebServer* webServer = new WebServer ( getXProcessor(), getTypedModuleForge<WebServerModuleForge&>(), item );
    webServer->registerMyself(getXProcessor());
  }

  void WebServerModule::webInstructionListen ( __XProcHandlerArgs__ )
  {
    ElementRef webServerElement = getXProcessor().getVariable ( __builtin.nons.this_() )->toElement();
    
    XemServiceModule& xemServiceModule = XemServiceModule::getMe(getXProcessor());
    String serviceName = webServerElement.getEvaledAttr ( getXProcessor(), xemServiceModule.xem_service.name() );

    Log_WebServerModule ( "LISTEN : serviceName='%s'\n", serviceName.c_str() );

    WebServer* webServer = dynamic_cast<WebServer*> ( getServiceManager().getService ( serviceName ) );
    if ( ! webServer ) throwException ( Exception, "Could not find service '%s'\n", serviceName.c_str() );
    
    String bindAddress = item.getEvaledAttr ( getXProcessor(), xem_web.bind_address() );
    int port = item.getEvaledAttr ( getXProcessor(), xem_web.port() ).toInteger();
    
    webServer->openTCPServer ( bindAddress.c_str(), port );
  }

  void WebServerModule::webInstructionPutParametersInEnv ( __XProcHandlerArgs__ )
  {
    XemProcessor& xemProcessor = XemProcessor::getMe ( getXProcessor() );
  
    NodeSet parameters;
    XPath parametersXPath ( getXProcessor(), item, xem_web.select(), false );
    
    parametersXPath.eval ( parameters );

    Log_WebServerModule ( "Put parameters in env : found %lu parameters.\n", (unsigned long) parameters.size() );
    
    for ( NodeSet::iterator paramIter(parameters) ; paramIter ; paramIter++ )
      {
        ElementRef param = paramIter->toElement ();
        String name = param.getAttr ( xem_web.name() );
        String value = param.getChild() ? param.getChild().getText() : "";
        
        Log_WebServerModule ( "[WS] : param name='%s' value='%s'\n", name.c_str(), value.c_str() );
        
        KeyId keyId = 0;
        try
          {
            keyId = param.getAttrAsKeyId ( xem_web.name() );
          }
        catch ( Exception *e )
          {
            detailException ( e, "Could not resolve parameter name '%s'\n", name.c_str() );
            throw ( e );
          }
        
        if ( NodeRef::isValidNodeId ( value.c_str() ) )
          {
            /*
             * We do not have to care about freeing nodeSet in case of an Exception
             * Env will take good care of it.
             */           
            NodeSet* nodeSet = getXProcessor().setVariable ( keyId, true );
            xemProcessor.resolveNodeAndPushToNodeSet ( *nodeSet, value.c_str() );

            Log_WebServerModule ( "[WS] : Variable '%s' (%x) : Resolved Node '%s' to '%s'\n",
                name.c_str(), keyId,
                value.c_str(), nodeSet->front().toNode().generateVersatileXPath().c_str() );
          }
        else
          {
            NodeSet* nodeSet = getXProcessor().setVariable ( keyId, true );
            nodeSet->setSingleton ( value );
          }
      }
  }

  void WebServerModule::webFunctionURLEncode ( __XProcFunctionArgs__ )
  {
    String encoded;
    String toEncode = args[0]->toString();
    for ( const char* c = toEncode.c_str() ; *c ; c++ )
      {
        switch ( *c )
        {
        case ' ': encoded += "%20"; break;
        default: encoded += *c;
        }
      }
    result.setSingleton ( encoded );
  }
  
  void WebServerModule::webFunctionProtectJS ( __XProcFunctionArgs__ )
  {
    String encoded;
    // String toEncode = args[0]->toString();
    for ( NodeSet::iterator iter(*args[0]) ; iter ; iter++ )
      {
      String toEncode = iter->toString();
        for ( const char* c = toEncode.c_str() ; *c ; c++ )
          {
            switch ( *c )
            {
            case '\'': encoded += "\\\'"; break;
            default: encoded += *c;
            }
          }
      }
    result.setSingleton ( encoded );
  }

  void WebServerModule::webInstructionResponseFile ( __XProcHandlerArgs__ )
  {  
    String href = item.getEvaledAttr ( getXProcessor(), xem_web.href() );

    __ui64 rangeStart = 0;
    if ( item.hasAttr ( xem_web.range_start() ) )
      rangeStart = atoi(item.getEvaledAttr ( getXProcessor(), xem_web.range_start() ).c_str());
    __ui64 rangeEnd = ~((__ui64)0);
    if ( item.hasAttr ( xem_web.range_end() ) )
      rangeEnd = atoi(item.getEvaledAttr ( getXProcessor(), xem_web.range_end() ).c_str());

    NodeFlowHTTP& nodeFlow = NodeFlowHTTP::asNodeFlowHTTP ( getNodeFlow() );
    nodeFlow.serializeFile ( href, rangeStart, rangeEnd );
  }

  void WebServerModule::webInstructionAddResponseParam ( __XProcHandlerArgs__ )
  {
    NodeFlowHTTP& nodeFlow = NodeFlowHTTP::asNodeFlowHTTP ( getXProcessor().getNodeFlow() );

    String name = item.getEvaledAttr ( getXProcessor(), xem_web.name() );
    String value = item.getEvaledAttr ( getXProcessor(), xem_web.value() );
    
    nodeFlow.addParam ( name, value );
  }
  
  void WebServerModule::webInstructionSetResultCode ( __XProcHandlerArgs__ )
  {
    String resultCode = item.getEvaledAttr ( getXProcessor(), xem_web.code() );
    NodeFlowHTTP& nodeFlow = NodeFlowHTTP::asNodeFlowHTTP ( getXProcessor().getNodeFlow() );
    nodeFlow.setResultCode ( resultCode );
  }

  void WebServerModule::webInstructionSetContentLength ( __XProcHandlerArgs__ )
  {
    String contentLength = item.getEvaledAttr ( getXProcessor(), xem_web.length() );
    if ( ! contentLength.c_str() ) throwException ( Exception, "Invalid contentLength !\n" );

    NodeFlowHTTP& nodeFlow = NodeFlowHTTP::asNodeFlowHTTP ( getXProcessor().getNodeFlow() );
    nodeFlow.setContentLength ( atoll(contentLength.c_str()) );
  }

  void WebServerModule::webInstructionSetContentType ( __XProcHandlerArgs__ )
  {
    String contentType = item.getEvaledAttr ( getXProcessor(), xem_web.mime_type() );
    NodeFlowHTTP& nodeFlow = NodeFlowHTTP::asNodeFlowHTTP ( getXProcessor().getNodeFlow() );
    nodeFlow.setContentType ( contentType );
  }

  void WebServerModule::webInstructionSerializeQueryToBlob ( __XProcHandlerArgs__ )
  {
    NotImplemented("xem-web:serialize-query-to-blob is deprecated !\n" );
#if 0
    XPath queryXPath ( getXProcessor(), item, xem_web.query() );
    ElementRef query = queryXPath.evalElement();

    QueryDocument& queryDocument = dynamic_cast<QueryDocument&> ( query.getDocument() );

    XPath blobXPath ( getXProcessor(), item,xem_web.blob() );
    ElementRef blob = blobXPath.evalElement();
  
    KeyId blobNameId = item.getAttrAsKeyId(getXProcessor(),xem_web.blob_name());
    
    BlobRef blobRef = blob.findAttr ( blobNameId, AttributeType_SKMap );
    if ( ! blobRef )
      {
        blobRef = blob.addBlob ( blobNameId );
      }
    queryDocument.serializeToBlob ( blobRef );
#endif
  }
  
  
  void WebServerModule::webInstructionSerializeBlobToResponse ( __XProcHandlerArgs__ )
  {
    XPath blobXPath ( getXProcessor(), item,xem_web.select() );
    BlobRef blobRef = blobXPath.evalAttribute();
    Log_WebServerModule ( "Serialize Blob '%s'\n", blobRef.generateVersatileXPath().c_str() );
    __ui64 rangeStart = 0;
    if ( item.hasAttr ( xem_web.range_start() ) )
      rangeStart = atoi(item.getEvaledAttr ( getXProcessor(), xem_web.range_start() ).c_str() );
    __ui64 rangeEnd = blobRef.getSize();
    if ( item.hasAttr ( xem_web.range_end() ) )
      rangeEnd = atoi(item.getEvaledAttr ( getXProcessor(), xem_web.range_end() ).c_str() );
    
    NodeFlowHTTP& nodeFlow = NodeFlowHTTP::asNodeFlowHTTP ( getXProcessor().getNodeFlow() );
    nodeFlow.serializeBlob ( blobRef, rangeStart, rangeEnd );
  }

  String WebServerModule::serializeQueryHeader ( ElementRef& query )
  {
    NamespaceAlias nsAlias ( getKeyCache() );
    nsAlias.setNamespacePrefix(xem_web.defaultPrefix(), xem_web.ns());

    String serializedQuery;

    XPathParser responseExpression ( getKeyCache(), nsAlias, "xem-web:headers/xem-web:response");
    NodeSet responseNS; XPath(getXProcessor(),responseExpression).eval(responseNS, query);

    if ( responseNS.size() )
      {
        ElementRef response = responseNS.toElement();
        serializedQuery += "HTTP/1.1 ";
        serializedQuery += response.getAttr(xem_web.response_code());
        serializedQuery += " ";
        serializedQuery += response.getAttr(xem_web.response_string());
      }
    else
      {
        XPathParser methodExpression ( getKeyCache(), nsAlias, "xem-web:method/text()");
        String method = XPath(getXProcessor(),methodExpression).evalString(query);
        serializedQuery += method + " /";

        XPathParser baseUrlExpression ( getKeyCache(), nsAlias, "xem-web:url/xem-web:base/text()");
        String baseUrl = XPath(getXProcessor(),baseUrlExpression).evalString(query);

        serializedQuery += baseUrl;

        XPathParser paramsExpression ( getKeyCache(),nsAlias, "xem-web:url/xem-web:parameters/xem-web:param");
        NodeSet params; XPath(getXProcessor(),paramsExpression).eval(params,query);

        bool first = true;

        for ( NodeSet::iterator iter(params) ; iter ; iter++ )
          {
            if ( first )
              {
                serializedQuery += "?";
                first = false;
              }
            else
              {
                serializedQuery += "&";
              }
            serializedQuery += iter->toElement().getAttr(xem_web.name());
            serializedQuery += "=";
            if ( iter->toElement().getChild() )
              serializedQuery += iter->toElement().getChild().getText();
          }


        serializedQuery += " ";

        XPathParser protocolExpression ( getKeyCache(), nsAlias, "xem-web:protocol/text()");
        String protocol = XPath(getXProcessor(),protocolExpression).evalString(query);

        serializedQuery += protocol;
      }
    serializedQuery += "\r\n";

    XPathParser headersExpression ( getKeyCache(),nsAlias, "xem-web:headers/xem-web:param");
    NodeSet headers; XPath(getXProcessor(),headersExpression).eval(headers,query);

    for ( NodeSet::iterator iter(headers) ; iter ; iter++ )
      {
        serializedQuery += iter->toElement().getAttr(xem_web.name());
        serializedQuery += ": ";
        serializedQuery += iter->toElement().getChild().getText();
        serializedQuery += "\r\n";
      }

    XPathParser cookiesExpression ( getKeyCache(),nsAlias, "xem-web:cookies/xem-web:cookie");
    NodeSet cookies; XPath(getXProcessor(),cookiesExpression).eval(cookies,query);

    for ( NodeSet::iterator iter(cookies) ; iter ; iter++ )
      {
        serializedQuery += "Cookie: ";
        serializedQuery += iter->toElement().getAttr(xem_web.name());
        serializedQuery += "=";
        serializedQuery += iter->toElement().getChild().getText();
        serializedQuery += "\r\n";
      }

    serializedQuery += "\r\n";

    Log_WebServerModule ( "SERIALIZED QUERY : %s\n", serializedQuery.c_str() );
    return serializedQuery;
  }

  int __doCreateSocket ( struct addrinfo* infos )
  {
    int sock;
    if ( ( sock = socket(infos->ai_family, infos->ai_socktype, infos->ai_protocol )) <= -1)
      {
        throwException ( Exception, "Could not create socket : error = %d:%s\n", errno, strerror(errno) );
      }
    if ( connect ( sock, infos->ai_addr, infos->ai_addrlen ) == -1 )
      {
        throwException ( Exception, "Could not connect to '%s' : error = %d:%s\n", infos->ai_canonname, errno, strerror(errno) );
      }
    return sock;
  }

  void WebServerModule::webFunctionSendQuery ( __XProcFunctionArgs__ )
  {
    if ( args.size() < 2 )
      {
        throwException ( Exception, "Invalid number of arguments !\n" );
      }
    String connectionString = args[0]->toString();
    ElementRef query = args[1]->toElement();

    Log_WebServerModule ( "CONNECTIONSTRING : %s\n", connectionString.c_str() );

    std::list<String> connectionTokens;
    connectionString.tokenize(connectionTokens,':');

    String connectionHost = connectionTokens.front();
    String connectionPort = "80";
    if ( connectionTokens.size() > 1 )
      connectionPort = connectionTokens.back();

    struct addrinfo* infos = NULL;
    int res = getaddrinfo ( connectionHost.c_str(), connectionPort.c_str(), NULL, &infos );
    if ( res )
      {
        throwException ( Exception, "Could not getaddrinfo for %s : err=%d:%s\n", connectionHost.c_str(), res, gai_strerror(res) );
      }

    /**
     * Todo : we shall round-robin a bit here !
     */
    int sock = __doCreateSocket(infos);

    Log_WebServerModule ( "SOCK Successfull connection to : %s\n", connectionHost.c_str() );

    freeaddrinfo(infos);

    String serializedQuery = serializeQueryHeader(query);
    ssize_t reswr = ::write ( sock, serializedQuery.c_str(), serializedQuery.size() );
    if ( reswr <= 0 || (StringSize) reswr != serializedQuery.size() )
      {
        Warn ( "Could not fully write : res=%ld, serializedQuery.size() = %lu\n",
            (long) reswr, serializedQuery.size() ) ;
      }

    QueryDocument* resultQueryDocument = new QueryDocument ( getXProcessor().getStore(),
          getXProcessor().getCurrentDocumentAllocator(true),
          dynamic_cast<Xem::WebServerModuleForge&> (getXProcessorModuleForge()), sock );
    getXProcessor().bindDocument(resultQueryDocument,true);

    resultQueryDocument->parseQuery(getXProcessor(),false);
    ElementRef resultQuery = resultQueryDocument->getRootElement().getChild();

    result.pushBack(resultQuery);

    ::close(sock);
  }

  void WebServerModule::webInstructionSerializeQueryResponse ( __XProcHandlerArgs__ )
  {
    NamespaceAlias nsAlias ( getKeyCache() );
    nsAlias.setNamespacePrefix(xem_web.defaultPrefix(), xem_web.ns());

    XPath queryXPath ( getXProcessor(), item, xem_web.select(), false );
    ElementRef query = queryXPath.evalElement();

    String serializedResponse = serializeQueryHeader(query);

    XPathParser contentExpression ( getKeyCache(), nsAlias, "xem-web:content");
    ElementRef contentElement = XPath(getXProcessor(),contentExpression).evalElement(query);

    NodeFlowHTTP& nodeFlow = NodeFlowHTTP::asNodeFlowHTTP ( getXProcessor().getNodeFlow() );

    if ( contentElement && contentElement.hasAttr(xem_web.blob_contents(),AttributeType_SKMap) )
      {
        BlobRef blob = contentElement.findAttr(xem_web.blob_contents(),AttributeType_SKMap);

        nodeFlow.serializeRawResponse((void*) serializedResponse.c_str(), serializedResponse.size() );

        nodeFlow.serializeBlob(blob,0,~((__ui64)0), false);
        nodeFlow.finishSerialization();


      }
    else
      {
        nodeFlow.serializeRawResponse((void*) serializedResponse.c_str(), serializedResponse.size() );
        nodeFlow.finishSerialization();
      }
  }

  void WebServerModule::install ()
  {
  }

  void WebServerModuleForge::install ()
  {
    registerHandler ( xem_web.service(), &WebServerModule::webInstructionService );
    registerHandler ( xem_web.put_parameters_in_env(), &WebServerModule::webInstructionPutParametersInEnv );
    registerHandler ( xem_web.response_file(), &WebServerModule::webInstructionResponseFile );
    registerHandler ( xem_web.add_response_param(), &WebServerModule::webInstructionAddResponseParam );
    registerHandler ( xem_web.set_result_code(), &WebServerModule::webInstructionSetResultCode );
    registerHandler ( xem_web.set_content_type(), &WebServerModule::webInstructionSetContentType );
    registerHandler ( xem_web.set_content_length(), &WebServerModule::webInstructionSetContentLength );
    registerHandler ( xem_web.listen(), &WebServerModule::webInstructionListen );

    registerHandler ( xem_web.serialize_query_to_blob(), &WebServerModule::webInstructionSerializeQueryToBlob );
    registerHandler ( xem_web.serialize_blob_to_response(), &WebServerModule::webInstructionSerializeBlobToResponse );

    registerFunction ( xem_web.url_encode(), &WebServerModule::webFunctionURLEncode );
    registerFunction ( xem_web.protect_js(), &WebServerModule::webFunctionProtectJS );

    registerFunction ( xem_web.send_query(), &WebServerModule::webFunctionSendQuery );

    registerHandler ( xem_web.serialize_query_response(), &WebServerModule::webInstructionSerializeQueryResponse );
  }

  void WebServerModuleForge::registerEvents ( Document& doc )
  {
    registerXPathAttribute(doc,xem_web.select());
  }
};
