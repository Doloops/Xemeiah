#include <Xemeiah/xpath/xpathparser.h>
#include <Xemeiah/trace.h>

#include <Xemeiah/auto-inline.hpp>

#if 0 // def LOG
#define XPATH_LOG_TOKENS
#endif

#define Log_XPathParser Debug
#define Log_XPathParser_Optim Debug //< Log_XPathParser, but for optimization stuff
#define Log_XPathParser_Number Debug
#define Log_XPathParser_Tag Debug

#define throwNotImplemented(...) throwXPathException ( "Not Implemented : " __VA_ARGS__ )

#define Invalid(...) throwException ( XPathParserException, __VA_ARGS__ )

#define AssertInvalid(__cond,...) \
  if ( ! (__cond) ) Invalid ( __VA_ARGS__ );

#define __XEM_XPATHPARSER_OPTIMIZE_LOCALNAME_EQUAL
#define __XEM_XPATHPARSER_OPTIMIZE_CHILDLOOKUP_PREDICATE
#define __XEM_XPATHPARSER_OPTIMIZE_DESCENDANT_AXIS

namespace Xem
{
    void
    XPathParser::init ()
    {
        expression = NULL;
        parsingFromElementRef = NULL;
        parsingFromKeyCache = NULL;
        parsingFromNamespaceAlias = NULL;
        resourceBlock = NULL;
        allocedResourceBlockSize = 0;
        nextResourceOffset = 0;
        firstStep = XPathStepId_NULL;
        packedSegment = NULL;
    }

    void
    XPathParser::init (ElementRef& eltRef, AttributeRef& attrRef, bool isAVT)
    {
        Log_XPathParser ( "No _XPath format, parsing '%s'. elt=%llx key=%x(%s), attr=%x(%s) (context=%p, wr=%d)\n",
                attrRef.getData<char,Read>(),
                eltRef.getElementId(), eltRef.getKeyId(), eltRef.getKey().c_str(),
                attrRef.getKeyId(), attrRef.getKey().c_str(), &(eltRef.getDocument()), eltRef.getDocument().isWritable() );
        expression = attrRef.toString().c_str();
        parsingFromElementRef = &eltRef;
        parsingFromKeyCache = &(eltRef.getKeyCache());
        eltRef.getDocument().stats.numberOfXPathParsed++;
        eltRef.getDocument().getStore().stats.numberOfXPathParsed++;
        parse(attrRef.toString().c_str(), isAVT);
    }

    XPathParser::XPathParser (ElementRef& eltRef, AttributeRef& attrRef, bool isAVT)
    {
        init();
        init(eltRef, attrRef, isAVT);
    }

    XPathParser::XPathParser (ElementRef& eltRef, KeyId attrKeyId, bool isAVT)
    {
        init();

        AttributeRef attrRef = eltRef.findAttr(attrKeyId, AttributeType_String);
        if (!attrRef)
        {
            Error("Could not get xpath : elementId '%llx' :  %s (%x), attr %s (%x)\n", eltRef.getElementId(),
                  eltRef.getKey().c_str(), eltRef.getKeyId(), eltRef.getKeyCache().dumpKey(attrKeyId).c_str(),
                  attrKeyId);
            throwXPathException("Could not get xpath : elementId '%llx' :  %s (%x), attr %s (%x)\n",
                                eltRef.getElementId(), eltRef.getKey().c_str(), eltRef.getKeyId(),
                                eltRef.getKeyCache().dumpKey(attrKeyId).c_str(), attrKeyId);

        }
        init(eltRef, attrRef, isAVT);
    }

    XPathParser::XPathParser (KeyCache& keyCache, NamespaceAlias& nsAlias, const char* expr, bool isAVT)
    {
        // Warn ( "[STATIC] XPath : parsing fixed string '%s'\n", expr );
        init();
        expression = expr;
        parsingFromKeyCache = &keyCache;
        parsingFromNamespaceAlias = &nsAlias;
        parse(expr, isAVT);
        keyCache.getStore().stats.numberOfStaticXPathParsed++;
    }

    XPathParser::XPathParser (KeyCache& keyCache, NamespaceAlias& nsAlias, const String& expr, bool isAVT)
    {
        // Warn ( "[STATIC] XPath : parsing fixed string '%s'\n", expr );
        init();
        expression = expr.c_str();
        parsingFromKeyCache = &keyCache;
        parsingFromNamespaceAlias = &nsAlias;
        parse(expr.c_str(), isAVT);
        keyCache.getStore().stats.numberOfStaticXPathParsed++;
    }

    XPathParser::XPathParser (ElementRef& eltRef, const String& expr)
    {
        static const bool isAVT = false;
        init();
        expression = expr.c_str();
        parsingFromElementRef = &eltRef;
        parsingFromKeyCache = &(eltRef.getKeyCache());
        eltRef.getDocument().stats.numberOfXPathParsed++;
        eltRef.getDocument().getStore().stats.numberOfXPathParsed++;
        parse(expr.c_str(), isAVT);
    }

    void
    XPathParser::clearInMem ()
    {
#if 0
        if ( xpathSegment ) free ( xpathSegment );
        if ( stepBlock ) free ( stepBlock );
        xpathSegment = NULL;
        stepBlock = NULL;
#endif

        if (resourceBlock)
            free(resourceBlock);
        if (packedSegment)
            free(packedSegment);

        resourceBlock = NULL;
        nextResourceOffset = 0;
        packedSegment = NULL;
    }

    XPathParser::~XPathParser ()
    {
        clearInMem();
    }

    bool
    XPathParser::isNumeric (const char* expr)
    {
        if (!expr)
            return false;
        bool hasDot = false;
        if (*expr == '.' && !('0' <= expr[1] && expr[1] <= '9'))
            return false;
        if (('0' <= *expr && *expr <= '9') || *expr == '-' || *expr == '.')
        {
            if (*expr == '.')
                hasDot = true;
            for (const char* c = &(expr[1]); *c; c++)
            {
                if ('0' <= *c && *c <= '9')
                    continue;
                if (!hasDot && *c == '.')
                {
                    hasDot = true;
                    continue;
                }
                return false;
            }
            return true;
        }
        return false;
    }

    KeyId
    XPathParser::getKeyId (const String& key)
    {
        AssertBug(parsingFromKeyCache, "No keyCache !\n");
        if (parsingFromNamespaceAlias)
        {
            return parsingFromKeyCache->getKeyId(*parsingFromNamespaceAlias, key, true);
        }
        else if (parsingFromElementRef)
        {
            KeyCache& keyCache = parsingFromElementRef->getDocument().getKeyCache();
            KeyId keyId = parsingFromKeyCache->getKeyIdWithElement(*parsingFromElementRef, key, false);
            if (KeyCache::getNamespaceId(keyId) == keyCache.getBuiltinKeys().xhtml.ns()
                    && parsingFromElementRef->getDefaultNamespaceId() == keyCache.getBuiltinKeys().xhtml.ns())
            {
                /*
                 * It looks like xhtml may not be considered default if set at root element..
                 * This is completely stupid, but...
                 */
                return KeyCache::getKeyId(0, KeyCache::getLocalKeyId(keyId));
            }
            return keyId;
        }
        Bug("I have been asked to parse, but neither parsingFromNamespaceAlias or parsingFromElement have been set !\n");
        return 0;
    }

    XPathStepId
    XPathParser::allocStep ()
    {
        XPathStepId stepId = parsedSteps.size();
        XPathStep* step = new XPathStep();
        parsedSteps.push_back(step);
#if PARANOID
        AssertBug ( parsedSteps[stepId] == step, "Wrong way" );
#endif    
        memset(step, 0, sizeof(XPathStep));
        step->lastStep = XPathStepId_NULL;
        step->nextStep = XPathStepId_NULL;

        for (int i = 0; i < XPathStep_MaxPredicates; i++)
            step->predicates[i] = XPathStepId_NULL;
        return stepId;
    }

    XPathStep*
    XPathParser::getStep (XPathStepId stepId)
    {
        if (stepId >= parsedSteps.size())
        {
            throwException(XPathParserException, "StepId too high : %lu (current length: %lu)\n",
                           (unsigned long ) stepId, (unsigned long ) parsedSteps.size());
        }
        return parsedSteps[stepId];
    }

    XPathStepId
    XPathParser::allocStepResource (const char* text)
    {
        __ui64 size = (__ui64) (strlen (text) + 1);
        if ( nextResourceOffset + size >= allocedResourceBlockSize )
        {
            allocedResourceBlockSize = nextResourceOffset + size + 64;
            resourceBlock = (char*) realloc ( resourceBlock, allocedResourceBlockSize );
            if ( ! resourceBlock )
            {
                throwException ( XPathParserException, "Could not allocate 0x%llx bytes for resources.\n", allocedResourceBlockSize );
            }
        }
        __ui64 offset = nextResourceOffset;
        memcpy ( &(resourceBlock[offset]), text, size );
        nextResourceOffset += size;
        XPathStepId resStepId = allocStep ();
        XPathStep* resStep = getStep ( resStepId );
        resStep->action = XPathAxis_Resource;
        resStep->resource = offset;

        return resStepId;
    }

    void
    XPathParser::parse (const char* expr, bool embedded)
    {
        if (!expr || !expr[0])
        {
            Log_XPathParser ( "Parsing an empty content !\n" );
            firstStep = XPathStepId_NULL;
            return;
        }

        Log_XPathParser ( "Parsing '%s'\n", expr );

        /*
         * Parse steps.
         */
        try
        {
            if (embedded)
            {
                firstStep = parseAVT(expr);
            }
            else
            {
                firstStep = doParse(expr);
                computeLastSteps();
            }
        }
        catch (Exception * xpe)
        {
            detailException(xpe, "While parsing expression '%s'\n", expr);
            throw xpe;
        }

#ifdef XPATH_LOG_TOKENS
        fprintf ( stderr, "\nParsed XPath '%s' (%lu bytes) :\n", expr, strlen(expr) );
        XProcessor& _xproc = *((XProcessor*)NULL);
        XPath _xpath ( _xproc, *this );
        _xpath.logXPath ();
#endif
    }

    XPathStepId
    XPathParser::doParse (const char* xpath)
    {
        /*
         * First, create a TokenFactory
         */
        TokenFactory tokenFactory;

        /*
         * Then, tokenize the expression, returning the head token of the main token chain.
         */
        Token* headToken = tokenize(xpath, &tokenFactory);
        if (!headToken)
            return XPathStepId_NULL;
        Log_XPathParser ( "Parsed : parsed '%lu' tokens.\n", (unsigned long) tokenFactory.size() );

#ifdef XPATH_LOG_TOKENS
        fprintf ( stderr, "-------------------------------- BEFORE --------------------------------------\n" );
        headToken->log ();
        fprintf ( stderr, "----------------------------------------------------------------------\n" );
#endif

        /*
         * Then, rearrange tokens, in order to respect operators precedence
         */
        rearrangeTokens(headToken, &tokenFactory);

#ifdef XPATH_LOG_TOKENS
        fprintf ( stderr, "--------------------------------- AFTER --------------------------------------\n" );
        headToken->log ();
        fprintf ( stderr, "----------------------------------------------------------------------\n" );
#endif

        /*
         * Finally, recursively convert the Token tree into a list of XPathSteps
         */
        XPathStepId headStepId = tokenToStep(headToken);

        return headStepId;
    }

    /*
     * ********************** All Functions bellow are Token to XPathStep converters **********************************
     *
     * tokenToStep() : default converter, will try to find which function to use
     * tokenToFunctionStep() : converts a "QName (" into a XPath function (if it is not a node test).
     * binaryTokenToStep : converts all binary operators '+', '-', ... to their appropriate XPath function.
     * qnameTokenToTest : converts QName tests to their KeyId-based test.
     */
    XPathStepId
    XPathParser::tokenToStep (Token* token)
    {
        Log_XPathParser ( "Token to Step : token = %p,(%c/%s), isText=%d, isQName=%d\n",
                token, token->symbol, token->token.c_str(), token->isText(), token->isQName() );
        if (token->isText())
        {
            return allocStepResource(token->token.c_str());
        }
        else if (token->isQName())
        {
            XPathStepId stepId = allocStep();
            XPathStep* step = getStep(stepId);
            if (isNumeric(token->token.c_str()))
            {
                Integer constInteger = atoll(token->token.c_str());
                Number constNumber = atof(token->token.c_str());
                Log_XPathParser_Number ( "Chosing '%s' : cI=%lld, cN=%lf\n", token->token.c_str(), constInteger, constNumber );
                bool hasDot = (strchr(token->token.c_str(), '.') != NULL);
                if ((!hasDot) || (constNumber == constInteger))
                {
                    step->action = XPathAxis_ConstInteger;
                    step->constInteger = constInteger;
                    Log_XPathParser_Number ( "New Const Integer : '%lld'\n", step->constInteger );
                }
                else
                {
                    step->action = XPathAxis_ConstNumber;
                    step->constNumber = constNumber;
                    Log_XPathParser_Number ( "New Const Number : '%.32f'\n", step->constNumber );
                }
                AssertInvalid(!token->next, "Invalid next token after numeric constant !\n");
            }
            else if (token->next)
            {
                if (token->next->isSymbol() && token->next->symbol == '(')
                {
                    tokenToFunctionStep(step, token);
                    token = token->next;
                }
                else
                {
                    qnameTokenToTest(step, token);
                    if (token->next)
                    {
                        Log_XPathParser ( "Token after QName : '%c/%s'\n", token->next->symbol, token->next->token.c_str() );
                        nextTokenToStep(step, token, token->next);
                    }
                }
            }
            else
            {
                qnameTokenToTest(step, token);
            }
            return stepId;
        }
        else
        {
            switch (token->symbol)
            {
                case '/':
                {
                    AssertInvalid(token->token.size() == 0, "Symbol defined with token = '%s'\n", token->token.c_str());
                    XPathStepId stepId = allocStep();
                    XPathStep* step = getStep(stepId);
                    step->action = XPathAxis_Root;
                    step->keyId = 0;
                    if (token->next)
                        step->nextStep = tokenToStep(token->next);
                    return stepId;
                }
                case '$':
                {
                    XPathStepId stepId = allocStep();
                    XPathStep* step = getStep(stepId);
                    AssertInvalid(token->subExpression->isQName() && token->subExpression->next == NULL,
                                  "Invalid variable contents.\n");
                    step->action = XPathAxis_Variable;
                    step->keyId = getKeyId(token->subExpression->token);
                    nextTokenToStep(step, token, token->next);
                    Log_XPathParser ( "nextTokenToStep : step=%p, step->nextStep=%u\n", step, step->nextStep );
                    return stepId;
                }
                case '@':
                {
                    XPathStepId stepId = allocStep();
                    XPathStep* step = getStep(stepId);

                    step->action = XPathAxis_Attribute;
                    if (token->subExpression->token == "*")
                        step->keyId = 0;
                    else if (strchr(token->subExpression->token.c_str(), '*'))
                    {
                        char* tk = strdup(token->subExpression->token.c_str());
                        char* p = strchr(tk, '*');
                        p[0] = 'p';
                        step->keyId = getKeyId(tk);
                        step->keyId = KeyCache::getKeyId(KeyCache::getNamespaceId(step->keyId), 0);
                        step->flags |= XPathStepFlags_NodeTest_Namespace_Only;
                        p[0] = '*';
                        free(tk);
                    }
                    else
                        step->keyId = getKeyId(token->subExpression->token.c_str());
                    nextTokenToStep(step, token, token->next);
                    return stepId;
                }
                case '<':
                case '>':
                case '!':
                case '=':
                case '+':
                case '*':
                case '-':
                case 'B':
                case '|':
                {
                    AssertInvalid(!token->next, "Invalid token '%c' (%s) with next '%c' (%s) (at %p, next=%p)!\n",
                                  token->symbol, token->token.c_str(), token->next->symbol, token->next->token.c_str(),
                                  token, token->next);
                    XPathStepId stepId = allocStep();
                    XPathStep* step = getStep(stepId);
                    binaryTokenToStep(step, token);
                    return stepId;
                }
                case 'N':
                {
                    AssertInvalid(token->next, "Token has no next !\n");
                    // TODO : We may negate the constant if it is a constant
                    XPathStepId negateId = tokenToStep(token->next);

                    XPathStepId stepId = allocStep();
                    XPathStep* step = getStep(stepId);
                    step->action = XPathFunc_Minus;
                    step->functionArguments[0] = allocStep();
                    XPathStep* zeroStep = getStep(step->functionArguments[0]);
                    zeroStep->action = XPathAxis_ConstInteger;
                    zeroStep->constInteger = 0;
                    step->functionArguments[1] = negateId;
                    step->functionArguments[2] = XPathStepId_NULL;
                    return stepId;
                }
                case '(':
                {
                    AssertInvalid(token->subExpression, "Token has no subexpression.\n");
                    if (token->next)
                    {
                        XPathStepId stepId = allocStep();
                        XPathStep* step = getStep(stepId);
                        step->action = XPathFunc_NodeSetTest;
                        step->functionArguments[0] = tokenToStep(token->subExpression);
                        step->functionArguments[1] = XPathStepId_NULL;
                        step->functionArguments[2] = XPathStepId_NULL;

                        Token* next = token->next;
                        int predicateId = 0;
                        while (next)
                        {
                            AssertInvalid(next->isSymbol(), "Next token is not a symbol !\n");
                            if (next->symbol == '[')
                            {
                                AssertInvalid(next->subExpression, "Next token has no subExpression !\n");
                                step->predicates[predicateId] = tokenToStep(next->subExpression);
                                predicateId++;
                                AssertInvalid(predicateId < XPathStep_MaxPredicates,
                                              "Maximum number of predicates reached !\n");
                            }
                            else if (next->symbol == '/' || next->symbol == '~')
                            {
                                step->nextStep = tokenToStep(next->next);
                                break;
                            }
                            else
                            {
                                Invalid("Invalid symbol '%c'(%s) (tk=%p)\n", next->symbol, next->token.c_str(), next);
                            }
                            next = next->next;
                        }
                        tagStepPredicatesFlags(step);
                        return stepId;
                    }
                    return tokenToStep(token->subExpression);
                }
                case '~':
                    AssertInvalid(token->token == "//", "Invalid Symbol with complex token '%s'\n",
                                  token->token.c_str())
                    ;
                    AssertInvalid(token->next, "Next has no next.\n")
                    ;

                    if (token->next->isSymbol())
                    {
                        if (token->next->symbol != '@')
                        {
                            Invalid("May not have symbol after '//' : token %c/%s\n", token->next->symbol,
                                    token->next->token.c_str());
                        }
                        XPathStepId stepId = allocStep();
                        XPathStep* step = getStep(stepId);
                        step->action = XPathAxis_Descendant_Or_Self;
                        step->keyId = 0;
                        step->nextStep = tokenToStep(token->next);
                        return stepId;
                    }
                    /*
                     * TODO : Optimize this conversion
                     */
                    else
                    {
                        AssertInvalid(token->next->isQName(), "Next is not a QName.\n");
                        XPathStepId stepId = tokenToStep(token->next);
                        XPathStep* step = getStep(stepId);

#ifdef __XEM_XPATHPARSER_OPTIMIZE_DESCENDANT_AXIS
                        tagStepPredicatesFlags(step);
                        if (step->action != XPathAxis_Child
                                || (step->predicates[0] != XPathStepId_NULL
                                        && (step->flags
                                                & (XPathStepFlags_PredicateHasPosition | XPathStepFlags_PredicateHasLast))))
                        {
                            XPathStepId beforeStepId = allocStep();
                            XPathStep* beforeStep = getStep(beforeStepId);
                            beforeStep->action = XPathAxis_Descendant_Or_Self;
                            beforeStep->keyId = 0;
                            beforeStep->nextStep = stepId;
                            return beforeStepId;
                        }
#endif // __XEM_XPATHPARSER_OPTIMIZE_DESCENDANT_AXIS
                        step->action = XPathAxis_Descendant;
                        return stepId;
                    }
                case '{':
                    /*
                     * The XPath Indirection mechanism, a Xemeiah extension.
                     */
                {
                    XPathStepId stepId = allocStep();
                    XPathStep* step = getStep(stepId);
                    step->action = XPathFunc_XPathIndirection;
                    step->functionArguments[0] = tokenToStep(token->subExpression);
                    step->functionArguments[1] = XPathStepId_NULL;
                    if (token->next)
                        step->nextStep = tokenToStep(token->next);
                    return stepId;
                }
                case '#':
                    /*
                     * The '->' operator
                     */
                {
                    XPathStepId funcId = allocStep();
                    XPathStep* funcStep = getStep(funcId);

                    AssertInvalid(token->next && token->next->isQName(), "Invalid operator after '->' : '%s'\n",
                                  token->next ? token->next->token.c_str() : "(none)");

                    AssertInvalid(
                            token->next->next && token->next->next->isSymbol() && token->next->next->symbol == '(',
                            "Invalid operator after '->'\n");

                    tokenToFunctionStep(funcStep, token->next);
                    token = token->next;

                    Log_XPathParser ( "Operator '->' : funcStep: action=%x, next=%u, args=[%u:%u:%u...]\n",
                            funcStep->action, funcStep->nextStep,
                            funcStep->functionArguments[0], funcStep->functionArguments[1], funcStep->functionArguments[2] );

                    AssertInvalid(funcStep->action == XPathFunc_FunctionCall,
                                  "Operator following '->' did not resolve as a function !\n");
                    funcStep->action = XPathFunc_ElementFunctionCall;

                    return funcId;
                }
                default:
                    NotImplemented("Symbol Token '%c', in expression '%s'.\n", token->symbol, getExpression());
            }

        }
        Bug("Shall not be here.\n");
        return XPathStepId_NULL;
    }

    void
    XPathParser::tokenToFunctionStep (XPathStep* step, Token* token)
    {
        char* funcName = (char*) token->token.c_str();
        bool found = false;

        /*
         * Function parsing is operated in two phases :
         * First, we try to eliminate the node-tests node(), text(), processing-instruction()
         * with their optional preliminary axis prefix (child::text(), preceding::node(), ...).
         * Then, if this conversion did not succeed, we try to parse the function according to
         * the known builtin functions list.
         */
        if (strstr(token->token.c_str(), "::"))
        {
            char* axis = strdup(token->token.c_str());
            funcName = strstr(axis, "::");
            funcName[0] = '\0';
            funcName += 2;
#define __XPath_Axis(__name,__id) \
        if ( ! found && strcmp(__name, axis) == 0 )	\
          { found = true; step->action = XPathAxis_##__id; }
#include <Xemeiah/xpath/xpath-axis.h>
#undef __XPath_Axis
            free(axis);
            if (!found)
            {
                throwXPathException("Invalid axis : '%s'\n", token->token.c_str());
            }
            Log_XPathParser ( "Found axis %d for '%s'\n", step->action, token->token.c_str() );
            funcName = (char*) strstr(token->token.c_str(), "::");
            funcName += 2;
        }
        else
            step->action = XPathAxis_Child;

        found = false;

        Log_XPathParser ( "funcName = '%s'\n", funcName );
        if (strcmp(funcName, "node") == 0)
        {
            found = true;
            step->keyId = 0;
        }
#define __XPath_Test(__name,__id) \
    if ( !found && strcmp(__name, funcName) == 0 ) \
    { found = true; step->keyId = parsingFromKeyCache->getBuiltinKeys().xemint.__id();  }

        __XPath_Test("text", textnode);
        __XPath_Test("comment", comment);
        __XPath_Test("element", element);
#undef __XPath_Test

        if (!found && strcmp(funcName, "processing-instruction") == 0)
        {
            found = true;
            step->keyId = KeyCache::getKeyId(parsingFromKeyCache->getBuiltinKeys().xemint_pi.ns(), 0);

            if (token->next->subExpression)
            {
                if (token->next->subExpression->isText())
                {
                    LocalKeyId localKeyId;
                    if (token->next->subExpression->token == "*")
                    {
                        localKeyId = ~0;
                        Warn("Invalid processing-instruction() argument : '%s'\n",
                             token->next->subExpression->token.c_str());
                    }
                    else
                    {
                        localKeyId = parsingFromKeyCache->getKeyId(0, token->next->subExpression->token.c_str(), true);
                    }

                    step->keyId = KeyCache::getKeyId(parsingFromKeyCache->getBuiltinKeys().xemint_pi.ns(), localKeyId);
                    /*
                     * Clear out the subExpression for future tests.
                     */
                    token->next->subExpression = NULL;
                }
                else
                {
                    Log_XPathParser ( "processing-instruction : shall implement subExpression.\n" );
                    token->next->subExpression->log();
                    NotImplemented("processing-instruction : shall implement subExpression.\n");
                }
            }
        }

        if (found)
        {
            if (step->action == XPathAxis_Namespace)
            {
                if (step->keyId == 0)
                {
                    // NamespaceId xmlNSKeyId = parsingFromKeyCache->getBuiltinKeys().xmlns.ns();
                    // step->keyId = parsingFromKeyCache->getKeyId ( xmlNSKeyId, step->keyId );
                }
                else
                {
                    Warn("Strange node test '%s' on namespace axis !\n", funcName);
                }
            }
            Log_XPathParser ( "Found a NodeTest step : axis=%u, test=0x%x\n", step->action, step->keyId );

            AssertInvalid(!token->next->subExpression, "Token '(' had a subexpression !\n");
            if (token->next->next)
            {
                nextTokenToStep(step, token, token->next->next);
            }
            return;
        }

        step->action = XPathAction_NULL;
        int cardinality = 0;
        found = false;
        bool defaultsToSelf = false;
#define __XPath_Func(__name,__id,__cardinality,__defaultsToSelf)	\
    if ( ! found && strcmp ( token->token.c_str(), __name ) == 0 )	\
    {							\
        step->action = XPathFunc_##__id;		\
        cardinality = __cardinality;			\
        found = true; defaultsToSelf = __defaultsToSelf;	\
    }
#include <Xemeiah/xpath/xpath-funcs.h>
#undef __XPath_Func
        if (!found)
        {
            step->action = XPathFunc_FunctionCall;
            step->keyId = getKeyId(token->token);
            cardinality = -1;
            found = true;
            defaultsToSelf = false;
        }
        if (!found)
        {
            throwXPathException("Invalid function : '%s'\n", token->token.c_str());
        }
        Log_XPathParser ( "Function : '%s' -> '%d'\n", token->token.c_str(), step->action );
        std::list<Token*> arguments;

        if (!token->next)
        {
            Invalid("No next ??\n");
        }
        else if (token->next->subExpression && token->next->subExpression->isSymbol()
                && token->next->subExpression->symbol == ',')
        {
            for (Token* tk = token->next->subExpression; tk; tk = tk->next)
            {
                AssertInvalid(tk->isSymbol() && tk->symbol == ',', "Invalid sub-component %c/%s\n", tk->symbol,
                              tk->token.c_str());
                arguments.push_back(tk->subExpression);
            }
        }
        else if (token->next->subExpression)
        {
            arguments.push_back(token->next->subExpression);
        }
        Log_XPathParser ( "Function has '%lu' arguments, defined cardinality '%d', defaultsToSelf '%d'\n",
                (unsigned long) arguments.size(), cardinality, defaultsToSelf );
        for (int a = 0; a < XPathStep_MaxFuncCallArgs; a++)
        {
            step->functionArguments[a] = XPathStepId_NULL;
        }
        if (arguments.size() == 0 && defaultsToSelf)
        {
            step->functionArguments[0] = allocStep();
            XPathStep* selfStep = getStep(step->functionArguments[0]);
            selfStep->action = XPathAxis_Self;
            selfStep->keyId = 0;
        }
        int i = 0;
        int done = 0;
        XPathStep* functionStep = step;
        for (std::list<Token*>::iterator iter = arguments.begin(); iter != arguments.end(); iter++)
        {
            int remains = arguments.size() - done;
            i++;
            done++;
            if (step->action == XPathFunc_Concat && i == XPathStep_MaxFuncCallArgs && remains > 1)
            {
                step->functionArguments[i - 1] = allocStep();
                step = getStep(step->functionArguments[i - 1]);
                step->action = XPathFunc_Concat;
                i = 1;
                for (int a = 0; a < XPathStep_MaxFuncCallArgs; a++)
                {
                    step->functionArguments[a] = XPathStepId_NULL;
                }
            }
            if (i >= XPathStep_MaxFuncCallArgs)
            {
                throwXPathException("Too much arguments : '%ld'\n", (long int )arguments.size());
            }
            AssertInvalid(*iter, "Invalid NULL Iter !\n");
            XPathStepId argId = XPathStepId_NULL;
            Token* arg = *iter;
            if (arg->isText()
                    && ((i == 1
                            && (step->action == XPathFunc_Key || step->action == XPathFunc_SystemProperty
                                    || step->action == XPathFunc_ElementAvailable
                                    || step->action == XPathFunc_FunctionAvailable))
                            || (i == 3 && (step->action == XPathFunc_FormatNumber))))
            {
                /*
                 * for key() and system-property(), the first argument must be parsed as a QName if it is a text
                 */
                KeyId qnameId = getKeyId(arg->token.c_str());
                if (qnameId == 0)
                {
                    throwXPathException("Invalid QName value '%s'\n", arg->token.c_str());
                }
                Log_XPathParser ( "key() or system-property() : QName '%s' -> keyId '%x'\n",
                        arg->token.c_str(), qnameId );
                argId = allocStep();
                XPathStep* qnameStep = getStep(argId);
                qnameStep->action = XPathAxis_ConstInteger;
                qnameStep->constInteger = qnameId;
            }
            else
            {
                /*
                 * Normal behavior : transform the argument token to a step chain
                 */
                argId = tokenToStep(*iter);
            }
            Log_XPathParser ("\tArg %i=%p, stepId=%d\n", i, *iter, argId );
            step->functionArguments[i - 1] = argId;
        }
        if (token->next->next)
        {
            nextTokenToStep(functionStep, token, token->next->next);
        }
    }

    void
    XPathParser::qnameTokenToTest (XPathStep* step, Token* token)
    {
        if (token->token == ".")
        {
            step->action = XPathAxis_Self;
            step->keyId = 0;
            return;
        }
        if (token->token == "..")
        {
            if (token->next && (token->next->symbol != '/' && token->next->symbol != '~'))
            {
                throwXPathException("Invalid token after '..' : symbol='%c'/token='%s'\n", token->next->symbol,
                                    token->next->token.c_str());
            }
            step->action = XPathAxis_Parent;
            step->keyId = 0;
            return;
        }
        char* axis = strdup(token->token.c_str());
        char* postAxis = strstr(axis, "::");
        if (postAxis)
        {
            postAxis[0] = '\0';
            postAxis += 2;
        }
        else
        {
            free(axis);
            axis = NULL;
            postAxis = (char*) token->token.c_str();
        }
        if (axis)
        {
            bool found = false;
#define __XPath_Axis(__name,__id) \
        if ( ! found && strcmp(__name, axis) == 0 )	\
          { found = true; step->action = XPathAxis_##__id; }
#include <Xemeiah/xpath/xpath-axis.h>
#undef __XPath_Axis
            if (!found)
            {
                throwXPathException("Invalid axis : '%s'\n", axis);
            }
        }
        else
        {
            step->action = XPathAxis_Child;
        }

        if (postAxis[0] == '*')
        {
            AssertInvalid(postAxis[1] == '\0', "Invalid postAxis : '%s'\n", postAxis);
            if (step->action == XPathAxis_Attribute)
                step->keyId = 0;
            else
                step->keyId = parsingFromKeyCache->getBuiltinKeys().xemint.element();
        }
        else if (strchr(postAxis, '*'))
        {
            char* nsKey = strdup(postAxis);
            char* sep = strchr(nsKey, ':');
            AssertInvalid(sep, "Invalid keyName : '%s'\n", nsKey);
            AssertInvalid(sep[1] == '*', "Invalid keyName : '%s'\n", nsKey);
            AssertInvalid(sep[2] == '\0', "Invalid keyName : '%s'\n", nsKey);
            /**
             * \todo remove the fake localKey here !
             */
            sep[1] = 'x';
            step->keyId = getKeyId(nsKey);
            free(nsKey);
            step->keyId = KeyCache::getKeyId(KeyCache::getNamespaceId(step->keyId), 0);
            step->flags |= XPathStepFlags_NodeTest_Namespace_Only;
        }
        else
            step->keyId = getKeyId(postAxis);
        if (step->action == XPathAxis_Namespace)
        {
            if (step->keyId == parsingFromKeyCache->getBuiltinKeys().xemint.element())
            {
                step->keyId = 0;
            }
            if (KeyCache::getNamespaceId(step->keyId))
            {
                NotImplemented("Namespace axis handling : keyId has a namespace defined : '%s'\n",
                               token->token.c_str());
            }
            if (step->keyId)
            {
                KeyId xmlNSKeyId = parsingFromKeyCache->getBuiltinKeys().xmlns.ns();
                step->keyId = KeyCache::getKeyId(xmlNSKeyId, step->keyId);
            }
            Log_XPathParser ( "Namespace axis handling : final keyId is 0x%x\n", step->keyId );
        }
        if (axis)
            free(axis);
    }

    void
    XPathParser::nextTokenToStep (XPathStep* step, Token* token, Token* next)
    {
        if (!next)
            return;
        if (!next->isSymbol())
        {
            Invalid("Invalid next : '%c/%s'\n", next->symbol, next->token.c_str());
        }
        Log_XPathParser ( "nextTokenToStep : step=%p, token=%p(%c/%s), next=%p(%c/%s)\n",
                step, token, token->symbol, token->token.c_str(),
                next, next->symbol, next->token.c_str() );
        if (token->isSymbol() && token->symbol == '[')
        {
            Log_XPathParser ( "nextTokenToStep : skipping next token as I am on a predicate token !\n" );
            return;
        }

        switch (next->symbol)
        {
            case '/':
                AssertInvalid(next->next, "Next has no next expression.\n")
                ;
                step->nextStep = tokenToStep(next->next);
                break;
            case '[':
            {
                AssertInvalid(token->symbol != '(', "Invalid '(' after a '['\n");
                Token* nextPredicate = next;
                int predicateId = 0;
                while (nextPredicate)
                {
                    if (!nextPredicate->isSymbol())
                    {
                        Invalid("Invalid next Predicate non-symbol : '%s'\n", nextPredicate->token.c_str());
                    }

                    if (nextPredicate->symbol == '[')
                    {
                        AssertInvalid(nextPredicate->subExpression, "Next has no subexpression.\n");
#ifdef __XEM_XPATHPARSER_OPTIMIZE_CHILDLOOKUP_PREDICATE
#define __XPATHPARSER_IS_EQUALS_ATTRIBUTE_VARIABLE(__token) \
       ( (__token)->symbol == '=' \
      && (__token)->subExpression->symbol == '@' \
      && (__token)->subExpression2->symbol == '$' \
      && !((__token)->subExpression->next) \
      && !((__token)->subExpression2->next) )

                        if (predicateId == 0 && step->action == XPathAxis_Child && KeyCache::getLocalKeyId(step->keyId)
                                && (!nextPredicate->next || nextPredicate->next->symbol != '[')
                                && ( __XPATHPARSER_IS_EQUALS_ATTRIBUTE_VARIABLE(nextPredicate->subExpression)
                                        || (nextPredicate->subExpression->symbol == 'B'
                                                && nextPredicate->subExpression->token == "and"
                                                && ( __XPATHPARSER_IS_EQUALS_ATTRIBUTE_VARIABLE(
                                                        nextPredicate->subExpression->subExpression)
                                                        || (nextPredicate->subExpression->subExpression->symbol == 'B'
                                                                && nextPredicate->subExpression->subExpression->token
                                                                        == "and"
                                                                && __XPATHPARSER_IS_EQUALS_ATTRIBUTE_VARIABLE(
                                                                        nextPredicate->subExpression->subExpression->subExpression))))

                                ))
                        {
                            Token* equalsToken, *attributeToken, *variableToken, *postPredicateToken = NULL;

                            Log_XPathParser_Optim ( "[OPTIM] At predicateId=0 axis[@=$], token='%c/%s' of expr='%s'\n",
                                    token->symbol, token->token.c_str(), getExpression() );

                            if (nextPredicate->subExpression->symbol == '=')
                            {
                                equalsToken = nextPredicate->subExpression;
                            }
                            else if (nextPredicate->subExpression->symbol == 'B'
                                    && nextPredicate->subExpression->token == "and")
                            {
                                Token* andToken = nextPredicate->subExpression;
                                equalsToken = andToken->subExpression;
                                if (equalsToken->symbol != '=')
                                {
                                    Token* andToken2 = andToken->subExpression;
                                    equalsToken = andToken2->subExpression;
                                    if (equalsToken->symbol != '=')
                                    {
                                        Bug(".");
                                    }
                                    andToken2->subExpression = andToken2->subExpression2;
                                    andToken2->subExpression2 = andToken->subExpression2;
                                    postPredicateToken = andToken2;
                                    // We have and(and(@=$,misc1),misc2)
                                    // postPredicateToken shall be and(misc1, misc2)
                                }
                                else
                                {
                                    // We have a (simple) format : and(@=$,misc), so set misc as
                                    postPredicateToken = nextPredicate->subExpression->subExpression2;
                                }
                            }
                            else
                            {
                                equalsToken = NULL;
                                Bug("Wrong predicate form !\n");
                            }

                            attributeToken = equalsToken->subExpression;
                            variableToken = equalsToken->subExpression2;

                            step->action = XPathFunc_ChildLookup;
                            step->functionArguments[0] = allocStep();
                            XPathStep* eltStep = getStep(step->functionArguments[0]);
                            eltStep->action = XPathAxis_ConstInteger;
                            eltStep->constInteger = step->keyId;

                            step->functionArguments[1] = tokenToStep(attributeToken);
                            step->functionArguments[2] = tokenToStep(variableToken);

                            char synthetizedKey[256];
                            sprintf(synthetizedKey, "__synth_childLookup_%x_%x", step->keyId,
                                    getStep(step->functionArguments[1])->keyId);
                            KeyId synthetizedKeyId = getKeyId(synthetizedKey);
                            step->functionArguments[3] = allocStep();
                            XPathStep* synthStep = getStep(step->functionArguments[3]);
                            synthStep->action = XPathAxis_ConstInteger;
                            synthStep->constInteger = synthetizedKeyId;

                            Log_XPathParser_Optim ( "Optimize predicateId=0 axis[@=$], token='%s' of expr='%s'\n", token->token.c_str(), getExpression() );
                            Log_XPathParser_Optim ( "\t args[0]=%x/%llx, [1]=%x/%x, [2]=%x/%x\n",
                                    eltStep->action, eltStep->constInteger,
                                    getStep(step->functionArguments[1])->action, getStep(step->functionArguments[1])->keyId,
                                    getStep(step->functionArguments[1])->action, getStep(step->functionArguments[2])->keyId );
                            Log_XPathParser_Optim ( "\tSynthetized : %s => %x\n", synthetizedKey, synthetizedKeyId );
                            Log_XPathParser_Optim ( "\tFrom element : %s\n",
                                    parsingFromElementRef ? parsingFromElementRef->generateVersatileXPath().c_str() : "(unknown)" );

                            if (postPredicateToken)
                            {
                                step->predicates[0] = tokenToStep(postPredicateToken);
                            }
                            if (nextPredicate->next)
                            {
                                nextTokenToStep(step, token, nextPredicate->next);
                            }
                            return;
                        }
#endif // __XEM_XPATHPARSER_OPTIMIZE_CHILDLOOKUP_PREDICATE                  
                        step->predicates[predicateId] = tokenToStep(nextPredicate->subExpression);
                        predicateId++;
                        if (predicateId == XPathStep_MaxPredicates)
                            throwXPathException("Maximum number of predicates reached !\n");
                    }
                    else
                    {
                        nextTokenToStep(step, token, nextPredicate);
                        break;
                    }
                    nextPredicate = nextPredicate->next;
                }
                tagStepPredicatesFlags(step);
                break;
            }
            case '~': // The '//' descending operator
            {
                step->nextStep = tokenToStep(next);
                break;
            }
            case '#': // The '->' operator
            {
                step->nextStep = tokenToStep(next);
                break;
            }
            default:
                Invalid("Invalid symbol for next : '%c'\n", next->symbol);
        }
    }

    void
    XPathParser::binaryTokenToStep (XPathStep* step, Token* token)
    {
        step->action = XPathAction_NULL;
        switch (token->symbol)
        {
            case '+':
                step->action = XPathFunc_Plus;
                break;
            case '*':
                step->action = XPathFunc_Multiply;
                break;
            case '=':
            {
#ifdef __XEM_XPATHPARSER_OPTIMIZE_LOCALNAME_EQUAL
                if (token->subExpression->isQName() && token->subExpression->token == "local-name"
                        && token->subExpression2->isText())
                {
                    /*
                     * Optimize expressions of the form [local-name() = 'value']
                     */
                    Token* funcToken = token->subExpression->next;
                    if (funcToken && funcToken->isSymbol() && !funcToken->subExpression)
                    {
                        // token->log ();
                        Log_XPathParser_Optim ( "Optimize, subExpr '%c/%s' : '%s'\n",
                                funcToken ? funcToken->symbol : '_',
                                funcToken ? funcToken->token.c_str() : "(none)",
                                getExpression() );
                        KeyId parsedKeyId = getKeyId(token->subExpression2->token);
                        Log_XPathParser_Optim ( "Parsed : '%s' => %x\n", token->subExpression2->token.c_str(), parsedKeyId );
                        step->flags |= XPathStepFlags_NodeTest_LocalName_Only;
                        step->action = XPathAxis_Self;
                        step->keyId = parsedKeyId;
                        return;
                    }
                    else
                    {
                        // Warn ( "Could not optimize : '%s' \n", getExpression() );
                        // token->log ();
                    }
                }
#endif
                step->action = XPathComparator_Equals;

                break;
            }
            case '!':
                step->action = XPathComparator_NotEquals;
                break;
            case '|':
                step->action = XPathFunc_Union;
                break;
            case 'B':
                if (token->token == "div")
                    step->action = XPathFunc_Div;
                else if (token->token == "mod")
                    step->action = XPathFunc_Modulo;
                else if (token->token == "and")
                    step->action = XPathFunc_BooleanAnd;
                else if (token->token == "or")
                    step->action = XPathFunc_BooleanOr;
                else
                {
                    Invalid("Invalid token '%s'\n", token->token.c_str());
                }
                break;
            case '-':
                step->action = XPathFunc_Minus;
                break;
            case '<':
                if (token->token == "<=")
                    step->action = XPathComparator_LessThanOrEquals;
                else
                    step->action = XPathComparator_LessThan;
                break;
            case '>':
                if (token->token == ">=")
                    step->action = XPathComparator_GreaterThanOrEquals;
                else
                    step->action = XPathComparator_GreaterThan;
                break;
            default:
                NotImplemented("Binary Operator '%c'\n", token->symbol);
        }
        AssertInvalid(token->subExpression, "No subExpression..\n");
        AssertInvalid(token->subExpression2, "No subExpression2..\n");
        step->functionArguments[0] = tokenToStep(token->subExpression);
        step->functionArguments[1] = tokenToStep(token->subExpression2);
        step->functionArguments[2] = XPathStepId_NULL;
    }

    void
    XPathParser::checkPositionnalExpression (XPathStep* step, XPathStepFlags& flags)
    {
        Log_XPathParser_Tag ( "At step action=%x\n", step->action );
        if (step->action == XPathFunc_Position)
        {
            Log_XPathParser_Tag ( "Found position !\n" );
            flags |= XPathStepFlags_PredicateHasPosition;
            return;
        }
        if (step->action == XPathFunc_Last)
        {
            flags |= XPathStepFlags_PredicateHasPosition;
            flags |= XPathStepFlags_PredicateHasLast;
            return;
        }
        for (int predIndex = 0; predIndex < XPathStep_MaxPredicates; predIndex++)
        {
            if (step->predicates[predIndex] == XPathStepId_NULL)
                break;
            Log_XPathParser_Tag ( "At pred=%x, step->pred[]=%x\n", predIndex, step->predicates[predIndex] );
            XPathStep* pred = getStep(step->predicates[predIndex]);
            checkPositionnalExpression(pred, flags);
        }
        if ( __XPathAction_isFunction(step->action) || __XPathAction_isComparator(step->action))
        {
            for (int argIndex = 0; argIndex < XPathStep_MaxFuncCallArgs; argIndex++)
            {
                if (step->functionArguments[argIndex] == XPathStepId_NULL)
                    break;
                Log_XPathParser_Tag ( "At arg=%x -> %x\n", argIndex, step->functionArguments[argIndex] );
                XPathStep* argStep = getStep(step->functionArguments[argIndex]);
                checkPositionnalExpression(argStep, flags);
            }
        }
        if (step->nextStep != XPathStepId_NULL)
        {
            Log_XPathParser_Tag ( "Next step : %x\n", step->nextStep );
            XPathStep* nextStep = getStep(step->nextStep);
            checkPositionnalExpression(nextStep, flags);
        }
    }

    void
    XPathParser::tagStepPredicatesFlags (XPathStep* step)
    {
        Log_XPathParser_Tag ( "Expression : '%s'\n", expression );

        for (int predIndex = 0; predIndex < XPathStep_MaxPredicates; predIndex++)
        {
            if (step->predicates[predIndex] == XPathStepId_NULL)
                break;
            XPathStep* pred = getStep(step->predicates[predIndex]);
            switch (pred->action)
            {
                case XPathAxis_Variable:
                case XPathAxis_ConstInteger:
                case XPathAxis_ConstNumber:
                    step->flags |= XPathStepFlags_PredicateHasPosition;
                    break;
                default:
                    Log_XPathParser_Tag ( "Predicate %x->%x : setting positionnal ?\n", predIndex, step->predicates[predIndex] );
                    checkPositionnalExpression(pred, step->flags);
                    /*
                     * Enforce position flag if the function is a math function (match/match15.xsl)
                     */
                    if (__XPathAction_isNumberFunction(pred->action))
                    {
                        step->flags |= XPathStepFlags_PredicateHasPosition;
                    }
                    break;
            }

        }
        Log_XPathParser_Tag ( "Setting positionnal for action=%x, flags=%x\n", step->action, step->flags );
    }

    void
    XPathParser::computeLastSteps ()
    {
        for (XPathStepId stepId = 0; stepId < parsedSteps.size(); stepId++)
        {
            XPathStep* step = getStep(stepId);
            if (step->nextStep != XPathStepId_NULL)
            {
                Log_XPathParser ( "Step=%p, stepId=%u, next=%u\n", step, stepId, step->nextStep );
                XPathStep* next = getStep(step->nextStep);
                AssertBug(next->lastStep == XPathStepId_NULL, "Last step already set !\n");
                if (step->keyId && !KeyCache::getLocalKeyId(step->keyId))
                {
                    if (!(step->flags & XPathStepFlags_NodeTest_Namespace_Only))
                    {
                        Bug(".");
                    }
                }
                if (step->action == XPathAxis_Root
                        && (next->action == XPathAxis_Descendant || next->action == XPathAxis_Descendant_Or_Self))
                {
                    Log_XPathParser ( "Skipping // from root !\n" );
                }
                else
                {
                    next->lastStep = stepId;
                }
            }
        }
    }
}
;

