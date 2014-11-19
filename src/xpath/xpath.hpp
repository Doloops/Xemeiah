#include <Xemeiah/xpath/xpath.h>
#include <Xemeiah/xpath/xpathparser.h>

#define Log_XPathHPP Debug

namespace Xem
{
  __INLINE void XPath::init ()
  {
#ifdef __XEM_XPATH_RUNTIME_GET_EXPRESSION
    expression = "(disabled expression)";
#endif    
    xpathSegment = NULL;
    stepBlock = NULL;
    resourceBlock = NULL;  
#ifdef __XEM_XPATH_RUNTIME_KEEP_NODEREF
    sourceNodeRef = NULL;
#endif
    temporaryParserForReadOnlyDocuments = NULL;
  }
  
  __INLINE void XPath::init ( ElementRef& elt, KeyId attrKeyId, bool isAVT )
  {
    init ();

#ifdef __XEM_XPATH_RUNTIME_KEEP_NODEREF
    sourceNodeRef = &elt;
#endif

    AttributeRef attrRef = elt.findAttr ( attrKeyId, AttributeType_XPath );
    if ( attrRef )
      {
        elt.getDocument().stats.numberOfXPathInstanciated++;
#ifdef __XEM_XPATH_RUNTIME_GET_EXPRESSION
        AttributeRef expressionAttr = elt.findAttr ( attrKeyId, AttributeType_String );
        if ( expressionAttr )
          {
            expression = expressionAttr.toString().c_str();
          }
        else
          {
            expression = "(unknown source)";
          }
#endif
        Log_XPathHPP ( "Load from store : '%s'\n", expression );
        loadFromStore ( attrRef );
#if 0
        logXPath ();  
#endif
        if ( ! xpathSegment ) Bug ( "Empty XPath !\n" );
        return;
      }
    parseFromAttribute ( elt, attrKeyId, isAVT );
    if ( ! xpathSegment ) Bug ( "Empty XPath !\n" );  
  }

  __INLINE XPath::XPath ( XProcessor& _xproc )
  : xproc(_xproc)
  {
    init ();
  }

  __INLINE XPath::XPath ( XProcessor& _xproc, XPathSegment* xpathSegment )
  : xproc(_xproc)
  {
    init ();
    loadFromPacked ( xpathSegment );
  }

  __INLINE XPath::XPath ( XProcessor& _xproc, XPathParser& xpathParser )
  : xproc(_xproc)
  {
    init ();
    loadFromPacked ( xpathParser.getPackedParsed() );       
  }

  __INLINE XPath::XPath ( XProcessor& _xproc, XPathParser* xpathParser, bool mayDelete )
  : xproc(_xproc)
  {
    init ();
    if ( mayDelete )
    {
      temporaryParserForReadOnlyDocuments = xpathParser;
    }
    loadFromPacked ( xpathParser->getPackedParsed() );       
  }

  __INLINE XPath::XPath ( XProcessor& _xproc, AttributeRef& attr )
  : xproc(_xproc)
  {
    init ();
    AssertBug ( attr.getType() == AttributeType_XPath,
      "Wrong attribute for XPath : %x\n", attr.getType() );

#ifdef __XEM_XPATH_RUNTIME_KEEP_NODEREF
    sourceNodeRef = &attr;
#endif

    loadFromStore ( attr );
  }
  
  __INLINE XPath::XPath ( XProcessor& _xproc, ElementRef& elt, KeyId attrKeyId, bool isAVT )
  : xproc(_xproc)
  {
    init ( elt, attrKeyId, isAVT );
  }  

  __INLINE XPath& XPath::operator= (XPath& xpath)
  {
    AssertBug ( xpath.temporaryParserForReadOnlyDocuments == NULL, "Could not copy a temporary XPath !\n" );

#ifdef __XEM_XPATH_RUNTIME_GET_EXPRESSION
    expression = xpath.expression;
#endif
    
    xpathSegment = xpath.xpathSegment;
    stepBlock = xpath.stepBlock;
    resourceBlock = xpath.resourceBlock;  
#ifdef __XEM_XPATH_RUNTIME_KEEP_NODEREF
    sourceNodeRef = xpath.sourceNodeRef;
#endif    
    temporaryParserForReadOnlyDocuments = xpath.temporaryParserForReadOnlyDocuments;
    return *this;
  }
  
  __INLINE void XPath::loadFromPacked ( XPathSegment* __xpathSegment )
  {
    xpathSegment = __xpathSegment;
    char* data = (char*) xpathSegment;
    Log_XPathHPP ( "XPathSegment at %p\n", xpathSegment );

    Log_XPathHPP ( "steps=%u (first=%u)\n",
        xpathSegment->nbStepId, xpathSegment->firstStep );

    __ui64 offset = sizeof ( XPathSegment );
    stepBlock = (XPathStepBlock*) &(data[offset]);
    offset += xpathSegment->nbStepId * sizeof ( XPathStep );
    resourceBlock = (XPathResourceBlock*) &(data[offset]);
    Log_XPathHPP ( "[LOAD XPATH] XPath seg=%p, steps=%p\n", xpathSegment, stepBlock );
  }
  
  __INLINE bool XPath::loadFromStore ( AttributeRef& attrRef )
  {
    XPathSegment* segment = attrRef.getData<XPathSegment,Read>();
    loadFromPacked ( segment );
    return true;
  }

  /*
   * Accession routines. 
   */
  __INLINE XPathStep* XPath::getStep ( XPathStepId stepId )
  {
#if PARANOID
    AssertBug ( stepBlock, "No step block !\n" );
    AssertBug ( stepId < XPathStepId_NULL, "Step too high.\n" );
#endif
    return &(stepBlock->steps[stepId]);
  }

  __INLINE const char* XPath::getResource ( XPathResourceOffset offset )
  {
    Log_XPathHPP ( "Getting resource at offset %lu : '%lu'\n",
        (unsigned long) offset, (unsigned long) resourceBlock[offset] );
    return &(resourceBlock[offset]);
  }

};
