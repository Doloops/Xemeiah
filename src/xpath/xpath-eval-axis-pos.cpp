#include <Xemeiah/xpath/xpath.h>
#include <Xemeiah/xprocessor/xprocessor.h>
#include <Xemeiah/trace.h>

#include <Xemeiah/dom/childiterator.h>

#include <Xemeiah/auto-inline.hpp>

/**
 * Here is the default, positional implementation of Axis walkthrough.
 */

#define __XEM_XPATH_AXIS_CHILD_ALLOW_SHORT_PATH // Option : allow XPath::evalAxisChild() not to use a pivot NodeSet if step has no predicates.

#define Log_Eval Debug

namespace Xem
{
  /**
   * Standard Axes
   */
  void XPath::evalAxisAncestorGeneric ( __XPath_Functor_Args, bool includeSelf )
  {
    ElementRef current(node.getDocument());
    ElementRef rootElement(node.getDocument());
    NodeSet myNodeSet;
    if ( node.isAttribute() )
      {
        if ( includeSelf )
            evalNodeKey ( myNodeSet, node, step );
        current = node.toAttribute().getElement();
        includeSelf = true;
        rootElement = current.getRootElement();
      }
    else
      {
        current = node.toElement();
        rootElement = current.getRootElement();
        if ( current == rootElement ) return;
        if ( ! includeSelf ) current = current.getFather();
        if ( current == rootElement ) return;
      }
    for ( ElementRef ancestor = current ; ancestor ; ancestor = ancestor.getFather() )
      {
        Log_Eval ( "ANCESTOR finder : node=%s, root=%s, at elt=%s\n",
            node.getKey().c_str(), rootElement.getKey().c_str(), ancestor.getKey().c_str() );
        evalNodeKey ( myNodeSet, ancestor, step );
        if ( ancestor == rootElement )
        break;
      }
    /**
     * TODO : We may sort the myNodeSet NodeSet !
     */
    evalNodeSetPredicate ( result, myNodeSet, step, true );
  }

  void XPath::evalAxisAncestor ( __XPath_Functor_Args )
  {
    evalAxisAncestorGeneric ( result, node, step, false );
  }
  
  void XPath::evalAxisAncestor_Or_Self ( __XPath_Functor_Args )
  {
    evalAxisAncestorGeneric ( result, node, step, true );
  }

  void XPath::evalAxisAttribute ( __XPath_Functor_Args )
  {
    if ( !node )
      {
        throwXPathException ( "Invalid null node !\n" );
      }
    if ( !node.isElement() )
      return;
    ElementRef& current = node.toElement();
    if ( current.isText() || current.isComment() || current.isPI() )
      return;
    NodeSet myNodeSet;
    
    for ( AttributeRef attr = current.getFirstAttr() ;
            attr ; attr = attr.getNext() )
      {
        if ( ! attr.isBaseType() ) continue;
        if ( attr.getType() == AttributeType_NamespaceAlias ) continue;
        if ( KeyCache::getNamespaceId(attr.getKeyId()) == xproc.getKeyCache().getBuiltinKeys().xemint.ns() ) continue;
        Log_Eval ( "attr='%s' (%x), type=%x, step->keyId=%x\n", attr.getKey().c_str(), attr.getKeyId(), attr.getType(), step->keyId );
        evalNodeKey ( myNodeSet, attr, step );
      }
    evalNodeSetPredicate ( result, myNodeSet, step );
  }


  void XPath::evalAxisChild ( __XPath_Functor_Args )
  {
    if ( ! node.isElement() ) return;
    AssertBug ( node.isElement(), "Node is not element !\n" );
    ElementRef& current = node.toElement();
    
    Log_Eval ( "current=%s, step->predicates=%d\n",
        current.getKey().c_str(), step->predicates[0] );
#ifdef __XEM_XPATH_AXIS_CHILD_ALLOW_SHORT_PATH
    if ( step->predicates[0] == XPathStepId_NULL )
      {
        /*
         * Short path : do not use a temporary NodeSet, process nodes directly
         */
        for ( ChildIterator child ( current ) ; child ; child++ )
          {
            Log_Eval ( "\tchild=%s, id=%llu\n",
                child.getKey().c_str(), child.getElementId() );
            if ( evalNodeKey ( child, step ) )
              {
                Log_Eval ( "\t\tmatch, nextStep=%d\n", step->nextStep );
                if ( step->nextStep == XPathStepId_NULL )
                  {
                    Log_Eval ( "\t\tpushing back : size=%lu\n", result.size() );
                    result.pushBack ( (ElementRef&) child, true );
                    Log_Eval ( "\t\tresult : size=%lu\n", result.size() );
                  }
                else
                  {
                    evalStep ( result, child, step->nextStep );
                  }
              }
          }
        Log_Eval ( "End of short path : result=%lu\n", result.size() );
        return;
      }
#endif // __XEM_XPATH_AXIS_CHILD_ALLOW_SHORT_PATH
    NodeSet myNodeSet;
#ifdef __XEM_XPATH_AXIS_CHILD_COUNT_CHILDREN
    __ui64 nbChildren = 0;
#endif // __XEM_XPATH_AXIS_CHILD_COUNT_CHILDREN
    for ( ChildIterator child(current) ; child ; child++ )
      {
        Log_Eval ( "At child %llx keyId=%x (%s), keyId=%x\n",
            child.getElementId(), child.getKeyId(), child.getKey().c_str(),
            step->keyId );
        evalNodeKey ( myNodeSet, child, step );
#ifdef __XEM_XPATH_AXIS_CHILD_COUNT_CHILDREN
        nbChildren++;
#endif // __XEM_XPATH_AXIS_CHILD_COUNT_CHILDREN
      }
#ifdef __XEM_XPATH_AXIS_CHILD_COUNT_CHILDREN
    fprintf ( stderr, "[XPATH-CHILD] children=%llu, selected=%lu\n",
      nbChildren, myNodeSet.size() );
#endif // __XEM_XPATH_AXIS_CHILD_COUNT_CHILDREN
    evalNodeSetPredicate ( result, myNodeSet, step );
  }

  void XPath::evalAxisDescendantGenericElement ( NodeSet& result, ElementRef& current, XPathStep* step )
  {
    Log_Eval ( "current='%s' (%x), step->keyId=%x\n", 
        current.generateVersatileXPath().c_str(), current.getKeyId(), step->keyId );
#if 1
    ElementRef child = current.getChild ();
    if ( ! child ) return;
#ifdef __count
    __ui64 totalWalked = 0;
#endif
    while ( true )
      {
#ifdef __count
        totalWalked ++;
#endif
        evalNodeKey ( result, child, step );
        if ( child.getChild () )
          {
            child = child.getChild ();
            continue;
          }
        while ( ! child.getYounger() )
          {
            child = child.getFather ();
            if ( child == current ) 
              {
#ifdef __count
                Log_Eval ( "Total for '%s' : matched/walked=%lu/%llu, isRoot=%s, from '%s'\n",
                    getExpression(), (unsigned long) result.size(), totalWalked,
                    current.isRootElement() ? "YES" : "NO",
                    current.generateVersatileXPath().c_str() );
#endif
                return;
              }
          }
        child = child.getYounger ();
      }
#else
    for ( ChildIterator child ( current ) ; child ; child++ )
      {
        evalNodeKey ( result, child, step );
        evalAxisDescendantGenericElement ( result, child, step );
      }
#endif      
  }

  INLINE void XPath::evalAxisDescendantGeneric ( __XPath_Functor_Args, bool includeSelf )
  {
    NodeSet myNodeSet;
    if ( includeSelf )    
      {
        evalNodeKey ( myNodeSet, node, step );
        // evalNodeSetPredicate ( result, myNodeSet, step );
      }
    if ( node.isAttribute() )
      {
        /*
         * Nothing to be done if I am on an attribute
         */
      }
    else
      {
        ElementRef current = node.toElement ();
        evalAxisDescendantGenericElement ( myNodeSet, current, step );
      }
    evalNodeSetPredicate ( result, myNodeSet, step );
    return; 

#if 0 // DEPRECATED
    if ( ! includeSelf )
    {
        current = current.getChild ();
        if ( ! current ) return;
    }
    else
    {
      NotImplemented ( "." );
    }

    NodeSet myNodeSet;
    while ( true )
      {
        Log_Eval ( "DESCENDANTGEN : elt=%s, current=%s\n", element.getKey(), current.getKey() );
        evalNodeKey (xproc, myNodeSet, current, step);

        ElementRef child = current.getChild ();
        if ( child )
          {
            current = child;
            continue;
          }
        else if ( current == element ) 
            break;

        while ( ! current.getYounger() )
          {
#if PARANOID
            AssertBug ( current.getFather(), "Current '%s' has no father ! element='%s', expr='%s'\n",
                current.getKey().c_str(), element.getKey().c_str(), expression );
#endif
            current = current.getFather ();
            if ( current == element ) 
            goto end_of_iteration_loop;
          }
        current = current.getYounger ();
      }
end_of_iteration_loop:
    Log_Eval ( "--------------- POST DESCENDANT BEFORE PREDICATE ------------------\n" );
    myNodeSet.log ();
    evalNodeSetPredicate ( result, myNodeSet, step );
    Log_Eval ( "--------------- POST DESCENDANT AFTER PREDICATE ------------------\n" );
#endif // DEPR
  }

  void XPath::evalAxisDescendant ( __XPath_Functor_Args )
  {
    evalAxisDescendantGeneric ( result, node, step, false );
  }


  void XPath::evalAxisDescendant_Or_Self ( __XPath_Functor_Args )
  {
    evalAxisDescendantGeneric ( result, node, step, true );
  }

  void XPath::evalAxisFollowing ( __XPath_Functor_Args )
  {
    ElementRef element(node.getDocument());
    ElementRef current = element;
    if ( node.isAttribute() )
    {
        element = node.toAttribute().getElement();
        current = element;
        if ( current.getChild() )
            current = element.getChild();
    }
    else
    {
        element = node.toElement();
        current = element;
    }
    ElementRef rootElement = element.getRootElement();
    
    Log_Eval ( "FOLLOWING : Called on element=%llx:%s, root=%llx:%s\n",
        element.getElementId(), element.getKey().c_str(),
        rootElement.getElementId(), rootElement.getKey().c_str() );

    if ( element == rootElement ) return;

    NodeSet myNodeSet;
    while ( true )
    {
        Log_Eval ( "FOLLOWING : root=%llx:%s, element=%llx:%s, current=%llx:%s\n", 
            rootElement.getElementId(), rootElement.getKey().c_str(),
            element.getElementId(), element.getKey().c_str(),
            current.getElementId(), current.getKey().c_str() );

        if ( current != element )
        {
            evalNodeKey ( myNodeSet, current, step );
            if ( current.getChild() )
            {
                current = current.getChild();
                continue;
            }    
        }
        while ( ! current.getYounger() )
        {
            if ( current == rootElement ) goto end_of_iteration;
            current = current.getFather();
            if ( current == rootElement ) goto end_of_iteration;
        }
        current = current.getYounger();
    }
end_of_iteration:
    evalNodeSetPredicate ( result, myNodeSet, step );
  }

  void XPath::evalAxisFollowing_Sibling ( __XPath_Functor_Args )
  {
    
    if ( node.isAttribute() ) return;

    ElementRef current = node.toElement();
    Log_Eval ( "Call following sibling from elt=%s\n", node.getKey().c_str() );
    NodeSet myNodeSet;
    for ( ElementRef following = current.getYounger() ;
        following ; following = following.getYounger() )
      {
        Log_Eval ( "Processing Following_Sibling node=%s following=%s\n",
            node.getKey().c_str(), following.getKey().c_str() );
        evalNodeKey ( myNodeSet, following, step );
      }
    evalNodeSetPredicate ( result, myNodeSet, step );
  }

  void XPath::evalAxisNamespace ( __XPath_Functor_Args )
  {
    if ( ! node.isElement() ) return;

    Log_Eval ( "TODO : Reimplement this cleanly.\n" );
    NodeSet myNodeSet;

    for ( ElementRef ancestor = node.toElement() ; ancestor ; ancestor = ancestor.getFather() )
      {
        for ( AttributeRef attr = ancestor.getFirstAttr() ; attr ; attr = attr.getNext() )
          {
            Log_Eval ( "At attr %x:%s : type=%d\n", attr.getKeyId(), attr.getKey().c_str(), attr.getType() );
            if ( attr.getType() != AttributeType_NamespaceAlias )
              continue;
            evalNodeKey ( myNodeSet, attr, step );
          }
        if ( ancestor.isRootElement() )
            break;
      }
    Log_Eval ( "Axis Namespace : Preliminary result :\n" );
    myNodeSet.log ();
    /**
     * \todo Build the namespace list in a sorted way.
     */
    myNodeSet.sortInDocumentOrder ();
    Log_Eval ( "Axis Namespace : Preliminary result sorted :\n" );
    evalNodeSetPredicate ( result, myNodeSet, step );
  }

  void XPath::evalAxisParent ( __XPath_Functor_Args )
  {
    if ( node.isAttribute() )
    {
        ElementRef current = node.toAttribute().getElement();
        NodeSet myNodeSet;
        evalNodeKey ( myNodeSet, current, step );
        evalNodeSetPredicate ( result, myNodeSet, step );
        return;
    }
    ElementRef current = node.toElement();
#if PARANOID   
    ElementRef rootElement = current.getRootElement();
    if ( rootElement == current ) return;
#endif
    ElementRef father = current.getFather();
    if ( ! father )
      {
        return;
#if 0
        throwXPathException ( "Element %llx (%x:%s) has no father !\n",
                      current.getElementId(), 
                      current.getKeyId(),
                      current.getKey() );
#endif
      }
    if ( step->keyId || step->predicates[0] != XPathStepId_NULL )
      {
        NodeSet myNodeSet;
        evalNodeKey ( myNodeSet, father, step );
        evalNodeSetPredicate ( result, myNodeSet, step );
      }
    else
      {
        evalStep ( result, father, step->nextStep );
      }
  }

  void XPath::evalAxisPreceding ( __XPath_Functor_Args )
  {
    ElementRef current ( node.getDocument() );
    if ( node.isAttribute() )
    {
        current = node.toAttribute().getElement();
    }
    else
    {
        current = node.toElement ();
    }
    ElementRef rootElement = current.getRootElement();
    Log_Eval ( "PREC : root=%llx:%s, current=%llx:%s\n",
    rootElement.getElementId(), rootElement.getKey().c_str(),
    current.getElementId(), current.getKey().c_str() );

    if ( rootElement == current ) return;

    ElementRef preceding = current;
    NodeSet myNodeSet;
    
    /**
     * We must find the element to start with.
     *
     */
    while ( ! preceding.getElder() )
      {
        preceding = preceding.getFather ();
        if ( preceding == rootElement ) return;
      }
    preceding = preceding.getElder();
    ElementRef parentElement = preceding.getFather ();
      
    while ( true )
      {
        ElementRef lastChild = preceding.getLastChild();
        if ( lastChild )
        {
            preceding = lastChild;  continue;
        }
        // We don't have any child.

        evalNodeKey ( myNodeSet, preceding, step );
      
        while ( ! preceding.getElder() )
        {
            ElementRef father = preceding.getFather ();
            Log_Eval ( "PRECEDING : While finding an elder : father=%llx:%s, preceding=%llx:%s\n",
                father.getElementId(), father.getKey().c_str(),
                preceding.getElementId(), preceding.getKey().c_str() );
            if ( father == rootElement )
              {
                goto preceding_iteration_ends;
              }
            else if ( father == parentElement )
              {
                /*
                 * We are on an ancestor of the current element.
                 * We must find the next ancestor that has an elder
                 */
                while ( ! father.getElder() )
                  {
                    father = father.getFather();
                    AssertBug ( father, "Could not get father !\n" );
                    if ( father == rootElement )
                    goto preceding_iteration_ends;
                  }
                /*
                 * Ok, now we have a father with an elder.
                 * Make this our preceding, and record parentElement.
                 */
                preceding = father;
                parentElement = father.getFather ();
              }
            else
              {
                Log_Eval ( "PRECEDING : Jumping to father=%llx:%s\n",
                    father.getElementId(), father.getKey().c_str() );
                preceding = father;

                evalNodeKey ( myNodeSet, preceding, step );
                continue;
              }	
        }
        preceding = preceding.getElder();
      }
preceding_iteration_ends:
    evalNodeSetPredicate ( result, myNodeSet, step, true );
  }

  void XPath::evalAxisPreceding_Sibling ( __XPath_Functor_Args )
  {
    if ( node.isAttribute() ) return;
    ElementRef current = node.toElement();
    
    ElementRef root = current.getRootElement();
    if ( current == root ) return;

    AssertBug ( current.getFather(), "At preceding-sibling : element has no father !\n" );

    Log_Eval ( "Call preceding sibling from elt=%s\n", node.getKey().c_str() );

    NodeSet myNodeSet;

    for ( ElementRef preceding = current.getElder() ; preceding ; preceding = preceding.getElder() )
      {
        Log_Eval ( "Preceding : '%s' of '%s'\n", preceding.getKey().c_str(), current.getKey().c_str() );
        evalNodeKey ( myNodeSet, preceding, step );
      }
    evalNodeSetPredicate ( result, myNodeSet, step, true );
  }

  void XPath::evalAxisSelf ( __XPath_Functor_Args )
  {
    /*
     * Section 2.2 : the self axis contains just the context node itself.
     * It is a matter of interpretation, but that means that we must only select elements when
     * a keyId has been provided.
     */
    if ( node.isAttribute() && step->keyId )
      return;

    if ( step->predicates[0] == XPathStepId_NULL && step->nextStep == XPathStepId_NULL )  
      {
        if ( evalNodeKey ( node, step ) )
          result.pushBack ( node, true );
        return;
      }

    NodeSet myNodeSet;
    evalNodeKey ( myNodeSet, node, step );
    evalNodeSetPredicate ( result, myNodeSet, step );
  }
};
