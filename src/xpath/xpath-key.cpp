#include <Xemeiah/xpath/xpath.h>
#include <Xemeiah/xpath/xpathparser.h>
#include <Xemeiah/dom/nodeset.h>
#include <Xemeiah/dom/elementmapref.h>
#include <Xemeiah/trace.h>
#include <Xemeiah/xprocessor/xprocessor.h>

#include <Xemeiah/auto-inline.hpp>

/**
 * \file
 * Implementation of the xsl:key() mechanism.
 * Implementation details of xsl:key() :
 * -# uses SKMultiMapRef to store hashed values
 * -# SKMultiMapRef stored values are direct pointers (ElementPtr) to the elements.
 * -# This mapping only makes sense in a per-Document mapping.
 * -# It is not assumed that all the mappings are created at xsl:stylesheet time. 
 *    - For example, we can still have a xsl:key() on a document(), from which we did not generate the xsl:key() mapping.
 * -# 
 * Persistence-specific issues :
 * -# SubDocument (i.e. having a fake root on a Document()) is quite a big issue.
 * -# How to check that the mapping we are trying to re-use was built with the same matchXPath and useXPath ? hashing ? 
 */

#define Log_EvalKey Debug

namespace Xem
{ 
#define __evalArgNodeSet(__idx) \
  NodeSet res##__idx; evalStep ( res##__idx, node, step->functionArguments[__idx] );
#define __evalArgStr(__idx)						\
  NodeSet __res##__idx; String res##__idx;				\
    evalStep ( __res##__idx, node, step->functionArguments[__idx] ); \
    res##__idx = __res##__idx.toString ( );
#define __evalArgInteger(__idx)						\
  NodeSet __res##__idx; Integer res##__idx;				\
    evalStep ( __res##__idx, node, step->functionArguments[__idx] ); \
    res##__idx = __res##__idx.toInteger ( );

  /* 
   * ****************************** KEY BUILDING FUNCTION **********************************
   */

  void XPath::evalFunctionKeyBuild ( ElementRef& baseElement, ElementMultiMapRef& map,
      XPath& matchXPath, XPath& useXPath, XPath& scopeXPath, KeyId mapId )
  { 
    bool recursive = true; // Warn !!
    
    Log_EvalKey ( "evalFunctionKeyBuild(baseElement=%s, match=%s, use=%s, scope=%s\n",
                  baseElement.getKey().c_str(), matchXPath.expression, useXPath.expression, scopeXPath.expression);

    if ( ! recursive )
      {
        Warn ( "Building key in a non-recursive mode ! This is against XSL specifications !\n" );
      }
    
    if ( ! baseElement.getChild() )
      {
        Log_EvalKey ( "Base element has no children !\n" );
        return;
      }
    
    ElementRef current = baseElement;
    
    Log_EvalKey ( "Building Key !\n" );
    Log_EvalKey ( "baseElement = '%s'\n", baseElement.getKey().c_str() );
    Log_EvalKey ( "baseElement = '%s'\n", baseElement.generateVersatileXPath().c_str() );

    __ui64 iterated = 0, found = 0;
    bool currentMatches;

    while ( true )
      {
        Log_EvalKey ( "[At element %s (0x%llx:%x/%s), matching '%s' ?\n",
            current.generateVersatileXPath().c_str(), current.getElementId(), current.getKeyId(), current.getKey().c_str(),
            matchXPath.expression );

        currentMatches = false;
        iterated++;

        if ( matchXPath.matches ( current ) )
          {
            Log_EvalKey ( "Matches !\n" );
            currentMatches = true;
            found++;
            
            if ( map )
              {
                map.insert ( current, useXPath );
              }
            else
              {
                NodeSet scopeNodeSet;
                scopeXPath.eval ( scopeNodeSet, current );
                if ( scopeNodeSet.size() == 1 && scopeNodeSet.front().isElement() )
                  {                
                    ElementRef scopeElement = scopeNodeSet.toElement();
                    ElementMultiMapRef currentMap = scopeElement.findAttr ( mapId, AttributeType_SKMap );
                    if ( ! currentMap )
                      {
                        Info ( "New skMap at '%s'\n", scopeElement.generateVersatileXPath().c_str() );
                        currentMap = scopeElement.addSKMap ( mapId, SKMapType_ElementMultiMap );
                      }
                    Log_EvalKey ( "Inserting '%s'\n", current.generateVersatileXPath().c_str() );
                    currentMap.insert ( current, useXPath );
                  }
                else
                  {
                    Info ( "Silently ignoring '%s' : scope did not eval to an Element.\n",
                        current.generateVersatileXPath().c_str() );
                  }
              }
          }
        if ( current.getChild() && ( !currentMatches || recursive ) )
          {
            Log_EvalKey ( "Don't match, but has children.\n" );
            current = current.getChild();
            continue;
          }
        else
          {
            Log_EvalKey ( "Don't match, but no has child.\n" );
          }
        while ( ! current.getYounger() )
          {
            AssertBug ( current != baseElement, "How did I get there ?\n" );

            current = current.getFather();

            if ( ! current )
              {
                throwException ( Exception, "Preliminary end of element !\n" );
              }

            AssertBug ( current, "current had no father !\n" );

            if ( current == baseElement ) 
              {
                Log_EvalKey ( "Indexing finished : iterated over %llu, found %llu.\n", iterated, found );
                return;
              }
          }
        current = current.getYounger ();
      }
  }

  /* 
   * ****************************** KEY FETCHING FUNCTION : id('...') **********************************
   */
  void XPath::evalFunctionId ( __XPath_Functor_Args )
  {
    NodeSet myNodeSet;
    evalFunctionIdGet ( myNodeSet, node, step );
    evalNodeSetPredicate ( result, myNodeSet, step ); 
  }


  void XPath::evalFunctionIdGet ( __XPath_Functor_Args )
  {
    __evalArgNodeSet(0);

    ElementRef rootElement = node.getRootElement();

    if ( !rootElement.findAttr(__builtin.xemint.id_match(),AttributeType_XPath)
      || !rootElement.findAttr(__builtin.xemint.id_use(),AttributeType_XPath) )
      {
        Warn ( "Document '%s' has no attribute defined as ID !\n", rootElement.getDocument().getDocumentTag().c_str() );
        return;
      }

    XPath matchXPath ( xproc, rootElement, __builtin.xemint.id_match() );
    XPath useXPath ( xproc, rootElement, __builtin.xemint.id_use() );

    ElementMultiMapRef map = node.getDocument().getKeyMapping ( __builtin.xemint.id_map() );
    if ( ! map )
      {
        SKMapConfig config;
        SKMapRef::initDefaultSKMapConfig ( config );

        map = node.getDocument().createKeyMapping ( __builtin.xemint.id_map(), config );
        AssertBug ( map, "Could not create SKMultiMapRef !\n" );

        XPath scopeXPath ( xproc );
        evalFunctionKeyBuild ( rootElement, map, matchXPath, useXPath, scopeXPath, 0 );
      }
    if ( res0.size() == 0 )
      {
        return;
      }

    if ( ( res0.size() == 1 ) && 
         ( res0.front().getItemType() == Item::Type_String || res0.front().getItemType() == Item::Type_Attribute ) )
      {
        String res = res0.toString();
        std::list<String> tokenList;
        res.tokenize ( tokenList );
        while ( tokenList.size() )
          {
            String token = tokenList.front ();
            tokenList.pop_front ();
            evalFunctionKeyGet ( map, useXPath, result, token );
          }
        
      }
    else
      {
        evalFunctionKeyGet ( map, useXPath, result, res0 );
      }
  }

  /* 
   * ****************************** KEY FETCHING FUNCTION : key(name,'...') **********************************
   */
  void XPath::evalFunctionKey ( __XPath_Functor_Args )
  {
    NodeSet myNodeSet;
    evalFunctionKeyGet ( myNodeSet, node, step );
    evalNodeSetPredicate ( result, myNodeSet, step ); 
  }
  
  void XPath::evalFunctionKeyGet ( __XPath_Functor_Args )
  {
    KeyId keyNameId = evalStepAsKeyId ( node, step->functionArguments[0] );
    __evalArgNodeSet(1);

    Log_EvalKey ( "xsl:key : expression '%s'\n", expression );

    XPath matchXPath ( xproc );
    XPath useXPath ( xproc );
    XPath scopeXPath(xproc);

    getXProcessor().getXPathKeyExpressions(keyNameId, matchXPath,useXPath);

    ElementMultiMapRef map = node.getDocument().getKeyMapping ( keyNameId );
    if ( ! map )
      {
        map = node.getDocument().createKeyMapping ( keyNameId );
        
        AssertBug ( map, "Could not create SKMultiMapRef !\n" );

        Log_EvalKey ( "Building for key '%x'\n", keyNameId );

        ElementRef baseElement = node.getRootElement ();
#if 0
        if ( instructKey.findAttr ( __builtinKey(xsl.base) ) )
          {
            throwXPathException ( "Invalid xsl:base attribute set !\n" );
            XPath baseXPath ( instructKey, __builtinKey(xsl.base) );

            Log_EvalKey ( "xsl:base : expression '%s'\n", baseXPath.expression );
            
            Log_EvalKey ( "xsl:base - before : base : %llx:%s\n", baseElement.getElementId(), baseElement.getKey().c_str() );
            baseElement = baseXPath.evalElement ( baseElement );
            Log_EvalKey ( "xsl:base - after : base : %llx:%s\n", baseElement.getElementId(), baseElement.getKey().c_str() );
          }
#endif
        evalFunctionKeyBuild ( baseElement, map, matchXPath, useXPath, scopeXPath, 0 );
      }

    evalFunctionKeyGet ( map, useXPath, result, res1 );
  }
  
  void XPath::evalFunctionKeyGet ( ElementMultiMapRef& map,
      XPath& useXPath, NodeSet& resultNodeSet, NodeSet& argNodeSet )
  {
    for ( NodeSet::iterator iter(argNodeSet) ; iter ; iter++ )
      {
        String value = iter->toString ();
        Log_EvalKey ( "LOOKUP : '%s'\n", value.c_str() );
        evalFunctionKeyGet ( map, useXPath, resultNodeSet, value );
      }
  }
  
  
  void XPath::evalFunctionKeyGet ( ElementMultiMapRef& map,
      XPath& useXPath, NodeSet& resultNodeSet, String& value )
  {
    __ui64 hash = SKMapRef::hashString ( value );
    Log_EvalKey ( "Computed value : '%s', hash='%llx'\n", value.c_str(), hash );

    for ( SKMultiMapRef::multi_iterator miter ( map, hash ); miter ; miter++ )
      {
        Log_EvalKey ( "Found iter %llx, val=%llx\n", miter.getHash(), miter.getValue() );
        if ( miter.getHash() != hash )
          {
            Log_EvalKey ( "Diverging keys : %llx != %llx\n", miter.getHash(), hash );
            break;
          }
        ElementRef foundElt = map.get ( miter );
        Log_EvalKey ( "Found elt=0x%llx\n", foundElt.getElementId() );
        // AssertBug ( foundElt.getElementId(), "Null elementId ! Element was deleted ?\n" );
        NodeSet foundValue;
        useXPath.eval ( foundValue, foundElt );
        for ( NodeSet::iterator valiter ( foundValue ) ; valiter ; valiter++ )
          {
            String eltValue = valiter->toString();
            Log_EvalKey ( "Found node 0x%llx:%s, eltValue='%s'\n", foundElt.getElementId(), foundElt.getKey().c_str(), eltValue.c_str() );
            if ( eltValue == value )
              {
                resultNodeSet.pushBack ( foundElt, true );
                Log_EvalKey ( "Matches !\n" );
              }
            
          }
      }
  }  
};

