#if 0 // DEPRECATED STUFF FROM QUERYDOCUMENT

/*
 * querydocument-deprecated.cpp
 *
 *  Created on: 6 févr. 2010
 *      Author: francois
 */

// #define __XEM_WEB_QUERYDOCUMENT_DUMP
// #define __XEM_WEB_QUERYDOCUMENT_DUMP_HEAD
// #define __XEM_WEB_QUERYDOCUMENT_PARSEFORMDATA_DUMP
// #define __XEM_WEB_QUERYDOCUMENT_SERIALIZE_BLOB_FAKE

inline String QueryDocument::URLDecode ( const String& src )
  {
    size_t len = src.size() + 1;
    char* dest = (char*) malloc ( len );
    char* d = dest;
    int hexVal = 0;
    for ( const char* s = src.c_str() ; *s ; s++ )
      {
        hexVal = 0;
        switch ( *s )
        {
          case '%':
            s++;
            if ( ! *s )
              {
                free ( dest );
                Error ( "Invalid non-finished '%%'\n" );
                return NULL;
              }
            if ( 'a' <= *s && *s <= 'f' )
              hexVal += *s - 'a' + 10;
            else if ( 'A' <= *s && *s <= 'F' )
              hexVal += *s - 'A' + 10;
            else if ( '0' <= *s && *s <= '9' )
              hexVal += *s - '0';
            hexVal *= 16; s++;
            if ( ! *s )
              Bug ( "Invalid non-finished '%%'\n" );
            if ( 'a' <= *s && *s <= 'f' )
              hexVal += *s - 'a' + 10;
            else if ( 'A' <= *s && *s <= 'F' )
              hexVal += *s - 'A' + 10;
            else if ( '0' <= *s && *s <= '9' )
              hexVal += *s - '0';
            *d = hexVal; d++;
            break;
          default:
            *d = *s;
            d++;
        }
      }
    *d = '\0';
    return stringFromAllocedStr(dest);
  }

  char QueryDocument::getNextChar ()
  {
    if ( buffIdx < bytesRead )
      {
        char c = buff[buffIdx];
        buffIdx++;
        return c;
      }
    bytesRead = read ( sock, buff, bufferSize );
    if ( bytesRead <= 0 )
      {
        throwException ( Exception, "Could not read from sock %d : bytesRead=%lu\n", sock, (unsigned long) bytesRead );
      }
    buff[bytesRead] = '\0';

#ifdef __XEM_WEB_QUERYDOCUMENT_DUMP_HEAD
    int rfd = open ( "/tmp/xem_web_querydocument_dump_head", O_CREAT|O_APPEND|O_RDWR, 00644 );
    write ( rfd, buff, bytesRead );
    const char* sep = "\n\n*****************************\n\n";
    write ( rfd, sep, strlen(sep) );
    close ( rfd );
#endif


    Log_QD ( "parseQuery : read '%d' bytes\n", bytesRead );
    buffIdx = 1;
    return buff[0];
  }

  void QueryDocument::parseURLLine ( const String& line )
  {
    static const int bufferSize = 40;
    char methodString[bufferSize], urlString[bufferSize], protocolString[bufferSize];
    int res = sscanf ( line.c_str(), "%s %s %s", methodString, urlString, protocolString );

    if ( res != 3 )
      {
        throwException ( Exception, "Could not parse URL line '%s', res='%d'\n", line.c_str(), res );
      }
    String decodedURLString = URLDecode ( urlString );
    if ( ! decodedURLString.size() )
      {
        throwException ( Exception, "Could not decode URL string !\n" );
      }

    addTextualElement ( queryElement, xem_web.url_source(), decodedURLString );
    addTextualElement ( queryElement, xem_web.protocol(), protocolString );
    addTextualElement ( queryElement, xem_web.method(), methodString );

    ElementRef url = createElement ( queryElement, xem_web.url() );
    queryElement.appendLastChild ( url );

    String modifiedUrlString = stringFromAllocedStr(strdup(urlString));
    char* baseString = (char*) modifiedUrlString.c_str();// decodedURLString.c_str();
    if ( baseString[0] == '/' ) baseString++;

    char* param = strchr ( baseString, '?' );
    if ( param )
      {
        param[0] = '\0';
        param++;
      }

    addTextualElement ( url, xem_web.base(), baseString );
    ElementRef params = createElement ( url, xem_web.parameters() );
    url.appendLastChild ( params );
    while ( param )
      {
        char* value = strchr ( param, '=' );
        char* nextParam = strchr ( param, '&' );
        if ( nextParam )
          {
            nextParam[0] = '\0';
            nextParam++;
          }
        if ( value && ( ! nextParam || ( nextParam > value ) ) )
          {
            value[0] = '\0';
            value++;
          }
        else
            value = NULL;

        Log_QD ( "param = '%s', value = '%s'\n", param, value );

        ElementRef eltParam = createElement ( params, xem_web.param() );
        params.appendLastChild ( eltParam );
        eltParam.addAttr ( xem_web.name(), param );
        if ( value )
          {
            String decodedValue = URLDecode ( value );
            if ( value[0] && !decodedValue.size() )
              {
                throwException ( Exception, "Could not decode value '%s' !\n", value );
              }
            ElementRef valueNode = createTextNode ( eltParam, decodedValue.c_str() );
            eltParam.appendLastChild ( valueNode );
          }
        param = nextParam;
      } // while ( param )
  }

  void QueryDocument::parseCookies ( ElementRef& cookiesList, const String& line )
  {
    AssertBug ( stringStartsWith(line,"Cookie: "), "Invalid cookie line !\n" );
    String cookiesLine = &((line.c_str())[7]);
    std::list<String> cookiesTokens;
    cookiesLine.tokenize(cookiesTokens,';');
    for ( std::list<String>::iterator cookie = cookiesTokens.begin() ; cookie != cookiesTokens.end() ; cookie++ )
      {
        std::list<String> ckToken;
        cookie->tokenize(ckToken, '=');
        String cookieName = ckToken.front().trim();
        ckToken.pop_front();

        String cookieValue;
        while ( ckToken.size() )
          {
            if ( cookieValue.size() )
              cookieValue += "=";
            cookieValue += ckToken.front();
            ckToken.pop_front();
          }

        ElementRef cookieElt = createElement ( cookiesList, xem_web.cookie() );
        cookiesList.appendLastChild ( cookieElt );
        cookieElt.addAttr ( xem_web.name(), cookieName );
        ElementRef cookieValueNode = createTextNode ( cookieElt, cookieValue.c_str() );
        cookieElt.appendLastChild ( cookieValueNode );

      }

    char cookieName[bufferSize], cookieValue[bufferSize];
    int cookieNameIdx = 0, cookieValueIdx = 0;
    const char* g = NULL;


    for ( g = &(line.c_str()[7]) ; *g ; g++ )
      {
        if ( *g == ' ' ) continue;
        if ( *g == '=' ) { g++; break; }
        if ( cookieNameIdx == bufferSize-1)
          {
            Bug ( "Too long name for cookie.\n" );
          }
        cookieName[cookieNameIdx++] = *g;
      }
    cookieName[cookieNameIdx++] = '\0';

    for ( ; *g ; g++ )
      {
        cookieValue[cookieValueIdx++] = *g;
      }
    cookieValue[cookieValueIdx++] = '\0';
    Log_QD ( "Cookie : '%s' = '%s'\n", cookieName, cookieValue );

    ElementRef cookie = createElement ( cookiesList, xem_web.cookie() );
    cookiesList.appendLastChild ( cookie );
    cookie.addAttr ( xem_web.name(), cookieName );
    ElementRef cookieValueNode = createTextNode ( cookie, cookieValue );
    cookie.appendLastChild ( cookieValueNode );
  }

  void QueryDocument::parseHeaderField ( ElementRef& headersList, const String& line )
  {
    Log_QD ( "[HTTP] parseHeaderField line : %s\n", line.c_str() );
    if ( stringStartsWith(line,"HTTP/1.1") )
      {
        Log_QD ( "Status line : '%s'\n", line.c_str() );
        static const int bufferSize = 40;
        char protocolString[bufferSize], responseCode[bufferSize], responseString[bufferSize];
        int res = sscanf ( line.c_str(), "%s %s %s", protocolString, responseCode, responseString);

        if ( res != 3 )
          {
            throwException ( Exception, "Could not parse URL line '%s', res='%d'\n", line.c_str(), res );
          }
        ElementRef eltResponse = createElement ( headersList, xem_web.response() );
        headersList.appendLastChild ( eltResponse );

        eltResponse.addAttr ( xem_web.response_code(), responseCode );
        eltResponse.addAttr ( xem_web.response_string(), responseString );
        return;
      }
    char* midval = (char*) strstr(line.c_str(), ": ");
    if ( midval )
      {
        char* propname = strdup(line.c_str());
        midval = strstr(propname, ": ");
        midval[0] = 0;
        midval+=2;

        ElementRef eltParam = createElement ( headersList, xem_web.param() );
        headersList.appendLastChild ( eltParam );
        eltParam.addAttr ( xem_web.name(), propname );
        ElementRef valueNode = createTextNode ( eltParam, midval );
        eltParam.appendLastChild ( valueNode );

        Log_QD ( "New prop : %s = %s\n", propname, midval );

        if ( strcmp ( propname, "Transfer-Encoding") == 0 ) // && strcmp ( midval, "chunked") == 0 )
          {
            transferEncoding = stringFromAllocedStr(strdup(midval));
          }
        else if ( strcmp ( propname, "Content-Encoding" ) == 0 )
          {
            contentEncoding = stringFromAllocedStr(strdup(midval));
          }
        free ( propname );
      }
    else
      {
        Error ( "Invalid null property for line '%s'\n", line.c_str() );
        throwException(Exception, "Invalid null property for line '%s'\n", line.c_str() );
      }
    /*
     * Now parse that property
     */
    if ( strncmp ( line.c_str(), "Content-Length:", 15 ) == 0 )
      {
        contentLength = atoi ( &(line.c_str()[15]) );
        if ( contentLength == 0 )
          {
            Warn ( "Invalid contentLength : '%s'\n", line.c_str() );
          }
        else
          {
            Log_QD ( "Content length : '%llu'\n", contentLength );
          }
      }
    else if ( strncmp ( line.c_str(), "Content-Type: ", 14 ) == 0 )
      {
        contentType = stringFromAllocedStr ( strdup(&(line.c_str()[14])) );
      }
  }

  void QueryDocument::parseQuery ( XProcessor& xproc, bool hasURLLine )
  {
#define __INVALID(...) do { Warn (__VA_ARGS__); return false; } while (true)
#define __nextChar()                                                    \
    if ( buffIdx + 1 == bytesRead ) { foundEndOfHeader = true; Warn ( "Header too long.\n" );   break; }  \
    buffIdx ++;

    ElementRef cookiesList = queryElement.getDocument().createElement ( queryElement, xem_web.cookies() );
    queryElement.appendLastChild ( cookiesList );

    ElementRef headersList = queryElement.getDocument().createElement ( queryElement, xem_web.headers() );
    queryElement.appendLastChild ( headersList );

    bool foundEndOfHeader = false;

    String line;

    while ( ! foundEndOfHeader )
      {
        char c = getNextChar ();
        if ( c == '\r' ) continue;
        if ( c == '\n' )
          {
            Log_QD ( "[HTTP] LINE=[%s]\n", line.c_str() );
            if ( line.size() == 0 )
              {
                foundEndOfHeader = true;
                break;
              }
            else if ( hasURLLine )
              {
                parseURLLine ( line );
                hasURLLine = false;
              }
            else if ( stringStartsWith ( line, "Cookie:" ) )
              {
                parseCookies ( cookiesList, line );
              }
            else
              {
                parseHeaderField ( headersList, line );
              }
            line = "";
            continue;
          }
        line += c;
      }

    if ( ! foundEndOfHeader )
      {
        throwException ( Exception, "Could not find end of header !\n" );
      }

    if ( contentLength || ( transferEncoding == "chunked" ) )
      {
        Log_QD ( "Parsed contentLength : %llx\n", contentLength );
        parseContents ( xproc);
      }
#ifdef __XEM_WEB_QUERYDOCUMENT_DUMP
    queryElement.toXML ( stdout, 0 );
    fflush ( stdout );
#endif //  __XEM_WEB_QUERYDOCUMENT_DUMP
  }

  void QueryDocument::serializeToBlob ( BlobRef& blobRef )
  {
    Bug ( "." );
    if ( !hasBinaryContents() )
      {
        throwException ( Exception, "Query has no binary contents !\n" );
      }
    if ( transferEncoding == "chunked" )
      {
        Log_QBlob ( "bytesRead=%ld, buffIdx=%ld\n", (long int) bytesRead, (long int) buffIdx );

#define __reload() \
  do { buffIdx = 0;     bytesRead = read ( sock, buff, bufferSize ); \
  if ( bytesRead <= 0 ) \
    { \
      throwException ( Exception, "Could not read from sock %d : bytesRead=%lu\n", sock, (unsigned long) bytesRead ); \
    } \
  Log_QBlob ( "bytesRead=%ld, buffIdx=%ld\n", (long int) bytesRead, (long int) buffIdx ); \
  buff[bytesRead] = '\0'; } while (0)

        __ui64 chunkOffset = 0;
        while ( true )
          {
            String chunkLengthStr;
            while ( true )
              {
                if ( buffIdx == bytesRead )
                  {
                    __reload();
                  }
                char c = buff[buffIdx++];
                if ( c == '\r' ) continue;
                if ( c == '\n' ) break;
                chunkLengthStr += c;
              }
            __ui64 chunkLength;
            chunkLength = strtoull(chunkLengthStr.c_str(),NULL, 16);

            Log_QBlob ( "chunkLength : %s => 0x%llx (%llu bytes)\n", chunkLengthStr.c_str(), chunkLength, chunkLength );

            if ( chunkLength == 0 )
              {
                Log_QBlob ( "Finished chunk !\n" );
                break;
              }

            __ui64 fromOffset = buffIdx;
            for ( __ui64 cursor = 0 ; cursor < chunkLength ; cursor++ )
              {
                if ( buffIdx == bytesRead )
                  {
                    Log_QBlob ( "[BLOB] fromOffset=%lx, count=%lx, start=%llx\n",
                        (long int) fromOffset, (long int) (buffIdx-fromOffset), chunkOffset );
                    blobRef.writePiece(&(buff[fromOffset]), buffIdx - fromOffset, chunkOffset );
                    chunkOffset += buffIdx - fromOffset;
                    __reload ();
                    fromOffset = 0;
                  }
                buffIdx++;
              }
            Log_QBlob ( "After read : buffIdx=%ld\n", (long int) buffIdx );
            if ( fromOffset < buffIdx )
              {
                Log_QBlob ( "[BLOB] fromOffset=%lx, count=%lx, start=%llx\n",
                    (long int) fromOffset, (long int) (buffIdx-fromOffset), chunkOffset );
                blobRef.writePiece(&(buff[fromOffset]), buffIdx - fromOffset, chunkOffset );
                chunkOffset += buffIdx - fromOffset;
              }

            while ( true )
              {
                if ( buffIdx == bytesRead )
                  {
                    __reload();
                  }
                char c = buff[buffIdx++];
                Log_QBlob ( "c=%c (%d)\n", c, c );
                if ( c == '\r' ) continue;
                if ( c == '\n' ) break;
              }
          }
        Log_QBlob ( "Total blob : 0x%llx (%llu bytes)\n", chunkOffset, chunkOffset );
        return;
      }
    if ( contentLength == 0 )
      {
        Warn ( "Serialize with a zero contentLength !\n" );
        return;
      }
    __ui64 offset = 0;

    Log_QBlob ( "Serialize blob, buffIdx=0x%llx, bytesRead=0x%llx, contentLength=0x%llx\n",
      (__ui64) buffIdx, (__ui64) bytesRead, contentLength );

    if ( contentLength < ( bytesRead-buffIdx ) )
      {
        Warn ( "That is very stupid because, we have been provided too much already.\n" );
        throwException ( Exception, "Client is sending strange things.\n" );
      }

    /**
     * First, serialize the contents that we have parsed while reading the header
     */
    while ( buffIdx < bytesRead )
      {
        /*
         * First, write remaining part fetched at header time
         */
        void* piece;

        /**
         * We deliberately get a large window, for Blob to allocate good size.
         */
        __ui64 toWrite = blobRef.allowWritePiece ( &piece, contentLength - offset, offset );
        __ui64 written = toWrite < bytesRead - buffIdx ? toWrite : bytesRead - buffIdx;

        AssertBug ( written <= toWrite, "." );
        AssertBug ( written <= bytesRead - buffIdx, "." );
        Log_QBlob ( "toWrite=0x%llx, bytesRead-buffIdx=0x%llx, written=0x%llx\n",
            toWrite, (__ui64)(bytesRead-buffIdx), written );
#ifdef __XEM_WEB_QUERYDOCUMENT_SERIALIZE_BLOB_FAKE

#else
        memcpy ( piece, &(buff[buffIdx]), written );
#endif
        blobRef.protectWritePiece ( piece, toWrite );
        offset += written;
        buffIdx += written;
#if 0
        __ui64 res = blobRef.writePiece ( &(buff[buffIdx]), bytesRead - buffIdx, offset );
        if ( !res ) throwException ( Exception, "Could not write !!!\n" );
        offset += res;
#endif
      }
    Log_QBlob ( "After header serialize, offset=0x%llx\n", offset );
    buffIdx = 0;
    bytesRead = 0;
    /**
     * Next, read the contents on the socket
     */
    while ( offset < contentLength )
      {
        void* piece = NULL;

        Log_QBlob ( "LOOP BEGIN : offset=0x%llx, remains=0x%llx\n", offset, contentLength-offset );
        __ui64 toWrite = blobRef.allowWritePiece ( &piece, contentLength - offset, offset );
        if ( !toWrite ) throwException ( Exception, "Could not write !!!\n" );

        __ui64 written = toWrite < contentLength-offset ? toWrite : contentLength-offset;

#ifdef __XEM_WEB_QUERYDOCUMENT_SERIALIZE_BLOB_FAKE
        // char buff[4096];
        char* buff = (char*) malloc(written);
        ssize_t readBytes = read ( sock, buff, written );
        free ( buff );
#else
        ssize_t readBytes = read ( sock, piece, written );
#endif
        blobRef.protectWritePiece ( piece, toWrite );
        if ( readBytes <= 0 )
          {
            Log_QBlob ( "Finished (or premature end of) reading.\n" );
            break;
          }
        if ( toWrite != (__ui64) readBytes )
          {
            Log_QBlob ( "Partial read : toWrite=0x%llx, readBytes=0x%llx\n",
                (__ui64) toWrite, (__ui64) readBytes );
          }
        offset += readBytes;
        Log_QBlob ( "LOOP END : offset=0x%llx, contentLength=0x%llx, remains=0x%llx\n", offset, contentLength, contentLength-offset );
      }
    blobRef.check ();
#undef Log_QBlob
    hasBinaryContents_ = false;
  }

  void QueryDocument::parseContents ( XProcessor& xproc )
  {
    Log_QD ( "Parsing contents contentType='%s', contentLength=%llu.\n", contentType.c_str(), contentLength );
    ElementRef content = createElement ( queryElement, xem_web.content() );
    queryElement.appendLastChild ( content );

    if ( 0 )
      {

      }
#if 0
    else if ( contentType == "text/html" || contentType == "application/xml" )
      {
        EventHandlerDom eventHandler ( xproc, content );
        eventHandler.setKeepTextAtRootElement ( true );
        SocketFeeder feeder ( sock, buff, buffIdx, bytesRead, bufferSize );

        feeder.setTotalToRead ( contentLength );

        Parser parser ( feeder, eventHandler );

        parser.parse (); //< May throw exceptions

        Log_QD ( "Parse content end.\n" );
      }
#endif
    else if ( stringStartsWith ( contentType, "multipart/form-data;" ) )
      {
        parseFormData ( content );
      }
    else
      {
        hasBinaryContents_ = true;
        if ( contentEncoding == "gzip" )
          {
            // Bug ( "." );
            throwException ( Exception, "Not implemented : contentEncoding = '%s'\n", contentEncoding.c_str() );
#if 0 // IMMATURE
            BlobRef deflatedBlob = content.addBlob(xem_web.blob_contents_gzip());
            serializeToBlob(deflatedBlob);
            BlobRef inflatedBlob = content.addBlob(xem_web.blob_contents());
            deflatedBlob.inflate(inflatedBlob);
#endif
          }
        else
          {
            BlobRef blob = content.addBlob(xem_web.blob_contents());
            serializeToBlob(blob);
          }
      }
  }

  void QueryDocument::parseFormData ( ElementRef& content )
  {
    String boundary = substringAfter(contentType, "boundary=");
    if ( boundary.size() == 0 ) throwException ( Exception, "Invalid boundary set for multipart/form-data\n" );

    __ui64 current = 0;
    __ui64 boundary_size = boundary.size();

    ElementRef currentPart ( queryElement.getDocument() );
    BlobRef currentBlob ( queryElement.getDocument() );
    String line;

    bool inPartContents = false;

#ifdef __XEM_WEB_QUERYDOCUMENT_PARSEFORMDATA_DUMP
    int tmpfd = open ( "form-data-dir.dump", O_CREAT|O_RDWR, 00644 );
#endif

    __ui64 postBufferSz = 4096 + boundary_size;
    __ui64 postBufferIdx = 0;
    __ui64 postBufferWriteRemains = boundary_size;
    __ui64 postBufferWriteChunk = postBufferSz - postBufferWriteRemains;
    __ui64 blobOffset = 0;

    bool hadWrittenPartial = false;

    char* postBuffer = (char*) malloc ( postBufferSz );

#define __addChar(__c) \
    do { \
    if ( postBufferIdx == postBufferSz ) Bug("Too large"); \
    postBuffer[postBufferIdx] = __c; postBufferIdx++; } while(0)

    while ( current < contentLength )
      {
        char c = getNextChar(); current++;
        __addChar(c);

        if ( postBufferIdx >= boundary_size && strncmp ( &(postBuffer[postBufferIdx-boundary_size]), boundary.c_str(), boundary_size ) == 0 )
          {
            Warn ( "Found a boundary !\n" );
            if ( inPartContents && postBufferIdx > boundary_size )
              {
                // We shall flush that stuff !
                postBufferIdx -= boundary_size;

                // Rewind to the last '\n' we have found in stream
                while ( postBufferIdx && postBuffer[postBufferIdx] != '\n' ) postBufferIdx --;
                if ( postBufferIdx && postBuffer[postBufferIdx] == '\n' ) postBufferIdx --;

                Warn ( "--> Finalize : partial=%d, idx=0x%llx,  blobOffset=0x%llx\n", hadWrittenPartial, postBufferIdx, blobOffset );
                if ( hadWrittenPartial && postBufferIdx > postBufferWriteChunk )
                  {
#ifdef __XEM_WEB_QUERYDOCUMENT_PARSEFORMDATA_DUMP
                    pwrite ( tmpfd, &(postBuffer[postBufferWriteChunk]), postBufferIdx - postBufferWriteChunk, blobOffset );
#endif
                    currentBlob.writePiece ( &(postBuffer[postBufferWriteChunk]), postBufferIdx - postBufferWriteChunk, blobOffset );
                    blobOffset += postBufferIdx - postBufferWriteChunk;
                  }
                else
                  {
#ifdef  __XEM_WEB_QUERYDOCUMENT_PARSEFORMDATA_DUMP
                    pwrite ( tmpfd, postBuffer, postBufferIdx, blobOffset );
#endif
                    currentBlob.writePiece ( postBuffer, postBufferIdx, blobOffset );
                    blobOffset += postBufferIdx;
                  }
                Warn ( "--> After finalize, blobOffset = 0x%llx\n", blobOffset );
              }
            postBufferIdx = 0;

            currentPart = createElement ( content, xem_web.part() );
            content.appendLastChild ( currentPart );
            currentBlob = currentPart.addBlob ( xem_web.blob_contents() );

            line = "(boundary)";
            while ( current < contentLength )
              {
                char c = getNextChar (); current++;
                if ( c == '\r' ) continue;
                if ( c == '\n' )
                  {
                    Warn ( "--> Header : '%s'\n", line.c_str() );
                    if ( line.size() == 0 || line == "\r" )
                      {
                        hadWrittenPartial = false;
                        inPartContents = true;
                        postBufferIdx = 0;
                        break;

                      }
                    if ( ! stringStartsWith(line,"(boundary)" ) )
                      parseHeaderField ( currentPart, line );
                    line = "";
                    continue;
                  }
                line += c;
              }
            Warn ( " End of header !\n" );
            if ( current == contentLength ) { Warn ( "End of contents !\n" ); break; }
            continue;
          }
        if ( postBufferIdx == postBufferSz )
          {
            Warn ( "--> Writting chunk '0x%llx/0x%llx'\n", postBufferIdx, postBufferWriteChunk );

#ifdef  __XEM_WEB_QUERYDOCUMENT_PARSEFORMDATA_DUMP
            pwrite ( tmpfd, postBuffer, postBufferWriteChunk, blobOffset );
#endif
            currentBlob.writePiece ( postBuffer, postBufferWriteChunk, blobOffset );

            blobOffset += postBufferWriteChunk;
            memcpy ( postBuffer, &(postBuffer[postBufferWriteChunk]), postBufferWriteRemains );
            postBufferIdx = postBufferWriteRemains;
            hadWrittenPartial = true;
          }
      }

#ifdef  __XEM_WEB_QUERYDOCUMENT_PARSEFORMDATA_DUMP
    close ( tmpfd );
#endif
#if 0
    int rfd = open ( "form-data.dump", O_CREAT|O_RDWR, 00644 );
    __ui64 dumped = bytesRead-buffIdx;
    write ( rfd, &(buff[buffIdx]), bytesRead-buffIdx );
    while ( dumped < contentLength )
      {
        size_t windowsz = contentLength - dumped < bufferSize ? contentLength - dumped : bufferSize;
        ssize_t rd = read ( sock, buff, windowsz );
        write ( rfd, buff, rd );
      }
    close ( rfd );
#endif

  }

#endif
