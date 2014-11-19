#include <Xemeiah/xpath/xpath.h>
#include <Xemeiah/xprocessor/xprocessor.h>
#include <Xemeiah/trace.h>
#include <Xemeiah/dom/childiterator.h>

#include <Xemeiah/auto-inline.hpp>

#include <math.h>

/*
 * XPath Matching algorithm.
 * TODO : reimplement XPath::matches() this without a base, with a reverse algorithm :
 * Start from the last step of the main step list, and simulate a reverse xpath.
 */

/*
 * When the step to evaluate has no predicate, we do not need to build up the complete context nodeset.
 */
#define __XEM_XPATH_MATCHES_NON_PREDICATE_SHORTCUT  
#define __XEM_XPATH_MATCHES_LAST_PREDICATE_SHORTCUT

#define __XEM_XPATH_MATCHES_USE_PREDICATEHASPOSITION
#define __XEM_XPATH_MATCHES_USE_PREDICATEHASLAST

#if 1
#undef NotImplemented
#define NotImplemented(...) throwXPathException ( "Not Implemented : " __VA_ARGS__)
#endif

#define Log_Matches Debug

namespace Xem
{
  bool XPath::matchesPredicate ( NodeSet::iterator& iter, NodeRef& node, XPathStepId predicateStepId )
  {    
    NodeSet predicateNodeSet;
    evalStep ( predicateNodeSet, iter->toNode(), predicateStepId );
    if ( predicateNodeSet.isInteger() || predicateNodeSet.isNumber() )
      {
        Log_Matches ( "Asked for a positionnal predicate : asked=%lld, pos=%lld\n",
              predicateNodeSet.toInteger(), iter.getPosition() );
        if ( predicateNodeSet.toInteger() != iter.getPosition() )
          {
            return false;
          }
      }
    else if ( ! predicateNodeSet.toBool() )
      {
        Log_Matches ( "Failed a comparison (boolean) predicate !\n" );
        return false;
      }
    Log_Matches ( "This item fits !\n" );
    return true;
  }

  bool XPath::matchesPredicate ( NodeSet& result, 
    NodeRef& previousNode, NodeRef& node, XPathStep* step,
    int predicateId, Number& effectivePriority )
  {
    XPathStepId predicateStepId = step->predicates[predicateId];
    
#ifdef __XEM_XPATH_MATCHES_LAST_PREDICATE_SHORTCUT
    bool lastPredicate = step->predicates[predicateId+1] == XPathStepId_NULL;
#endif // #ifdef __XEM_XPATH_MATCHES_LAST_PREDICATE_SHORTCUT

    bool final = step->lastStep == XPathStepId_NULL;

    
    Log_Matches ( "Matches Predicate : previous='%s', node='%s', "
        "predicateId=%d, predicateStepId=%d, final=%d, lastStep=%u, action=%x\n",
    	  previousNode.getKey().c_str(), node.getKey().c_str(), predicateId,
    	  predicateStepId, final, step->lastStep, step->action );

    if ( predicateStepId == XPathStepId_NULL )
      {
        if ( final ) return true;
        Log_Matches ( "Predicate ends, jumping to previous %u\n", step->lastStep );
        step = getStep ( step->lastStep );
        return matches ( previousNode, step, effectivePriority );      
      }
    
    NodeSet myNodeSet;
    for ( NodeSet::iterator iter(result, xproc) ; iter ; iter++ )
      {
        AssertBug ( iter->isNode(), "Iterator is not a node !\n" );
#ifdef __XEM_XPATH_MATCHES_LAST_PREDICATE_SHORTCUT
        if ( lastPredicate && iter->toNode() != node ) continue;
#endif // __XEM_XPATH_MATCHES_LAST_PREDICATE_SHORTCUT
        if ( matchesPredicate ( iter, node, predicateStepId ) )
          {
            // if ( iter->toNode() == node && lastPredicate )
            //  return true;
            myNodeSet.pushBack ( iter->toNode(), true );
          }
        else
          {
            if ( iter->toNode() == node )
              return false;
          }
      }
    Log_Matches ( "Post-predicate matching stuff, myNodeSet is : \n" );
    myNodeSet.log ();
    return matchesPredicate ( myNodeSet, previousNode, node, step, predicateId + 1, effectivePriority );  
  }

  /*
   * Match stuff
   * This function only validates the local conformance of the node for *this* step.
   */
  bool XPath::matches ( NodeRef& node, XPathStep* step, Number& effectivePriority )
  {
    Log_Matches ( "Matching %s/%x against %x/%x\n", node.getKey().c_str(),
        node.getKeyId(), step->action, step->keyId );
    AssertBug ( step, "Called on null step !\n" );

#define __nodeToElement() \
    if ( ! node.isElement() ) { Log_Matches ( "Node is not an element.\n" ); return false; } \
    ElementRef current = node.toElement ()
#define __nodeToAttribute() \
    if ( ! node.isAttribute() ) { Log_Matches ( "Node is not an attribute.\n" ); return false; } \
    AttributeRef current = node.toAttribute ();
#define __getRootElement() \
    ElementRef rootElement = node.getRootElement()

    bool final = (step->lastStep == XPathStepId_NULL );
    switch ( step->action )
      {
      case XPathAxis_Root:
        {
          __nodeToElement();
          __getRootElement();
          AssertBug ( final, "Root operator in the middle of an getExpression() ?\n" );
          if ( current == rootElement )
            {
              if ( step->nextStep == XPathStepId_NULL )
                effectivePriority = computeEffectivePriority ( step );
              return true;
            }
          return false;
        }
      case XPathAxis_Child:
      case XPathAxis_Descendant:
      case XPathAxis_Descendant_Or_Self:
        {
          __nodeToElement();
          // Log_Matches ( "Matches : Child or descendant axis\n" );
          if ( ! evalNodeKey ( current, step ) )
            {
              Log_Matches ( "Matches (%x) : does not match key (%x)!\n", 
                  current.getKeyId(), step->keyId );
              return false;
            }
          __getRootElement();
          if ( current == rootElement )
            {
              Log_Matches ( "Current is root ! So there's few chances it will match !\n" );
              return false;
            }

          ElementRef father = current.getFather();

#ifdef __XEM_XPATH_MATCHES_NON_PREDICATE_SHORTCUT
          if ( step->predicates[0] == XPathStepId_NULL 
            && step->lastStep == XPathStepId_NULL )
            {
              if ( step->lastStep == XPathStepId_NULL )
                {
                  if ( step->nextStep == XPathStepId_NULL )
                    effectivePriority = computeEffectivePriority ( step );
                  return true;
                }
              if ( step->action == XPathAxis_Child )
                {
                  bool res = matches ( father, getStep(step->lastStep), effectivePriority );
                  if ( res && step->nextStep == XPathStepId_NULL )
                    effectivePriority = computeEffectivePriority ( step );
                  return res;
                }
              for ( ElementRef ancestor = (step->action == XPathAxis_Descendant) ? father : current; 
                ancestor != rootElement ; ancestor = ancestor.getFather() )
                if ( matches ( ancestor, getStep(step->lastStep), effectivePriority ) )
                  {
                    if ( step->nextStep == XPathStepId_NULL )
                      effectivePriority = computeEffectivePriority ( step );
                    return true;
                  }
              return false;
            }
#endif // NON-PREDICATE SHORTCUT


#ifdef __XEM_XPATH_MATCHES_USE_PREDICATEHASPOSITION
#ifdef __XEM_XPATH_MATCHES_USE_PREDICATEHASLAST
          if ( step->flags & XPathStepFlags_PredicateHasPosition 
            && ! (step->flags & XPathStepFlags_PredicateHasLast)
            && step->predicates[1] == XPathStepId_NULL 
            && step->action == XPathAxis_Child 
            && step->lastStep == XPathStepId_NULL )
            {
              __ui64 total = 0;
              for ( ChildIterator child(father) ; child ; child++ )
                {
                  if ( !evalNodeKey ( child, step ) ) continue;
                  total++;
                  if ( child == current )
                    {
                      Log_Matches ( "At %s, total=%llu, getExpression()=%s\n", 
                          current.getKey().c_str(),
                          total, getExpression() );
                      NodeSet fakeNodeSet;
                      fakeNodeSet.pushBack ( current, false );
                      NodeSet::iterator fakeIterator ( fakeNodeSet, getXProcessor() );
                      fakeIterator.setPosition ( total );
                      bool res = matchesPredicate ( fakeIterator, current, step->predicates[0] );
                      if ( res && step->nextStep == XPathStepId_NULL )
                        {
                          effectivePriority = computeEffectivePriority ( step );
                        }
                      Log_Matches ( "res => %s\n", res ? "TRUE" : "FALSE" );
                      return res;
                    }
                }
            }
#endif // __XEM_XPATH_MATCHES_USE_PREDICATEHASLAST
#endif // __XEM_XPATH_MATCHES_USE_PREDICATEHASPOSITION
            
          NodeSet effectiveNodeSet;
          NodeSet& myNodeSet = effectiveNodeSet;
          
          if ( 0 ) {}
#ifdef __XEM_XPATH_MATCHES_NON_PREDICATE_SHORTCUT
          else if ( step->predicates[0] == XPathStepId_NULL )
            {
              // Bug ( "Shall have handled before !\n" );
              effectiveNodeSet.pushBack ( current, false );
            }
#endif // __XEM_XPATH_MATCHES_NON_PREDICATE_SHORTCUT
#ifdef __XEM_XPATH_MATCHES_USE_PREDICATEHASPOSITION
          else if ( ! ( step->flags & XPathStepFlags_PredicateHasPosition ) )
            effectiveNodeSet.pushBack ( current, false );
#endif // __XEM_XPATH_MATCHES_USE_PREDICATEHASPOSITION
          else
            {
              /*
               * Compute the child list, as we have to know the position() and last()
               * We *have* to compute full NodeSet, in order for the predicate to compute sub-predicates.
               * That may be thoroughly optimized when the XPath is non-positionnal.
               * Thougths : It's no use to build the complete NodeSet result, 
               * only these couple of integers are enough.
               */
              for ( ChildIterator child(father) ; child ; child++ )
                {
                  evalNodeKey ( effectiveNodeSet, child, step );
#ifdef __XEM_XPATH_MATCHES_USE_PREDICATEHASLAST
                  if ( ( !( step->flags & XPathStepFlags_PredicateHasLast )) && child == current )
                    break;
#endif
                }
              Log_Matches ( "Built child list for '%s' => %lu nodes !\n", 
                  getExpression(), (unsigned long) effectiveNodeSet.size() );
            }
          if ( step->action == XPathAxis_Child 
            || ( (step->action == XPathAxis_Descendant || step->action == XPathAxis_Descendant_Or_Self )
                && step->lastStep == XPathStepId_NULL ) )
            {
              bool res = matchesPredicate ( myNodeSet, father, current, step, 0, effectivePriority );
              Log_Matches ( "Matches (%x) : matches key (%x), res=%d !\n", 
                  current.getKeyId(), step->keyId, res );
              if ( res && step->nextStep == XPathStepId_NULL )
                {
                  effectivePriority = computeEffectivePriority ( step );
                }
              return res;
            }
          for ( ElementRef ancestor = (step->action == XPathAxis_Descendant) ? father : current; 
            ancestor != rootElement ; ancestor = ancestor.getFather() )
            if ( matchesPredicate ( myNodeSet, ancestor, current, step, 0, effectivePriority ) )
              {
                if ( step->nextStep == XPathStepId_NULL )
                  {
                    effectivePriority = computeEffectivePriority ( step );
                  }
                return true;
              }
          return false;
        }
      case XPathFunc_Union:
      {
        if ( ! final )
          {
            NotImplemented ( "matches() : step with a non-final Union : '%s'\n", getExpression() );
          }
        bool isMatching = false;
        for ( int a = 0 ; a < XPathStep_MaxFuncCallArgs ; a++ )
          {
            if ( step->functionArguments[a] == XPathStepId_NULL ) break;
            XPathStep* myStep = getStep(step->functionArguments[a]);
            while ( myStep->nextStep != XPathStepId_NULL )
              myStep = getStep ( myStep->nextStep );
            Number myEffectivePriority = computeEffectivePriority ( myStep );
            if ( matches ( node, myStep, effectivePriority ) )
              {
                Log_Matches ( "Element '%s' (%x) matches '%s' (a=%d, step=%u)\n", 
                    node.getKey().c_str(), node.getKeyId(), getExpression(), a, step->functionArguments[a] );

                isMatching = true;
                if ( effectivePriority < myEffectivePriority )
                  {
                    effectivePriority = myEffectivePriority;
                  }
                break;
              }
          }
        Log_Matches ( "At union, isMatching=%d\n", isMatching );
        return isMatching;
      }
      case XPathFunc_BooleanAnd:
        if ( ! final )
          {
            NotImplemented ( "matches() : step with a non-final Union : '%s'\n", getExpression() );
          }
        for ( int a = 0 ; a < 2 ; a++ )
          {
            AssertBug ( step->functionArguments[a] != XPathStepId_NULL, "Operand not set !\n" );
            XPathStep* myStep = getStep(step->functionArguments[a] );            
            if ( ! matches ( node, myStep, effectivePriority ) )
              {
                Log_Matches ( "Element '%s' does not match (at a=%d, step=%u)\n", 
                    node.getKey().c_str(), a, step->functionArguments[a] );
                return false;
              }
          }            
        return true;
      case XPathFunc_BooleanOr:
        if ( ! final )
          {
            NotImplemented ( "matches() : step with a non-final Union : '%s'\n", getExpression() );
          }
        for ( int a = 0 ; a < 2 ; a++ )
          {
            AssertBug ( step->functionArguments[a] != XPathStepId_NULL, "Operand not set !\n" );
            XPathStep* myStep = getStep(step->functionArguments[a] );            
            if ( ! matches ( node, myStep, effectivePriority ) )
              {
                Log_Matches ( "Element '%s' does not match (at a=%d, step=%u)\n", 
                    node.getKey().c_str(), a, step->functionArguments[a] );
                return true;
              }
          }            
        return false;
      case XPathFunc_Not:
        {
          if ( ! final )
            {
              NotImplemented ( "matches() : step with a non-final Union : '%s'\n", getExpression() );
            }
          XPathStep* myStep = getStep(step->functionArguments[0] );            
          
          bool res = ( ! matches ( node, myStep, effectivePriority ) );
          Log_Matches ( "At boolean not (arg=%u) : res=%d\n", step->functionArguments[0], res );
          return res;
        }
      case XPathFunc_Key:
        {
          if ( step->predicates[0] != XPathStepId_NULL )
            {
              NotImplemented ( "matches : key() : have predicate after !\n" );
            }
          NodeSet resultNodeSet;
          evalFunctionKeyGet ( resultNodeSet, node, step );
          
          Log_Matches ( "MATCHKEY %s ------------------\n", node.getKey().c_str() );
          resultNodeSet.log ();
          Log_Matches ( "MATCHKEY %s ------------------\n", node.getKey().c_str() );
          bool res = resultNodeSet.contains ( node );
          if ( res && step->nextStep == XPathStepId_NULL )
            effectivePriority = computeEffectivePriority ( step );
          return res;
        }
      case XPathFunc_Id:
        {
          if ( step->predicates[0] != XPathStepId_NULL )
            {
              NotImplemented ( "matches : key() : have predicate after !\n" );
            }
          NodeSet resultNodeSet;
          evalFunctionIdGet ( resultNodeSet, node, step );
          bool res = resultNodeSet.contains ( node );
          if ( res && step->nextStep == XPathStepId_NULL )
            effectivePriority = computeEffectivePriority ( step );
          return res;
        }
      case XPathAxis_Attribute:
        {
          __nodeToAttribute();
          if ( ! evalNodeKey ( current, step ) )
            return false;
          NodeSet result; result.pushBack ( current, false );
          ElementRef element = current.getElement();
          bool res = matchesPredicate ( result, element, current, step, 0, effectivePriority );
          if ( res && step->nextStep == XPathStepId_NULL )
            effectivePriority = computeEffectivePriority ( step );
          return res;
        }
      default:
        NotImplemented ( "Action %u\n", step->action );
    }

    Bug ( "Shall not be here !\n" );
    return false; // matches ( node, xpathSegment->firstStep );
  }

  /**
   * XPath matching function
   */
  bool XPath::matches ( NodeRef& node, Number& effectivePriority )
  {
    if ( xpathSegment->firstStep == XPathStepId_NULL )
      return false;

    Log_Matches ( "Matching '%s' against '%s' :\n", 
		   node.generateVersatileXPath().c_str(), getExpression() );
       
    if ( xpathSegment->firstStep == XPathStepId_NULL )
      {
      	throwXPathException ( "matches() called on an empty xpath '%s'\n", getExpression() );
      }

    XPathStep* firstStep = getStep(xpathSegment->firstStep);
    while ( firstStep->nextStep != XPathStepId_NULL )
      {
        firstStep = getStep(firstStep->nextStep);
      }
    
    if ( firstStep->action == XPathFunc_Union )
      {
        effectivePriority = -INFINITY;
      }
    else
      {
        effectivePriority = -INFINITY;
        // effectivePriority = computeEffectivePriority ( firstStep );
      }
    bool res = matches ( node, firstStep, effectivePriority );

    Log_Matches ( "Matching '%s' against '%s' : res=%s\n", 
		   node.generateVersatileXPath().c_str(), 
       getExpression(), res ? "TRUE" : "FALSE" );
    
    return res;
  }

  Number XPath::computeEffectivePriority ( XPathStep* step )
  {
    /*
     * Priority list ordering
     - 
     
     - node() -0.5
     - a/node() -0.45
     - * -0.4
     - a/ * -0.35
     - b:* -0.25
     - a/b:* -0.2
     - c 0
     - a/c 0.5 
     */
    bool final = step->lastStep == XPathStepId_NULL;
    
    bool hasPredicates = step->predicates[0] != XPathStepId_NULL;
    if ( hasPredicates && final )
      final = false;
    switch ( step->action )
    {
    case XPathFunc_Key:
    case XPathFunc_Id:
        return final ? 0 : 0.5;
    case XPathAxis_Root:
      return 0.5;  
    case XPathAxis_Variable:
    case XPathAxis_Resource:
    case XPathAxis_ConstInteger:
    case XPathAxis_ConstNumber:
      return -INFINITY;

    case XPathAxis_Ancestor:
    case XPathAxis_Ancestor_Or_Self:
    case XPathAxis_Attribute:
    case XPathAxis_Child:
    case XPathAxis_Descendant:
    case XPathAxis_Descendant_Or_Self:
    case XPathAxis_Following:
    case XPathAxis_Following_Sibling:
    case XPathAxis_Namespace:
    case XPathAxis_Parent:
    case XPathAxis_Preceding:
    case XPathAxis_Preceding_Sibling:
    case XPathAxis_Self:
    
      if ( ! step->keyId )
        return final ? -0.5 : 0.5;
      /*
       * Disambiguate between xemint:element (the '*' keyword) and fully-qualified names.
       */
      if ( step->keyId == xproc.getKeyCache().getBuiltinKeys().xemint.element() )
        return final ? -0.5 : 0.5;
      if ( KeyCache::getNamespaceId(step->keyId) && ! KeyCache::getLocalKeyId(step->keyId) )
        return final ? -0.25 : 0.5;
    default:
      if ( step->action == XPathFunc_FunctionCall || step->keyId == 0 )
        {
          return -INFINITY;
        }
    }
    return final ? 0 : 0.5;
  }

};
