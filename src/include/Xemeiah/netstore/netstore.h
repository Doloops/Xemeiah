#ifndef __XEM_NETSTORE_NETSTORE_H
#define __XEM_NETSTORE_NETSTORE_H
#include <Xemeiah/kern/store.h>

#include <pthread.h>

#include <list>

namespace Xem
{
  typedef int Socket;
  class QueryDocument;
  class NetBranchManager;

  class NetStore : public Store
  {
  protected:
    NetBranchManager* branchManager;

    std::list<Socket> serverSockets;
    
    String connectionString;
    
    pthread_mutex_t socketLock;
    
    void bootstrap ();
    
    bool queryServer ( const String& action, const String& arguments, QueryDocument& queryDocument );

    ElementRef queryXMLResult ( const String& action, const String& arguments, QueryDocument& queryDocument, KeyCache& keyCache );
    
    Socket getServerSocket ();
    void releaseServerSocket ( Socket sock, bool failed );


    /**
     * Key persistence : add a key in the store.
     * @param keyName the local part of the key
     * @return a new LocalKeyId corresponding to the key
     */
    virtual LocalKeyId addKeyInStore ( const char* keyName );

    /**
     * Namespace persistence : add a namespace to the store.
     * @param namespaceURL the URL of the namespace
     * @return a new NamespaceId for it.
     */
    virtual NamespaceId addNamespaceInStore ( const char* namespaceURL );

    /**
     * Element Indexing
     * Free elementIds are reserved per-revision to reduce access to SuperBlock
     */ 
    virtual bool reserveElementIds ( ElementId& nextId, ElementId& lastId );

  public:
    NetStore ();
    ~NetStore ();
    
    bool open ( const String& connectionString );

    virtual BranchManager& getBranchManager();

    String queryString ( const String& action, const String& arguments );
    Document* queryXMLDocument ( const String& action, const String& arguments );
    
    bool queryData ( const String& action, const String& arguments, void*& buff, __ui64& buffSize );
    
  };
};

#endif // __XEM_NETSTORE_NETSTORE_H

