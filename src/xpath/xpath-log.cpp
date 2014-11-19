#include <Xemeiah/xpath/xpath.h>
#include <Xemeiah/trace.h>

#include <Xemeiah/auto-inline.hpp>

#define __XEM_XPATH_LOG
#ifdef __XEM_XPATH_LOG
#define Dump_XPath(...) fprintf(stderr,__VA_ARGS__);
#else
#define Dump_XPath(...)
#endif

namespace Xem
{
  void XPath::logXPath ()
  {
    if ( ! xpathSegment )
      {
	      Error ( "Empty XPath Segment !\n" );
	      return;
      }
    Dump_XPath ( "XPathSegment (at %p) :\n", xpathSegment );
    Dump_XPath ( "\t%u steps, firstStep=%u\n",
        xpathSegment->nbStepId, xpathSegment->firstStep );
    for ( XPathStepId stepId = 0 ; stepId < xpathSegment->nbStepId ; stepId ++ )
      {
	      logStep ( stepId );
      }
#if 0
    for ( XPathResourceId resourceId = 0 ; resourceId < xpathSegment->nbResourceId ; resourceId ++ )
      {
	      logResource ( resourceId );
      }
#endif
  }

  void XPath::logStep ( XPathStepId stepId )
  {
   
    XPathStep* step = getStep ( stepId );
    const char* actionName = NULL;
    if ( 0 ) {}
#define __XPath_Axis(__name,__id) \
    else if ( XPathAxis_##__id == step->action ) actionName = __name
#include <Xemeiah/xpath/xpath-axis.h>
#undef __XPath_Axis

#define __XPath_Func(__name,__id,__card,__defaults) \
    else if ( XPathFunc_##__id == step->action ) actionName = __name
#include <Xemeiah/xpath/xpath-funcs.h>
#undef __XPath_Func
    else if ( step->action == XPathComparator_Equals ) actionName = "=";
    else if ( step->action == XPathComparator_NotEquals ) actionName = "!=";
    else if ( step->action == XPathComparator_LessThan ) actionName = "<";
    else if ( step->action == XPathComparator_LessThanOrEquals ) actionName = "<=";
    else if ( step->action == XPathComparator_GreaterThan ) actionName = ">";
    else if ( step->action == XPathComparator_GreaterThanOrEquals ) actionName = ">=";
    else
      {
        Bug ( "Invalid action %u\n", step->action );
      }
      
    Dump_XPath ( "Step=%x action=%x:'%s', ", stepId, step->action, actionName );

    if ( step->nextStep != XPathStepId_NULL )
      {
        Dump_XPath ( "next=%x, ", step->nextStep );
      }
    if ( step->predicates[0] != XPathStepId_NULL )
      {
        Dump_XPath ( "pred" );
        for ( int predIndex = 0 ; predIndex < XPathStep_MaxPredicates ; predIndex++ )
          {
            if ( step->predicates[predIndex] == XPathStepId_NULL ) break;
            Dump_XPath ( "%c%x", predIndex ? '.' : '=', step->predicates[predIndex] );
          }
        Dump_XPath ( ", " );
      }
    if ( step->action == XPathFunc_XSLNumbering_SingleCharacterConverter )
      {
        Dump_XPath ( "preToken=%u, ", step->xslNumberingFormat.preToken );
        Dump_XPath ( "single char=%c\n", step->xslNumberingFormat.singleCharacter );
        return;
      }

    if ( step->action == XPathFunc_XSLNumbering_IntegerConverter )
      {
        Dump_XPath ( "preToken=%u, ", step->xslNumberingFormat.preToken );
        Dump_XPath ( "precision=%u, grouping : sep=%c, size=%u\n", 
            step->xslNumberingFormat.integerConversion.precision,
            step->xslNumberingFormat.integerConversion.groupingSeparator,
            step->xslNumberingFormat.integerConversion.groupingSize );    
        return;
      }

//    if ( __XPath_IsFunction ( step->action ) )
    if ( __XPathAction_isFunction(step->action) || __XPathAction_isComparator(step->action) )
      {
        Dump_XPath ( "args" );
        if ( step->functionArguments[0] == XPathStepId_NULL )
          {
            Dump_XPath ( "=(none)" );
          }
        for ( int arg = 0 ; arg < XPathStep_MaxFuncCallArgs ; arg++ )
          {
            if ( step->functionArguments[arg] == XPathStepId_NULL )
              break;
            Dump_XPath ( "%c%x", arg ? ':' : '=', step->functionArguments[arg] );
          }
        Dump_XPath ( "\n" );
        return;
      }
    
    switch ( step->action )
      {
      case XPathAxis_Resource:
        Dump_XPath ( "resourceId=%x, resource='%s'\n",
      		step->resource, getResource(step->resource) );
      	break;
      case XPathAxis_ConstInteger:
        Dump_XPath ( "Integer=%lld\n", step->constInteger );
      	break;
      case XPathAxis_ConstNumber:
        Dump_XPath ( "Number=%g\n", step->constNumber );
        break;
      default:
        Dump_XPath ( "keyId=0x%x\n", step->keyId );
      }
  }
};
