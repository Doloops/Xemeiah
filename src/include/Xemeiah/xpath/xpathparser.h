#ifndef __XEM_XPATHPARSER_H
#define __XEM_XPATHPARSER_H

#include <Xemeiah/xpath/xpath.h>

#include <vector>

namespace Xem
{
  XemStdException ( XPathParserException );

  /**
   * Parser for XPath expressions ; whereas Xem::XPath only handles already-parsed expressions, 
   * XPathParser only handles XPath parsing.
   */
  class XPathParser
  {
  protected:
    /*
     * The parsing context
     */
    /**
     * The ElementRef from which we are parsing
     * Can be usefull when trying to resolve an unknown namespace.
     */
    ElementRef* parsingFromElementRef;
    
    /**
     * The KeyCache and NamespaceAlias to use when parsing.
     */
    KeyCache* parsingFromKeyCache;
    NamespaceAlias* parsingFromNamespaceAlias;   
    
    /**
     * The expression being parsed
     */
    const char* expression;

    const char* getExpression() const { return expression; }

    /*
     * The parsed contents
     */
    /**
     * The list of steps currently parsed
     */
    typedef std::vector<XPathStep*> ParsedSteps;
    ParsedSteps parsedSteps;
    
    /**
     * The (linearized) resource block
     */
    char* resourceBlock;

    /**
     * While parsing : indicates the next resource offset available for resource storage.
     */
    __ui64 nextResourceOffset;    
    
    /**
     * Indicates the size of the resourceBlock currently allocated.
     */
   __ui64 allocedResourceBlockSize;

    /**
     * The first effective step
     */
    XPathStepId firstStep;

    /**
     * At end of parsing, when all things went right, the parsed stuff is packed in the packedSegment
     */
    XPathSegment* packedSegment;
    
    /**
     * XPathStep accessor
     */
    XPathStep* getStep ( XPathStepId stepId );

    /**
     * XPathStep Allocation routine.
     * @return a free XPathStepId. The corresponding XPathStep is inited and ready-to-use.
     */
    XPathStepId allocStep ();

    /**
     * Initial constructor reset of members.
     */
    void init ();

    /**
     * Initial constructor from an AttributeRef
     */
    void init ( ElementRef& eltRef, AttributeRef& attrRef, bool isAVT );

    /**
     * protected constructor for call by sub-classes.
     */
    XPathParser () { init (); }
  
    /**
     * Allocate a step containing a Resource fetcher
     * @param text the text to put in the resource
     * @return the allocated XPathStepId.
     */
    XPathStepId allocStepResource ( const char* text );
   
    /**
     * Primary XPath parsing routine.
     * @param xpath the expression to parse
     * @param embedded if true, parse as an AVT (Attribute Value Template).
     * @return true on succes, false on error (Warning : Exception may arise).
     * \todo We may want to bind an explicit NamespaceAlias here.
     */
    void parse ( const char *xpath, bool isAVT );
    
    /**
     * XPath embedded (AVT) parsing.
     * @param expr the XPath AVT expression, splitted in non-AVT expressions '{...}' and joined up.
     * @return the XPathStepId of the head XPathStep parsed.
     */
    XPathStepId doParseAVT ( char* expr );

    /**
     * XPath embedded (AVT) parsing.
     * @param expr the XPath AVT expression, splitted in non-AVT expressions '{...}' and joined up.
     * @return the XPathStepId of the head XPathStep parsed.
     */
    XPathStepId parseAVT ( const char* expr );
    
    /**
     * XPath Key to KeyId parsing mechanism.
     * Depending on parsingFromElement or parsingFromKeyCache/parsingFromNamespaceAlias
     * @param key the Key to parse
     * @return the KeyId parsed.
     */
    KeyId getKeyId ( const String& key );

    /**
     * XPath concatenation mechanism
     * @param left the first XPathStep
     * @param right the second XPathStep
     * @return the XPathStepId of a XPathStep that concats left and right operator.
     * \deprecated Remove this.
     */
    XPathStepId concatStep ( XPathStepId left, XPathStepId right );    
    


    /**
     * Parsing token for XPath.
     * Everything in this class is public, because it is a protected class of XPath.
     */
    class Token
    {
    public:
      /**
       * The string representation of the token : keyword, or qname.
       */
      String token;
      /**
       * The short symbol of this token: '(', ',', '[', '{', '/', ...
       */     
      char symbol;
      /**
       * Two-character symbol
       * \deprecated Two-character symbol in XPath::Token
       */
      char symbol2[2];
      /**
       * A Token can be of three different types :
       * -# Protected textual representation, (in single or double quotes)
       * -# Non-ambiguous symbols : '+', '/', '@', '$', ...
       * -# Potential QNames, which can still be symbols after all. For example, 'div' can be the 
       *    division operator or a QName matching 'div' elements.
       */
      enum Type
      {
        QName,
        Symbol,
        Text
      };
      Type type;
      Token* next;
      Token* subExpression, *subExpression2;
      
      Token () { type = QName; symbol = '\0'; symbol2[0] = '\0'; subExpression = NULL; subExpression2 = NULL; next = NULL; }
      ~Token () { }

      bool isSymbol() { return type == Symbol; }
      bool isQName() { return type == QName; }
      bool isText() { return type == Text; }
      
      void log(int indent);
      void log() { log ( 0 ); }
    };

    /**
     * Token Factory for XPath
     * 
     */    
    class TokenFactory
    {
      std::list<Token*> tokens;
    public:
      /**
       * TokenFactory constructor
       */
      TokenFactory ();
      /**
       * TokenFactory destructor
       * Deletes all Token provided by a call to allocToken().
       */
      ~TokenFactory ();  
    
      /**
       * Provides a token
       * @return an allocated token.
       */
      Token* allocToken ();
      
      /**
       * @return Current number of tokens provided.
       */
      size_t size() { return tokens.size(); }
    };
    
    /**
     * The function tokenize() is in charge of converting the expression to a Token chain.
     * The provided Token is almost a Token chain (i.e. a single-linked list of Token), except for
     * parentheses, braces and curly braces, which are parsed directly.
     * That is, the expression 'floor ( 4 div 3 )' will generate a Token chain like this
     * {Token token='floor'}->{Token symbol='('
     *                            {Token token='4'}->{Token token='div'}->{Token token='3'}
     *                        }
     * @param keyCache the KeyCache to use when parsing.
     * @param nsAlias the NamespaceALias to use when parsing.
     * @param xpath the expression to parse
     * @param tokenFactory the TokenFactory to use for providing Token.
     * @return the first (head) Token parsed.
     */
    Token* tokenize ( const char* xpath, TokenFactory* tokenFactory );

    /**
     * Recursively transform a Token chain depending on the symbol provided.
     * If the Token chain starting by token contains a Token which as symbol as a symbol
     * then this head token will be transformed into a binary operator, with the same token at head,
     * but with Token::subExpression and Token::subExpression2 set to point at the left and right operands.
     * Example : Expression '1 + 3'
     * Original token chain : {Token token='1'}->{Token symbol='+'}->{Token token='3'}
     * Rearranged token :
     * {Token symbol='+'
     *    subExpression={Token token='1'}
     *    subExpression2={Token token='3'} }
     * rearrangeTokens() transforms Token chains this way :
     * -# check if the Token chain starts by the '//' descendant-or-self symbol
     *   If it is, it will prefix the chain by a root step (i.e. '/'), to make the descendant operations
     *   start from the root element.
     *   This allows to simply disambiguate globally descending expressions '//myElement', from locally
     *   descending expressions 'myChild//mySubChild'.
     * -# transform linear Token chain to binary operator Token tree, 
     *   starting from the operator with less precedence ('and')
     *   and finishing by the operator with the highest precedence ('div').
     * @param token the head Token of the token chain
     * @param tokenFactory the TokenFactory to use if we need to create a new Token.
     */
    void rearrangeTokens ( Token* token, TokenFactory* tokenFactory );
        
    /**
     * Internal XPath parsing function. It calls :
     * -# tokenize() to convert the xpath into a Token chain.
     * -# rearrangeTokens() to convert this Token chain into a Token tree (for binary operators)
     * -# tokenToStep() to convert this Token tree into a (serializable) list of XPathStep and XPathResource.
     * @param keyCache the KeyCache to use when parsing
     * @param nsAlias the NamespaceALias to use when parsing.
     * @param xpath the expression to parse.
     * @return the XpathStepId of the head XPathStep for this parsed expression.
     */
    XPathStepId doParse ( const char* xpath );
    
    /**
     * Compute reverse-path lastStep for an already parsed XPath
     */
    void computeLastSteps ();
    
    /**
     * tokenToStep() recursively converts each Token in the chain, according to the Token::Type defined.
     * - For Text tokens : a new resource is created
     * - For QName tokens : 
     *   - if the token has a numeric content, a numeric constant XPathStep is created.
     *   - if the token is followed by a '(' symbol, it is handled as a function name (@see tokenToFunctionStep()).
     *   - otherwise, it is converted to a regular XPathStep by the qnameTokenToStep() funciton.
     * - For symbol tokens :
     *   - Sub-tokens '/' (path delimiter), '$' (variable delimiter) '@'
     * - When the token is converted, and allows a follower, the follower is convert to a XPathStep using nextTokenToStep().
     * @param keyCache the KeyCache to use when parsing
     * @param nsAlias the NamespaceALias to use when parsing.
     * @param token the head Token to convert.
     * @return the XPathStepId of the resulting XPathStep.
     */
    XPathStepId tokenToStep ( Token* token );
    
    /**
     * Converts a QName detected as a function (because it is followed by the '(' symbol).
     * The function may be a Node Test (node(), text(), element(), ...), and may be prefixed by
     * an axis specifier (ancestor::node(), ...).
     * @param keyCache the KeyCache to use when parsing
     * @param nsAlias the NamespaceALias to use when parsing.
     * @param step the XPathStep to write to.
     * @param token the Token to convert.
     */
    void tokenToFunctionStep ( XPathStep* step, Token* token );
    
    /**
     * Converts a non-function QName token to a XPathStep.
     * The token may contain an axis definition (child::, attribute::, ...)
     * @param keyCache the KeyCache to use when parsing
     * @param nsAlias the NamespaceALias to use when parsing.
     * @param step the XPathStep to write to.
     * @param token the Token to convert.
     */
    void qnameTokenToTest ( XPathStep* step, Token* token );
    
    /**
     * Converts the next token to a XPathStep.
     * This function handles special position-sensitive syntaxes (predicates, ...), and calls
     * @param keyCache the KeyCache to use when parsing
     * @param nsAlias the NamespaceALias to use when parsing.
     * @param step the XPathStep to write to.
     * @param token the current token we were at.
     * @param nextToken the next token to convert.
     */
    void nextTokenToStep ( XPathStep* step, Token* token, Token* nextToken );

    /**
     * Converts a binary operator to the corresponding XPathStep.
     * @param keyCache the KeyCache to use when parsing
     * @param nsAlias the NamespaceALias to use when parsing.
     * @param step the XPathStep to write to.
     * @param token the Token to convert.
     */
    void binaryTokenToStep ( XPathStep* step, Token* token );
    
    /**
     * Dummy, stupid function telling if a string is a numeric value or not.
     * @param expr the string
     * @return true if it only contains valid characters for a numeric value, false if not.
     */
    bool isNumeric ( const char* expr );

      
    /**
     * Tag the step flags according to predicates characteristics
     */
    void tagStepPredicatesFlags ( XPathStep* step );
    
    /**
     * Check that a given expression is positionnal or not
     */
    void checkPositionnalExpression ( XPathStep* step, XPathStepFlags& flags );

    /**
     * Pack parsed stuff to a buffer
     */
    void packParsed ( char* buffer, __ui64 segmentSize );

  public:
    /**
     * Instanciate a parsed XPath using a given String
     * @param keyCache the KeyCache to use for parsing
     * @param nsAlias the NamespaceALias to use when parsing.
     * @param expr the expression to parse
     * @param isAVT true if the expr is to be parsed as a AVT, false if not.
     */
    XPathParser ( KeyCache& keyCache, NamespaceAlias& nsAlias, const String& expr, bool isAVT = false );

    /**
     * Instanciate a parsed XPath using a given String
     * @param nsAlias the NamespaceALias to use when parsing.
     * @param keyCache the KeyCache to use for parsing
     * @param expr the expression to parse
     * @param isAVT true if the expr is to be parsed as a AVT, false if not.
     */
    XPathParser ( KeyCache& keyCache, NamespaceAlias& nsAlias, const char* expr, bool isAVT = false );

#if 0 // DEPRECATED : This was _not_ the right approach
    /**
     * Builds an XPath expression reflecting the list of keys to strip or preserve
     */
    XPathParser ( std::list<KeyId>& stripList, std::list<KeyId>& preserveList );      
#endif
  
    /**
     * Direct parser from an AttributeRef
     */
    XPathParser ( ElementRef& eltRef, AttributeRef& attrRef, bool isAVT = false );

    /**
     * Parser from an ElementRef
     */
    XPathParser ( ElementRef& eltRef, KeyId attrKeyId, bool isAVT = false );

    /**
     * Parser from an expression with an ElementRef
     */
    XPathParser ( ElementRef& eltRef, const String& expr );

    /**
     * XPathParser destructor
     */
    ~XPathParser ();  

    /**
     * Get the parsed size
     */
    __ui64 getParsedSize ();
    
    /**
     * Save to Store, as a special XPath attribute.
     * @param elementRef the ElementRef to store to
     * @param attrKeyId the KeyId of the AttributeRef to create.
     * @return the created AttributeRef (with AttributeType_XPath type).
     */
    AttributeRef saveToStore ( ElementRef& elementRef, KeyId attrKeyId );
    
    /**
     * Get packed XPath
     */
    XPathSegment* getPackedParsed ();

    /**
     * Clears alloced structures from memory (used by destructor and after a saveToStore()).
     */
    void clearInMem (); 

  };



};

#endif // define __XEM_XPATHPARSER_H

