#include <Xemeiah/xpath/xpathparser.h>
#include <Xemeiah/trace.h>

#include <Xemeiah/auto-inline.hpp>

/**
 * \file XPath tokenizing functions
 */

#define Log_XPathParser_Tokenize Debug

namespace Xem
{
  XPathParser::Token*
  XPathParser::TokenFactory::allocToken()
  {
    Token* token = new Token();
    tokens.push_back(token);
    return token;
  }

  XPathParser::TokenFactory::TokenFactory()
  {

  }

  XPathParser::TokenFactory::~TokenFactory()
  {
    for (std::list<Token*>::iterator iter = tokens.begin(); iter
        != tokens.end(); iter++)
      delete (*iter);
  }

  inline bool
  isCharIn(char u, const char * k)
  {
    for (int p = 0; k[p] != '\0'; p++)
      if (k[p] == u)
        return true;
    return false;
  }

  inline bool
  __endsWith(const char* expr, char end)
  {
    if (!expr)
      return false;
    int i = strlen(expr);
    if (i == 0)
      return false;
    return (expr[i - 1] == end);
  }

  XPathParser::Token*
  XPathParser::tokenize(const char* xpath, TokenFactory* tokenFactory)
  {
    /*
     * First, Define some convenient Macros.
     * These macros operate on the current Token, on the last Token, and on the token Stack
     *
     * allocToken : allocates a new token, puts it in current
     * nextToken  : puts a new token in current, update last <- current
     * newSymbolToken : creates a new token, initiated to the current symbol character
     * pushQName : push the qname string buffer as a new QName token
     * pushToken : push the current token at the top of the stack
     * pushSingleToken : push the current token at the top of the stack, but just for one QName token
     * popToken : pop the top of the stack to current token.
     */

#define allocToken() do { current = tokenFactory->allocToken(); } while(0)

#define nextToken() \
    do { allocToken(); \
    Log_XPathParser_Tokenize ( "New Token at %p\n", current ); \
    if ( ! headToken ) { headToken = current; last = current; }\
    else if ( last ) { last->next = current; last = current; } \
    else if ( tokenStack.size() ) \
    { AssertBug ( ! tokenStack.front()->subExpression, "Front token has already a subExpression !\n" ); \
      tokenStack.front()->subExpression = current; last = current; } \
    else { Bug ( "Don't know how to link this token !\n" ); } } while (0)

#define newSymbolToken() \
    Log_XPathParser_Tokenize ( "New Symbol Token '%c'\n", *c ); \
    nextToken();  \
    current->type = Token::Symbol; \
    current->symbol = *c;

#define pushQName() \
    if ( qname.size() ) \
    { 	\
      if ( last && last->isQName() && ( qname == "::" || stringEndsWith(last->token,String("::") ) ) ) \
      { \
        last->token += qname; qname = ""; \
      } \
      else \
      { \
        nextToken(); \
        current->token = qname; \
        qname = ""; \
        Log_XPathParser_Tokenize ( "New QName : '%s'\n", current->token.c_str() ); \
        if ( pushedSingleToken ) \
        { popToken (); } \
      } \
    }

#define pushToken() \
    pushedSingleToken = false; \
    Log_XPathParser_Tokenize ( "Pushing token '%p'\n", current ); \
    tokenStack.push_front ( current ); \
    current = last = NULL;    

#define pushSingleToken() \
    pushToken (); \
    pushedSingleToken = true;

#define popToken() \
    AssertBug ( tokenStack.size(), "Empty Stack, can't pop !\n" ); \
    current = tokenStack.front ( ); \
    Log_XPathParser_Tokenize ( "Popped token '%p'\n", current ); \
    tokenStack.pop_front (); \
    last = current; \
    pushedSingleToken = false; 

    Log_XPathParser_Tokenize ( "**** Tokenize : '%s' *****\n", xpath );

    Token* current = NULL, *last = NULL, *headToken = NULL;
    char lastChar = '\0';
    std::list<Token*> tokenStack;
    String qname, text;
    bool pushedSingleToken = false;

    for (const char* c = xpath; *c; c++)
      {
        Log_XPathParser_Tokenize ( "At Char '%c'\n", *c );
        if (lastChar == '!' && *c != '=')
          {
            Bug ( "Invalid character following '!' : '%c'\n", *c );
          }
        switch (*c)
          {
        case '\'':
        case '"':
          {
            char start = *c;
            c++;
            text = "";
            for (; *c && (*c != start); c++)
              {
                text += *c;
              }
            if (!*c)
              {
                throwXPathException ( "Unbalanced quote caracter '%c'\n", start );
              }
            nextToken ();
            current->type = Token::Text;
            current->token = text;
            break;
          }
        case '{':
        case '(':
        case '[':
          pushQName ();
          if (*c == '[' && last && last->symbol == '*')
            {
              last->type = Token::QName;
              last->token = "*";
            }
          newSymbolToken ();
          pushToken ();
          break;
        case ']':
        case '}':
        case ')':
          {
            pushQName ();
            popToken ();
            char mustBe;
            if (*c == ']')
              mustBe = '[';
            else if (*c == ')')
              mustBe = '(';
            else if (*c == '}')
              mustBe = '{';
            else
              {
                mustBe = '\0';
                Bug ( "Invalid section matcher from character '%c'\n", *c );
              }

            if (!current->isSymbol())
              {
                throwXPathException ( "Originating token is not a symbol !\n" );
              }
            if (current->symbol != mustBe)
              {
                throwXPathException ( "Unmatched closing symbols : openned with '%c', close with '%c'\n",
                    *c, current->symbol );
              }
            if (*c == ')')
              {
                Token* lastArg = current->subExpression, *lastArg0 = NULL;
                while (lastArg)
                  {
                    if (lastArg->isSymbol() && lastArg->symbol == ',')
                      lastArg0 = lastArg;
                    lastArg = lastArg->next;
                  }
                if (lastArg0)
                  {
                    Token* father = current;
                    allocToken ();
                    current->type = Token::Symbol;
                    current->symbol = ',';
                    current->subExpression = lastArg0->next;
                    lastArg0->next = current;
                    current = last = father;
                  }
              }
          }
          break;
        case '$':
        case '@':
          /*
           * Variable and attribute short form
           * These short forms expect a QName defined after.
           */
          pushQName ();
          newSymbolToken ();
          pushSingleToken ();
          break;
        case ',':
          {
            /*
             * Here we must re-arrange the tokens
             * so that the ',' symbol appears with the contents underneath
             */
            pushQName ();
            AssertBug ( tokenStack.size(), "Empty token stack !\n" );
            Token* father = tokenStack.front();
            AssertBug ( father->isSymbol(), "father token is not a symbol !\n" );
            AssertBug ( father->symbol == '(', "father token is not the '(' symbol : %c\n", father->symbol );

            if (!father->subExpression)
              throwXPathException ( "Empty XPath function argument after '%c'", father->symbol );

            AssertBug ( father->subExpression, "father token has no subExpression !\n" );
            Token* lastArg = father->subExpression, *lastArg0 = NULL;
            while (lastArg)
              {
                if (lastArg->isSymbol() && lastArg->symbol == ',')
                  lastArg0 = lastArg;
                lastArg = lastArg->next;
              }
            allocToken ();
            current->type = Token::Symbol;
            current->symbol = ',';
            if (lastArg0)
              {
                current->subExpression = lastArg0->next;
                lastArg0->next = current;
              }
            else
              {
                current->subExpression = father->subExpression;
                father->subExpression = current;
              }
            last = current;
            break;
          }
        case '*':
          {
            /*
             * Meaning of the '*' character is ambiguous :
             * It may be a wildcard for node testing (as in '@*', 'axis::*', 'namespace:*', ..)
             * Or it may be the multiplication symbol.
             */
            bool isQName = false;
            if (qname.c_str() && *qname.c_str())
              {
                Log_XPathParser_Tokenize ( "QName : %s\n", qname.c_str() );
                if (__endsWith(qname.c_str(), ':'))
                  isQName = true;
              }
            else if (last && last->isQName())
              {
                Log_XPathParser_Tokenize ( "Last is qname : %s\n", last->token.c_str() );
                if (__endsWith(last->token.c_str(), ':') || last->token == "or") //< Fixup for docbook.xsl */self::ng:* or */self::db:*
                  isQName = true;
              }
            else if (!last || isCharIn(last->symbol, "/~|,+*-"))
              {
                isQName = true;
              }
            if (qname == "mod" || qname == "div")
              {
                /*
                 *  We must find the token before this one.
                 */
                if (last)
                  {
                    isQName = true;
                  }
                else
                  {
                    isQName = false;
                  }
                pushQName ();
                Log_XPathParser_Tokenize ( "Multiply with qname='%s', last=%p(%c/%s), qname=%d\n",
                    qname.c_str(), last, last ? last->symbol : '?',
                    last ? last->token.c_str() : "", isQName );
              }

            Log_XPathParser_Tokenize ( "While at char '*' : last=%p(%d,q=%d/%c/%s), qname='%s' isQName=%d\n",
                last, last ? last->type : -1, last ? last->isQName() : -1,
                last ? last->symbol : '?', last ? last->token.c_str() : "",
                qname.c_str(), isQName );

            if (isQName)
              {
                qname += *c;
              }
            else
              {
                pushQName ();
                newSymbolToken ();
              }
          }
          break;

        case '-':
          /*
           * Meaning of the '-' character is ambiguous :
           * It may be a character inside of a QName (as in 'xsl:for-each')
           * Or it may be the substraction operator ('op1 - op2')
           * Or it may be the unary negative operator ('-op1')
           */
          if (isNumeric(qname.c_str()) || lastChar == '\0' || isCharIn(
              lastChar, "\n\r ([)]=<>+-*"))
            {
              pushQName ();
              bool isUnary = false;
              if (!last || (last->isSymbol() && isCharIn(last->symbol,
                  "*+-=><|[/~,")))
                {
                  Log_XPathParser_Tokenize ( "Negate : lastChar='%c', last=%p, last->symbol=%c, c[1]=%c\n",
                      lastChar, last, last ? last->symbol : ' ', c[1] );
                  isUnary = true;
                }
              if (last && (last->token == "mod" || last->token == "div"))
                {
                  // We must find the token before this one.
                  Token* before = NULL;
                  if (tokenStack.size())
                    {
                      before = tokenStack.front()->subExpression;
                      if (before == last)
                        before = NULL;
                    }
                  else
                    before = headToken;
                  while (before)
                    {
                      if (before->next == last)
                        break;
                      before = before->next;
                    }
                  if (before)
                    isUnary = true;
                  Log_XPathParser_Tokenize ( "Minus with last='%s', before=%p(%c/%s), unary=%d\n",
                      last->token.c_str(), before, before ? before->symbol : '?',
                      before ? before->token.c_str() : "", isUnary );
                }
              newSymbolToken ( );
              if (isUnary)
                current->symbol = 'N'; // Negate !
            }
          else if (c[1] == '>')
            {
              pushQName ();
              newSymbolToken ();
            }
          else
            {
              qname += *c;
            }
          break;

        case '/':
          /*
           * The '/' character only appears in single step separator ('step/step') or initial root ('/step')
           * Or in descendant short form (as initial '//step' or inside 'step//step').
           */
          if (lastChar == '/')
            {
              AssertBug ( last->isSymbol() && last->symbol == lastChar, "Dropped the last in a lastChar\n" );
              last->token = last->symbol;
              last->token += *c;
              last->symbol = '~';
              break;
            }
        case '=':
          if (*c == '=' && (lastChar == '<' || lastChar == '>' || lastChar
              == '!'))
            {
              AssertBug ( last->isSymbol() && last->symbol == lastChar, "Dropped the last in a lastChar\n" );
              last->token = last->symbol;
              last->token += *c;
              break;
            }
        case '<':
        case '>':
          if (lastChar == '-')
            {
              if (last)
                {
                  Log_XPathParser_Tokenize ( "ElementFunctionCall : last=%c/'%s'\n", last->symbol, last->token.c_str() );
                  last->symbol = '#';
                }
              else
                {
                  Bug ( "No last !\n" );
                }
              Log_XPathParser_Tokenize ( "Operator '->' found !\n" );
              break;
            }
        case '+':
        case '!':
        case '|':
          pushQName ();
          newSymbolToken ( );
          break;
        case '\t':
        case ' ':
        case '\n':
        case '\r':
          pushQName ();
          break;

        default:
          qname += *c;
          break;
          }
        lastChar = *c;
      }
    pushQName ();
    if (tokenStack.size())
      {
        throwXPathException ( "Unbalanced parentheses : still %ld tokens on stack.\n", (unsigned long) tokenStack.size() );
      }
    return headToken;
#undef allocToken
#undef nextToken
#undef newSymbolToken
#undef pushQName
#undef pushToken
#undef pushSingleToken
#undef popToken
  }

  /*
   * Handle the operator precedence, ...
   * This is greatly under-productive, but no clue on how to optimize it yet...
   */
  void
  XPathParser::rearrangeTokens(Token* token, TokenFactory* tokenFactory)
  {
#define copyToken(__to,__from) \
    (__to)->type = (__from)->type; \
    (__to)->token = (__from)->token; \
    (__to)->symbol = (__from)->symbol;
    AssertBug ( token, "Null token provided !\n" );
    Log_XPathParser_Tokenize ( "rearrange Token '%p' (%c/%s)\n", token, token->symbol, token->token.c_str() );

    if (token->isSymbol() && token->symbol == '~')
      {
        AssertBug ( token->token == "//", "Invalid complex token for descendant : '%s'\n", token->token.c_str() );
        /*
         * The first token of a token chain is '//', so we have to insert a root token before.
         */
        Token* nextToken = tokenFactory->allocToken();
        nextToken->type = Token::Symbol;
        nextToken->symbol = '~';
        nextToken->token = "//";
        nextToken->next = token->next;

        token->next = nextToken;
        token->symbol = '/';
        token->token = "";
      }

    Token* lastToken = NULL;

    int candidatePrecedence = 0;

    Token* candidateToken = NULL;
    Token* candidateLastToken = NULL;

    Log_XPathParser_Tokenize ( "***** Rearranging tokens from chain-start Token '%p' (%c/%s)\n", token, token->symbol, token->token.c_str() );

    for (Token* tk = token; tk; tk = tk->next)
      {
        Log_XPathParser_Tokenize ( "Top Of Loop (token=%p) - rearrange : at tk '%p' (%c/%s)\n", token, tk, tk->symbol, tk->token.c_str() );

        if (tk->subExpression2)
          {
            Log_XPathParser_Tokenize ( "Token symbol %p ('%c') already has a subExpression !\n", tk, tk->symbol );
            lastToken = tk;
            continue;
          }
        if (tk->subExpression)
          {
            rearrangeTokens(tk->subExpression, tokenFactory);
          }

        if (tk == token)
          {
            Log_XPathParser_Tokenize ( "--> Not a token, is at head.\n" );
            lastToken = tk;
            continue;
          }

        if (!tk->next)
          {
            Log_XPathParser_Tokenize ( "--> Not a token, has no next.\n" );
            lastToken = tk;
            continue;
          }

        bool found = false;
        int myPrecedence = 0;
#define __checkToken(__short,__long,__precedence) \
        if ( ! found && ( ( tk->isSymbol() && tk->symbol == __short ) \
             || ( tk->isQName() && tk->token == __long ) ) && __precedence >= candidatePrecedence ) \
          { \
            Log_XPathParser_Tokenize ( "Rearrange : found a new token at %p, %c/%s, precedence=%d\n", tk, __short, __long, __precedence); \
            found = true; \
            myPrecedence = __precedence; \
          }

        __checkToken ( 'B', "or", 30 )
        __checkToken ( 'B', "and", 29 )
        __checkToken ( '=', "#Equals", 28 )
        __checkToken ( '!', "#NotEquals", 27 )
        __checkToken ( '<', "#Less", 26 )
        __checkToken ( '>', "#Greater", 25 )
        __checkToken ( '|', "#Union", 24 )
        __checkToken ( '+', "#Plus", 23 )
        __checkToken ( '-', "#Minus", 22 )
        __checkToken ( '*', "#Multiply", 21 )
        __checkToken ( 'B', "div", 20 )
        __checkToken ( 'B', "mod", 19 )

        if (!found)
          {
            Log_XPathParser_Tokenize ( "--> Not a token.\n" );
            lastToken = tk;
            continue;
          }

        if (tk->isQName())
          {
            if ((tk == token) || (lastToken && (lastToken->symbol == '/'
                || lastToken->symbol == '~' || (candidatePrecedence
                && lastToken->token == tk->token))))
              {
                Log_XPathParser_Tokenize ( "Token '%p' (%c/%s) -> this is NOT a REAL symbol token\n", token, token->symbol, token->token.c_str() );
                lastToken = tk;
                continue;
              }
          }
        Log_XPathParser_Tokenize ( "Token '%p' (%c/%s) -> this is a token (last:%p=%c/%s)\n", token, token->symbol, token->token.c_str(),
            lastToken, lastToken ? lastToken->symbol : '?', lastToken ? lastToken->token.c_str() : "(none)" );

        AssertBug ( tk != token, "Token at head !\n" );
        /*
         * Swap tokens : 
         * The token given as argument is the head token, so it's this one we have to requalify as holding the symbol
         * The tk, which previously hold the binary operand symbol, will hold the first token.
         */

        AssertBug ( ! tk->subExpression, "Token symbol '%c' already has a subExpression !\n", tk->symbol );
        AssertBug ( ! tk->subExpression2, "Token symbol '%c' already has a subExpression !\n", tk->symbol );

        candidateToken = tk;
        candidateLastToken = lastToken;
        candidatePrecedence = myPrecedence;

        lastToken = tk;
      }

    if (candidateToken)
      {
        Log_XPathParser_Tokenize ( "CandidateToken : %p, lastToken : %p, precedence = %d, full chain is :\n",
            candidateToken, candidateLastToken, candidatePrecedence );
        // token->log ();
#if PARANOID
        AssertBug ( candidateToken->next, "Candidate token has no next token !\n" );
#endif

        if (candidateToken->isQName())
          {
            candidateToken->type = Token::Symbol;
            candidateToken->symbol = 'B';
          }
        Token temp;

        copyToken(&temp, token);
        copyToken(token, candidateToken);
        copyToken(candidateToken, &temp);

        candidateToken->subExpression = token->subExpression;
        token->subExpression2 = candidateToken->next;
        if (token == candidateLastToken)
          candidateToken->next = NULL;
        else
          candidateToken->next = token->next;
        token->subExpression = candidateToken;
        token->next = NULL;
        candidateLastToken->next = NULL;

        rearrangeTokens(token->subExpression, tokenFactory);
        rearrangeTokens(token->subExpression2, tokenFactory);
      }

  }

  void
  XPathParser::Token::log(int indent)
  {
    static const char* types[] =
      { "QName", "Symbol", "Text", NULL };
    char cindent[256];
    for (int i = 0; i < indent * 4; i++)
      cindent[i] = ' ';
    cindent[indent * 4] = '\0';
    fprintf(stderr,
        "%sXPath Token at '%p', type=%s%s%c%s%s next=%p, sub=%p, sub2=%p\n",
        cindent, this, types[type], symbol ? ", symbol=" : "", symbol ? symbol
            : ' ', token.size() ? ", token=" : "", token.size() ? token.c_str()
            : "", next, subExpression, subExpression2);
    if (subExpression)
      subExpression->log(indent + 1);
    if (subExpression2)
      subExpression2->log(indent + 1);
    if (next)
      next->log(indent);
  }
};
