#include <Xemeiah/nodeflow/nodeflow.h>
#include <Xemeiah/xprocessor/xprocessor.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_NodeFlow Debug

namespace Xem
{
  NodeFlow::NodeFlow ( XProcessor& _xproc )
  : xprocessor(_xproc), keyCache(_xproc.getKeyCache())
  {
    isBoundToXProcessor = false;
    previousNodeFlow = NULL;
    indent = false;
    standalone = false;
    isOutputFormatExplicit = false;
    omitXMLDeclaration = false;
    encoding = "UTF-8";
    outputMethod = OutputMethod_XML;
  }
    
  void NodeFlow::setPreviousNodeFlow ( NodeFlow* _previousNodeFlow )
  {
    Log_NodeFlow ( "Setting current NodeFlow to %p, last was %p\n", this, _previousNodeFlow );
    previousNodeFlow = _previousNodeFlow;
    isBoundToXProcessor = true;
  }

  NodeFlow::~NodeFlow ()
  {
    if ( isBoundToXProcessor ) 
      xprocessor.restorePreviousNodeFlow ( previousNodeFlow );
  }  

  void NodeFlow::setOutputFormat ( const String& method, const String& _encoding, bool _indent, bool _standalone, bool _omitXMLDeclaration )
  {
    isOutputFormatExplicit = true;
    indent = _indent;
    standalone = _standalone;
    omitXMLDeclaration = _omitXMLDeclaration;

    if ( method == "xml" )
      {
        outputMethod = OutputMethod_XML;
      }
    else if ( method == "text" )
      {
        omitXMLDeclaration = true;
        outputMethod = OutputMethod_Text;
      }
    else if ( method == "html" )
      {
        omitXMLDeclaration = true;
        outputMethod = OutputMethod_HTML;
      }

    encoding = _encoding;

    Log_NodeFlow ( "[NFDF] : output : method='%s', encoding='%s', indent='%s', standalone='%s', omitXMLDeclaration='%s'\n",
        method.c_str(), encoding.c_str(),
        indent ? "true" : "false", standalone ? "true" : "false", omitXMLDeclaration ? "true" : "false" );

  }

  void NodeFlow::requalifyOutput ( KeyId rootKeyId )
  {
    Log_NodeFlow ( "Requalify using '%s', current method is %d, explicit=%d\n",
        getKeyCache().dumpKey(rootKeyId).c_str(), outputMethod, isOutputFormatExplicit );
    if ( !isOutputFormatExplicit )
      {
        if ( outputMethod == OutputMethod_XML && getKeyCache().isHTMLKeyHTML (rootKeyId) )
          {
            Log_NodeFlow ( "Requalify from XML to HTML !\n" );
            setOutputFormat ( "html", encoding.c_str(), true, standalone, omitXMLDeclaration );
          }
      }
    else
      {
        if ( outputMethod == OutputMethod_HTML && KeyCache::getNamespaceId(rootKeyId) )
          {
            Log_NodeFlow ( "Requalify from XML to HTML !\n" );
            setOutputFormat ( "xml", encoding.c_str(), indent, standalone, omitXMLDeclaration );
          }
      }
  }

  void NodeFlow::verifyNamespacePrefix ( NamespaceId nsId )
  {
    if ( ! nsId ) return;
    if ( getNamespacePrefix ( nsId ) ) return;
    
    /*
     * Now we have to build up a new prefix
     */
    char prefixKey[256];
    for ( __ui32 id = 0 ; id < 256 ; id ++ )
      {
        sprintf ( prefixKey, "ns%d", id );
        LocalKeyId prefixId = getKeyCache().getKeyId ( 0, prefixKey, true );
        if ( getNamespaceIdFromPrefix ( prefixId ) )
          {
            continue;
          }
        Log_NodeFlow ( "Building prefix : associating prefix='%s' (%x) to nsId %x (%s)\n",
              prefixKey, prefixId, nsId, xprocessor.getKeyCache().getNamespaceURL ( nsId ) );
        KeyId declarationId = getKeyCache().buildNamespaceDeclaration ( prefixId );
        setNamespacePrefix ( declarationId, nsId, false );
        return;
      }
    Bug ( "Could not generate a prefix for anonymous NamespaceAlias nsId=%x\n", nsId );
  }

  void NodeFlow::processSequence ( XPath& xpath )
  {
    Bug ( "Not implemented : we shall fallback to a standard serialization approach here !\n" );
  }

#ifdef __XEM_NODEFLOW_HAS_CDATA_SECTION_ELEMENTS    
  void NodeFlow::setCDataSectionElement ( KeyId keyId )
  {
    cdataSectionElements[keyId] = true;    
  }

  bool NodeFlow::isCDataSectionElement ( KeyId keyId )
  {
    return (cdataSectionElements.find(keyId) != cdataSectionElements.end());
  }
#endif // #ifdef __XEM_NODEFLOW_HAS_CDATA_SECTION_ELEMENTS    

};
