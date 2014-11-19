#include <Xemeiah/xpath/xpathparser.h>
#include <Xemeiah/trace.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_XPathParser_AVT Debug

#define throwNotImplemented(...) throwXPathException ( "Not Implemented : " __VA_ARGS__ )

namespace Xem
{
  XPathStepId XPathParser::parseAVT ( const char* __expr )
  {
    char* expr = strdup ( __expr );
    XPathStepId stepId = XPathStepId_NULL;
    try
      {
        stepId = doParseAVT ( expr );
      }
    catch ( Exception* e )
      {
        free ( expr );
        throw ( e );
      }
    free ( expr );
    return stepId;
  }
  
  XPathStepId XPathParser::doParseAVT ( char* expr )
  {
    if ( strncmp ( expr, "{f", 2 ) == 0 ) Bug ( "." );
    Log_XPathParser_AVT ( "Parsing AVT !\n" );
    Log_XPathParser_AVT ( "Expr '%s'\n", expr );
    AssertBug ( expr && *expr, "Empty expr !\n" );

    XPathStepId stepList[256];
    int stepListIndex = 0;
#define appendStep(__stepId) stepList[stepListIndex++] = __stepId

    /*
     * Embdedded XPath parsing logic
     * General XPath embedded parsing format is as follows :
     * [<text>* '{' <xpath expression> '}' ]+
     * text may contain '{{' and '}}' sequences, which must be converted to '{' and '}', respectively.
     * xpath expressions may contain matching '{' and '}', which must be provided as-is to the XPath expression parser (doParse())
     * 
     * Important note for XPath indirections :
     * xpath expressions in Xem are extended to handle indirections, id est sub-xpath expression fetching.
     * For example, "element/{@xpath-query}" will use the @xpath-query attribute as an XPath expression to continue
     * evaluation based on the 'element' Element.
     * Xem handles recursive XPath indirections.
     *
     * For embedded XPath expressions, XPath indirections can not start by two '{', because they will be converted to a single '{' and
     * handled as text.
     * As a result, '{{@id}}' will be converted to '{@id}', whereas '{ {@id}}' will be handled as a two-level XPath indirection.
     *
     * XPath parsing is performed in two distinct steps :
     * -# First, the expression is linearly parsed, building a list of XPath steps from textual resources and XPath expressions
     *    These steps are stored orderly in stepList.
     * -# Then, the global XPath expression is built, concatenating all the steps parsed.
     */
    
    int inExpressionDepth = 0; //!< 0 if we are outside of an expression, count number of unclosed '{' otherwise.
    char* segmentHead = expr;
    char* segmentTail = expr;
    
#define __copyChar() \
    if ( current == segmentTail ) { segmentTail++; } \
    else { *segmentTail = *current; segmentTail++; }
#define __endSegment() { *segmentTail = '\0'; }

    for ( char* current = expr ; *current ; current++ )
      {
        Log_XPathParser_AVT ( "Expr=%p / current=%p %c, inExprDepth=%d\n",
            expr, current, *current, inExpressionDepth );
        switch ( *current )
        {
        case '{':
          if ( current[1] == '{' )
            {
              Log_XPathParser_AVT ( "Reduced '{{' to '{' in text.\n" );
              __copyChar();
              current++;
              continue;            
            }
          if ( inExpressionDepth )
            {
              inExpressionDepth++; continue;
            }
          __endSegment();
          if ( *segmentHead )
            {
              appendStep ( allocStepResource ( segmentHead ) );
            }
          inExpressionDepth ++;
          segmentHead = &(current[1]);
          segmentTail = segmentHead;
          break;
        case '}':
          if ( inExpressionDepth )
            {
              inExpressionDepth--;
              if ( ! inExpressionDepth )
                {
                  __endSegment ();
                  appendStep ( doParse ( segmentHead ) );
                  segmentHead = segmentTail = &(current[1]);
                }
              else
                {
                  __copyChar ();
                }
              continue;
            }
          if ( current[1] == '}' )
            {
              Log_XPathParser_AVT ( "Reduced '}}' to '}' in text.\n" );
              __copyChar ();
              current++;
              continue;
            }
          throwXPathException ( "Invalid out-of-expression single closing curly brace : next character=%c.\n", current[1] );
        default:
          __copyChar();
        }
      }
    if ( inExpressionDepth )
      {
        throwXPathException ( "Unbalanced curly braces in XPath AVT expression.\n" );
      }

    Log_XPathParser_AVT ( "Post parse expr=%p : seg head=%p %c, tail=%p %c\n", expr, segmentHead, *segmentHead, segmentTail, *segmentTail );
    __endSegment();
    if ( *segmentHead )
      {
        appendStep ( allocStepResource ( segmentHead ) );
      }
      
    if ( stepListIndex == 0 )
      {
        throwXPathException ( "Empty Step List !\n" );
      }
    if ( stepListIndex == 1 )
      {
        return stepList[0];
      }

    XPathStepId headStepId = allocStep ();
    XPathStep* currentStep = getStep ( headStepId );
    currentStep->action = XPathFunc_Concat;
    for ( int ustep = 0 ; ustep < XPathStep_MaxFuncCallArgs ; ustep++ )
      currentStep->functionArguments[ustep] = XPathStepId_NULL;
    int currentArg = 0;
    for ( int step = 0 ; step < stepListIndex ; step++ )
      {
        if ( ( currentArg == XPathStep_MaxFuncCallArgs-1 )
             && ( step < (stepListIndex-1) ) )
          {
            XPathStepId nextStepId = allocStep ();
            currentStep->functionArguments[currentArg] = nextStepId;
            XPathStep* currentStep = getStep ( nextStepId );
            currentStep->action = XPathFunc_Concat;
            for ( int ustep = 0 ; ustep < XPathStep_MaxFuncCallArgs ; ustep++ )
              currentStep->functionArguments[ustep] = XPathStepId_NULL;
            currentArg = 0;
          }
        currentStep->functionArguments[currentArg] = stepList[step];
        currentArg++;
      }
    return headStepId;
  }

  XPathStepId XPathParser::concatStep ( XPathStepId arg1, XPathStepId arg2 )
  {
    XPathStepId newStepId = allocStep ();
    XPathStep* newStep = getStep(newStepId);
    newStep->action = XPathFunc_Concat;
    newStep->functionArguments[0] = arg1;
    newStep->functionArguments[1] = arg2;
    newStep->functionArguments[2] = XPathStepId_NULL;
    return newStepId;
  }
};
