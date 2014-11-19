#include <Xemeiah/xpath/xpath.h>
#include <Xemeiah/xprocessor/xprocessor.h>
#include <Xemeiah/dom/nodeset.h>
#include <Xemeiah/io/stringreader.h>
#include <Xemeiah/trace.h>

#include <Xemeiah/auto-inline.hpp>

#include <math.h>

#define Log_XPathFunction Debug

namespace Xem
{
#if 1
#define __NotImplemented() do {	throwXPathException ( "NotImplemented : Function not implemented : '%s'\n", __FUNCTION__); } while (0)
#else
#define __NotImplemented() return;
#endif

#define __builtinKey(__key) ( node.getKeyCache().getBuiltinKeys().__key() )

#define __evalArgNodeSet(__idx) \
  NodeSet res##__idx; evalStep ( res##__idx, node, step->functionArguments[__idx] );

#define __evalArgNodeSetMaybe(__idx) \
  NodeSet res##__idx; \
  if ( step->functionArguments[__idx] != XPathStepId_NULL ) { evalStep ( res##__idx, node, step->functionArguments[__idx] ); }

#define __evalArgStr(__idx)						\
  NodeSet __res##__idx; String res##__idx;				\
  evalStep ( __res##__idx, node, step->functionArguments[__idx] ); \
  res##__idx = __res##__idx.toString ( );

#define __evalArgInteger(__idx)						\
  NodeSet __res##__idx; Integer res##__idx;				\
  evalStep ( __res##__idx, node, step->functionArguments[__idx] ); \
  res##__idx = __res##__idx.toInteger ( );

#define __evalArgInteger_checkNaN(__idx)						\
  NodeSet __res##__idx; Integer res##__idx;				\
  evalStep ( __res##__idx, node, step->functionArguments[__idx] ); \
  if ( __res##__idx.isNaN() ) { Log_XPathFunction ( "Number is NAN, escaping.\n" ); Number n = NAN; result.setSingleton ( n ); return ; } \
  res##__idx = __res##__idx.toInteger ( );


#define __evalArgNumber(__idx)						\
  NodeSet __res##__idx; Number res##__idx;				\
    evalStep ( __res##__idx, node, step->functionArguments[__idx] ); \
    res##__idx = __res##__idx.toNumber ( );

#define __evalArgNode(__idx)						\
  NodeSet __res##__idx; 							\
    evalStep ( __res##__idx, node, step->functionArguments[__idx] ); \
    NodeRef& res##__idx = __res##__idx.toNode ( );

#define __evalArgBool(__idx)						\
  NodeSet __res##__idx; 							\
    evalStep ( __res##__idx, node, step->functionArguments[__idx] ); \
    bool res##__idx = __res##__idx.toBool ( );


  void XPath::evalFunctionGenerateId ( __XPath_Functor_Args )
  {
    __evalArgNodeSet(0);
    if ( res0.size() == 0 )
        result.setSingleton ( String ( ) );
    else
        result.setSingleton ( res0.front().toNode().generateId() );
  }

  void XPath::evalFunctionDocument ( __XPath_Functor_Args )
  {
    __evalArgNodeSet ( 0 );
    if ( res0.size() == 0 )
      return;
    
    if ( res0.size() == 1 
        && res0.front().getItemType() == Item::Type_String 
        && res0.front().toString() == "" )
      {
        ElementRef defaultDocument = xproc.getDefaultDocument ();
        evalStep ( result, defaultDocument, step->nextStep );
        return;
      }
    String baseURI;
    bool hasBaseURI = false;
    if ( step->functionArguments[1] != XPathStepId_NULL )
      {
        __evalArgNodeSet ( 1 );
        if ( res1.size() > 0 )
          {
            baseURI = res1.front().toNode().getDocument().getDocumentBaseURI ();
            hasBaseURI = true;
          }
      }
    for ( NodeSet::iterator iter ( res0 ) ; iter ; iter++ )
      {
        String path = iter->toString();
        if ( !hasBaseURI && iter->isNode() )
          {
            hasBaseURI = true;
            baseURI = iter->toNode().getDocument().getDocumentBaseURI ();
            Log_XPathFunction ( "baseURI='%s', node=%s\n", baseURI.c_str(), iter->toNode().generateVersatileXPath().c_str() );
            if ( baseURI.size() == 0 )
              hasBaseURI = false;
          }
        if ( ! path.size() )
          {
            Warn ( "In XPath document() : skipping empty string.\n" );
            continue;
          }
        try 
          {
            ElementRef documentRoot = xproc.getDocumentRoot ( path, hasBaseURI ? &baseURI : NULL );
            AssertBug ( documentRoot, "Could not get file '%s'\n", path.c_str() );
            Log_XPathFunction ( "path='%s', document='%p', rootDocument='%llx'\n",
                path.c_str(), &(documentRoot.getDocument()), documentRoot.getElementId() );
            evalStep ( result, documentRoot, step->nextStep );
          }
        catch ( Exception* e )
          {
            detailException ( e, "While trying to get document '%s' (hasBaseURI=%s, baseURI=%s)\n", 
                path.c_str(), hasBaseURI ? "true" : "false", baseURI.c_str() );
            throw ( e );
          }
        hasBaseURI = 0;
      }
  }

  void XPath::evalFunctionCurrent ( __XPath_Functor_Args )
  {
    evalStep ( result, getBaseNode(), step->nextStep );
  }

  void XPath::evalFunctionMatches ( __XPath_Functor_Args )
  {
    __NotImplemented ();
#if 0
    __evalArgNodeSet(0);
    __evalArgNode(1);
    if ( res0.size() == 0 )
      {
    	throwXPathException ( "Could not get valid XPath !\n" );
      }
    for ( NodeSet::iterator iter(res0) ; iter ; iter ++ )
      {
        Item& item = (*iter);
        XPath matchesXPath = item.toXPath ( xproc.getBaseNode().getKeyCache() );
        Number effectivePriority;
        if ( matchesXPath.matches ( res1, effectivePriority ) )
          {
            result.setSingleton ( true );
            return;
          }
      }
     result.setSingleton ( false );
#endif 
  }

  void XPath::evalFunctionXPathIndirection ( __XPath_Functor_Args )
  {
    /*
     * TODO When dealing with Indirection, the node from which the xpath is computed
     * is rebased to the actual baseNode of the first XPath evaluation.
     * This may be problematic for cascaded indirections.
     * Here, we have to set a new xproc baseNode.
     */
    NodeRef& baseNode = getBaseNode();
    NodeSet res0; evalStep ( res0, baseNode, step->functionArguments[0] );
    Log_XPathFunction ( "XPath Indirection : result size %lu\n", (unsigned long) res0.size() );
    if ( res0.size() != 1 )
      {
        throwXPathException ( "XPathIndirection : called XPath expression is not single : "
            "found '%lu' matching nodes, baseNode is '%s'  !\n",
            (unsigned long) res0.size(), baseNode.generateVersatileXPath().c_str() );
      }
    if ( ! res0.front().isAttribute() )
      {
        throwXPathException ( "XPathIndirection : called XPath result has invalid type %x !\n",
            res0.front().getItemType() );
      }
    // XPath subXPath = res0.toXPath ( node.getKeyCache() );
    AttributeRef& attrRef = res0.front().toAttribute();
    ElementRef eltRef = attrRef.getElement();
    XPath subXPath ( xproc, eltRef, attrRef.getKeyId() );
    subXPath.evalStep ( result, node, subXPath.xpathSegment->firstStep );
  }
  
  void XPath::evalFunctionAVT ( __XPath_Functor_Args )
  {
    /*
     * \todo When dealing with Indirection, the node from which the xpath is computed
     * is rebased to the actual baseNode of the first XPath evaluation.
     * This may be problematic for cascaded indirections.
     * Here, we have to set a new xproc baseNode.
     */
    NodeRef& baseNode = getBaseNode();
    NodeSet res0; evalStep ( res0, baseNode, step->functionArguments[0] );
    Log_XPathFunction ( "XPath AVT Indirection : result size %lu\n", (unsigned long) res0.size() );
    if ( res0.size() == 0 )
      {
        throwXPathException ( "XPathAVT : result is empty !\n" );
      }

    AttributeRef& attrRef = res0.toNode().toAttribute();
    
    if ( attrRef.isAVT() )
      {
        ElementRef eltRef = attrRef.getElement();
        XPath subXPath ( xproc, eltRef, attrRef.getKeyId(), true );
        subXPath.evalStep ( result, node, subXPath.xpathSegment->firstStep );
        return;
      }
    result.setSingleton ( attrRef.toString() );
  }

  void XPath::evalFunctionHasVariable ( __XPath_Functor_Args )
  {
    KeyId keyId = evalStepAsKeyId ( node, step->functionArguments[0] );
    result.setSingleton ( xproc.hasVariable ( keyId ) );
  }

  void XPath::evalFunctionString ( __XPath_Functor_Args )
  {
    __evalArgStr ( 0 );
    result.setSingleton ( res0 );
  }

  void XPath::evalFunctionConcat ( __XPath_Functor_Args )
  {
    AssertBug ( step->functionArguments[0] != XPathStepId_NULL,
      "Null StepId for arg0\n" );
    AssertBug ( step->functionArguments[1] != XPathStepId_NULL,
      "Null StepId for arg1\n" );

    String cRes;
    
    for ( int arg = 0 ; arg < XPathStep_MaxFuncCallArgs ; arg++ )
      {
        if ( step->functionArguments[arg] == XPathStepId_NULL )
          break;
          
        NodeSet res;
        evalStep ( res, node, step->functionArguments[arg] );
        cRes += res.toString();
      }
    
    Log_XPathFunction ( "CONCAT : Final value '%s'\n", cRes.c_str() );
    result.setSingleton ( cRes );
  }

  void XPath::evalFunctionStartsWith ( __XPath_Functor_Args )
  {
    __evalArgStr ( 0 );
    __evalArgStr ( 1 );
    Log_XPathFunction ( "[STARTS-WITH] res0=%s, res1=%s\n", res0.c_str(), res1.c_str() );
    result.setSingleton ( stringStartsWith ( res0, res1 ) );
  }

  void XPath::evalFunctionEndsWith ( __XPath_Functor_Args )
  {
    __evalArgStr ( 0 );
    __evalArgStr ( 1 );
    Log_XPathFunction ( "[ENDS-WITH] res0=%s, res1=%s\n", res0.c_str(), res1.c_str() );
    result.setSingleton ( stringEndsWith ( res0, res1 ) );
  }

  void XPath::evalFunctionContains ( __XPath_Functor_Args )
  {
    __evalArgStr ( 0 );
    __evalArgStr ( 1 );
    Log_XPathFunction ( "[CONTAINS] : res0='%s', res1='%s'\n", res0.c_str(), res1.c_str() );
    result.setSingleton ( stringContains ( res0, res1 ) );
  }

  void XPath::evalFunctionSubstringBefore ( __XPath_Functor_Args )
  {
    __evalArgStr ( 0 );
    __evalArgStr ( 1 );

    StringSize found = res0.find ( res1, 0 );
    if ( found == String::npos )
      {
        result.setSingleton ( String() );
        return;
      }
    result.setSingleton ( res0.substr ( 0, found ) );
  }

  void XPath::evalFunctionSubstringAfter ( __XPath_Functor_Args )
  {
    __evalArgStr ( 0 );
    __evalArgStr ( 1 );

    StringSize found = res0.find ( res1, 0 );
    if ( found == String::npos )
      {
        result.setSingleton ( String() );
        return;
      }
    result.setSingleton ( res0.substr ( found + res1.size(), String::npos ) );
  }

  void XPath::evalFunctionSubstring ( __XPath_Functor_Args )
  {
    __evalArgStr ( 0 );
    __evalArgInteger ( 1 );

    if ( res1 == IntegerInfinity ) return;

    Integer size = String::npos;
    if ( step->functionArguments[2] != XPathStepId_NULL )
      {
        __evalArgNumber ( 2 );
        Log_XPathFunction ( "res2=%.32f\n", res2 );
        if ( isnormal ( res2 ) )
          size = Item::roundNumber (res2);
        else
          {
            size = 0;
          }
       }
    if ( res1 == 0 )
      {
      	res1 = 1;
      	size--;
      }
    else if ( res1 < 0 )
      {
        size -= res1;
        res1 = 1;
      }
    res1--;

    String subs = res0.substr ( res1, size );
    Log_XPathFunction ( "[%s] [%lld,%lld] -> [%s]\n", res0.c_str(), res1, size, subs.c_str() );
    
    result.setSingleton ( subs );
  }

  void XPath::evalFunctionStringLength ( __XPath_Functor_Args )
  {
    __evalArgStr ( 0 );
    result.setSingleton ( (Integer) res0.size() );
  }

  void XPath::evalFunctionNormalizeSpace ( __XPath_Functor_Args )
  {
    __evalArgStr ( 0 );
    String res = res0.normalizeSpace();
    Log_XPathFunction ( "[NORMALIZE] : res0='%s', res='%s'\n", res0.c_str(), res.c_str() );
    result.setSingleton ( res );
  }

  void XPath::evalFunctionTranslate ( __XPath_Functor_Args )
  {
    __evalArgStr ( 0 );
    __evalArgStr ( 1 );
    __evalArgStr ( 2 );

    String res;
    typedef std::map<int,int> CharMap;
    CharMap charMap;
    
    /*
     * We use .c_str() to have a non-malloced copy of the string, in order to avoid an extra strdup()
     * The fact is, all bsr have a shorter lifetime than the String they are based upon
     */
    StringReader bsr0 ( res0.c_str() );
    StringReader bsr1 ( res1.c_str() );
    StringReader bsr2 ( res2.c_str() );
    
    /*
     * First, build the integer map
     */
    while ( ! bsr1.isFinished() )
      {
        int char1 = bsr1.getNextChar();
        int char2 = bsr2.isFinished() ? 0 : bsr2.getNextChar();
        // Log_XPathFunction ( "char1=%d, char2=%d\n", char1, char2 );
        charMap[char1] = char2;
      }
    
    while ( ! bsr0.isFinished() )
      {
        int src = bsr0.getNextChar();
        CharMap::iterator iter = charMap.find ( src );
        if ( iter == charMap.end() )
          {
            res.appendUtf8 ( src );
          }
        else
          {
            int tgt = iter->second;
            if ( tgt )
              {
                res.appendUtf8 ( tgt );
              }
          }
      }
    /* Warn */ 
    Log_XPathFunction ( "[TRANSLATE] : res0='%s', res1='%s', res2='%s', res='%s'\n",
	   res0.c_str(), res1.c_str(), res2.c_str(), res.c_str() );
    result.setSingleton ( res );
  }

  void XPath::evalFunctionUpperCase ( __XPath_Functor_Args )
  {
    __evalArgStr ( 0 );
    result.setSingleton ( stringToUpperCase(res0) );
  }

  void XPath::evalFunctionLowerCase ( __XPath_Functor_Args )
  {
    __evalArgStr ( 0 );
    result.setSingleton ( stringToLowerCase(res0) );
  }
  
  void XPath::evalFunctionLast ( __XPath_Functor_Args )
  {
    Integer last = xproc.getLast();
    Log_XPathFunction ( "Last function : returns '%llu'\n", last );
    result.setSingleton ( last );
#if PARANOID
    if ( last == 0 )
    {
      Bug ( "." );
    }
#endif
  }

  void XPath::evalFunctionPosition ( __XPath_Functor_Args )
  {
    if ( ! xproc.hasIterator() )
      {
        Integer pos = 1;
        if ( node.isElement() )
          {
            ElementRef& eltRef = node.toElement ();
            for ( ElementRef prec = eltRef.getElder() ; prec ; prec = prec.getElder() )
              {
                pos++;
              }
          }
        result.setSingleton ( pos );
        return;
      }
    Integer pos = xproc.getPosition();
    Log_XPathFunction ( "Position function : returns '%llu'\n", pos );
    result.setSingleton ( pos );
  }

  void XPath::evalFunctionCount ( __XPath_Functor_Args )
  {
#if 1
    /*
     * Temporary shortcut when we are speaking of a variable
     */
    XPathStep* argStep = getStep(step->functionArguments[0]);
    if ( argStep->action == XPathAxis_Variable
        && argStep->nextStep == XPathStepId_NULL
        && argStep->predicates[0] == XPathStepId_NULL )
      {
        NodeSet* variable = xproc.getVariable(argStep->keyId);
        result.setSingleton ( (Integer) variable->size() );
        return;
      }
#endif
    __evalArgNodeSet(0);
    result.setSingleton ( (Integer) res0.size() );
  }

  void XPath::evalFunctionLocalName ( __XPath_Functor_Args )
  {
    __evalArgNodeSetMaybe ( 0 );
    if ( step->functionArguments[0] != XPathStepId_NULL && ! res0.size() )
      return;
    NodeRef& argNode = res0.size() ? res0.toNode() : node;
    if ( argNode.isElement() )
      {
        if ( argNode.toElement().isText() || argNode.toElement().isComment() )
          return;
        if ( argNode.toElement().isPI() )
          {
            result.setSingleton ( argNode.toElement().getPIName() );
            return;
          }
      }
    else
      {
        if ( argNode.getKeyId() == __builtinKey(nons.xmlns) )
          {
            result.setSingleton("");
            return;
          }
      }
    result.setSingleton ( argNode.getKeyCache().getLocalKey ( argNode.getKeyCache().getLocalKeyId ( argNode.getKeyId() ) ) );
  }

  void XPath::evalFunctionNameSpaceURI ( __XPath_Functor_Args )
  {
    __evalArgNodeSetMaybe ( 0 );
    if ( step->functionArguments[0] != XPathStepId_NULL && ! res0.size() )
      return;
    NodeRef& argNode = ( step->functionArguments[0] != XPathStepId_NULL ) ? res0.front().toNode() : node;
    if ( argNode.isAttribute() && argNode.toAttribute().getType() == AttributeType_NamespaceAlias )
      return;
    if ( argNode.isElement() 
      && ( argNode.toElement().isText() || argNode.toElement().isComment() || argNode.toElement().isPI() ) )
      return;
    result.setSingleton ( argNode.getKeyCache().getNamespaceURL ( argNode.getKeyCache().getNamespaceId ( argNode.getKeyId() ) ) );
  }

  void XPath::evalFunctionNameSpace ( __XPath_Functor_Args )
  {
    Bug ( "THIS IS DEPRECATED ! Shall not be called !\n" );
#if 0
    __evalArgStr ( 0 );
    if ( ! res0.size() )
    {
      throwXPathException ( "Empty source...\n" );
    }
    
    String ns = res0.substr ( 0, res0.find ( ':', 0 ) );
    result.setSingleton ( ns );
    Log_XPathFunction ( "Namespace for '%s' is '%s'\n", res0.c_str(), ns.c_str() );
#endif
  }

  void XPath::evalFunctionName ( __XPath_Functor_Args )
  {
    NodeSet argumentNodeSet;
    if ( step->functionArguments[0] != XPathStepId_NULL )
      {
        evalStep ( argumentNodeSet, node, step->functionArguments[0] );
        Log_XPathFunction ( "Argument : \n" );
        argumentNodeSet.log ();
        if ( ! argumentNodeSet.size() )
          {
            String emptyString;
            result.setSingleton ( emptyString );
            return;
          }
      }
    NodeRef& argumentNode = ( step->functionArguments[0] != XPathStepId_NULL ) ? argumentNodeSet.front().toNode() : node;

    KeyId nodeKeyId = argumentNode.getKeyId ();

    Log_XPathFunction ( "ArgumentNode = %s\n", argumentNode.generateVersatileXPath().c_str() );

#define __isKey(__key) ( nodeKeyId == __builtinKey(__key) )
#define __keyCache (node.getKeyCache())
    if ( __isKey(xemint.textnode) || __isKey(xemint.comment) || __isKey(xemint.root) )
      {
      
      }
    else if (  __isKey(xemint.root) )
      result.setSingleton ( String("#Root") );
    else if ( __isKey(xemint.textnode) )
      result.setSingleton ( String("#text") );
    else if ( __isKey(xemint.comment) )
      result.setSingleton ( String("#comment") );
    else if ( node.isElement() && node.toElement().isPI() )
      result.setSingleton ( node.toElement().getPIName() );
    else if ( argumentNode.isAttribute() && argumentNode.toAttribute().getType() == AttributeType_NamespaceAlias )
      {
        Log_XPathFunction ( "name() on NamespaceAlias attr %s => %s \n", argumentNode.generateVersatileXPath().c_str(),
            __keyCache.getLocalKey(__keyCache.getLocalKeyId(nodeKeyId)) );
        if ( nodeKeyId == __builtinKey(nons.xmlns) )
          result.setSingleton ( "" );
        else
          result.setSingleton ( __keyCache.getLocalKey(__keyCache.getLocalKeyId(nodeKeyId)) );
      }
    else if ( KeyCache::getNamespaceId(nodeKeyId) == __keyCache.getBuiltinKeys().xemint.ns() )
      {
        result.setSingleton ( __keyCache.getLocalKey(KeyCache::getLocalKeyId(nodeKeyId)) );
      }
    else
      {
        /*  
         * Warning ! We *must* use the source NamespaceAlias to re-generate this !
         */
        String nodeKey;
        if ( KeyCache::getNamespaceId(nodeKeyId) )
          {
            LocalKeyId prefixId = argumentNode.getNamespacePrefix ( __keyCache.getNamespaceId(nodeKeyId), true );
            if ( ! prefixId )
              { 
#if 0
                if ( __keyCache.getNamespaceId(nodeKeyId) == __keyCache.getBuiltinKeys().xslimpl.ns() )
                  {
                    prefixId = KeyCache::getLocalKeyId ( __keyCache.getBuiltinKeys().xslimpl.defaultPrefix() );
                  }
                else
                  {
                    throwException ( Exception, "Node %x (Document: role '%s', brid=[%llx:%llx]) has no default prefix for '%x'-'%s' in hierarchy.\n",
                      nodeKeyId, 
                      argumentNode.getDocument().getRole().c_str(), argumentNode.getDocument().getBranchRevId(),
                      __keyCache.getNamespaceId(nodeKeyId), __keyCache.getNamespaceURL(__keyCache.getNamespaceId(nodeKeyId)) );
                  }
#endif
              }
            else if ( prefixId == __keyCache.getBuiltinKeys().nons.xmlns() )
              {
                prefixId = 0;
              }
            if ( prefixId )
              nodeKey = __keyCache.getKey ( prefixId, KeyCache::getLocalKeyId ( nodeKeyId ) );
            else
              nodeKey = __keyCache.getLocalKey ( KeyCache::getLocalKeyId ( nodeKeyId ) );
          }
        else
          nodeKey = __keyCache.getLocalKey ( KeyCache::getLocalKeyId ( nodeKeyId ) );
        Log_XPathFunction ( "XPath name() node.getKeyId()=%x -> '%s'\n", nodeKeyId, nodeKey.c_str() );
        result.setSingleton ( nodeKey );
      }
  }

#define Log_XPathFunctionUniq Log_XPathFunction
  void XPath::evalFunctionUniq ( __XPath_Functor_Args )
  {
    __evalArgNodeSet(0);
    std::map<String, bool> uniqMap;
    Log_XPathFunctionUniq ( "Uniq : found %lu nodes.\n", (unsigned long) res0.size() );
    NodeSet myNodeSet;

    for ( NodeSet::iterator iter(res0) ; iter ; iter++ )
      {
        if ( iter->getItemType() == Item::Type_Element
         || iter->getItemType() == Item::Type_Attribute )
          {
            NodeRef& node = iter->toNode();
            NodeSet keyResult; evalStep ( keyResult, node, step->functionArguments[1] );
            if ( keyResult.size() != 1 )
              {
                throwXPathException ( "Uniq : Multiple keys for element.\n" );
              }
            String key = keyResult.toString ();
            if ( uniqMap.find ( key ) == uniqMap.end () )
              {
                uniqMap[key] = true;
                myNodeSet.pushBack ( node, true );
              }
          }
        else
          {
            NotImplemented ( "Uniq : Item not an element\n" );
          }
      }
    Log_XPathFunctionUniq ( "At the end of uniq : %lu elements\n", (unsigned long)  myNodeSet.size() );
    evalNodeSetPredicate ( result, myNodeSet, step ); 
  }

  void XPath::evalFunctionBoolean ( __XPath_Functor_Args )
  {
    __evalArgBool ( 0 );
    result.setSingleton ( res0 );
  }

  void XPath::evalFunctionNot ( __XPath_Functor_Args )
  {
    __evalArgBool ( 0 );
    Log_XPathFunction ( "bool=%d\n", res0 );
    result.setSingleton ( ! res0 );
  }

  void XPath::evalFunctionTrue ( __XPath_Functor_Args )
  {
    result.setSingleton ( true );
  }

  void XPath::evalFunctionFalse ( __XPath_Functor_Args )
  {
    result.setSingleton ( false );
  }

  void XPath::evalFunctionLang ( __XPath_Functor_Args )
  {
    __evalArgStr ( 0 );
    String lngToFind = stringToUpperCase ( res0 );
    for ( ElementRef elt = node.isElement() ? node.toElement() : node.toAttribute().getElement();
      elt ; elt = elt.getFather() )
      {
        AttributeRef xmlLang = elt.findAttr ( elt.getKeyCache().getBuiltinKeys().xml.lang(), AttributeType_String );
        if ( ! xmlLang ) continue;
        String lng = xmlLang.toString();
        StringSize separator = lng.find ( '-', 0 );
        if ( separator )
          {
            lng = lng.substr ( 0, separator );
          }
        lng = stringToUpperCase ( lng );
        if ( lng == lngToFind )
          {
            result.setSingleton ( true );
            return;
          }
      }
    result.setSingleton ( false );
  }

  void XPath::evalFunctionBooleanAnd ( __XPath_Functor_Args )
  {
    __evalArgBool ( 0 );
    if ( ! res0 )
      {
        result.setSingleton ( res0 );
        return;  
      }
    __evalArgBool ( 1 );
    result.setSingleton ( res1 );
  }

  void XPath::evalFunctionBooleanOr ( __XPath_Functor_Args )
  {
    __evalArgBool ( 0 );
    if ( res0 )
      {
        result.setSingleton ( res0 );
        return;  
      }
    __evalArgBool ( 1 );
    result.setSingleton ( res1 );
  }

  void XPath::evalFunctionNumber ( __XPath_Functor_Args )
  {
    __evalArgNumber ( 0 );
    Log_XPathFunction ( "number() : provided '%.32f', isnan=%s\n", res0, isnan(res0) ? "true" : "false" );
    result.setSingleton ( res0 );
  }

  void XPath::evalFunctionSum ( __XPath_Functor_Args )
  {
    __evalArgNodeSet ( 0 );
    Number sum = 0;
    for ( NodeSet::iterator iter(res0) ; iter ; iter++ )
      {
        sum += iter->toNumber();
      }
    result.setSingleton ( sum );
  }

  void XPath::evalFunctionFloor ( __XPath_Functor_Args )
  {
    __evalArgNumber ( 0 );
    result.setSingleton ( (Number) (floor (res0)) );
  }

  void XPath::evalFunctionCeiling ( __XPath_Functor_Args )
  {
    __evalArgNumber ( 0 );
    result.setSingleton ( (Number) (ceil (res0)) );
  }

  void XPath::evalFunctionRound ( __XPath_Functor_Args )
  {
    __evalArgNumber ( 0 );
    Number res = Item::roundNumber ( res0 );
    Log_XPathFunction ( "round(%.32f) = %.32f\n", res0, res );
    result.setSingleton ( res );
  }

#define __XPath_Func_Number_Operator(__operand) \
  __evalArgNodeSet(0); \
  __evalArgNodeSet(1); \
  if ( res0.isInteger() && res1.isInteger() ) \
  { \
    Integer res = res0.toInteger() __operand res1.toInteger(); \
    Log_XPathFunction ( "Operator as Integer : %lld Op %lld -> %lld\n", res0.toInteger(), res1.toInteger(), res ); \
    result.setSingleton ( res ); \
  } \
  else \
  { \
    Log_XPathFunction ( "Operator as Number : %.32f Op %.32f\n", res0.toNumber(), res1.toNumber() ); \
    result.setSingleton ( (Number) ( res0.toNumber() __operand res1.toNumber() ) ); \
  } 
  

  void XPath::evalFunctionPlus ( __XPath_Functor_Args )
  {
    __XPath_Func_Number_Operator(+);
  }

  void XPath::evalFunctionMinus ( __XPath_Functor_Args )
  {
    __XPath_Func_Number_Operator(-);
  }

  void XPath::evalFunctionMultiply ( __XPath_Functor_Args )
  {
    __XPath_Func_Number_Operator(*);
  }

  void XPath::evalFunctionDiv ( __XPath_Functor_Args )
  {
    __evalArgNumber(0);
    if ( ! isnormal(res0) && res0 != 0 )
      {
        result.setSingleton ( res0 );
        return;
      }
    __evalArgNumber(1);
    if ( isnan(res1) )
      result.setSingleton ( NAN );
    else
      result.setSingleton ( (Number) res0 / res1 );
  }

  void XPath::evalFunctionModulo ( __XPath_Functor_Args )
  {
    __evalArgInteger_checkNaN(0);
    __evalArgInteger_checkNaN(1);
    Log_XPathFunction ( "Modulo : res0=%lld, res1=%lld\n", res0, res1 );
    result.setSingleton ( (Integer) (res0 % res1) );
  }
  
  void XPath::evalFunctionIf ( __XPath_Functor_Args )
  {
    __evalArgBool(0);
    if ( res0 )
      evalStep ( result, node, step->functionArguments[1] );
    else
      evalStep ( result, node, step->functionArguments[2] );
  }
 
  void XPath::evalFunctionUnion ( __XPath_Functor_Args )
  {
    AssertBug ( step->functionArguments[2] == XPathStepId_NULL,
	    "Union set with a 3rd argument !\n" );
    
    evalStep ( result, node, step->functionArguments[0] );
    evalStep ( result, node, step->functionArguments[1] );

    Log_XPathFunction ( "Post-union result :\n" );
    result.log ();
  }

  void XPath::evalFunctionNodeSetTest ( __XPath_Functor_Args )
  {
    __evalArgNodeSet(0);
#if PARANOID
    res0.checkDocumentOrderness();
#endif
    Log_XPathFunction ( "Initial res0 log :\n" );
    res0.log ();
    evalNodeSetPredicate ( result, res0, step );
    Log_XPathFunction ( "Result log : \n" );
    result.log ();
  }

  void XPath::evalFunctionNodeSetRevert ( __XPath_Functor_Args )
  {
    evalFunctionNodeSetTest ( result, node, step );
  }

  void XPath::evalFunctionElementAvailable ( __XPath_Functor_Args )
  {
    KeyId keyId = evalStepAsKeyId ( node, step->functionArguments[0] );

    XProcessor::XProcessorHandler handler = getXProcessor().getXProcessorHandler ( keyId );
    bool elementAvailable = ( handler.module && handler.hook );
    
    if ( ! elementAvailable )
      {
        Warn ( "xpath:element-available() : Element '%s' is not available !\n",
           getXProcessor().getKeyCache().dumpKey ( keyId ).c_str() );
      }    
    result.setSingleton ( elementAvailable );
  }

  void XPath::evalFunctionFunctionAvailable ( __XPath_Functor_Args )
  {
    KeyId keyId = evalStepAsKeyId ( node, step->functionArguments[0] );

    bool functionAvailable = false;
    
    /*
     * First, check for built-in XPath functions
     */
    if ( KeyCache::getNamespaceId(keyId) == 0 )
      {
        const char* functionKey = getXProcessor().getKeyCache().getLocalKey ( keyId );
#define __XPath_Func(__name,__id,...)	\
        if ( strcmp ( functionKey, __name ) == 0 )	\
        {  functionAvailable = true;  result.setSingleton ( functionAvailable ); return; }
#include <Xemeiah/xpath/xpath-funcs.h>
#undef __XPath_Func
      }
    
    /**
     * Then, check for functions bound to our XProcessor
     */
    XProcessor::XProcessorFunction functionHook = getXProcessor().getXProcessorFunction ( keyId );
    
    functionAvailable = ( functionHook.module != NULL ) && ( functionHook.hook );
    
#if 0
    if ( ! functionAvailable )
      {
        Warn ( "xpath:function-available() : Function '%s' is not available !\n",
           getXProcessor().getKeyCache().dumpKey ( keyId ).c_str() );
      }
#endif
    result.setSingleton ( functionAvailable );
  }

  void XPath::evalFunctionSystemProperty ( __XPath_Functor_Args )
  {
    KeyId keyId = evalStepAsKeyId ( node, step->functionArguments[0] );
    getXProcessor().getXProcessorModuleProperty(result,keyId);
#if 0
    String res0 = getXProcessor().getKeyCache().dumpKey(keyId);
    Log ( "Pty : %s\n", res0.c_str() );
    if ( res0 == "(http://www.w3.org/1999/XSL/Transform):version" ) result.setSingleton( String("1.0" ) );
    else
      {
        NotImplemented ( "Function system-property()" );
      }
    return;
#endif
#if 0
    KeyId keyId = evalStepAsKeyId ( node, step->functionArguments[0] );
    NotImplemented ( "Function system-property()" );
    if ( keyId == node.getKeyCache().getBuiltinKeys().xsl.version() )
      result.setSingleton ( (Number) 1.0 );      
    else if ( keyId == node.getKeyCache().getBuiltinKeys().xsl.vendor() )
      result.setSingleton ( String("Xemeiah " __XEM_VERSION) );
    else if ( keyId == node.getKeyCache().getBuiltinKeys().xsl.vendor_url() )
      result.setSingleton ( String("http://www.xemeiah.org" ) );
    else
      {
        throwXPathException ( "Invalid or unknown system-property : '%x'\n", keyId );
      }
#endif
  }

  void XPath::evalFunctionUnparsedEntityURI ( __XPath_Functor_Args )
  {
    Document& doc = node.getDocument();
    __evalArgStr(0);
    Log_XPathFunction ( "Unparsed entity '%s'\n", res0.c_str() );
    result.setSingleton ( doc.getUnparsedEntity(res0));
  }
    
};
