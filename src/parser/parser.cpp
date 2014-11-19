#include <Xemeiah/kern/utf8.h>

#include <Xemeiah/io/filereader.h>
#include <Xemeiah/io/stringreader.h>
#include <Xemeiah/parser/parser.h>
#include <Xemeiah/trace.h>
#include <stdlib.h>
#include <string.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_Parser Debug
#define Log_Parser_Steps Debug
#define Log_Parser_AllocField Debug

#define __XEM_PARSER_EXCEPTION_IF_ENTITY_NOT_FOUND
#define __XEM_PARSER_PARSE_ENTITY
#define __XEM_PARSER_CONVERT_ENTITIES_TO_UTF8
#define __XEM_PARSER_CONVERT_HTML_ENTITIES_TO_UTF8

// #define __XMLPARSER_PARSE_NG_LOG_STEPS
#define __XEM_PARSER_NAMESPACE_VERBOUS_ASSERTFULL

namespace Xem
{

  Parser::Parser (BufferedReader& reader_, SAXHandler &saxHandler_) : saxHandler(saxHandler_), initialReader(reader_)
  {
    parseKeepAllText = false;
    parseKeepComments = true;
    currentReader = NULL;
  }

  Parser::~Parser ()
  {
  
  }
  
  Parser::Parsing::Parsing ()
  {
    specialLevel = 0;
    
    state = parseStateMarkupOutside;
    specialMarkup = specialMarkupNone;
    
    nameSz = nsSz = valueSz = textSz = entitySz = specialSz = 0;
    name = ns = value = text = special = entity = NULL;

    keywordInMarkup = 0;

    nameMx = nsMx = valueMx = textMx = entityMx = specialMx = 0;    

    initEntityTable ();
  }

  void Parser::Parsing::initEntityTable ()
  {
    xmlEntities[String("lt")] = String ( "<" );
    xmlEntities[String("gt")] = String ( ">" );
    xmlEntities[String("amp")] = String ( "&" );
    xmlEntities[String("quot")] = String ( "\"" );
    xmlEntities[String("apos")] = String ( "'" );
  }

  Parser::Parsing::~Parsing ()
  {
    free ( name );
    free ( value );
    free ( text );
    free ( entity );
    free ( special );
  }

  static const char * XMLParserStatesStr[] = 
  { "Oustide", // 0
    "Text", 
    "UNUSED", 
    "Enter", 
    "Name", 
    "Inside", // 5
    "AttrName",
    "AttrBeforeEqual", 
    "AttrEqual", 
    "AttrValue", 
    "UNUSED", // 10
    "End", 
    "EndName", 
    "PI", 
    "PIName", 
    "PIContents", // 15
    "PIEnd",
    "MarkupSpecialStart", 
    "MarkupSpecial", 
    "MarkupSpecialEnd", 
    "EndNamePost",  // 20
    "Entity",
    "DocTypeEntityDecl",
    "DocTypeEntityValue",
    "DocTypeEntityValQuot",
    "DocTypeEntityValEnd", // 25
    "DocTypeEntityValNData",
    "DocTypeEntityValNDVal",
    "MarkupComment",
    "MarkupCData",
    "DocTypeName", // 30
    "DocTypeValue",
    "DocTypeValQuot",
    NULL
  };

  String Parser::dumpCurrentContext ( int byte, BufferedReader& reader )
  {
    String context;
    stringPrintf ( context, "At char %d:'%c', state=%s, ",
		   byte, ( byte > 0x20 && byte < 0x80 ) ? byte : '?', XMLParserStatesStr[parsing.state] );
    context += reader.dumpCurrentContext ();

    return context;
  }

  /*
   * InvalidInput() logs
   * InvalidInputLogState is complex because we cannot touch the read buffer
   * In the other hand, we must take care that we do not explode the maximum GLog_Parser message size.
   */

  #define InvalidInput(...) \
    do { \
      ParserException * pe = new ParserException(); \
      detailException ( pe, __VA_ARGS__ ); \
      detailException ( pe, "At : \n" );  \
      detailException ( pe, "%s", dumpCurrentContext(curChar(), *currentReader).c_str() ); \
      throw ( pe ); \
    } while (0)

  #define AssertInvalid(__cond,...) \
    if ( ! (__cond) ) InvalidInput ( __VA_ARGS__ )

  /*
   * Transition between states
   * This shall be the last statement of a case:
   */
  
  #define willGoOutside() parsing.state = parseStateMarkupOutside; //< prepare to leave
  #define gotoState(__state) parsing.state = __state; goto case_begin;
  #define gotoSameState() goto case_begin;

  /*
   * Access to current Character
   */


  #define curChar() __curChar__

#define nextChar() { \
    if ( currentReader->isFinished() ) \
      { \
        dequeueReader(); \
        if ( currentReader->isFinished() ) goto case_end; \
      } \
    __curChar__ = currentReader->getNextChar(); }

#define __Actual_Step() "Step : state=%s, char=%d:'%c', line=%llu/b=%llu, specialLevel=%d, avd=%d\n", \
    XMLParserStatesStr[parsing.state], curChar(), curChar(), \
    currentReader->getTotalLinesParsed(), currentReader->getTotalParsed(), parsing.specialLevel, parsing.attrValueDelimiter

#define isText() ( ( curChar() >= 'A' && curChar() <= 'Z' )		\
             || ( curChar() >= 'a' && curChar() <= 'z' )		\
             || ( curChar() >= '0' && curChar() <= '9' )		\
             || ( curChar() == ':' ) || ( curChar() == '-' )	\
             || ( curChar() == '.' ) || ( curChar() == '_' ) || curChar() >= 0x80 )

#define isCur(__c) ( curChar() == __c )

  /*
   * Convenient macros to put the current character in the appropriate sub-buffer.
   * Parser uses 5 buffers 
   * name buffer, for local part of the keys
   * ns buffer, for namespace value (first put in name)
   * value buffer, for attribute values
   * text buffer, for textual contents of elements
   * entity buffer, for entity name parsing.
   */
  inline void __allocateField ( char*& field, __ui64 sz, __ui64 &mx, const char* __fieldName )
  {
    if ( mx == 0 ) mx = 32;
    if ( sz >= 1 << 30 )
      {
        throwException ( Exception, "Very large field '%s' value : 0x%llx (%llu Mbytes)\n", __fieldName, sz, sz >> 20 );
      }
    while ( mx < (sz+1) ) mx *= 2;
    field = (char*) realloc ( field, mx );
    if ( field == NULL )
      {
        throwException ( Exception, "Could not allocate field '%s' value up to 0x%llx (%llu Mbytes)\n", __fieldName, mx, mx >> 20 );
      }
    Log_Parser_AllocField ( "Reallocated field '%s' at %p, mx=0x%llx, sz=0x%llx\n", __fieldName, field, mx, sz );
  }

  #define checkSzMx_Size(__field,__required) \
    if ( parsing.__field##Sz + __required >= parsing.__field##Mx ) \
      __allocateField ( parsing.__field, parsing.__field##Sz + __required, parsing.__field##Mx, STRINGIFY(__field) );
  #define checkSzMx(__field) checkSzMx_Size(__field,0)

  #define __newField(__field) do { checkSzMx(__field); parsing.__field##Sz = 0; parsing.__field[parsing.__field##Sz] = '\0'; } while (0)

  #define __extendFieldChar(__field,__char) do { \
      if ( __char >= 0x80 ) \
        { checkSzMx_Size(__field,8); \
          int bytes = utf8CodeToChar ( __char, (unsigned char*) &(parsing.__field[parsing.__field##Sz]), 8 ); \
          parsing.__field##Sz += bytes; } \
      else { checkSzMx(__field); parsing.__field[parsing.__field##Sz++] = (__char); } \
    } while (0)      

  #define __extendField(__field) \
    do { checkSzMx(__field) ; __extendFieldChar(__field, curChar()); nextChar() } while ( 0 )
  #define __extendFieldStr(__field,__s) \
    do { for ( const char* __c = __s ; *__c ; __c++ ) \
     { checkSzMx (__field); __extendFieldChar(__field, (*__c)); } } while ( 0 ) 
  #define __finishField(__field) do { checkSzMx(__field); parsing.__field[parsing.__field##Sz] = '\0'; } while(0)

  #define newName() do { __newField(name); __newField(ns); } while(0)
  #define extendName() do { checkSzMx(name); \
    AssertInvalid ( curChar() != ':', "Invalid char in name\n" ); parsing.name[parsing.nameSz++] = curChar(); nextChar() } while (0)
  #define extendNameChar(__s) __extendFieldChar(name,__s)
  #define finishName() parsing.name[parsing.nameSz] = '\0'

  #define putNameInNs() do { if ( parsing.nsSz ) InvalidInput ( "Already have namespace '%s', setting '%s'\n", parsing.ns, parsing.name ); \
    checkSzMx_Size(ns,parsing.nameSz); memcpy ( parsing.ns, parsing.name, parsing.nameSz ); \
    parsing.nsSz = parsing.nameSz; parsing.ns[parsing.nsSz] = '\0'; parsing.nameSz = 0; parsing.name[0]='\0'; } while (0)

  #define pushText() \
    do { if ( parsing.textSz ) { finishText (); saxHandler.eventText ( parsing.text ); newText(); } } while ( 0 )

  #define newValue() __newField(value)
  #define extendValue() __extendField(value)
  #define extendValueChar(__s) __extendFieldChar(value,__s)
  #define extendValueStr(__s) __extendFieldStr(value,__s)
  #define finishValue() __finishField(value)

  #define newText() __newField(text)
  #define extendText() __extendField(text)
  #define extendTextChar(__s) __extendFieldChar(text,__s)
  #define extendTextStr(__s) __extendFieldStr(text,__s)
  #define finishText() __finishField(text)

  #define newEntity() __newField(entity)
  #define extendEntity() __extendField(entity)
  #define extendEntityStr(__s) __extendFieldStr(entity,__s)
  #define finishEntity() __finishField(entity)

  #define newSpecial() __newField(special)
  #define extendSpecial() __extendField(special)
  #define extendSpecialStr(__s) __extendFieldStr(special,__s)
  #define finishSpecial() __finishField(special)

  #define doExtendTextValue(__c) \
    if ( parsing.stateBeforeEntity == parseStateMarkupText ) \
      { extendTextStr ( __c ); } \
    else if ( parsing.stateBeforeEntity == parseStateMarkupAttrValue ) \
      { extendValueStr ( __c ); } \
    else { Bug ( "Invalid entity state before  '%d'\n", parsing.stateBeforeEntity ); }

  #define doExtendTextValueChar(__c) \
    if ( parsing.stateBeforeEntity == parseStateMarkupText ) \
      { extendTextChar ( __c ); } \
    else if ( parsing.stateBeforeEntity == parseStateMarkupAttrValue ) \
      { extendValueChar ( __c ); } \
    else { Bug ( "Invalid entity state before  '%d'\n", parsing.stateBeforeEntity ); }

  #define unnestSpecial() \
    if ( parsing.specialLevel == 0 ) { InvalidInput ( "Wrongly nested !!!\n" ); } \
    parsing.specialLevel --; \
    if ( parsing.specialLevel  ) { gotoState ( parseStateMarkupSpecialStart ); } \
    else                         { gotoState ( parseStateMarkupOutside ); }

  #define unnestSpecial_nextChar() \
    if ( parsing.specialLevel == 0 ) { InvalidInput ( "Wrongly nested !!!\n" ); } \
    parsing.specialLevel --; nextChar(); \
    if ( parsing.specialLevel  ) { gotoState ( parseStateMarkupSpecialStart ); } \
    else                         { gotoState ( parseStateMarkupOutside ); }


  void Parser::enqueueReader ( BufferedReader* reader )
  {
    StackedReader* sr = new StackedReader();\
    sr->reader = reader;
    sr->previousAttrValueDelimiter = parsing.attrValueDelimiter;
    parsing.attrValueDelimiter = 0;
    readerStack.push_front ( sr );
    Log_Parser ( "Enqueue reader current=%p, setting %p\n", currentReader, reader) ;
    currentReader = reader;
  }

  void Parser::dequeueReader ()
  {
    Log_Parser_Steps ( "CurrentReader Finished !\n" ); \
    while ( readerStack.size() )
    {
      StackedReader* sr = readerStack.front();
      readerStack.pop_front();
      parsing.attrValueDelimiter = sr->previousAttrValueDelimiter;
      delete ( sr->reader );
      delete ( sr );
      if ( readerStack.size() )
        currentReader = readerStack.front()->reader;
      else
        currentReader = &initialReader;
      if ( !currentReader->isFinished() )
        break;
    }
  }

  /*
   * The Parsing Function.
   * It is designed to be re-entrant, id est to resume exactly
   * where it has exited before with a throw of XMLParserException or XMLParserFullSpaceException
   */
  void Parser::internalParse ( )
  {
    currentReader = &initialReader;

    int __curChar__;
    
    /*
     * Read the first byte (to initialize, and to see what happens)
     */
    nextChar ();
    
    static const void * caseParseLabels[] = { 
      &&case_parseStateMarkupOutside, // 0
      &&case_parseStateMarkupText,
      &&case_parseStateUNUSED,
      &&case_parseStateMarkupEnter,
      &&case_parseStateMarkupName,
      &&case_parseStateMarkupInside, // 5
      &&case_parseStateMarkupAttrName,
      &&case_parseStateMarkupAttrBeforeEqual,
      &&case_parseStateMarkupAttrEqual,
      &&case_parseStateMarkupAttrValue,
      &&case_parseStateUNUSED, // 10
      &&case_parseStateMarkupEnd,
      &&case_parseStateMarkupEndName,
      &&case_parseStateMarkupPI,
      &&case_parseStateMarkupPIName,
      &&case_parseStateMarkupPIContents, // 15
      &&case_parseStateMarkupPIEnd,
      &&case_parseStateMarkupSpecialStart,
      &&case_parseStateMarkupSpecial,
      &&case_parseStateMarkupSpecialEnd,
      &&case_parseStateMarkupEndNamePost, //20
      &&case_parseStateEntity,
      &&case_parseStateDocTypeEntityDecl,
      &&case_parseStateDocTypeEntityValue,
      &&case_parseStateDocTypeEntityValQuot,      
      &&case_parseStateDocTypeEntityValEnd, // 25
      &&case_parseStateDocTypeEntityValNData,
      &&case_parseStateDocTypeEntityValNDVal,
      &&case_parseStateMarkupComment,
      &&case_parseStateMarkupCData,
      &&case_parseStateDocTypeName, // 30
      &&case_parseStateDocTypeValue,
      &&case_parseStateDocTypeValQuot, // 32
    };

  case_begin:
    Log_Parser_Steps ( __Actual_Step()  );
    goto *caseParseLabels[parsing.state];

  case_parseStateMarkupOutside:
    if ( isCur ( '<' ) )
      { nextChar(); gotoState ( parseStateMarkupEnter ); }
    if ( ! parseKeepAllText )
      {
        if ( isCur ( ' ' ) || isCur ( '\n' ) || isCur ( '\t' ) || isCur ( 13 ) || isCur ( 10 ) )
          { nextChar(); gotoSameState(); }
      }
    newText();
    gotoState ( parseStateMarkupText );
    
  case_parseStateMarkupText:
    if ( isCur ( '&' ) )
      { 
        nextChar(); 
        parsing.stateBeforeEntity = parseStateMarkupText;
        newEntity();
        gotoState ( parseStateEntity ); 
      }
    if ( isCur ( '<' ) )
      { 
        /*
         * We do not finish text right now, in order to skip processing instructions the easy way (concatenating before and after text gently)
         */
        nextChar(); gotoState ( parseStateMarkupEnter ); 
      }
    if ( isCur ( '\r' ) )
      {
        nextChar(); gotoSameState ();
      }
    extendText ();
    gotoSameState();

  case_parseStateMarkupEnter:
    if ( isCur ( '?' ) )
      {
        pushText();
        nextChar();
        gotoState ( parseStateMarkupPI );
      }
    if ( isCur ('!') )
      {
        // pushText();
        newName();
        parsing.specialLevel = 1;
        gotoState ( parseStateMarkupSpecialStart );
      }
    if ( isText() )
      { 
        pushText();
        newName ();
        gotoState ( parseStateMarkupName ); 
      }
    if ( isCur ( '/' ) )
      { 
        pushText();
        nextChar ();
        newName ();
        gotoState ( parseStateMarkupEndName ); 
      }
    InvalidInput ( "Invalid character at beginning of markup : '%c'\n", curChar() );
    
  case_parseStateMarkupName:
    AssertBug ( ! isCur ('!'), "Special markups (comments, PI, ...) must not enter MarkupName state !\n" );
    if ( isspace(curChar()) ) // isCur (' ') || isCur ('\n') || isCur ('\r') || isCur ('\t') )
      {
        if ( parsing.nameSz == 0 )
          {
            nextChar(); gotoSameState ();
          } 
        finishName ();
        saxHandler.eventElement ( parsing.ns, parsing.name );
        nextChar ();
        gotoState ( parseStateMarkupInside ); 
      }
    if ( isCur (':') )
      {
        putNameInNs ();
        nextChar(); gotoSameState ();
      }
    if ( isCur ('>') )
      {
        finishName ();
        saxHandler.eventElement ( parsing.ns, parsing.name );
        saxHandler.eventAttrEnd ();
        willGoOutside ();
        nextChar (); gotoState ( parseStateMarkupOutside ); 
      }
    if ( isCur ('/') )
      { 
        finishName ();
        saxHandler.eventElement ( parsing.ns, parsing.name );
        gotoState ( parseStateMarkupInside );  // BUG ???? Shall go to parseStateMarkupEnd
      }
    if ( isText () )
      { 
        extendName ();
        gotoSameState(); 
      }
    InvalidInput ( "Markup names can only contain alphanumeric characters (and char '%c' is not)\n", curChar() );
    
  case_parseStateMarkupInside:
    if ( isText() )
      {
        newName ();
        gotoState ( parseStateMarkupAttrName ); 
      }
    if ( isCur ( '/' ) )
      { 
        nextChar ();
        saxHandler.eventAttrEnd ();
        gotoState ( parseStateMarkupEnd ); 
      }
    if ( isCur ( '>' ) )
      { 
        willGoOutside ();
        nextChar ();
        finishName ();
        saxHandler.eventAttrEnd ();
        gotoState ( parseStateMarkupOutside ); 
      }
    if ( isCur (' ') || isCur ('\n') || isCur ('\r') || isCur ('\t') )
      { nextChar (); gotoSameState(); }
    InvalidInput ( "Invalid character inside of the markup.\n" );
    
  case_parseStateMarkupAttrName:
    if ( isCur (':') )
      {
        putNameInNs ();
        nextChar(); 
        gotoSameState();
      }
    if ( isText () )
      { 
        extendName();
        gotoSameState(); 
      }
    if ( isCur ( '=' ) )
      { 
        finishName ( ); nextChar ();
        gotoState ( parseStateMarkupAttrEqual ); 
      }
    if ( isCur ( ' ' ) )
      { 
        finishName ( ); nextChar ();
        gotoState ( parseStateMarkupAttrBeforeEqual ); 
      }
    InvalidInput ( "Markup attribute names can only contain alphanumeric characters "
		   "(and char '%c' is not)\n",
		   curChar() );
    
  case_parseStateMarkupAttrBeforeEqual:
    if ( isCur ( '=' ) )
      { 
        nextChar ();
        gotoState ( parseStateMarkupAttrEqual ); 
      }
    if ( isCur ( ' ' ) || isCur ( '\n' ) || isCur ( '\r' ) || isCur ( '\n' ) )
      { nextChar (); gotoSameState(); }
    InvalidInput ( "Invalid character before attribute name and equal sign : '%c'\n", curChar() );
    
  case_parseStateMarkupAttrEqual:
    if ( isCur ( '"' )  || isCur ( '\'' ) )
      { 
        parsing.attrValueDelimiter = curChar();
        nextChar ();
        newValue ();
        gotoState ( parseStateMarkupAttrValue ); 
      }
    if ( isspace(curChar()) ) // isCur ( ' ' ) || isCur ( '\n' ) || isCur ( '\t' ) || isCur ( '\r' ) )
      { nextChar (); gotoSameState(); }
    if ( isCur ( '&' ) )
      {
        nextChar(); 
        parsing.stateBeforeEntity = parseStateMarkupAttrEqual;
        newEntity();
        gotoState ( parseStateEntity ); 
      }
    InvalidInput ( "Invalid character after equal sign : %d : '%c'\n", curChar(), curChar() );
    
  case_parseStateMarkupAttrValue:
    if ( isCur ( parsing.attrValueDelimiter ) )
      {
        finishValue ();
        saxHandler.eventAttr ( parsing.ns, parsing.name, parsing.value );
        nextChar ();
        gotoState ( parseStateMarkupInside ); 
      }
    if ( isCur ( '&' ) )
      { 
        nextChar ();
        parsing.stateBeforeEntity = parseStateMarkupAttrValue;
        newEntity ();
        gotoState ( parseStateEntity ); 
      }
    if ( isCur ( '<' ) || isCur ( '>' ) )
      {
        Log_Parser ( "Dangerous character '%c' inside of attribute value !\n", curChar() );
      }
    extendValue ();
    gotoSameState();
    
  case_parseStateMarkupEnd:
    if ( isCur ('>' ) )
      { 
        /*
         * We do not keep a trace of the elements we openned.
         * EventHandler is in charge of it.
         */      
        saxHandler.eventElementEnd ( NULL, NULL );
        willGoOutside ();
        nextChar ();
        newText ();
        gotoState ( parseStateMarkupOutside ); 
      }
    InvalidInput ( "Invalid character for Markup end : '%c'\n",
		   curChar () );
    
  case_parseStateMarkupEndName:
    if ( isCur ('>') )
      { 
        finishName ();
        saxHandler.eventElementEnd ( parsing.ns, parsing.name );
        
        willGoOutside (); 
        nextChar ();
        newText ();
        gotoState ( parseStateMarkupOutside ); 
      }
    if ( isCur(':') )
      {
        putNameInNs(); nextChar(); gotoSameState();
      }
    if ( isText () )
      { 
        extendName ();
        gotoSameState();
      }
    if ( isCur ( ' ' ) || isCur ( '\t' ) || isCur ( '\n' ) || isCur ( '\r' ) )
      {
        finishName ();
        saxHandler.eventElementEnd ( parsing.ns, parsing.name );
        nextChar ();
        parsing.textSz = 0;
        parsing.text[0] = 0;
        gotoState ( parseStateMarkupEndNamePost );
      }
    InvalidInput ( "Invalid character at markup end : '%c'\n", curChar() );
        
  case_parseStateMarkupEndNamePost:
    if ( isspace(curChar()) ) // isCur ( ' ' ) || isCur ( '\t' ) || isCur ( '\n' ) || isCur ( '\r' ) )
      {
        nextChar ();
        gotoSameState ();
      } 
    if ( isCur ( '>' ) )
      {
        willGoOutside ();       
        nextChar ();
        gotoState ( parseStateMarkupOutside ); 
      }
    InvalidInput ( "Invalid character at markup end : '%c'\n", curChar() );

  case_parseStateMarkupPI:
    if ( isText() )
      {
	      newName ();
	      gotoState ( parseStateMarkupPIName );
      }
    nextChar();
    gotoSameState();

  case_parseStateMarkupPIName:
    if ( isText() )
      {
	      extendName ();
	      gotoSameState ();
      }
    finishName ();
    nextChar();
    newText();
    gotoState ( parseStateMarkupPIContents );

  case_parseStateMarkupPIContents:
    if ( isCur ( '?' ) )
      {
	      finishText ();
	      nextChar();
	      gotoState ( parseStateMarkupPIEnd );
      }
    extendText ();
    gotoSameState ();

  case_parseStateMarkupPIEnd:
    if ( isCur ( '>' ) )
      {
	      Log_Parser ( "New PI name='%s', content='%s'\n", parsing.name, parsing.text );
	      if ( strcmp ( parsing.name, "xml" ) == 0 )
	        {
	          if ( strstr(parsing.text, "encoding=\"ISO-8859-1\"" ) 
	            || strstr(parsing.text, "encoding=\"iso-8859-1\"" ) 
	            || strstr(parsing.text, "encoding='iso-8859-1'" ) 
	            )
	            {
	              currentReader->setEncoding ( Encoding_ISO_8859_1 );
	            }
	        }
	      saxHandler.eventProcessingInstruction ( parsing.name, parsing.text );
	      
	      willGoOutside ();
        newText();
	      nextChar ();
	      gotoState ( parseStateMarkupOutside );
      }
    InvalidInput ( "Invalid Processing-Instruction syntax '%c'.\n", curChar() );

  case_parseStateMarkupSpecialStart:
    if ( isspace(curChar()) )
      {
        finishName ();
        Log_Parser_Steps ( "Special name : '%s'\n", parsing.name );
        nextChar ();
        newValue ();
        if ( strcmp ( parsing.name, "!ENTITY" ) == 0 )
          {
            newName ();
            parsing.specialMarkup = specialMarkupDocTypeEntity;
            parsing.docTypeSelfEntity = false;
            gotoState ( parseStateDocTypeEntityDecl );
          }
        else if ( strcmp ( parsing.name, "!DOCTYPE" ) == 0 )
          {
            Log_Parser_Steps ( "At doctype : '%s'\n", parsing.name );
            newName ();
            parsing.specialMarkup = specialMarkupDocType;
            gotoState ( parseStateDocTypeName );
          }
        gotoState ( parseStateMarkupSpecial );
      }
    if ( isCur ( '>' ) )
      {
        if ( parsing.specialLevel == 1 )
          {
            nextChar();
            unnestSpecial ();
          }
        InvalidInput ( "Invalid '>' character at special start !\n" );
      }
    if ( isCur ( '%' ) )
      {
        NotImplemented ( "Entity inside DOCTYPE definition !\n" );
      }
    if ( isCur ( '[' ) )
      {
        finishName (); Log_Parser_Steps ( "cdata-candidate at '[' : name=%s\n", parsing.name );
        if ( parsing.nameSz == 7 && strncmp(parsing.name, "![CDATA", 7 ) == 0 )
          {
            newName ();
            newValue ();
            nextChar();
            gotoState ( parseStateMarkupCData ); 
          }
      }
    extendName ();
    if ( parsing.nameSz > 2 && strncmp(parsing.name, "!--",3) == 0 )
      {
        newName ();
        newValue ();
        gotoState ( parseStateMarkupComment );
      }
    
    gotoSameState ();

  case_parseStateMarkupSpecial:
    if ( isCur ( '<' ) || isCur ( '[' ) )
      {
        Log_Parser_Steps ( "Nesting !\n" );
        parsing.specialLevel ++;
        nextChar ();
        newName ();
        gotoState ( parseStateMarkupSpecialStart ); 
      }
    if ( isCur ( '>' ) || isCur ( ']' ) )
      {
        finishValue ();
        Log_Parser_Steps ( "de-Nesting : name='%s', value=[%s]\n", parsing.name, parsing.value );
        if ( strcmp(parsing.name,"!ELEMENT") == 0 || strcmp(parsing.name,"!ATTLIST") == 0 )
          {
            const char* markupName = parsing.name; markupName++;
            saxHandler.eventDoctypeMarkupDecl ( markupName, parsing.value );
            newName();
          }
        nextChar ();
        unnestSpecial ();
      }
    if ( isCur ( '%' ) )
      {
        nextChar ();
        parsing.stateBeforeEntity = parseStateMarkupSpecial;
        newEntity();
        gotoState ( parseStateEntity ); 
      }
    extendValue ();
    gotoSameState ();

  case_parseStateDocTypeEntityDecl:
    if ( isspace(curChar()) )
      {
        if ( parsing.nameSz == 0 )
          {
            nextChar();
            gotoSameState ();
          }
        finishName ();
        Log_Parser_Steps ( "Finished entity name : '%s'\n", parsing.name );
        if ( strcmp ( parsing.name, "%" ) == 0 )
          {
            if ( parsing.docTypeSelfEntity ) 
              {
                InvalidInput ( "ENTITY already set for '%s'\n", parsing.name );
              }
            parsing.docTypeSelfEntity = true;
            newName ();
            nextChar ();
            gotoSameState ();
          }
        nextChar ();
        newValue ();
        parsing.docTypeSystemEntity = false;
        gotoState ( parseStateDocTypeEntityValue );              
      }
    if ( isText() || isCur ( '%' ) )
      {
        extendName ();
        gotoSameState ();
      }
    InvalidInput ( "Invalid ENTITY name !\n" );
    
  case_parseStateDocTypeEntityValue:
    if ( isspace(curChar()) )
      {
        if ( parsing.valueSz == 0 )
          {
            nextChar();
            gotoSameState();
          }
        finishValue ();
        Log_Parser_Steps ( "Finished entity value : '%s'\n", parsing.value );
        if ( strcmp(parsing.value, "SYSTEM") == 0 )
          {
            if ( parsing.docTypeSystemEntity ) InvalidInput ( "Entity already set as SYSTEM !\n" );
            parsing.docTypeSystemEntity = true;
            newValue ();
            nextChar ();
            gotoSameState ();
          }
        else
          {
            InvalidInput ( "Invalid preliminary (non-protected) value '%s'\n", parsing.value );
          }
      }
    if ( isCur ( '"' ) || isCur ( '\'' ) )
      {
        parsing.attrValueDelimiter = curChar ();
        nextChar();
        newValue();
        gotoState ( parseStateDocTypeEntityValQuot );
      }
    extendValue ();
    gotoSameState ();

  case_parseStateDocTypeEntityValQuot:
    if ( isCur ( parsing.attrValueDelimiter ) )
      {
        finishValue ();
        Log_Parser_Steps ( "New entity : %s (self=%d, system=%d) : '%s'\n",
            parsing.name, parsing.docTypeSelfEntity, parsing.docTypeSystemEntity, parsing.value );
        setDocTypeEntity ( parsing.name, parsing.value, parsing.docTypeSystemEntity );
        saxHandler.eventEntity ( parsing.name, parsing.value );
        nextChar ();
        gotoState ( parseStateDocTypeEntityValEnd );
      }
    if ( isCur ( '%' ) )
      {
        nextChar ();
        parsing.stateBeforeEntity = parseStateDocTypeEntityValQuot;
        newEntity();
        gotoState ( parseStateEntity ); 
      }
    extendValue ();
    gotoSameState ();
  case_parseStateDocTypeEntityValEnd:
    if ( isspace ( curChar() ) )
      {
        nextChar();
        gotoSameState();
      }
    if ( isCur ( '>' ) )
      {
        gotoState(parseStateMarkupSpecialEnd);
      }
    if ( isCur ( 'N' ) )
      {
        newValue ();
        gotoState(parseStateDocTypeEntityValNData);
      }
    InvalidInput("At cur=%c\n", curChar() );
  case_parseStateDocTypeEntityValNData:
    if ( isText() )
      {
        extendValue ();
        gotoSameState();
      }
    else if ( isspace(curChar()))
      {
        finishValue ();
        Log_Parser ( "At value = '%s'\n", parsing.value );
        if ( strcmp ( parsing.value, "NDATA" ) == 0 )
          {
            newValue ();
            gotoState ( parseStateDocTypeEntityValNDVal );
          }
        InvalidInput ( "Invalid keyword '%s' after entity\n", parsing.value );
      }
    InvalidInput("At cur=%c\n", curChar() );
  case_parseStateDocTypeEntityValNDVal:
    if ( isspace(curChar()) && parsing.valueSz == 0 )
      {
        nextChar();
        gotoSameState ();
      }
    else if ( isspace(curChar()) || isCur ( '>' ) )
      {
        finishValue ();
        Log_Parser ( "Set NDATA unparsed entity : '%s'='%s'\n", parsing.name, parsing.value );
        saxHandler.eventNDataEntity ( parsing.name, parsing.value );
        gotoState(parseStateMarkupSpecialEnd);
      }
    else if ( isText() )
      {
        extendValue ();
        gotoSameState ();
      }
    InvalidInput("At cur=%c\n", curChar() );
  case_parseStateMarkupComment:
    if ( isCur ( '-' ) )
      {
        if ( parsing.nameSz == 2 )
          { nextChar(); }
        else
          { extendName (); }
        gotoSameState (); 
      }
    if ( isCur ( '>' ) && parsing.nameSz == 2 )
      {
        // extendName ();
        extendNameChar ( curChar() );
        finishName(); Log_Parser_Steps ( "At name = '%s'\n", parsing.name );
        if ( parsing.nameSz == 3 && strncmp(parsing.name, "-->",3) == 0 )
          {
            finishValue ();
            Log_Parser_Steps ( "New comment : '%s'\n", parsing.value );
            
            if ( parseKeepComments )
              {
                pushText ();
                saxHandler.eventComment ( parsing.value );
              }
            else
              {
                Log_Parser ( "Skipping comment '%s'!\n", parsing.value );
              }
              
            if ( parsing.specialLevel == 1 ) 
              {
                willGoOutside (); //< wont I ?
                parsing.specialLevel --;
                nextChar();
                if ( parsing.textSz ) gotoState ( parseStateMarkupText );
                gotoState ( parseStateMarkupOutside );
              }

            unnestSpecial_nextChar();
          }
        nextChar();
        gotoSameState ();
      }
    if ( parsing.nameSz ) 
      {
        finishName ();
        extendValueStr ( parsing.name );
        newName ();
      }
    extendValue ();
    gotoSameState ();
  case_parseStateMarkupCData:
    if ( isCur ( ']' ) )
      {
        if ( parsing.nameSz == 2 )
          { nextChar(); }
        else
          { extendName (); }
        gotoSameState ();
      }
    if ( isCur ( '>' ) && parsing.nameSz == 2 )
      {
        extendName ();
        finishName(); Log_Parser_Steps ( "At name = '%s'\n", parsing.name );
        if ( parsing.nameSz == 3 && strncmp(parsing.name, "]]>",3) == 0 )
          {
            finishValue ();
            Log_Parser_Steps ( "New CDATA : '%s'\n", parsing.value );

            extendTextStr ( parsing.value );
            newValue ();
            
            if ( ! parsing.specialLevel ) InvalidInput ( "Wrongly nested !\n" );
            parsing.specialLevel --;
            gotoState ( parseStateMarkupText );
          }
        gotoSameState ();
      }
    if ( parsing.nameSz ) 
      {
        finishName ();
        extendValueStr ( parsing.name );
        newName ();
      }
    extendValue ();
    gotoSameState ();
    
  case_parseStateDocTypeName:
    if ( isspace(curChar()) )
      {
        if ( !parsing.nameSz )
          {
            nextChar();
            gotoSameState ();
          }
        finishName ();
        nextChar ();
        Log_Parser_Steps ( "DocType name : '%s'\n", parsing.name );
        newValue ();
        parsing.docTypeSystem = false;
        parsing.docTypePublic = false;
        gotoState ( parseStateDocTypeValue );
      }
    if ( isCur('[') || isCur('<') || isCur (']') || isCur('>') )
      {
        finishName ();
        InvalidInput ( "While processing docTypeName %s\n", parsing.name );
      }
    if ( isCur (':') )
      {
        putNameInNs ();
        nextChar(); gotoSameState ();
      }
    extendName ();
    gotoSameState ();

  case_parseStateDocTypeValue:
    if ( isspace(curChar()) )
      {
        nextChar ();
        if ( !parsing.valueSz )
          {
            gotoSameState ();
          }
        finishValue ();
        Log_Parser_Steps ( "DocType Value : '%s'\n", parsing.value );
        if ( strcmp ( parsing.value, "SYSTEM" ) == 0 )
          {
            parsing.docTypeSystem = true;            
            newValue ();
            gotoSameState ();
          }
        if ( strcmp ( parsing.value, "PUBLIC" ) == 0 )
          {
            parsing.docTypePublic = true;            
            newValue ();
            gotoSameState ();
          }
        else
          {
            InvalidInput ( "Doctype value '%s' not handled !\n", parsing.value );
          }
      }
    if ( isCur ( '"' ) || isCur ( '\'' ) )
      {
        parsing.attrValueDelimiter = curChar();
        nextChar ();
        newValue ();
        gotoState (parseStateDocTypeValQuot);
      }
    if ( isCur ( '[' ) )
      {
        parsing.specialLevel ++;
        nextChar ();
        gotoState ( parseStateMarkupSpecial );
      }
    extendValue ();
    gotoSameState ();
  case_parseStateDocTypeValQuot:
    if ( isCur(parsing.attrValueDelimiter) )
      {
        finishValue ();
        Log_Parser_Steps ( "Doctype reference value : '%s'\n", parsing.value );

        if ( parsing.docTypeSystem )
          {
            String url = currentReader->getBaseURI ();
            url += parsing.value;
            
            newName (); newValue ();
            
            FileReader* fileReader = new FileReader ( url );
            enqueueReader ( fileReader );
          }
        nextChar ();

        gotoState ( parseStateMarkupSpecial );
      }
    extendValue ();
    gotoSameState ();
  case_parseStateMarkupSpecialEnd:
    if ( isCur ( '>' ) )
      {
        nextChar();
        unnestSpecial ();
      }
    if ( isspace(curChar()) )
      {
        nextChar();
        gotoSameState ();
      }
    InvalidInput ( "Invalid input at special end : %d (name=%s)\n", curChar(), parsing.name );

  case_parseStateEntity:
      if ( isCur ( ';' ) )
        {
          finishEntity ();
          
          if ( parsing.entity[0] == '#' )
            {
              char* valueStr = &(parsing.entity[1]);
              int val = 0;
              if ( valueStr[0] == 'x' ) // hexadecimal
                {
                  int res = sscanf ( &(valueStr[1] ), "%x", &val );
                  if ( res != 1 )
                    {
	                    Bug ( "Could not parse hexa '%s'\n", valueStr );
                    }
	              }
	            else // decimal
	              {
	                Log_Parser ( "Parser : valueStr ='%s'\n", valueStr );
	                val = atoi ( valueStr );
	              }
	            if ( val == 0 )
                {
                  throwException ( Exception, "Null val, valueStr='%s'\n", valueStr );
                }
#ifdef __XEM_PARSER_CONVERT_ENTITIES_TO_UTF8
              else
                {
                  Log_Parser ( "Parsed UTF8 entity : '%s'\n", parsing.entity );
                  doExtendTextValueChar ( val );
                }
#else //  __XEM_PARSER_CONVERT_ENTITIES_TO_UTF8
	            else
                { 
                  doExtendTextValue ( "&" );          	
                  doExtendTextValue ( parsing.entity );
                  doExtendTextValue ( ";" );
                }
#endif //  __XEM_PARSER_CONVERT_ENTITIES_TO_UTF8
              nextChar ();
              gotoState ( parsing.stateBeforeEntity );
            }
          bool found = false;
          
          String entity = parsing.entity;
          if ( ! found )
            {
              Parsing::EntityMap::iterator iter = parsing.xmlEntities.find ( entity );
              if ( iter != parsing.xmlEntities.end() )
                {
                  found = true;
                  doExtendTextValue ( iter->second.c_str() );
                  Log_Parser_Steps ( "Found entity : '%s' => '%s'\n", entity.c_str(), iter->second.c_str() );
                }
            }
          if ( ! found )
            {
              Parsing::EntityMap::iterator iter = parsing.userDefinedEntities.find ( entity );
              if ( iter != parsing.userDefinedEntities.end() )
                {
                  found = true;

                  Log_Parser_Steps ( "Found entity : '%s' => '%s'\n",
                      entity.c_str(), iter->second.c_str() );
                  
                  String& entityContents = iter->second;
                  
                  Log_Parser_Steps ( "parse state=%x, parsing.valueFlag=[%c]\n",
                      parsing.stateBeforeEntity, parsing.attrValueDelimiter );
                  
                  StringReader* stringReader = new StringReader ( entityContents.c_str() );
                  enqueueReader ( stringReader );
                }
            }
          if ( ! found )
            {
              Parsing::EntityMap::iterator iter = parsing.userDefinedSystemEntities.find ( entity );
              if ( iter != parsing.userDefinedSystemEntities.end() )
                {
                  found = true;
                  FileReader* fileReader = new FileReader ( iter->second.c_str() );
                  enqueueReader ( fileReader );
                }
            }
#ifdef __XEM_PARSER_CONVERT_HTML_ENTITIES_TO_UTF8
          if ( ! found )
            {
              int code = htmlEntityToUtf8 ( parsing.entity );
              if ( code )
                {
                  found = true;
                  /*
                   * Enforce source encoding to UTF-8, because we get that source here.
                   */                
                  char utf[16];
                  utf8CodeToChar ( code, (unsigned char*) utf, 8 );
                  doExtendTextValue ( utf );
                }             
            }
#endif //  __XEM_PARSER_CONVERT_HTML_ENTITIES_TO_UTF8            
#ifdef __XEM_PARSER_EXCEPTION_IF_ENTITY_NOT_FOUND
          if ( ! found )
            {
              InvalidInput ( "Could not resolve entity : '%s'\n", parsing.entity );
            }         
#endif //__XEM_PARSER_EXCEPTION_IF_ENTITY_NOT_FOUND
          if ( ! found )
            {
              doExtendTextValue ( "&" );          	
              doExtendTextValue ( parsing.entity );
              doExtendTextValue ( ";" );
            }
          nextChar ();
          ParserState stateBeforeEntity = parsing.stateBeforeEntity;
          parsing.stateBeforeEntity = parseStateMarkupUNUSED;
          gotoState ( stateBeforeEntity );
        }
    extendEntity ();
    gotoSameState ();

  case_parseStateUNUSED:
    Bug ( "Internal error : pushed to an unused state !\n" );

  case_end:
    pushText ();
    Log_Parser_Steps ( "Parse Finished. Total parsed '%llu' bytes, total lignes parsed '%llu', "
      "parsing.specialMarkup=%d, parsing.state=%d\n",
    	currentReader->getTotalParsed(), currentReader->getTotalLinesParsed(), parsing.specialMarkup, parsing.state );
    	
    if ( parsing.state != parseStateMarkupOutside && parsing.state != parseStateMarkupText )
      {
        Warn ( "Parsing did not finish in 'Outside' state : parsing.state=%s (%d)!\n", 
            XMLParserStatesStr[parsing.state], parsing.state );
      }
    if ( parsing.specialLevel )
      {
        Warn ( "Parsing did not finish at specialLevel 0 : specialLevel=%d\n", parsing.specialLevel );
      }
  }

  void Parser::parse ()
  {
    try
      {
        internalParse ();
        saxHandler.parsingFinished ();
        currentReader = NULL;
      }
    catch ( Exception *e )
      {
        detailException ( e, "Invalid input, aborting Document parsing at byte '%llu', line '%llu'.\n",
           initialReader.getTotalParsed(), initialReader.getTotalLinesParsed() );
        currentReader = NULL;
        if ( readerStack.size() )
          {
            /*
             * TODO : dequeue all stack here !
             */
            Warn ( "Shall dequeue and cleanup reader stack here !\n" );
          }
        throw e;
      }
  }
};


