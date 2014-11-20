#include <Xemeiah/xpath/xpath.h>
#include <Xemeiah/xprocessor/xprocessor.h>
#include <Xemeiah/trace.h>

#if 0
#define __XEM_XPATH_EVAL_TIME
#define __XEM_XPATH_EVAL_TIME_THRESHOLD 10000UL
#endif
#if 0
#define __XEM_XPATH_EVAL_STEP_TIME
#endif

#define Log_EvalHPP Debug

namespace Xem
{
  __INLINE NodeRef& XPath::getBaseNode ()
  {
    return xproc.getXPathBaseNode();
  }

  __INLINE void XPath::eval ( NodeSet& result )
  {
    Log_EvalHPP ( "XPath %p : seg=%p, stepBlock=%p, res=%p\n",
        this, xpathSegment, stepBlock, resourceBlock );
    if ( xpathSegment->firstStep == XPathStepId_NULL )
      {
        /**
         * The XPath is empty. Return now !
         */
        return;
      }
    try
      {
        xproc.setXPathBaseNode ();
        
#ifdef __XEM_XPATH_EVAL_TIME
        NTime begin = getntime ();
#endif // __XEM_XPATH_EVAL_TIME

        evalStep ( result, xproc.getCurrentNode(), xpathSegment->firstStep );
        
#ifdef __XEM_XPATH_EVAL_TIME
        NTime end = getntime ();
        unsigned long timespent = diffntime ( &(begin.tp_cpu), &(end.tp_cpu));
        if ( timespent >= __XEM_XPATH_EVAL_TIME_THRESHOLD )
          {
            fprintf ( stderr, "[XPATHTIME] T%12lu|%s\n", timespent, getExpression() );
            logXPath ();
          }
#endif // __XEM_XPATH_EVAL_TIME
      }
    catch ( Exception *e )
      {
        detailException ( e, "At XPath '%s' - current node : %s\n", getExpression(), xproc.getCurrentNode().generateVersatileXPath().c_str() );
        throw ( e );
      }
#if PARANOID
    result.checkDocumentOrderness ();
#endif
#if 1
    Log_EvalHPP ( "---------------- XPath Result : --------------------\n" );
    result.log ();
#endif
  }

  __INLINE String XPath::evalString ( )
  {
    NodeSet result;
    eval ( result );
    return result.toString ();
  }

  __INLINE Number XPath::evalNumber ( )
  {
    NodeSet result;
    eval ( result );
    return result.toNumber ();
  }

  __INLINE bool XPath::evalBool ( )
  {
    NodeSet result;
    eval ( result );
    return result.toBool ();
  }

  __INLINE NodeRef& XPath::evalNode ( )
  {
    Log_EvalHPP ( "XPath %p : seg=%p, stepBlock=%p, res=%p\n",
        this, xpathSegment, stepBlock, resourceBlock );

    NodeSet result;
    eval ( result );

    if ( result.size() == 0 || result.size() > 1 || !result.front().isNode() )
      {
        throwXPathException("XPath result is not a single node !\n");
      }

    try
      {
        return result.toNode();
      }
    catch ( Exception *e )
      {
        detailException ( e, "At XPath '%s'\n", getExpression() );
        throw ( e );
      }
    return *((NodeRef*) NULL);
  }

  __INLINE ElementRef XPath::evalElement ( )
  {
    Log_EvalHPP ( "XPath %p : seg=%p, stepBlock=%p, res=%p\n",
        this, xpathSegment, stepBlock, resourceBlock );

    NodeSet result;
    eval ( result );

    if ( result.size() == 0 )
      {
        Warn ( "Empty NodeSet for expression=%s ! returning empty element !\n", expression);
        return ElementRef(getXProcessor().getCurrentNode().getDocument());
      }

    try
      {
        return result.toElement ();
      }
    catch ( Exception *e )
      {
        detailException ( e, "At XPath '%s'\n", getExpression() );
        throw ( e );
      }
    return ElementRefNull;
  }

  __INLINE AttributeRef XPath::evalAttribute ()
  {
    NodeSet result;
    eval (result);
    return result.toAttribute();
  }

  /**
   * Eval with a different NodeRef than the one provided
   */
  __INLINE void XPath::eval ( NodeSet& result, NodeRef& node )
  {
    NodeSet temporaryNodeSet;
    temporaryNodeSet.pushBack ( node );
    
    for ( NodeSet::iterator iter(temporaryNodeSet,xproc) ; iter ; iter++ )
      eval ( result );
  }

  __INLINE String XPath::evalString ( const NodeRef& node )
  {
    NodeSet temporaryNodeSet;
    temporaryNodeSet.pushBack ( node, false );
    
    for ( NodeSet::iterator iter(temporaryNodeSet,xproc) ; iter ; iter++ )
      return evalString ( );
    return "(Invalid nodeset ?)";
  }  

  __INLINE Number XPath::evalNumber ( NodeRef& node )
  {
#if 0
    NodeSet temporaryNodeSet;
    temporaryNodeSet.pushBack ( node );
    
    for ( NodeSet::iterator iter(temporaryNodeSet,xproc) ; iter ; iter++ )
      return evalNumber ( );
    return 0;
#endif
    NodeSet result;
    evalStep ( result, node, xpathSegment->firstStep );
    return result.toNumber();
  }

  __INLINE ElementRef XPath::evalElement ( NodeRef& node )
  {
    NodeSet temporaryNodeSet;
    temporaryNodeSet.pushBack ( node );
    
    for ( NodeSet::iterator iter(temporaryNodeSet,xproc) ; iter ; iter++ )
      return evalElement ( );
    return ElementRef(node.getDocument());
  }

  /**
   * Internal evalStep()
   */

  __INLINE void XPath::evalStep ( NodeSet& result, 
        NodeRef& node, XPathStepId stepId )
  {
    if ( stepId == XPathStepId_NULL ) 
      {
#ifdef LOG
        if ( node.isElement() )
          {
            Log_EvalHPP ( "[TERMINAL] : Add element %llx:'%s'\n",
                node.toElement().getElementId(), node.toElement().getKey().c_str() );
          }
        else if ( node.isAttribute() )
          {
            Log_EvalHPP ( "[TERMINAL] : Add attribute %llx:'%s'/'%s', value='%s'\n",
                node.toAttribute().getElement().getElementId(),
                node.toAttribute().getElement().getKey().c_str(),
                node.toAttribute().getKey().c_str(),
                node.toAttribute().toString().c_str() );
          }
#endif
        result.pushBack ( node, true );
        return;
     }

    XPathStep* step = getStep(stepId);

    Functor action = actionMap[step->action];
#if PARANOID
    AssertBug ( action, "Axis not implemented '%u'\n", step->action );
#endif
    Log_EvalHPP ( "*** evalStep : stepId=%u, axis=%u\n", stepId, step->action );
#ifdef __XEM_XPATH_EVAL_DETAIL_EXCEPTION_STEPS
    try
      {
#endif
#ifdef __XEM_XPATH_EVAL_STEP_TIME
        fprintf ( stderr, "[XPATHTIME] T% 12lu,S->%04u,A%04x|%s\n", 0UL, stepId, step->action, expression );
        NTime begin = getntime ();
#endif // __XEM_XPATH_EVAL_STEP_TIME

        (this->*action) ( result, node, step );

#ifdef __XEM_XPATH_EVAL_STEP_TIME
        NTime end = getntime ();
        unsigned long timespent = diffntime ( &(begin.tp_cpu), &(end.tp_cpu));
        fprintf ( stderr, "[XPATHTIME] T% 12lu,S<-%04u,A%04x|%s\n", timespent, stepId, step->action, expression );
#endif // __XEM_XPATH_EVAL_STEP_TIME



#ifdef __XEM_XPATH_EVAL_DETAIL_EXCEPTION_STEPS
      }
    catch ( Exception * e )
      {
        detailException ( e, "While processing : stepId=%u\n", stepId );
        throw (e);
      }
#endif
  }


  __INLINE KeyId XPath::evalStepAsKeyId ( NodeRef& node, XPathStepId stepId )
  {
    if ( stepId == XPathStepId_NULL )
      {
        throwXPathException ( "Invalid null step for KeyId step !\n" );
      }
    XPathStep* step = getStep ( stepId );
    if ( step->action == XPathAxis_ConstInteger )
      {
        Integer constInt = step->constInteger;
        AssertBug ( constInt, "Invalid QName keyId for name key : 0x%llx\n", constInt );
        AssertBug ( constInt < (1LL << 32), "Invalid QName keyId for name key : 0x%llx\n", constInt ); 
        KeyId keyNameId = (KeyId) constInt;
        return keyNameId;  
      }

    NodeSet resultNodeSet;
    evalStep ( resultNodeSet, node, stepId );
    String res0 = resultNodeSet.toString();

    if ( strchr ( res0.c_str(), ':' ) )
      {
        throwXPathException ( "NotImplemented : evalStepAsKeyId('%s') : string is a fully-qualified name !\n", res0.c_str() );
        NotImplemented ( "evalStepAsKeyId('%s') : string is a fully-qualified name !\n", res0.c_str() );
      }
    KeyId keyNameId = node.getKeyCache().getKeyId ( 0, res0.c_str(), false );
    if ( ! keyNameId )
      {
        throwXPathException ( "Invalid QName '%s'\n", res0.c_str() );
      }
    return keyNameId;
  }
  
  INLINE bool XPath::isSingleVariableReference ( KeyId varKeyId )
  {
    if ( xpathSegment->firstStep == XPathStepId_NULL )
      return false;
    XPathStep* step = getStep(xpathSegment->firstStep);
    return ( step->action == XPathAxis_Variable
          && step->nextStep == XPathStepId_NULL
          && step->predicates[0] == XPathStepId_NULL
          && step->keyId == varKeyId );
  }
};
