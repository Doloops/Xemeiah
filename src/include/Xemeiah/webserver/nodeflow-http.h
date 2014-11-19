#ifndef __XEM_WEBSERVER_NODEFLOW_HTTP_H
#define __XEM_WEBSERVER_NODEFLOW_HTTP_H

#include <Xemeiah/kern/format/core_types.h>
#include <Xemeiah/nodeflow/nodeflow-stream.h>

namespace Xem
{
  class BlobRef;
  
  /**
   * NodeFlowHTTP : transform a NodeFLow XProcessor output to a HTTP response
   */
  class NodeFlowHTTP : public NodeFlowStream
  {
  protected:
    /**
     * The effective socket to use
     */
    int sock;

    /**
     * The result code of this answer
     */
    String resultCode;    

    /**
     * The HTTP mime-type content type
     */
    String contentType;

    /**
     * Optional charset to provide
     */
    String charset;

    /**
     * Actual content length to serialize
     */
    __ui64 contentLength;
    
    /**
     * Class : List of HTTP headers to serialize
     */
    typedef std::map<String, String> ResponseParams;

    /**
     * List of HTTP headers to serialize
     */
    ResponseParams responseParams;
    
    /**
     * Serialize my header
     */
    void serializeHeader ();
  public:
    /**
     * NodeFlowHTTP constructor, must provide the socket to use
     */
    NodeFlowHTTP ( XProcessor& xproc, int sock );

    /**
     * NodeFlowHTTP destructor
     */
    virtual ~NodeFlowHTTP ();

    /**
     * Convert a generic NodeFlow to my NodeFlowHTTP
     */
    static NodeFlowHTTP& asNodeFlowHTTP ( NodeFlow& nodeFlow )
    { return dynamic_cast<NodeFlowHTTP&> ( nodeFlow ); }
    
    /**
     * Add an header to the response (in the form Name: Value)
     */
    void addParam ( const String& name, const String& value );

    /**
     * Set the result code for this response
     */
    void setResultCode ( const String& resultCode );
    
    /**
     * Set the mime type for this response
     */
    void setContentType ( const String& mimeType );
    
    /**
     * Force the content length for this response
     */
    void setContentLength ( __ui64 length );
    
    /**
     * Serialize HTTP response contents, using HTTP headers and serialized contents provided by the NodeFlowStream class
     */
    void serialize ();

    /**
     * Specialized response : serialize a blob response
     */
    void serializeBlob ( BlobRef& blobRef, __ui64 rangeStart, __ui64 rangeEnd, bool doSerializeHeader = true );

    /**
     * Specialized response : serialize a file
     */
    void serializeFile ( const String& href, __ui64 rangeStart, __ui64 rangeEnd );
    
    /**
     * Direct write to sock : serialize raw contents
     */
    void serializeRawResponse ( void* buff, __ui64 length );

    /**
     * Force to mark that the HTTP response was sent (ie prevent future writing)
     */
    void finishSerialization ();
  };

};

#endif // __XEM_WEBSERVER_NODEFLOW_HTTP_H
