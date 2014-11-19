#include <Xemeiah/xpath/xpath.h>
#include <Xemeiah/xprocessor/xprocessor.h>
#include <Xemeiah/trace.h>

/**
 * \file Node Test evaluation (QName test and Predicate)
 */

#ifdef LOG
#define __XEM_XPATH_LOG_NODESET
#endif

// #define __XEM_XPATH_NODETEST_ATTRIBUTES_ALLOW_NON_FULLY_QUALIFED
#define __XEM_XPATH_NODETEST_USE_FLAG_NODETEST_NAMESPACE_ONLY
#define __XEM_XPATH_NODETEST_USE_FLAG_NODETEST_LOCALNAME_ONLY

#define Log_EvalNodeTestHPP Debug

namespace Xem
{
#define __keyCache (node.getKeyCache())
  __INLINE bool XPath::evalNodeKeyNonWildCard ( NodeRef& node, XPathStep* step, KeyId nodeKeyId )
  {  
    if ( step->keyId )
      {
        /*
         * If the step->keyId is defined without a local-part of the KeyId
         * Then it's a namespace checking. Check this !
         * The check is done by extracting the Namespace-part of the key for each key,
         * and compare results.
         */
#ifdef ____XEM_XPATH_NODETEST_USE_FLAG_NODETEST_NAMESPACE_ONLY         
        if ( step->flags & XPathStepFlags_NodeTest_Namespace_Only )
          {
            AssertBug ( ! KeyCache::getLocalKeyId ( step->keyId ), "Flag NODETEST_NAMESPACE_ONLY set, but has local part !\n" );
            Log_EvalNodeTestHPP ( "Comparing local part\n" );
            return ( KeyCache::getNamespaceId(nodeKeyId) == KeyCache::getNamespaceId ( step->keyId ) );
          }
#else
        if ( ! KeyCache::getLocalKeyId ( step->keyId ) )
          {
            Log_EvalNodeTestHPP ( "Comparing namespace part, nodeKeyId=%x, step->keyId=%x\n",
                nodeKeyId, step->keyId );
            if ( KeyCache::getNamespaceId(nodeKeyId) == KeyCache::getNamespaceId ( step->keyId ) )
                return true;
          }
#endif
#ifdef __XEM_XPATH_NODETEST_USE_FLAG_NODETEST_LOCALNAME_ONLY
        else if ( step->flags & XPathStepFlags_NodeTest_LocalName_Only )
          {
            AssertBug ( ! KeyCache::getNamespaceId ( step->keyId ), "Flag NODETEST_NAMESPACE_ONLY set, but has local part !\n" );
            return ( KeyCache::getLocalKeyId(nodeKeyId) == step->keyId );
          }
#endif

#ifdef __XEM_XPATH_NODETEST_ATTRIBUTES_ALLOW_NON_FULLY_QUALIFED
        /*
         * For attributes, non fully-qualified names are allowed
         */
        if ( step->action == XPathAxis_Attribute && ! KeyCache::getNamespaceId ( step->keyId )
             && step->keyId == KeyCache::getLocalKeyId  ( nodeKeyId ) )
            return true;
#endif //  __XEM_XPATH_NODETEST_ATTRIBUTES_ALLOW_NON_FULLY_QUALIFED
        /*
         * The node has the exact same keyId than the Step one.
         * This may be the most frequent case, so we may push it as a first test.
         */
        if ( step->keyId != nodeKeyId ) return false;

      }
    /*
     * If we filtered a text() node, we must check that it is not candidate to filtering by xsl:strip-space.
     */
    if ( nodeKeyId == __keyCache.getBuiltinKeys().xemint.textnode() )
      {
        if ( node.isElement() && node.toElement().isText() && node.toElement().isWhitespace() )
          {
            return ! (node.toElement().mustSkipWhitespace ( xproc ));
          }
      }
    return true;
  }

  __INLINE bool XPath::evalNodeKey ( ElementRef& node, XPathStep* step )
  {
    Log_EvalNodeTestHPP ( "Checking keys : node=0x%x:'%s', step->keyId=0x%x\n",
        node.getKeyId(), node.getKey().c_str(), step->keyId );
    KeyId nodeKeyId = node.getKeyId();
    /*
     * The xemint:element key corresponds to the '*' wildcard of XPath expressions.
     * It only accepts (true) Element types, not text(), comment() or processing-instruction().
     */
    if ( step->keyId == __keyCache.getBuiltinKeys().xemint.element() )
      {
        // if ( node.isRootElement() ) return false;
        Log_EvalNodeTestHPP ( "FILTERING : node=%s, xemint ns=%x, node ns=%x\n",
              node.getKey().c_str(),
              KeyCache::getNamespaceId ( __keyCache.getBuiltinKeys().xemint.element() ),
              KeyCache::getNamespaceId ( nodeKeyId ) );
        /*
         * We must remove all the elements that have a NSKeyId equal to xemint 
         * - xemint:textnode
         * - xemint:comment
         * - xemint:root
         */
        if ( __keyCache.getBuiltinKeys().xemint.ns() == KeyCache::getNamespaceId (nodeKeyId) )
          return false;
        /*
         * We must also remove all xemint-pi Processing Instructions
         */
        if ( __keyCache.getBuiltinKeys().xemint_pi.ns() == KeyCache::getNamespaceId (nodeKeyId) )
          return false;
        Log_EvalNodeTestHPP ( "Selected wildcard xemint.element() for node='%s'\n", node.getKey().c_str() );
        return true;
      }
    return evalNodeKeyNonWildCard ( node, step, nodeKeyId );
  }

  __INLINE bool XPath::evalNodeKey ( NodeRef& node, XPathStep* step )
  {
    Log_EvalNodeTestHPP ( "Checking keys : node=0x%x:'%s', step->keyId=0x%x\n",
        node.getKeyId(), node.getKey().c_str(), step->keyId );
    KeyId nodeKeyId = node.getKeyId();
    /*
     * The xemint:element key corresponds to the '*' wildcard of XPath expressions.
     * It only accepts (true) Element types, not text(), comment() or processing-instruction().
     */
    if ( step->keyId == __keyCache.getBuiltinKeys().xemint.element() )
      {
        if ( ! node.isElement() ) return false;
        //if ( node.isRootElement() ) return false;
        Log_EvalNodeTestHPP ( "FILTERING : node=%s, xemint ns=%x, node ns=%x\n",
              node.getKey().c_str(),
              KeyCache::getNamespaceId ( __keyCache.getBuiltinKeys().xemint.element() ),
              KeyCache::getNamespaceId ( nodeKeyId ) );
        /*
         * We must remove all the elements that have a NSKeyId equal to xemint (xemint:textnode, xemint:pi, ...)
         */
        if ( __keyCache.getBuiltinKeys().xemint.ns() == KeyCache::getNamespaceId (nodeKeyId) )
          return false;
        if ( __keyCache.getBuiltinKeys().xemint_pi.ns() == KeyCache::getNamespaceId (nodeKeyId) )
          return false;
        Log_EvalNodeTestHPP ( "Selected wildcard xemint.element() for node='%s'\n", node.getKey().c_str() );
        return true;
      }
    return evalNodeKeyNonWildCard ( node, step, nodeKeyId );
  }
  
  /**
   * Evaluate the Key part of the step, and push the node in the pushNodeSet if it matches
   * This function eases the evalNodeKey() usage for Axis functions.
   */
  __INLINE void XPath::evalNodeKey ( NodeSet& pushNodeSet, NodeRef& node, XPathStep* step )
  {
    if ( evalNodeKey(node, step) )
      {
        Log_EvalNodeTestHPP ( "EvalNodeKey returns true.\n" );
        pushNodeSet.pushBack ( node, false );
      }
    Log_EvalNodeTestHPP ( "EvalNodeKey returns false.\n" );
  }

  __INLINE void XPath::evalNodeKey ( NodeSet& pushNodeSet, ElementRef& node, XPathStep* step )
  {
    if ( evalNodeKey(node, step) )
      {
        Log_EvalNodeTestHPP ( "EvalNodeKey returns true.\n" );
        pushNodeSet.pushBack ( node, false );
      }
    Log_EvalNodeTestHPP ( "EvalNodeKey returns false.\n" );
  }
#undef __keyCache  
};

