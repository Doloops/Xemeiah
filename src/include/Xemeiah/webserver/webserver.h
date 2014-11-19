#ifndef __XEMEIAH__WEBSERVER_H
#define __XEMEIAH__WEBSERVER_H

#include <Xemeiah/dom/string.h>
#include <Xemeiah/dom/elementref.h>


#include <Xemeiah/kern/store.h>
#include <Xemeiah/xemprocessor/xemservice.h>
#include <Xemeiah/xprocessor/xprocessormoduleforge.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>
#include <map>

#include <errno.h>
#include <sys/timeb.h>

namespace Xem
{
#include <Xemeiah/kern/builtin_keys_prolog.h>
#include <Xemeiah/webserver/builtin-keys.h>
#include <Xemeiah/kern/builtin_keys_postlog.h>

  /**
   * Socket as provided by socket(2)
   */
  typedef int Socket;
  
  class WebServerModuleForge;
  
  /**
   * WebServer handles incomming TCP connexions
   */
  class WebServer : public XemService
  {
  protected:
    /**
     * Our webServerModuleForge;
     */
    WebServerModuleForge& webServerModuleForge;

    /**
     * The starter thread, which handles service starting
     */
    void runStarterThread ( void* arg );

    /**
     * The accept thread, which accepts new clients
     */
    void runAcceptThread ( Socket listenSocket );

    /**
     * The handle thread, which handles incomming client requests
     */
    void runHandleThread ( Socket clientSocket );

    /**
     * Handle a client request once
     */
    bool handleClient ( Socket clientSocket );

    /**
     * Handle dynamic query
     */
    void processQuery ( XProcessor& xproc, ElementRef& queryElement );
    
    /**
     * Serialize exception to socket
     */
    void serializeException ( Socket sock, Exception* exception );

    /**
     * Start service (implemented in XemService)
     */
    // virtual void start ();
    
    /**
     * Stop Service
     */
    virtual void stop ();

#if 0
    /**
     * Argument class for the AcceptThread
     */
    class AcceptThreadArguments
    {
    public:
      Socket listenSocket;
      
    };
    
    /**
     * Arguments class for the HandleThread
     */
    class HandleThreadArguments
    {
    public:
      Socket clientSocket;
      
    };
#endif
  public:
    __BUILTIN_NAMESPACE_CLASS(xem_web) &xem_web;

    /**
     * WebServer default constructor
     * Additionnal configuration may be performed, after instanciation, but before call to openTCPServer()
     */
    WebServer ( XProcessor& xproc, WebServerModuleForge& webServerModuleForge, ElementRef& configurationElement );
    
    /**
     * WebServer destructor
     */
    ~WebServer ();

    /**
     * Access to the Store's KeyCache
     */
    inline KeyCache& getKeyCache() { return getStore().getKeyCache(); }
    
    /**
     * Open the webserver connection
     */
    bool openTCPServer ( const char * addr, int port );

#if 0
    /**
     * Set the codeScope document containing XML implementation
     */
    void bindCodeScopeDocument ( XProcessor& xproc );
#endif
    
  };
};

#endif // __XEMEIAH__WEBSERVER_H
