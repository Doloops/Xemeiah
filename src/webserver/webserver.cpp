#include <Xemeiah/webserver/webserver.h>
#include <Xemeiah/webserver/querydocument.h>
#include <Xemeiah/webserver/webservermodule.h>

#include <Xemeiah/kern/branchmanager.h>
#include <Xemeiah/kern/volatiledocumentallocator.h>
#include <Xemeiah/xprocessor/xprocessor.h>
#include <Xemeiah/xemprocessor/xemprocessor.h>

#include <Xemeiah/webserver/nodeflow-http.h>
#include <Xemeiah/io/bufferedwriter.h>

#include <Xemeiah/log-time.h>

#include <Xemeiah/auto-inline.hpp>

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#define Log_WebServer Debug

// #define __XEM_WEBSERVER_DISABLE_MULTITHREAD

namespace Xem
{
#include <Xemeiah/kern/builtin_keys_prolog_inst.h>
#include <Xemeiah/webserver/builtin-keys.h>
#include <Xemeiah/kern/builtin_keys_postlog.h>

  WebServer::WebServer ( XProcessor& xproc, WebServerModuleForge& _webServerModuleForge, ElementRef& _configurationElement )
  : XemService(xproc, _configurationElement), webServerModuleForge(_webServerModuleForge),
    xem_web(webServerModuleForge.xem_web)
  {
  }

  WebServer::~WebServer()
  {
  }

  void WebServer::stop ()
  {
    Info ( "Stopping WebServer\n" );
  }

  void webServerSIGPIPEHandler ( int sig, siginfo_t* info, void* ucontext )
  {
    Error ( "SIGPIPE Handler !\n" );  
  }
  
  static void configureSignalSIGPIPE ()
  {
    struct sigaction sigPipeAction;
    memset ( &sigPipeAction, 0, sizeof(struct sigaction) );
    sigPipeAction.sa_handler = NULL;
    sigPipeAction.sa_sigaction = webServerSIGPIPEHandler;
    sigPipeAction.sa_restorer = NULL;  
    sigPipeAction.sa_flags = SA_SIGINFO;
    
    
    int res = sigaction ( SIGPIPE, &sigPipeAction, NULL );
    if ( res == -1 )
      {
        Fatal ( "Could not set SIGPIPE handler !\n" )
      }
    else
      {
        Log_WebServer ( "sigPipeAction configured OK for SIGPIPE.\n" );
      }
  }

  bool WebServer::openTCPServer ( const char* addr, int port )
  {
    AssertBug ( isStarting(), "Service %p not starting ?\n", this );
    configureSignalSIGPIPE ();

    int res;

    int __sockAddrSz__ = sizeof ( struct sockaddr );
    struct sockaddr_in AdrServ;

    Socket listenSocket;

    if (  (listenSocket = socket(PF_INET, SOCK_STREAM, 0)) <= -1)
      { 
        Error ( "Unable to create listening socket\n" ); 
        return false;
      }

    memset(&AdrServ,0,sizeof AdrServ);
    AdrServ.sin_port = htons(port);
    AdrServ.sin_family = PF_INET;
    inet_aton(addr,&(AdrServ.sin_addr));
    int reuse = 1;
    res = setsockopt ( listenSocket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof ( reuse ) );
    Log_WebServer ( "Result of setsockopt for SO_REUSEADDR : %d\n", res );
    if ( (res = bind(listenSocket, (struct sockaddr *) &AdrServ, __sockAddrSz__ )) <= -1 )
      { 
        Error ( "Unable to bind() : result %d, error %d:%s\n", res, errno, strerror (errno) ); 
      }
    if ( (listen(listenSocket, 5)) <= -1 )
      { 
        Error ( "Unable to listen() : result %d, error %d:%s\n", res, errno, strerror (errno) ); 
      }

    Info ( "Xemeiah WebServer started at (%s:%d), state=%s\n", addr, port, getState().c_str() );

    startThread ( boost::bind(&WebServer::runAcceptThread, this, listenSocket) );
    return true;
  }

  void WebServer::runAcceptThread ( Socket listenSocket )
  {
    int interval = 1;

    Socket clientSocket;
    struct sockaddr_in clientAddr;
    socklen_t clientAddrSz = sizeof (struct sockaddr_in);
    fd_set fds;
  
    while (true)
      {
        FD_ZERO ( &fds );
        FD_SET ( listenSocket, &fds );
        struct timeval tv = { interval, 0 };
        int res = select ( listenSocket + 1, &fds, NULL, NULL, &tv );

        Log_WebServer ( "Finished : res=%d, state=%s\n", res, getState().c_str() );
        
        if ( isStopping () ) 
          {
            Info ( "Service is trying to stop, exiting...\n" );
            break;
          }

        if ( res == -1 )
          {
            if ( errno == EINTR )
              {
                Warn ( "select() recieved EINTR !\n" );
                continue;
              }
            Error ( "Stopping Accept Thread because of error %d:%s\n", errno, strerror(errno) );
            break; 
          }
        if ( res == 0 || ! FD_ISSET ( listenSocket, &fds ) )
          continue;

        Log_WebServer ( "Accepting... (state=%s)\n", getState().c_str() );
        clientSocket = accept ( listenSocket, (struct sockaddr *)&clientAddr, &clientAddrSz );
        if ( clientSocket == -1 )
          {
            Error ( "Invalid clientSocket in runAcceptThread() err=%d:%s (listenSocket=%d). Aborting client acceptation.\n",
                errno, strerror(errno), listenSocket );
            continue;
          }
        Log_WebServer ( "New client at fd=%d (from '%s')\n", clientSocket, inet_ntoa ( clientAddr.sin_addr ) );

        if ( isStopping() )
          {
            ::close ( clientSocket );
            Info ( "Service is trying to stop, exiting...\n" );
            break;
          }
        
#ifndef __XEM_WEBSERVER_DISABLE_MULTITHREAD
        startThread ( boost::bind(&WebServer::runHandleThread, this, clientSocket ) );
#else
        runHandleThread ( clientSocket );
#endif
        static __ui64 responsed = 0;
        responsed++;
        Log_WebServer ( "Total responsed : %llu responses.\n", responsed );
      }
    Info ( "Exiting Accept Thread !\n" );
    ::close ( listenSocket );
    listenSocket = -1;
  }


  void WebServer::runHandleThread ( Socket clientSocket )
  {
    int ttl = 5;
    int interval = 1;

    Log_WebServer ( "Created new thread for conn=%d, thread=%lx\n", clientSocket, (unsigned long) pthread_self() );

    time_t lastUse;
    fd_set fds;
    bool result;
  
    while ( true )
      {
        lastUse = time(NULL);
        result = handleClient ( clientSocket );
        if ( result == false ) { Log_WebServer ( "End of handling for socket %d\n", clientSocket ); break; }
#ifdef __XEM_WEBSERVER_DISABLE_MULTITHREAD
        break;
#endif
      just_wait:
        if ( ! isRunning() ) break;
        FD_ZERO ( &fds );
        FD_SET ( clientSocket, &fds );
        struct timeval tv = { interval, 0 };
        int res = select ( clientSocket + 1, &fds, NULL, NULL, &tv );
        Log_WebServer ( "pselect : clientSocked=%d, res=%d\n", clientSocket, res );
        if (  res == -1 )
          {
            break;
          }
        if ( res <= 0 || ! FD_ISSET ( clientSocket, &fds ) )
          {
            time_t now = time(NULL);
            if ( now - lastUse >= ttl )
              break;
            goto just_wait;
          }
        Log_WebServer ( "Reuse client socket %d\n", clientSocket );
      }
    if ( ::shutdown ( clientSocket, SHUT_RDWR ) == -1 )
      {
        Error ( "shutdown : clientSocket=%d, error=%d:%s\n", clientSocket, errno, strerror(errno) );
      }
    Log_WebServer ( "Kill client socket %d\n", clientSocket );
    ::close ( clientSocket );
  }

  bool WebServer::handleClient ( Socket sock ) 
  {
    if ( ! isRunning() )
      {
        Error ( "WebServer stopping (status=%s), exiting handle thread !\n",
            getState().c_str() );
        return false;
      }
    Log_WebServer ( "Run Handle Thread for socket=%d\n", sock );

    NTime startTime = getntime ();

    bool result = true;

    /*
     * First, create the XProcessor
     */
    XProcessor& xproc = getPerThreadXProcessor();

    /*
     * Then, create the output nodeflow as XHTML
     */
    NodeFlowHTTP nodeFlow ( xproc, sock );
    xproc.setNodeFlow ( nodeFlow );

    /*
     * It is no use to stack the documentAllocator in XProcessor
     * So create a documentAllocator directly
     */
    // VolatileDocumentAllocator documentAllocator (getStore());
    xproc.pushEnv();

    DocumentAllocator& documentAllocator = xproc.getCurrentDocumentAllocator(false);
    QueryDocument queryDocument ( getStore(), documentAllocator, webServerModuleForge, sock );

    try
      {
        /**
         * First, parse the Query
         */
        queryDocument.parseQuery ( xproc, true );
        
        /**
         * Get the root queryElement
         */
        ElementRef queryElement = queryDocument.getQueryElement ();

        /**
         * Then, finally, process the query
         */
        processQuery ( xproc, queryElement );
      }
    catch ( Exception* e )
      {
        serializeException ( sock, e );
        delete ( e );
        result = false;
      }
    
    xproc.popEnv();

    if ( result )
      {
        try
          {
            nodeFlow.serialize ();
          }
        catch ( Exception* e )
          {
            Error ( "Exception will serialization : '%s'\n", e->getMessage().c_str() );
            delete ( e );
            result = false;
          }
      }
      
    NTime endTime = getntime ();
    WarnTime ( "Total spent time : ", startTime, endTime );

    return result;
  }

  void WebServer::processQuery ( XProcessor& xproc, ElementRef& queryElement )
  {
#if 0
    xproc.getNodeFlow().setOutputFormat ( 
        "xml",     // Method
        "UTF-8",   // Encoding
        false,     // indent
        false,     // standalone
        true       // omitXMLDeclaration
        );
#endif

    /**
     * Now, bind the document that instanciated the webserver
     */
    KeyId fullKeyId = getKeyCache().getKeyId ( webServerModuleForge.xem_role.ns(),
          configurationElement.getDocument().getRole().c_str(), true );
    xproc.setElement ( fullKeyId, configurationElement );

#if PARANOID
    AssertBug ( queryElement, "Null element ?\n" );
    if ( queryElement.getKeyId() != xem_web.query() )
      throwException ( Exception, "Invalid query type : %s\n", getKeyCache().dumpKey(queryElement.getKeyId()).c_str() );
#endif

    xproc.setElement ( xem_web.query(), queryElement );

    /*
     * Position the xem-web:query as currentNodeSet of the evaluation
     */
    NodeSet initialNodeSet; initialNodeSet.pushBack ( queryElement );
    NodeSet::iterator initialNodeSetIterator ( initialNodeSet, xproc );

    /**
     * Find the xprocessor
     */
    XemProcessor& xemProc = XemProcessor::getMe ( xproc );
    xemProc.callMethod ( configurationElement, "handle-client" );
  }

  void WebServer::serializeException ( Socket sock, Exception* e )
  {
    Error ( "Got an exception '%s'\n", e->getMessage().c_str() );
    // char buffer[4096];
    const char* contentType = "text/html";

#if 0
    String errorMessage = "<html><header><title>Xemeiah : Exception Occured</title></header><body>";
    errorMessage += "<b>An exception occured : </b><br/>";

    StringSize pos = 0, lpos = 0;
    const String& msg = e->getMessage();
    while ( ( pos = msg.find ( '\n', pos ) ) != String::npos )
      {
        errorMessage += msg.substr ( lpos, pos - lpos );
        errorMessage += "<br/>";
        pos ++;
        lpos = pos;
      }
    errorMessage += msg.substr ( lpos, msg.size() - lpos );
    errorMessage += "</body></html>";
#endif
    String errorMessage = e->getMessageHTML ();

    __ui64 contentLength = errorMessage.size ();

    BufferedWriter writer;
    writer.doPrintf( "HTTP/1.1 404 OK\r\n"
           "Response Code: 404\r\n"
           "Content-Length: %llu\r\n"
           "Content-Type: %s; charset=utf-8\r\n\r\n%s",
           contentLength, contentType, errorMessage.c_str() );
    ssize_t res = write ( sock, writer.getBuffer(), writer.getBufferSize() );
    if ( res < 0 || (size_t) res == writer.getBufferSize() )
      {
        Error ( "Could not serialize exception !\n" );
      }
#if 0
    res = write ( sock, errorMessage.c_str(), contentLength );
    if ( res < 0 || (size_t) res == bufferSz )
      {
        Error ( "Could not serialize exception !\n" );
      }
#endif
  }
};

