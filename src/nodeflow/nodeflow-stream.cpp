#include <Xemeiah/nodeflow/nodeflow-stream.h>
#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/xprocessor/xprocessor.h>

#include <Xemeiah/auto-inline.hpp>

// #define __XEM_NODEFLOW_STREAM_DUMP_TEXT_BORDER // Option : delimit text output.

#define Log_NFS Debug
#define Log_NFS_Flush Debug

namespace Xem
{
  NodeFlowStream::NodeFlowStream ( XProcessor& xproc, BufferedWriter* writer_ )
  : NodeFlow(xproc), keyCache(xproc.getKeyCache()), namespaceAlias(xproc.getKeyCache())
  {
    hasAnticipatedNamespaceAliases = false;

    currentIndentation = 0;
    forceKeepText = false;
    atRootElement = true;
    hasQualifiedRootElement = false;
    hasWrittenXMLDeclaration = false;
    isOutputFormatExplicit = false;

    lastWasText = true;

    writer = writer_;
    currentMarkupKeyId = 0;
    currentElementKeyId = 0;
    Log_NFS ( "New NodeFlowStream at %p\n", this );
  }
  
  NodeFlowStream::~NodeFlowStream ()
  {
    delete ( writer );
    writer = NULL;
    Log_NFS ( "Delete NodeFlowStream at %p\n", this );
  }

  /*
   * ************************************** Attribute Writer Helpers ***********************************************
   */
     
  NodeFlowStream::AttributeToWrite::AttributeToWrite ( KeyId _keyId, char* _value )
  {
    keyId = _keyId;
    value = _value;
    name = NULL;
  }

  NodeFlowStream::AttributeToWrite::~AttributeToWrite ( )
  {
  }

  void NodeFlowStream::addAttributeToWrite ( KeyId keyId, const char* value )
  {
    AttributesMap::iterator iter = attributesMap.find ( keyId );
    if ( iter != attributesMap.end() )
      {
        AttributeToWrite* attr = iter->second;
        free ( attr->value );
        attr->value = strdup(value);
        return;
      }
    AttributeToWrite* attr = new AttributeToWrite ( keyId, strdup(value) );
    attributesList.push_back ( attr );
    attributesMap[keyId] = attr;
  }

  void NodeFlowStream::deleteAttributeToWrite ( AttributeToWrite* attr )
  {
    free ( attr->value );
    delete ( attr );  
  }

  void NodeFlowStream::deleteAttributesToWrite ()
  {
    attributesList.clear ();
    attributesMap.clear ();
  }  

  void NodeFlowStream::setAttributesNames ()
  {
    for ( AttributesList::iterator iter = attributesList.begin() ; iter != attributesList.end() ; iter++ )
      {
        AttributeToWrite* attr = *iter;
        attr->name = KeyCache::getNamespaceId(attr->keyId) == KeyCache::getNamespaceId(currentMarkupKeyId)
          ? keyCache.getLocalKey ( KeyCache::getLocalKeyId(attr->keyId) )
          : keyCache.getKey ( getNamespaceAlias(), attr->keyId );
      }
  }

  void NodeFlowStream::sortAttributes ( )
  {
    AttributeSorter attributeSorter;
    attributesList.sort ( attributeSorter );
  }  

  /*
   * ************************************** Output Helpers ***********************************************
   */
  void NodeFlowStream::doAddChar ( char c )
  {
    writer->addChar(c);
  }

  void NodeFlowStream::doAddStr ( const char* str )
  {
    writer->addStr(str);
  }

  void NodeFlowStream::serializeText ( const char* text, bool protectLTGT, bool protectQuote, bool protectAmp )
  {
    bool htmlEntity = ( outputMethod == OutputMethod_HTML );
    writer->serializeText(text,protectLTGT,protectQuote,protectAmp,htmlEntity,false);
  }

  void NodeFlowStream::flush() 
  {
    writer->flushBuffer();
  }

  void NodeFlowStream::setNamespacePrefix ( KeyId keyId, NamespaceId nsId, bool anticipated )
  {
    Log_NFS ( "[at %s (%x)] : Namespace Prefix %s (%x) -> %s (%x), anticipated=%d\n",
        currentMarkupKeyId ? getKeyCache().dumpKey(currentMarkupKeyId).c_str() : "(none)", currentMarkupKeyId,
        getKeyCache().dumpKey(keyId).c_str(), keyId,
        getKeyCache().getNamespaceURL(nsId), nsId,
        anticipated );
    if ( anticipated )
      {
        anticipatedNamespaceAliases[keyId] = nsId;
        return;
      }
    addAttributeToWrite ( keyId, getKeyCache().getNamespaceURL(nsId) );
    namespaceAlias.setNamespacePrefix ( keyId, nsId, true );
  }  
  
  void NodeFlowStream::writeXMLDeclaration ()
  {
    if ( outputMethod == OutputMethod_XML && ! omitXMLDeclaration && !hasWrittenXMLDeclaration )
      {
        String prolog = "<?xml version=\"1.0\" encoding=\"";
        prolog += encoding.c_str();
        prolog += "\"";
        if ( standalone )
          {
            prolog += "standalone=\"yes\"";
          }
        prolog += "?>";

        doAddStr(prolog.c_str());
        doAddStr ( "\n" );
        hasWrittenXMLDeclaration = true;
      }
  }

  void NodeFlowStream::newElement ( KeyId keyId, bool forceDefaultNamespace )
  {
    Log_NFS ( "New Element '%x' (ns=%s, local=%s), currentIndent=%d, outputMethod=%d\n", keyId, keyCache.getNamespaceURL(KeyCache::getNamespaceId(keyId)),
        keyCache.getLocalKey ( KeyCache::getLocalKeyId(keyId) ),
        currentIndentation, outputMethod );
    if ( outputMethod == OutputMethod_Text ) return;

    if ( !hasQualifiedRootElement )
      {
        requalifyOutput(keyId);
        Encoding enc = ParseEncoding(encoding);
        if ( enc == Encoding_Unknown )
          {
            encoding = "UTF-8";
            enc = Encoding_UTF8;
          }
        writer->setEncoding(enc);
        writeXMLDeclaration ();
        hasQualifiedRootElement = true;
      }
    closeElementSection ();

    namespaceAlias.push ();
    
    for ( AnticipatedNamespaceAliases::iterator iter = anticipatedNamespaceAliases.begin() ;
        iter != anticipatedNamespaceAliases.end() ; iter++ )
      {
        KeyId keyId = iter->first;
        NamespaceId nsId = iter->second;
        addAttributeToWrite ( keyId, getKeyCache().getNamespaceURL(nsId) );
        namespaceAlias.setNamespacePrefix ( keyId, nsId, true );        
      }
    anticipatedNamespaceAliases.clear ();

    currentMarkupKeyId = keyId;
    currentElementKeyId = keyId;
    // lastWasText = false;
  }

  void NodeFlowStream::newAttribute ( KeyId keyId, const String& value )
  {
#if PARANOID
    if ( KeyCache::getNamespaceId(keyId) == keyCache.getBuiltinKeys().xmlns.ns() 
        || keyId == keyCache.getBuiltinKeys().nons.xmlns() )
      {
        Warn ( "Event on a namespace attribute ! keyId=%x:'%s'\n", keyId, keyCache.dumpKey(keyId).c_str() );
      }
#endif // PARANOID

    if ( outputMethod == OutputMethod_Text ) return;
    if ( ! currentMarkupKeyId )
      {
        Warn ( "NodeFlow : silently ignoring attribute writing of '%s' outside of attribute section !\n",
          keyCache.getKey ( getNamespaceAlias(), keyId) );
        return;
      }
    addAttributeToWrite ( keyId, value.c_str() );
  }

  void NodeFlowStream::elementEnd ( KeyId keyId )
  {
    if ( outputMethod == OutputMethod_Text ) return;
    
    Log_NFS ( "elementEnd %s (%x)\n", getKeyCache().dumpKey(keyId).c_str(), keyId );
    
    if ( ! closeElementSection ( true ) )
      {
        if ( currentIndentation )
          {
            currentIndentation --;
          }
        else
          {
            Error ( "Trying to write an element while currentIndentation is zero !\n" );
          }
       
        if ( ! lastWasText && indent )
          doAddStr ( "\n" );
        doAddStr ( "</" );
        doAddStr ( keyCache.getKey ( getNamespaceAlias(), keyId ) );  
        doAddStr ( ">" );
      }
    if ( namespaceAlias.canPop() )
      {
        namespaceAlias.pop ();
      }
    else
      {
        Error ( "Could not pop namespaceAlias !\n" );
      }
    lastWasText = false;
    currentElementKeyId = 0;
  }

  bool NodeFlowStream::closeElementSection ( bool elementClose )
  {
    /*
     * If no currentMarkupKeyId is defined, that means we have no element to close
     */
    if ( ! currentMarkupKeyId )
      {
        return false;
      }
    
    Log_NFS ( "CloseElementSection : currentMarkupKeyId=%x, indent=%x, elementClose=%d\n",
      currentMarkupKeyId, currentIndentation, elementClose );

    /*
     * If the element to write has no namespace, but we have a default namespace set,
     * Then we have to reset this namespace
     */
    if ( KeyCache::getNamespaceId(currentMarkupKeyId) == 0
        && getNamespaceAlias().getDefaultNamespaceId() )
      {
        addAttributeToWrite ( getKeyCache().getBuiltinKeys().nons.xmlns(), "" ); 
        namespaceAlias.setDefaultNamespaceId ( 0 );        
      }
      
    if ( indent && !lastWasText ) doAddStr ( "\n" );
    doAddStr ( "<" );
    doAddStr ( keyCache.getKey ( getNamespaceAlias(), currentMarkupKeyId ) );

    atRootElement = false;

    setAttributesNames ();
    sortAttributes ();
    
    /*
     * Now serialize all attributes
     */
    for ( AttributesList::iterator iter = attributesList.begin() ; iter != attributesList.end() ; iter++ )
      {
        AttributeToWrite* attr = *iter;
        
        doAddChar ( ' ' );
        doAddStr ( attr->name );
        if ( outputMethod == OutputMethod_HTML && getKeyCache().isHTMLKeyDisableAttributeValue(attr->keyId) )
          {

          }
        else
          {
            doAddStr ( "=\"" );
            if ( outputMethod == OutputMethod_HTML && getKeyCache().isHTMLKeyURIAttribute(currentMarkupKeyId, attr->keyId ) )
              {
                writer->addStrProtectURI(attr->value);
              }
            else
              {
                Log_NFS ( "Serialize attr %s/%s outputMethod=%d '%s'\n",
                    getKeyCache().dumpKey(currentMarkupKeyId).c_str(),
                    getKeyCache().dumpKey(attr->keyId).c_str(),
                    outputMethod, attr->value );

                serializeText ( attr->value, true, true, true );
              }
          }
        doAddChar ( '\"' );

        deleteAttributeToWrite ( attr );
      }
    deleteAttributesToWrite ();
    
    if ( outputMethod == OutputMethod_HTML && getKeyCache().isHTMLKeyHead(currentMarkupKeyId) )
      {
        doAddStr ( ">" );
        currentIndentation ++;
        if ( indent )
          {
            doAddChar ( '\n' );
          }
        doAddStr ( "<META http-equiv=\"Content-Type\" content=\"text/html; charset=" );
        doAddStr ( encoding.c_str() );
        doAddStr ( "\">" );
      }
    else if ( elementClose )
      {
        if ( outputMethod == OutputMethod_XML )
          {
            doAddStr ( "/>" );
          }
        else
          {
            doAddStr ( "></" );
            doAddStr ( keyCache.getKey ( getNamespaceAlias(), currentMarkupKeyId ) );
            doAddStr ( ">" );
          }
      }
    else
      {
        doAddStr ( ">" );
        currentIndentation ++;
      }

    currentMarkupKeyId = 0;
    lastWasText = false;
    return true;  
  }  

  void NodeFlowStream::appendText ( const String& text, bool disableOutputEscaping )
  {
    if ( !text.c_str() || !*text.c_str() )
      return;
    closeElementSection ();
    if ( !forceKeepText && !currentIndentation && outputMethod != OutputMethod_Text )
      {
        Warn ( "Skipping text '%s' as I am not in element (currentIdentation=%d).\n", text.c_str(), currentIndentation );
        return;
      }
    if ( outputMethod == OutputMethod_HTML && getKeyCache().isHTMLKeyDisableTextProtect(currentElementKeyId) )
      {
        disableOutputEscaping = true;
      }
    else if ( outputMethod == OutputMethod_Text )
      {
        disableOutputEscaping = true;
      }
    Log_NFS ( "[NFS] text='%s', disableOutputEscaping=%d, current=%s (%x), outputMethod=%x\n",
            text.c_str(), disableOutputEscaping,
            currentElementKeyId ? getKeyCache().dumpKey(currentElementKeyId).c_str() : "(null)", currentElementKeyId,
            outputMethod
            );
    if ( isCDataSectionElement(currentElementKeyId) )
      {
        writer->serializeText(text.c_str(),false,false,false,false,true);
      }
    else
      {
        serializeText ( text.c_str(), !disableOutputEscaping, false, !disableOutputEscaping );
      }
    lastWasText = true;
  }

  void NodeFlowStream::newComment ( const String& comment )
  {
    if ( outputMethod == OutputMethod_Text ) return;
    writeXMLDeclaration ();
    closeElementSection ();
    doAddStr ( "<!--" );
    doAddStr ( comment.c_str() );  
    doAddStr ( "-->" );
    lastWasText = false;
  }

  void NodeFlowStream::newPI ( const String& name, const String& contents )
  {
    if ( outputMethod == OutputMethod_Text ) return;
    writeXMLDeclaration ();
    closeElementSection ();
    doAddStr ( "<?" );
    doAddStr ( name.c_str() );
    doAddStr ( " " );
    doAddStr ( contents.c_str() );  
    doAddStr ( "?>" ); 
    lastWasText = false;
  }


  ElementRef NodeFlowStream::getCurrentElement()
  {
    Bug ( "Invalid getCurrentElement() in a NodeFlowStream !!!\n" );

    return *((ElementRef*)NULL);  
  }

  const char* NodeFlowStream::getContents() 
  {
    return writer->getBuffer();
  }

  __ui64 NodeFlowStream::getContentsSize() 
  { 
    return writer->getBufferSize();
  }
};

