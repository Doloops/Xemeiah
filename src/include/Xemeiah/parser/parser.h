#ifndef __XEM_XML_PARSER_H
#define __XEM_XML_PARSER_H

#include <Xemeiah/kern/exception.h>
#include <Xemeiah/parser/saxhandler.h>
#include <Xemeiah/io/bufferedreader.h>
#include <Xemeiah/dom/string.h>

#include <map>

namespace Xem
{
  XemStdException ( ParserException );
  class XProcessor;

  /**
   * High performance SAX-like XML parser ; uses Feeder to provide input buffers and EventHandler to process events.
   */
  class Parser
  {
  protected:
    /**
     * Internal parser uses 64-bit wide integers for internal position representations
     */
    typedef __ui64 __ui;

    /**
     * (SAX-like) XML parsing Event Handler reference
     */
    SAXHandler &saxHandler;

    /**
     * Input stream feeder reference
     */
    BufferedReader &initialReader;

    /**
     * The reader stack
     */
    class StackedReader
    {
    public:
      BufferedReader* reader;
      int previousAttrValueDelimiter;
    };

    /**
     * The buffered reader stack class
     */
    typedef std::list<StackedReader*> ReaderStack;

    /**
     * Our buffered reader stack
     */
    ReaderStack readerStack;

    /**
     * Our current reader
     */
    BufferedReader* currentReader;

    /**
     * Enqueue a given reader
     */
    void enqueueReader ( BufferedReader* reader );

    /**
     * Dequeue reader
     */
    void dequeueReader ();

    /** 
     * Option : keep all text (including white space)
     */
    bool parseKeepAllText;
    
    /**
     * Option : keep comments while parsing
     */
    bool parseKeepComments;
    
    /**
     * Parsing is performed with finite-state machine
     */
    enum ParserState
      {
        parseStateMarkupOutside         = 0,  //< Outside any markup
        parseStateMarkupText            = 1,  //< Text between markup
        parseStateMarkupUNUSED          = 2,  //< &entity; in text
        parseStateMarkupEnter           = 3,  //< At the "<" beginning of markup
        parseStateMarkupName            = 4,  //< Name of the markup
        parseStateMarkupInside          = 5,  //< Inside of the markup, after the name
        parseStateMarkupAttrName        = 6,  //< Attribute name 
        parseStateMarkupAttrBeforeEqual = 7,  //< After the attribute name, before equal
        parseStateMarkupAttrEqual       = 8,  //< "=" sign
        parseStateMarkupAttrValue       = 9,  //< After the '"', reading value till next '"'
        parseStateMarkupUNUSED2         = 10, //< &entity in value
        parseStateMarkupEnd             = 11, //< At the '/' in <markup/>
        parseStateMarkupEndName         = 12, //< At the the / in </markup>
        parseStateMarkupPI              = 13, //< In the <? .. ?>
        parseStateMarkupPIName          = 14, //< In the name part of <?name .. ?>
        parseStateMarkupPIContents      = 15, //< In the content part of the name
        parseStateMarkupPIEnd		= 16, //< Waiting for the terminating '>'
        parseStateMarkupSpecialStart    = 17, //< Special are all those '<!' markups, Start is waiting for a non-ambiguous starter
        parseStateMarkupSpecial         = 18, //< Inside of the Special, may requalify depending on the contents
        parseStateMarkupSpecialEnd      = 19, //< (may be unused)
        parseStateMarkupEndNamePost     = 20, //< At the end of the MarkupEndName, when no > is found yet.
        parseStateEntity                = 21, //< Parsing an entity name
        parseStateDocTypeEntityDecl     = 22, //< Parsing a DOCTYPE !ENTITY name
        parseStateDocTypeEntityValue    = 23, //< Parsing a DOCTYPE !ENTITY value
        parseStateDocTypeEntityValQuot  = 24, //< Parsing a DOCTYPE !ENTITY value "..."
        parseStateDocTypeEntityValEnd   = 25, //< Parsing a DOCTYPE !ENTITY value "..." end
        parseStateDocTypeEntityValNData = 26, //< Parsing a DOCTYPE !ENTITY value "..." NDATA
        parseStateDocTypeEntityValNDVal = 27, //< Parsing a DOCTYPE !ENTITY value "..." NDATA Value
        parseStateMarkupComment         = 28, //< Parsing the <!-- --> comment
        parseStateMarkupCData           = 29, //< Parsing the <![CDATA[ ]]> section
        parseStateDocTypeName           = 30, //< Parsing a DOCTYPE !DOCTYPE name
        parseStateDocTypeValue          = 31, //< Parsing a DOCTYPE !DOCTYPE name VALUE
        parseStateDocTypeValQuot        = 32, //< Parsing a DOCTYPE !DOCTYPE name VALUE
        parseStateUnknown
      };

    /**
     * SpecialMarkup represents the '<!' markups : <!DOCTYPE, <!--, <![CDATA[ ]]> ...
     */
    enum SpecialMarkup 
      {
        specialMarkupNone = 0,
        specialMarkupUNUSED = 1,
        specialMarkupComment = 2,
        specialMarkupSubset = 3, // Subset marks the '<!' start, but not a '<!--' comment. This includes CData, entity, document, ...
        specialMarkupCData = 4,
        specialMarkupDocType = 5, // External docType loading mechanism
        specialMarkupDocTypeInDocument = 6, // DocType definition in document.

        specialMarkupDocTypeEntity = 7 // DocType !ENTITY Definition
      };
    
    /**
     * Parsing holds the current state of the Xem::Parser parsing process.
     */
    class Parsing // Values for char-by-char reading
    {
      friend class ByteStreamReader;
    public:
      typedef std::map<String,String> EntityMap;
    
      EntityMap xmlEntities;
      EntityMap userDefinedEntities;
      EntityMap userDefinedSystemEntities;
    
      Parsing ();
      ~Parsing ();
      
      void initEntityTable ();
      
      ParserState state;
      char *ns, *name, *value, *text, *entity, *special;
      __ui nsSz, nameSz, valueSz, textSz, entitySz, specialSz;
      __ui nsMx, nameMx, valueMx, textMx, entityMx, specialMx;

      int attrValueDelimiter; //< The character that put us in the attribute value protected string : can be '"' or '\''.

      SpecialMarkup specialMarkup;
      int specialLevel;
      
      char valueFlag;
      bool keywordInMarkup;
      
      ParserState stateBeforeEntity;
  
      bool docTypeSelfEntity;    
      bool docTypeSystemEntity;
      
      bool docTypeSystem;
      bool docTypePublic;
    };
    /**
     * The parsing 
     */
    Parsing parsing;

    String dumpCurrentContext ( int byte, BufferedReader& reader );
    
    /**
     * Internal parsing mechanism
     */
    void internalParse ( );
    
    /**
     * Parse DOCTYPE contents
     */
    void parseDocTypeContents ( const char* contents );
    
    /**
     * Parse DOCTYPE External reference
     */
    void parseDocTypeReference ( const char* reference );
      
    /**
     * Parse a DOCTYPE file
     */      
    void parseDocTypeFile ( const String& reference );

    /**
     * Parse a DOCTYPE file
     */      
    String getDocTypeFileContents ( const String& reference );
    
    /**
     * Set a new entity
     */
    void setDocTypeEntity ( const String& name, const String& contents, bool system = false );
  public:
    /**
     * Simple constructor
     */
    Parser ( BufferedReader& reader, SAXHandler &saxHandler);
    
    /**
     * Parser destructor
     */
    ~Parser ();

    /**
     * Set option : keep all text
     */
    void setKeepAllText ( bool _keepAllText ) { parseKeepAllText = _keepAllText; }
    
    /**
     * Set option : keep all comments
     */
    void setKeepComments ( bool _keepComments ) { parseKeepComments = _keepComments; }
    
    /**
     * Perform parsing. May throw exceptions
     */
    void parse ();

    /**
     * Import a file using a DOM event handler
     * @param xproc the XProcessor to use for DOM Events
     * @param elt the root element to add parsed XML to
     * @param filePath path of the file to import
     * @param keepTextMode keep text mode (all, xsl, normal, none)
     */
    static void parseFile ( XProcessor& xproc, ElementRef& elt, const String& filePath, const String& keepTextMode = "all" );
  };

};

#endif // __XEM_XML_PARSER_H
