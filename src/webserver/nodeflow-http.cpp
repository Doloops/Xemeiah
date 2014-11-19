#include <Xemeiah/webserver/nodeflow-http.h>
#include <Xemeiah/webserver/webserver.h>
#include <Xemeiah/dom/blobref.h>
#include <Xemeiah/io/bufferedwriter.h>

#include <Xemeiah/auto-inline.hpp>

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <unistd.h>

#define Log_NodeFlowHTTP Debug

namespace Xem
{
  NodeFlowHTTP::NodeFlowHTTP ( XProcessor& xproc, int _sock )
  : NodeFlowStream ( xproc )
  {
    sock = _sock;
    AssertBug ( sock >= 0, "Invalid sock provided !\n" );
    resultCode = "200 OK";
    contentType = "text/html";
    contentLength = 0;
    charset = "utf-8";
  }

  NodeFlowHTTP::~NodeFlowHTTP ()
  {
  
  }


  void NodeFlowHTTP::setResultCode ( const String& _resultCode )
  {
    resultCode = _resultCode;
  }

  void NodeFlowHTTP::setContentType ( const String& mimeType )
  {
    contentType = mimeType;
  }
  
  void NodeFlowHTTP::setContentLength ( __ui64 length )
  {
    contentLength = length;
  }

  void NodeFlowHTTP::addParam ( const String& name, const String& value )
  {
    if ( name == "Content-Length" || name == "Content-Type" ) 
      throwException ( Exception, "Invalid param '%s' : use appropriate function instead.\n", name.c_str() );
    responseParams[name] = value;    
  }

  void NodeFlowHTTP::serializeHeader ()
  {
    if ( sock == -1 )
      {
        Warn ( "(already serialized)\n" );
        return;
      }

    BufferedWriter header;
    header.doPrintf ( "HTTP/1.1 %s\r\n"
        "Response-Code: %d\r\n"
        "Content-Length: %llu\r\n"
        "Content-Type: %s" /* "; charset=%s" */ "\r\n",
        resultCode.c_str(), 
        atoi(resultCode.c_str()),
        contentLength, contentType.c_str()
        /* charset.c_str() */
        );

    for ( ResponseParams::iterator iter = responseParams.begin() ; iter != responseParams.end() ; iter++ )
      {
        Log_NodeFlowHTTP ( "Add response : '%s' = '%s'\n", iter->first.c_str(), iter->second.c_str() );
        header.doPrintf ( "%s: %s\r\n", iter->first.c_str(), iter->second.c_str() );
      }
      
    header.addStr ( "\r\n" );

    Log_NodeFlowHTTP ( "HEADER size : %llu\n%s\n", header.getBufferSize(), header.getBuffer() );

    ssize_t wrt = write ( sock, header.getBuffer(), header.getBufferSize() );
    
    if ( wrt < 0 )
      {
        sock = -1;
        throwException ( Exception, "Could not write header : sock=%d, error=%d:%s\n", sock, errno, strerror(errno) );
      }
    if ( (size_t) wrt != header.getBufferSize() )
      {
        Error ( "Could not write header : sock=%d, wrt=%lu, headerSz=%lu\n", sock, (long)wrt, (long)header.getBufferSize() );
        NotImplemented ( "Partial header write is not implemented !\n" );
      }
  }
  
  void NodeFlowHTTP::serialize ()
  {
    if ( sock == -1 )
      {
        Log_NodeFlowHTTP ( "(already serialized)\n" );
        return;
      }

    if ( contentLength == 0 )  contentLength = getContentsSize ();

    Log_NodeFlowHTTP ( "------- serialize with type : '%s'\n", resultCode.c_str() );
    Log_NodeFlowHTTP ( "------ contentLength = %llu (0x%llx) ---------------\n", contentLength, contentLength );
    Log_NodeFlowHTTP ( "------ contents : \n" );
    Log_NodeFlowHTTP ( "%s", getContents() );
    Log_NodeFlowHTTP ( "------ contents.\n" );
    
    serializeHeader ();
    
    if ( getContentsSize() ) 
      {
        ssize_t res = write ( sock, getContents(), getContentsSize() );
        if ( res < 0 )
          {
            sock = -1;
            throwException ( Exception, "Could not write() : err=%d:%s\n", errno, strerror(errno) );
          }
        if ( (size_t) res != getContentsSize() )
          {
            /*
             * TODO We shall loop until we wrote everything here !
             */
            Error ( "Could not write() all ! wrote %lu bytes out of %lu\n", (unsigned long) res, (unsigned long) getContentsSize() );
            NotImplemented ( "Partial contents not implemented !\n" );
          }
      }
    fsync ( sock );
    sock = -1;
  }

  void NodeFlowHTTP::serializeBlob ( BlobRef& blobRef, __ui64 rangeStart, __ui64 rangeEnd, bool doSerializeHeader )
  {
    Log_NodeFlowHTTP ( "Called with rangeStart=0x%llx, rangeEnd=0x%llx\n", rangeStart, rangeEnd );
    if ( ! blobRef.getSize() )
      {
        Warn ( "blobRef has a null size, nothing to serialize.\n" );
        contentLength = 0;
        serializeHeader ();
        return;
      }

    if ( rangeEnd >= blobRef.getSize() ) rangeEnd = blobRef.getSize() - 1;

    Log_NodeFlowHTTP ( "Post-correction : with rangeStart=0x%llx, rangeEnd=0x%llx, getSize()=0x%llx\n", rangeStart, rangeEnd, blobRef.getSize() );

    contentLength = rangeEnd + 1 - rangeStart;
    
    __ui64 offset = rangeStart;
    __ui64 remains = contentLength;

    if ( doSerializeHeader )
      serializeHeader ();
    
    
    while ( remains )
      {
        void* window;
        __ui64 windowSize = blobRef.getPiece ( &window, remains, offset );
        if ( windowSize > remains ) windowSize = remains;
        if ( !windowSize )
          {
            Warn ( "Could not read !, windowSize=%ld, offset=0x%llx, remains=0x%llx\n",
                (long) windowSize, offset, remains );
          }
        ssize_t written = write ( sock, window, windowSize );
        if ( written < 0 )
          {
            Error ( "Could not write(). err=%d:%s\n", errno, strerror(errno) );
            break;
          }
        if ( (__ui64) written != windowSize )
          {
            Error ( "could not write all ! written=0x%llx, windowSize=0x%llx\n",
                (__ui64) written, windowSize );
          }
        offset += written;
        remains -= written;
      }
    fsync ( sock );
    sock = -1;
  }


  void NodeFlowHTTP::serializeFile ( const String& filePath, __ui64 rangeStart, __ui64 rangeEnd )
  {
    Log_NodeFlowHTTP ( "Responding with file : '%s'\n", filePath.c_str() );
    int fileFd = ::open ( filePath.c_str(), O_RDONLY );
    
    if ( fileFd == -1 )
      throwException ( Exception, "Could not open file '%s'\n", filePath.c_str() );

    struct stat fileStat;
    
    if ( fstat ( fileFd, &fileStat ) == -1 )
      {
        ::close ( fileFd );
        throwException ( Exception, "Could not fstat() file '%s'\n", filePath.c_str() );
      }
      
    __ui64 size = (__ui64) fileStat.st_size;

    Log_NodeFlowHTTP ( " filePath is '%s', size is 0x%llx, range=[0x%llx:0x%llx], mimeType is '%s'\n",
        filePath.c_str(), size, rangeStart, rangeEnd, contentType.c_str() );
  
    if ( rangeStart >= size )
      {
        ::close ( fileFd );
        throwException ( Exception, "File '%s' : Invalid start 0x%llx : file size is 0x%llx\n", 
            filePath.c_str(), rangeStart, size );
      }

    if ( rangeEnd >= size ) rangeEnd = size - 1;

    off_t offset = rangeStart;
    size_t count = (rangeEnd+1) - rangeStart;
    
    setContentLength ( count );
    
    serializeHeader ();

    ssize_t sentBytes = sendfile ( sock, fileFd, &offset, count );
    
    if ( sentBytes < 0 )
      {
        ::close ( fileFd );
        throwException ( Exception, "Could not sendfile(%s) : sent=%ld, size=%ld, error=%d:%s\n",
          filePath.c_str(), (long) sentBytes, (long) size, errno, strerror(errno) );
      }
    
    if ( (size_t) sentBytes != count )
      {
        ::close ( fileFd );
        throwException ( Exception, "Could not sendfile(%s) all ! sentBytes=0x%llx, count=0x%llx\n",
          filePath.c_str(), (__ui64) sentBytes, (__ui64) count );
      }
      
    ::close ( fileFd );    

    fsync ( sock );  
    sock = -1;
  }
  
#if 0 // serializeData() is deprecated at the moment
  void NodeFlowHTTP::serializeData ( const String& mimeType, void* buff, __ui64 size )
  {
    Log_NodeFlowHTTP ( "[DATA] mimeType=%s\n", mimeType.c_str() );
    char header[PageSize];
    
    int headerSz = sprintf ( header,
        "HTTP/1.1 200 OK\r\n"
        "Response-Code: 200\r\n"
        "Content-Length: %llu\r\n"
        "Content-Type: %s\r\n\r\n",
        size, mimeType.c_str() );

    ssize_t written = write  ( sock, header, headerSz );
    
    if ( written != headerSz )
      {
        Error ( "Could not write full header ! written=%ld on %d\n", (long) written, headerSz );
      }
  
    __ui64 remains = size;
    __ui64 starts = 0;
  
    const char* buffc = (const char*) buff;
    while ( remains )
      {
        written = write ( sock, &(buffc[starts]), remains );
        
        if ( written == -1 )
          {
            Error ( "Could not write, returns -1, exiting.\n" );
            return;
          }
        
        if ( written != (ssize_t) remains )
          {
            Error ( "Could not write full packet ! written=%ld on %llu\n", (long) written, size );
          }
        remains -= written;
        starts += written;
      }    
    fsync ( sock );
    
    sock = -1;  
  }
#endif

  void NodeFlowHTTP::serializeRawResponse ( void* buff, __ui64 length )
  {
    int res = ::write ( sock, buff, length );
    if ( res == -1 )
      {
        throwException ( Exception, "Could not serialize : err=%d:%s\n", errno, strerror(errno));
      }
    else if ( (__ui64) res != length )
      {
        Warn ( "Partial raw response !\n" );
      }
    fsync ( sock );
  }

  void NodeFlowHTTP::finishSerialization ()
  {
    fsync ( sock );
    sock = -1;
  }
};
