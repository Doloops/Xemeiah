#ifndef __XEM_WEBSERVER_QUERYDOCUMENT_H
#define __XEM_WEBSERVER_QUERYDOCUMENT_H

#include <Xemeiah/kern/volatiledocument.h>
#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/webserver/webserver.h>
#include <Xemeiah/io/socketreader.h>

// #define __XEM_WEBSERVER_QUERYDOCUMENT_HAS_HEADERFIELDSMAP //< Option : build a header fields map while parsing

namespace Xem
{
  class String;
  class Store;
  class ElementRef;
  class BlobRef;
  class WebServerModuleForge;

  /**
   * QueryDocument is responsible for parsing the HTTP query into a DOM representation using the xem-web namespace.
   */
  class QueryDocument : public VolatileDocument
  {
  protected:
    /**
     * A reference to our WebServerModuleForge for convenience instanciation
     */
    WebServerModuleForge& webServerModuleForge;
    
    /**
     * The client socket we have been instanciated for
     */
    Socket sock;

    /**
     * The binary stream reader based on the sock
     */
    SocketReader reader;
    
    /**
     * The root xem-web:query element representing the parsed HTTP query
     */
    ElementRef queryElement;

#ifdef __XEM_WEBSERVER_QUERYDOCUMENT_HAS_HEADERFIELDSMAP
    /**
     * Class : mapping between name and value of header fields
     */
    typedef std::map<String,String> HeaderFieldsMap;

    /**
     * Instance : mapping between name and value of header fields
     */
    HeaderFieldsMap headerFieldsMap;
#endif
    

    /**
     * Facility function : add an element containing a text() node
     * @param from the element from which a new element will be created
     * @param keyId the keyId of the container element to create
     * @param contents the text to add in the container element
     */
    ElementRef addTextualElement ( ElementRef& from, KeyId keyId, const String& contents );

    /**
     * Parse contents as XML, and put the result in the xem-web:contents element in queryElement
     * @param xproc use this XProcessor to instanciate
     */
    void parseContents ( XProcessor& xproc );
    
    /**
     * Parse POST form as XML, putting the result in content and creating Blob when appropriate
     * @param content the element containing the parsed DOM
     */
    void parseFormData ( ElementRef& content );
    
    /**
     * Parse contents to blob
     * @param blob the blob to write to
     * @param contentLength number of bytes to read
     * @param offset offset to start from when writing to blob
     */
    void parseToBlob ( BlobRef& blob, __ui64 contentLength, __ui64 offset = 0 );

    /**
     * Parse chuncked encoding to blob
     */
    void parseChunkedToBlob ( BlobRef& blob );
  public:
    /**
     * Reference to the xem_web namespace
     */
    __BUILTIN_NAMESPACE_CLASS(xem_web) &xem_web;

    /**
     * QueryDocument constructor
     * @param store a reference to a Store
     * @param documentAllocator a reference to the documentAllocator to use for this QueryDocument
     * @param webServerModuleForge a reference to the WebServerModuleForge
     * @param sock the client socket to parse data from
     */
    QueryDocument ( Store& store, DocumentAllocator& documentAllocator, WebServerModuleForge& webServerModuleForge, Socket sock );
    
    /**
     * QueryDocument destructor
     */
    ~QueryDocument ();
    
    /**
     * Parse the query
     * @param xproc the XProcessor to use
     * @param isQuery set to true for a HTTP/1.1 query, false for a HTTP/1.1 response
     */
    void parseQuery ( XProcessor& xproc, bool isQuery );
    
    /**
     * Get the query Element containing the parsed HTTP query
     * @return the xem-web:query Element
     */
    ElementRef getQueryElement () const { return queryElement; }

  };
};

#endif // __XEM_WEBSERVER_QUERYDOCUMENT_H

