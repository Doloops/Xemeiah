#include <Xemeiah/kern/encoding.h>

#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/dom/attributeref.h>

#include <Xemeiah/kern/namespacealias.h>

#include <Xemeiah/xpath/xpathparser.h>

#include <Xemeiah/io/filewriter.h>

#include <Xemeiah/auto-inline.hpp>

#include <list>

#define Log_toXML Debug
#define Log_SerializeText Debug

// #define __XEM_ELEMENTREF_TOXML_DUMP_KEYID // Option : Dump the KeyId of each element
// #define __XEM_ELEMENTREF_TOXML_DUMP_TEXT_BORDER // Option : delimit text output.

namespace Xem
{

  /*
   * Fast Dump to XML
   * Commodity function : write to a FILE*
   */
  void ElementRef::toXML ( FILE* fp, ToXMLFlags flags )
  {
    toXML ( fileno(fp), flags );
  }

  void ElementRef::toXML ( int fd, ToXMLFlags flags )
  {
    String encoding = "UTF-8";
    FileWriter writer(fd);
    toXML ( writer, flags, encoding );
  }

  /**
   * Sorts attributes at serialization (aka normalize attributes lists) ;
   * Attributes are sorted according to their name as it is serialized.
   */  
  class AttributeSorter
  {
    NamespaceAlias& namespaceAlias;
  public:
    AttributeSorter( NamespaceAlias& _namespaceAlias ) : namespaceAlias(_namespaceAlias) {}
    ~AttributeSorter() {}
    
    bool operator() ( Item* leftI, Item* rightI )
    {
      AttributeRef& leftA = leftI->toAttribute();
      AttributeRef& rightA = rightI->toAttribute();
      KeyCache& keyCache = leftA.getKeyCache ();
      const char* left = NULL;
      const char* right = NULL;
      
      try
        {
          left = keyCache.getKey ( namespaceAlias, leftA.getKeyId() );
          right = keyCache.getKey ( namespaceAlias, rightA.getKeyId() );
        }
      catch ( Exception * e )
        {
          Error ( "Could not get key !\n" );
          delete ( e );
          return false;
        }        
      return stringComparator ( left, right, true ) < 0;
    }
  
  };

  void ElementRef::toXML ( BufferedWriter& writer )
  {
    const String encoding = "UTF-8";
    bool omitXMLDeclaration = false;
    bool indent = false;
    bool standalone = false;
    toXML ( writer,
          ElementRef::Flag_SortAttributesAndNamespaces
        | ( omitXMLDeclaration ? 0 : ElementRef::Flag_XMLHeader )
        | ( indent ? 0 : ElementRef::Flag_NoIndent )
        | ( standalone ? ElementRef::Flag_Standalone : 0 )
          , encoding
          );
  }

  /*
   * Fast Serialization mechanism (will be deprecated by NodeFlowStream some day)
   */
  void ElementRef::toXML ( BufferedWriter& writer, ToXMLFlags flags, const String& encodingName_ )
  {
    String encodingName = encodingName_;
    Encoding encoding = ParseEncoding(encodingName);
    if ( encoding == Encoding_Unknown )
      {
        encodingName = "UTF-8";
        encoding = Encoding_UTF8;
      }
    writer.setEncoding(encoding);

    Log_toXML ( "ToXML id=%llx, pos=%llx, key=%x:%s, flags=0x%x\n",
          getElementId(), getElementPtr(), getKeyId(), getKey().c_str(), flags );

    if ( flags & Flag_ChildrenOnly )
      {
        flags -= Flag_ChildrenOnly;
        bool writeXMLHeader = false;
        if ( flags & Flag_XMLHeader )
          {
            flags -= Flag_XMLHeader;
            writeXMLHeader = true;
          }
        for ( ElementRef child = getChild() ; child ; child = child.getYounger() )
          {
            Log_toXML ( "At child '%s'\n", child.getKey().c_str() );
            child.toXML ( writer, flags | ( writeXMLHeader ? Flag_XMLHeader : 0 ), encodingName );
            writeXMLHeader = false;
          }
        return;
      }

    bool writeElementId = (( flags & Flag_ShowElementId) == Flag_ShowElementId);
    // bool noWrite = (( flags & Flag_NoWrite) == Flag_NoWrite);
    bool noIndent = (( flags & Flag_NoIndent) == Flag_NoIndent);
    bool sortAttributes = (( flags & Flag_SortAttributesAndNamespaces) == Flag_SortAttributesAndNamespaces );
    bool outputHTML = (( flags & Flag_Output_HTML ) == Flag_Output_HTML );
    bool unprotectText = (( flags & Flag_UnprotectText ) == Flag_UnprotectText );
    bool standalone = (( flags & Flag_Standalone ) == Flag_Standalone );
    bool allowShortMarkup = ! outputHTML;

    Log_toXML ( "Encoding='%s', Flags : \n\twriteElementId=%d, \n\tnoIndent=%d, \n\tsortAttributes=%d, \n\toutputHTML=%d"
        "\n\tunprotectText=%d, \n\tstandalone=%d, \n\tallowShortMarkup=%d, \n",
        encodingName.c_str(),
        writeElementId, noIndent, sortAttributes, outputHTML,
        unprotectText, standalone, allowShortMarkup );

    int depth = 0; int depthTab = 0;

#define doAddChar(__c) writer.addChar(__c)
#define doAddStr(__s) writer.addStr(__s)
#define doFlush() writer.flushBuffer()
#define doPrintHex(__val_) \
 do { __ui64 __val = __val_; char preBuff[32]; sprintf ( preBuff, "%llx", __val ); doAddStr ( preBuff ); } while (0)

#define doAddDepth() \
 do { if ( ! noIndent ) { for ( int d = 0 ; d < depth * depthTab ; d++) doAddChar ( ' ' ); } } while (0)

    if ( flags & Flag_XMLHeader )
      {
        doAddStr ( "<?xml version=\"1.0\" encoding=\"" );
        doAddStr ( encodingName.c_str() );
        if ( standalone )
          {
            doAddStr ( "\" standalone=\"yes" );
          }
        doAddStr ( "\"?>" );
        doAddChar ( '\n' );
      }
    if ( flags & Flag_XHTMLDocType )
      {
        doAddStr ( "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1 Strict//EN\""
         " \"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">\n" );
      }

    ElementRef current (*this);

    NamespaceAlias namespaceAlias ( getKeyCache() );
    AttributeSorter attributeSorter ( namespaceAlias );
    Log_toXML ( "---- Setting key prefix aliasing at %p\n", &namespaceAlias );

    namespaceAlias.push ();

    const char* key = NULL, * value;
    bool lastWasText = true;
    bool skipMyYounger = false;

    while ( true )
      {
        Log_toXML ( "At elt=%llx, eltid=%llx, key=%x (native='%s', in-context='%s')\n",
          current.getElementPtr(), current.getElementId(), current.getKeyId(),
          current.getKeyCache().dumpKey(current.getKeyId()).c_str(), "()" );

        if ( current.isText() )
          {
            value = current.getText().c_str();
#ifdef __XEM_ELEMENTREF_TOXML_DUMP_TEXT_BORDER
            doAddStr ( "T [[" );
            ElementId id = current.getElementId();
            doPrintHex ( id );
            doAddChar ( ']' );
#endif
            bool mustProtect = !unprotectText;
            if ( outputHTML )
              {
                KeyId parentKeyId = current.getFather().getKeyId();
                if ( getKeyCache().isHTMLKeyDisableTextProtect(parentKeyId) )
                  {
                    mustProtect = false;
                  }
              }
            if ( current.getDisableOutputEscaping() )
              {
                mustProtect = false;
              }
            if ( current.getWrapCData () )
              {
                writer.serializeText ( value, false, false, false, false, true );
              }
            else
              {
                writer.serializeText ( value, mustProtect, false, mustProtect, outputHTML );
              }
#ifdef __XEM_ELEMENTREF_TOXML_DUMP_TEXT_BORDER
            doAddChar ( ']' );
#endif
            lastWasText = true;
          }
        else if ( current.isComment() )
          {
            doAddStr ( "<!-- " );
            value = current.getText().c_str();
            doAddStr ( value );
            doAddStr ( " -->" );
            lastWasText = false;
          }
        else if ( current.isPI() )
          {
            doAddStr ( "<?" );
            doAddStr ( current.getPIName().c_str() );
            doAddChar ( ' ' );
            doAddStr ( current.getText().c_str() );
            if ( outputHTML )
              doAddStr ( ">" );
            else
              doAddStr ( "?>" );
            Log_toXML ( "PI : this=%s, current=%s\n", generateVersatileXPath().c_str(), current.generateVersatileXPath().c_str() );
            if ( getFather() == current.getFather() )
              doAddChar ( '\n' );
            lastWasText = false;
          }
        else          
          {
            AssertBug ( current.isRegularElement(), "Current is no regular element. Unknown type for '%s'\n",
                current.generateVersatileXPath().c_str() );
                
            if ( ! lastWasText && ! noIndent )
              {
                if ( outputHTML &&
                    ( ( current.getFather() && getKeyCache().isHTMLKeySkipIndent ( current.getFather().getKeyId() ) )
                    || ( current.getElder() && getKeyCache().isHTMLKeySkipIndent ( current.getElder().getKeyId() ) )
                      )
                   )
                  {
                  }
                else
                  doAddChar ( '\n' );
              }
            doAddDepth ();
                        
            if ( current.hasNamespaceAliases() )
              {
                for ( AttributeRef attr = current.getFirstAttr () ; attr ; attr = attr.getNext() )
                  if ( attr.getType() == AttributeType_NamespaceAlias )
                    {
                      Log_toXML ( "At elt=%llx '%s' : Setting key prefix from '%s' (%x) to '%s' (%x)\n",
                        current.getElementId(), current.getKey().c_str(), 
                        attr.getKey().c_str(), attr.getKeyId(), 
                        getKeyCache().getNamespaceURL(attr.getNamespaceAliasId()), attr.getNamespaceAliasId() );
                      namespaceAlias.setNamespacePrefix ( attr.getKeyId(), attr.getNamespaceAliasId() );
                    }
              }
            if ( current.getNamespaceId() == 0 && namespaceAlias.getDefaultNamespaceId() )
              {
                Warn ( "Invalid default namespace : current=%s (%x), default ns=%x\n",
                  current.getKey().c_str(), current.getKeyId(), namespaceAlias.getDefaultNamespaceId() );
              }
            if ( current.getNamespaceId() && namespaceAlias.getNamespacePrefix ( current.getNamespaceId() ) == 0
                 && current.getNamespaceId() != namespaceAlias.getDefaultNamespaceId() )
              {
                Warn ( "Invalid default namespace : current=%s (%x), default ns=%x\n",
                  current.getKey().c_str(), current.getKeyId(), namespaceAlias.getDefaultNamespaceId() );
              }
            doAddChar ( '<' );
            try
              {
                key = getKeyCache().getKey ( namespaceAlias, current.getKeyId() );
              }
            catch ( Exception* e )
              {
                Error ( "Could not get element key !\n" );
                delete ( e );
                key = strdup(getKeyCache().dumpKey(current.getKeyId()).c_str());
              }
            Log_toXML ( "Key for current=%s (%x) is '%s'\n", current.getKey().c_str(), current.getKeyId(), key );
            doAddStr ( key );
#ifdef __XEM_ELEMENTREF_TOXML_DUMP_KEYID
            doAddChar ( '(' );
            doPrintHex ( current.getKeyId() );
            doAddChar ( '/' );
            doPrintHex ( namespaceAlias.getDefaultNamespaceId() );
            doAddChar ( ')' );
#endif
            if ( writeElementId )
              {
                doAddStr ( " elementId=\"0x" );
                ElementId id = current.getElementId();
                doPrintHex ( id );
                doAddChar ( '\"' );
              }
            /*
             * Printing Attributes
             */
            NodeSet attributes;
            Log_toXML ( "[ORDER] ---------------\n" );
            for ( AttributeRef attr = current.getFirstAttr() ; attr ; attr = attr.getNext() )
              {
                if ( ! attr.isBaseType() ) continue;
                if ( attr.getNamespaceId() )
                  {
                    if ( ! ( namespaceAlias.getNamespacePrefix(attr.getNamespaceId())
                        || namespaceAlias.getDefaultNamespaceId() == attr.getNamespaceId() ) )
                      {
                        Error ( "Will not be able to write namespace for : '%s'\n", attr.generateVersatileXPath().c_str() );
                      }
                  }
                attributes.pushBack ( attr );
                Log_toXML ( "[ORIGINAL ORDER] (%x) %s = '%s'\n", attr.getKeyId(), attr.getKey().c_str(), attr.toString().c_str() );
              }
            if ( sortAttributes )
              {
                attributes.sort ( attributeSorter );
              }
            for ( NodeSet::iterator iter (attributes) ; iter ; iter++ )
              {
                AttributeRef attr = iter->toAttribute ();
                Log_toXML ( "[SORTED ORDER] %s\n", attr.getKey().c_str() );
                try
                  {
                    key = getKeyCache().getKey ( namespaceAlias, attr.getKeyId() );
                  }
                catch ( Exception* e )
                  {
                    Error ( "Could not get attribute key !\n" );
                    delete ( e );
                  }
                  
                value = attr.toString().c_str();
                Log_toXML( "Attr value for '%s' is '%s' (orig '%s')\n", attr.getKey().c_str(), value, attr.toString().c_str() );
                doAddChar ( ' ' );
                doAddStr ( key );
#ifdef __XEM_ELEMENTREF_TOXML_DUMP_KEYID
                doAddChar ( '(' );
                doPrintHex ( attr.getKeyId() );
                doAddChar ( ')' );
#endif
                if ( outputHTML && getKeyCache().isHTMLKeyDisableAttributeValue(attr.getKeyId()) )
                  {
                    /*
                     * For some values of HTML attributes, we do not write contents
                     */
                    continue;
                  }
                doAddStr ( "=\"" );
                
                if ( outputHTML && getKeyCache().isHTMLKeyURIAttribute(current.getKeyId(), attr.getKeyId()))
                  writer.addStrProtectURI(value);
                else
                  writer.serializeText ( value, !outputHTML, true, true, outputHTML );
                doAddChar ( '\"' );
              }
            lastWasText = false;
          }

        /*
         * Printing Children
         */
        if ( current.isRegularElement() )
          {
            if ( current.getChild() )
              {
                doAddStr ( ">" );
                if ( outputHTML && getKeyCache().isHTMLKeyHead(current.getKeyId()) )
                  {
                    if ( !noIndent )
                      {
                        doAddChar ( '\n' );
                      }
                    doAddStr ( "<META http-equiv=\"Content-Type\" content=\"text/html; charset=" );
                    doAddStr ( encodingName.c_str() );
                    doAddStr ( "\">" );
                  }
                current = current.getChild ();
                depth++;
                namespaceAlias.push ();
                continue;
              }
            else if ( allowShortMarkup )
              {
                doAddStr ( "/>" );
              }
            else if ( outputHTML && getKeyCache().isHTMLKeySkipMarkupClose(current.getKeyId()) )
              {
                Log_toXML ( "Skipping markup close for %s\n", current.getKey().c_str() );
                doAddChar ( '>' );
              }
            else
              {
                doAddChar ( '>' );
                Log_toXML ( "Short closing for '%s'\n", current.getKey().c_str() );              
                key = getKeyCache().getKey ( namespaceAlias, current.getKeyId() );

                doAddChar ( '<' );
                doAddChar ( '/' );
                doAddStr ( key );
                doAddChar ( '>' );
              }
          }
        while ( skipMyYounger || ! current.getYounger() )
          {
            skipMyYounger = false;
            Log_toXML ( "Rewind : At elt=%llx, keyid=%x, eltid=%llx\n", 
                    current.getElementPtr(),
                    current.getKeyId(), current.getElementId()
                    );
            
            if ( current.getChild() )
              {
                if ( ! lastWasText && ! noIndent )
                  {
                    if ( outputHTML && getKeyCache().isHTMLKeySkipIndent(current.getKeyId()) )
                      {
                      }
                    else
                      {
                        doAddChar ( '\n' );
                        doAddDepth ();
                      }
                  }
                lastWasText = false;
                key = getKeyCache().getKey ( namespaceAlias, current.getKeyId() );
                doAddStr ( "</" );
                doAddStr ( key );
                doAddChar ( '>' );
              }

            if ( current.getElementPtr() == getElementPtr() ) 
              {
                if ( ! noIndent ) doAddChar ( '\n' );
                doFlush();
                return;
              }
            AssertBug ( current.getFather(), "Current has no father !\n" );
            namespaceAlias.pop ();
            current = current.getFather ();
            depth--;
          }

        if ( current.getChild() )
          {
            if ( ! lastWasText && ! noIndent )
              {
                doAddChar ( '\n' );
                doAddDepth ();
              }
            lastWasText = false;

            Log_toXML ( "Closing '%s'\n", current.getKey().c_str() );
            key = getKeyCache().getKey ( namespaceAlias, current.getKeyId() );
              
            doAddChar ( '<' );
            doAddChar ( '/' );
            doAddStr ( key );
            doAddChar ( '>' );
          }
        Log_toXML ( "Getting younger, this=%llx(%llx), current=%llx(%llx)\n",
              getElementPtr(), getElementId(), current.getElementPtr(), current.getElementId() );
        namespaceAlias.pop ();
        if ( current.getElementPtr() == getElementPtr() ) 
          { 
            doFlush();
            return;
          }
        namespaceAlias.push ();
        current = current.getYounger ();
      }
    Bug  ( "Shall not be here !\n" );
  }
};
