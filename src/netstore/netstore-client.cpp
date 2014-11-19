#include <Xemeiah/kern/volatilestore.h>
#include <Xemeiah/netstore/netstore.h>
#include <Xemeiah/netstore/netbranchmanager.h>
#include <Xemeiah/webserver/querydocument.h>

#include <Xemeiah/dom/elementref.h>

#include <Xemeiah/xpath/xpathparser.h>
#include <Xemeiah/kern/namespacealias.h>
#include <Xemeiah/kern/env.h>
#include <Xemeiah/dom/nodeset.h>

#include <Xemeiah/webserver/webserver.h>


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
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>

namespace Xem
{
  NetStore::NetStore ()
  {
    pthread_mutex_init ( &socketLock, NULL );
  }
  
  NetStore::~NetStore ()
  {
  
  }
  
  BranchManager& NetStore::getBranchManager()
  {
    if ( ! branchManager )
      {
        throwException ( Exception, "No BranchManager defined !\n" );
      }
    return *branchManager;  
  }


  LocalKeyId NetStore::addKeyInStore ( const char* keyName )
  {
    NotImplemented ( ". " );
    return 0;
  }

  NamespaceId NetStore::addNamespaceInStore ( const char* namespaceURL )
  {
    NotImplemented ( ". " );
    return 0;
  }

  bool NetStore::reserveElementIds ( ElementId& nextId, ElementId& lastId )
  {
    NotImplemented ( ". " );
    return false;
  }
  

  Socket NetStore::getServerSocket ()
  {
    pthread_mutex_lock ( &socketLock );  
    if ( ! serverSockets.empty() )
      {
        Socket sock = serverSockets.front ();
        serverSockets.pop_front ();
        pthread_mutex_unlock ( &socketLock );
        return sock;
      }
    pthread_mutex_unlock ( &socketLock );
    
    char* connectStr = strdup ( connectionString.c_str() );
    char* connectionPortString = strchr ( connectStr, ':' );
    if ( connectionPortString == NULL )
      { 
        Fatal ( "Port not specified !\n" );
      }
    *connectionPortString = '\0';
    connectionPortString++;
  
    int connectionPort = atoi ( connectionPortString );
  
    AssertBug ( connectionPort, "Invalid zero connectionPort\n" );
  
    Info ( "Connecting to '%s', port '%d'\n", connectStr, connectionPort );
    
    Socket sock;
  
    if (  ( sock = socket(PF_INET, SOCK_STREAM, 0)) <= -1)
      { 
        delete ( connectStr );
        throwException ( Exception, "Could not connect to '%s' : error = %d:%s\n", connectionString.c_str(), errno, strerror(errno) );
      }
       
    int __sockAddrSz__ = sizeof ( struct sockaddr );
    struct sockaddr_in connectionAddress;

    memset(&connectionAddress,0,sizeof connectionAddress);
    connectionAddress.sin_port = htons(connectionPort);
    connectionAddress.sin_family = PF_INET;
    inet_aton ( connectStr, &(connectionAddress.sin_addr) );

    if ( connect ( sock, (struct sockaddr*) &connectionAddress, __sockAddrSz__ ) == -1 )
      {
        delete ( connectStr );
        throwException ( Exception, "Could not connect to '%s' : error = %d:%s\n", connectionString.c_str(), errno, strerror(errno) );
      } 
    Info ( "Connected : sock=%d\n", sock );
    delete ( connectStr );
    
    return sock;
  }

  void NetStore::releaseServerSocket ( Socket sock, bool failed )
  {
    if ( failed )
      {
        ::shutdown ( sock, SHUT_RDWR );
        ::close ( sock );  
      }
    else
      {
        pthread_mutex_lock ( &socketLock );  
        serverSockets.push_back ( sock );
        pthread_mutex_unlock ( &socketLock );  
      }
  }
  
  bool NetStore::open ( const String& _connectionString )
  {
    setStringFromAllocedStr ( connectionString, strdup ( _connectionString.c_str() ) );
    
    try
      {
       bootstrap();
      }
    catch ( Exception* exception )
      {
        Error ( "Could not bootstrap ! exception : %s\n", exception->getMessage().c_str() );
        delete ( exception );
        return false;
      }
       
    return true;
  }
  
  void NetStore::bootstrap ()
  {
    VolatileStore bootstrapStore;
 
    QueryDocument queryDocument ( bootstrapStore ); 

    ElementRef wsiResult = queryXMLResult ( "bootstrap", "", queryDocument, bootstrapStore.getKeyCache() );

    NamespaceAlias nsAlias ( bootstrapStore.getKeyCache() );
 
    nsAlias.push ();
    nsAlias.setNamespacePrefix ( bootstrapStore.getKeyCache().builtinKeys.xem_web->defaultPrefix(), 
                                 bootstrapStore.getKeyCache().builtinKeys.xem_web->ns(), true );
    nsAlias.setNamespacePrefix ( bootstrapStore.getKeyCache().builtinKeys.xem_wsi->defaultPrefix(), 
                                 bootstrapStore.getKeyCache().builtinKeys.xem_wsi->ns(), true );

    XPathParser keysXPath ( bootstrapStore.getKeyCache(), nsAlias, 
        "xemwsi:bootstrap/xemwsi:keys/xemwsi:key", false );

    XPathParser namespacesXPath ( bootstrapStore.getKeyCache(), nsAlias, 
        "xemwsi:bootstrap/xemwsi:namespaces/xemwsi:namespace", false );

    nsAlias.pop ();
    
    Env env ( bootstrapStore );
    
    NodeSet keys, namespaces;
    
    keysXPath.eval ( env, keys, wsiResult );
    namespacesXPath.eval ( env, namespaces, wsiResult );
 
    for ( NodeSet::iterator iter ( keys ) ; iter ; iter++ )
      {
        ElementRef eltRef = iter->toElement ();
                
        String sId = eltRef.getAttr ( bootstrapStore.getKeyCache().builtinKeys.xem_wsi->id() );
        String value = eltRef.getAttr ( bootstrapStore.getKeyCache().builtinKeys.xem_wsi->value() );
        
        LocalKeyId localKeyId = (LocalKeyId) strtoul ( sId.c_str(), NULL, 16 );
        
        if ( localKeyId == 0 )
          {
            Warn ( "Wrong key value ! sId=%s, value=%s\n", sId.c_str(), value.c_str() );
            continue;
          }
        
        Warn ( "Key : sId=%s, id=%x, value=%s\n", sId.c_str(), localKeyId, value.c_str() );
        
        keyCache.localKeyMap.put ( localKeyId, value.c_str() );
        keyCache.keysBucket.put ( localKeyId, value.c_str() );
      }

    for ( NodeSet::iterator iter ( namespaces ) ; iter ; iter++ )
      {
        ElementRef eltRef = iter->toElement ();
                
        String sId = eltRef.getAttr ( bootstrapStore.getKeyCache().builtinKeys.xem_wsi->id() );
        String value = eltRef.getAttr ( bootstrapStore.getKeyCache().builtinKeys.xem_wsi->value() );
        
        LocalKeyId localKeyId = (LocalKeyId) strtoul ( sId.c_str(), NULL, 16 );
        
        if ( localKeyId == 0 )
          {
            Warn ( "Wrong namespace value ! sId=%s, value=%s\n", sId.c_str(), value.c_str() );
            continue;
          }
        
        Warn ( "Namespace : sId=%s, id=%x, value=%s\n", sId.c_str(), localKeyId, value.c_str() );

        keyCache.namespaceBucket.put ( localKeyId, value.c_str() );
        keyCache.namespaceMap.put ( localKeyId, value.c_str() );
        
      }

    if ( ! buildKeyCacheBuiltinKeys () )
      {
        throwException ( Exception, "Bootstrap : Could not get builtin keys !!!\n" );
      }
    
    Info ( "bootstrap finished.\n" );
    
    branchManager = new NetBranchManager ( *this );
  }
  
  bool NetStore::queryServer ( const String& action, const String& arguments, QueryDocument& queryDocument )
  {
    String request = "GET /xemwsi?xemwsi:action=";
    String protectedArgs;
    
    for ( const char* c = arguments.c_str() ; *c ; c++ )
      {
        if ( *c == ' ' )
          {
            protectedArgs += "%20";
          }
        else
          {
            protectedArgs += *c;
          }
      }
    
    request += action;
    request += "&";
    request += protectedArgs;
    request += " HTTP/1.1\r\n\r\n";
  
    Warn ( "Requesting : %s\n", request.c_str() );

    size_t requestLen = strlen(request.c_str());

    int maxTries = 10;
    int nbTry = 0;

    while ( true )
      {
        nbTry ++;
        
        if ( nbTry > maxTries )
          {
            Error ( "Max tries reached.\n" );
            return false;
          }
        Socket sock = getServerSocket ();

        ssize_t written = write ( sock, request.c_str(), requestLen ); 
        
        if ( written < 0 )
          {
            Error ( "Could not write : Written=%ld / len=%lu\n", written, requestLen );
            releaseServerSocket ( sock, true );
            continue;
          }
        else if (  (size_t) written < requestLen )
          {
            Error ( "Could not write : Written=%ld / len=%lu\n", written, requestLen );
            NotImplemented ( "." );
          }
        Warn ( "Written=%ld / len=%lu\n", written, requestLen );
        
        if ( queryDocument.parseQuery ( sock, false ) )
          {
            releaseServerSocket ( sock, false );
            break;
          }
        releaseServerSocket ( sock, true );
      }
    return true;
  }

  ElementRef NetStore::queryXMLResult ( const String& action, const String& arguments, QueryDocument& queryDocument, KeyCache& keyCache )
  {
    if ( ! queryServer ( action, arguments, queryDocument ) )
      {
        throwException ( Exception, "Could not query !\n" );
      }
    
    ElementRef rootElement = queryDocument.getRootElement ();
    
    rootElement.toXML ( 1, 0 );

    NamespaceAlias nsAlias ( keyCache );
 
    nsAlias.push ();
    nsAlias.setNamespacePrefix ( keyCache.builtinKeys.xem_web->defaultPrefix(), 
                                 keyCache.builtinKeys.xem_web->ns(), true );
    nsAlias.setNamespacePrefix ( keyCache.builtinKeys.xem_wsi->defaultPrefix(), 
                                 keyCache.builtinKeys.xem_wsi->ns(), true );

    XPathParser resultXPath ( keyCache, nsAlias, "/xemws:query/xemws:content/xemwsi:result", false );

    nsAlias.pop ();
    
    Env env ( *this );
    
    NodeSet result;
    
    resultXPath.eval ( env, result, rootElement );

    if ( result.size() == 0 )
      {
        rootElement.toXML ( 1, 0 );
        throwException ( Exception, "Empty result !\n" );
      }
    else if ( result.size() > 1 )
      {
        throwException ( Exception, "Multiple results !\n" );
      }
    return result.front().toElement ();
  }
  
  String NetStore::queryString ( const String& action, const String& arguments )
  {
    QueryDocument queryDocument ( *this ); 

    ElementRef result = queryXMLResult ( action, arguments, queryDocument, getKeyCache() );
    return result.toString ();
  }

  Document* NetStore::queryXMLDocument ( const String& action, const String& arguments )
  {
    QueryDocument queryDocument ( *this ); 

    ElementRef resultElement = queryXMLResult ( action, arguments, queryDocument, getKeyCache() );

    Document* result = createVolatileDocument ();
    
    ElementRef resultRoot = result->getRootElement ();
    
    resultRoot.copyContents ( resultElement );
    
    return result;
  }
      
  bool NetStore::queryData ( const String& action, const String& arguments, void*& buff, __ui64& buffSize )
  {
    QueryDocument queryDocument ( *this ); 

    if ( ! queryServer ( action, arguments, queryDocument ) )
      {
        throwException ( Exception, "Could not query !\n" );
      }
    
    return queryDocument.getBinaryContent ( buff, buffSize, true );
  }
}

