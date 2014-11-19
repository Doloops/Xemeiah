#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/dom/attributeref.h>
#include <Xemeiah/dom/namespacelistref.h>
#include <Xemeiah/nodeflow/nodeflow.h>
#include <Xemeiah/nodeflow/nodeflow-stream-file.h>
#include <Xemeiah/xprocessor/xprocessor.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_EltSerialize Debug

namespace Xem
{
  void ElementRef::serializeNamespaceAliases ( NodeFlow& nodeFlow, bool anticipated )
  {
    NamespaceListRef excludedList = findAttr(getKeyCache().getBuiltinKeys().xemint.exclude_result_prefixes(), AttributeType_NamespaceList );
    NamespaceListRef extensionList = findAttr(getKeyCache().getBuiltinKeys().xemint.extension_element_prefixes(), AttributeType_NamespaceList );

    if ( ! hasNamespaceAliases() && !excludedList && !extensionList ) 
      return;

    Log_EltSerialize ( "serialize at %s\n", generateVersatileXPath().c_str() );

    if ( excludedList )
      {
        Log_EltSerialize ( "Has a excludedList\n" );
        for ( NamespaceListRef::iterator iter(excludedList) ; iter ; iter++ )
          {
            nodeFlow.removeAnticipatedNamespaceId ( iter.getNamespaceId() );
          }
      }
    if ( extensionList )
      {
        Log_EltSerialize ( "Has a extensionList\n" );
        for ( NamespaceListRef::iterator iter(extensionList) ; iter ; iter++ )
          {
            nodeFlow.removeAnticipatedNamespaceId ( iter.getNamespaceId() );
          }
      }

    for ( AttributeRef attr = getFirstAttr() ; attr ; attr = attr.getNext() )
      {
        if ( attr.getType() != AttributeType_NamespaceAlias )
          continue;

        NamespaceId nsId = attr.getNamespaceAliasId ();
        if ( nsId == getKeyCache().getBuiltinKeys().xmlns.ns() )
          continue;
        if ( ( excludedList && excludedList.has ( nsId ) )
           || ( extensionList && extensionList.has ( nsId ) ) )
           {
             continue;
           }
        Log_EltSerialize ( "Process Element Namespace Aliases : sourceRef=%llx:%x:'%s', key=%x:'%s', nsId=%s (%x), anticipated=%s\n",
            getElementId(), getKeyId(), getKey().c_str(), attr.getKeyId(), attr.getKey().c_str(),
            getKeyCache().getNamespaceURL(nsId), nsId,
            anticipated ? "true" : "false" );
        if ( attr.getKeyId() == getKeyCache().getBuiltinKeys().nons.xmlns()
              && nsId == 0 && nodeFlow.getDefaultNamespaceId() == 0 )
           continue;
        Log_EltSerialize ( "Setting namespace prefix : %s (%s) => %s\n",
              getKeyCache().dumpKey(attr.getKeyId()).c_str(),
              attr.getKey().c_str(),
              getKeyCache().getNamespaceURL(nsId) );
            nodeFlow.setNamespacePrefix ( attr.getKeyId(), nsId, anticipated );
      }
  }

  void ElementRef::ensureNamespaceDeclaration ( NodeFlow& nodeFlow, NamespaceId nsId, bool anticipated, LocalKeyId prefixId )
  {
    if ( nsId == 0 ) return;
    
    if ( ( nodeFlow.getNamespacePrefix ( nsId ) == 0
        || nodeFlow.getNamespacePrefix ( nsId ) == getKeyCache().getBuiltinKeys().nons.xmlns() )
         && nodeFlow.getDefaultNamespaceId() != nsId )
      {
        /*
         * NodeFlow has no idea of which prefix to use for me.
         * I must provide him my namespace prefix
         */

        Log_EltSerialize ( "Serialize at %s, nsId=%s (%x), nodeflow prefix=%s (%x)\n",
            generateVersatileXPath().c_str(), getKeyCache().getNamespaceURL(nsId), nsId,
            nodeFlow.getNamespacePrefix(nsId) ? getKeyCache().getLocalKey(nodeFlow.getNamespacePrefix(nsId)) : "(nil)",
            nodeFlow.getNamespacePrefix(nsId) );

        if ( prefixId == 0 )
          {
            prefixId = getNamespacePrefix ( nsId, true );
          }
        Log_EltSerialize ( "nsId=%x, prefixId=%x\n", nsId, prefixId );
        if ( prefixId == 0 )
          {
            // throwException ( Exception, "Could not get a prefix for namespace %x\n", nsId );
            Log_EltSerialize ( "At ElementRef '%s', no namespace prefix, setting default namespace %x\n", getKey().c_str(), nsId );
            nodeFlow.setNamespacePrefix ( getKeyCache().getBuiltinKeys().nons.xmlns(), nsId, anticipated );
          }
        else if ( prefixId == getKeyCache().getBuiltinKeys().nons.xmlns() )
          {
            Log_EltSerialize ( "  -> xmlns default !\n" );                
            nodeFlow.setNamespacePrefix ( prefixId, nsId, anticipated );
          }
        else
          {
            KeyId nsDeclarationId = KeyCache::getKeyId ( getKeyCache().getBuiltinKeys().xmlns.ns(), prefixId );
            Log_EltSerialize ( "New ns alias %x(%s) -> %x(%s)\n",
              nsDeclarationId, getKeyCache().dumpKey ( nsDeclarationId ).c_str(),
              nsId, getKeyCache().getNamespaceURL ( nsId ) );
            nodeFlow.setNamespacePrefix ( nsDeclarationId, nsId, anticipated );
          }
      }
    else
      {
        Log_EltSerialize ( "nsId=%x properly mapped to %x, default=%x\n", nsId,
           nodeFlow.getNamespacePrefix ( nsId ),
           nodeFlow.getDefaultNamespaceId () );
      }
  }


  void ElementRef::serialize(FILE* fp)
  {
    XProcessor xproc(getStore());
    NodeFlowStreamFile nodeFlow(xproc);
    nodeFlow.setFD(fp);
    serialize(nodeFlow);
  }

  /**
   * \todo Optimize this in favor of a non self-calling function (see ElementRef::toXML()).
   */
  void ElementRef::serialize ( NodeFlow& nodeFlow )
  {
    if ( isText() )
      {
        nodeFlow.appendText ( getText(), getDisableOutputEscaping() );
        return;
      }
    if ( isComment() )
      {
        nodeFlow.newComment ( getText() );
        return;
      }
    if ( isPI() )
      {
        nodeFlow.newPI ( getPIName(), getText() );
        return;
      }
    Log_EltSerialize ( "At elt=%x:%s - id=%llx\n", getKeyId(), getKey().c_str(), getElementId() );
    bool isRoot = ((*this) == getRootElement());
    if ( !isRoot )
      {
        serializeNamespaceAliases ( nodeFlow );

        NamespaceId nsId = getNamespaceId ( );

        ensureNamespaceDeclaration ( nodeFlow, nsId, true );

        nodeFlow.newElement ( getKeyId() );

        for ( AttributeRef attr = getFirstAttr() ; attr ; attr = attr.getNext() )
          {
            if ( attr.getAttributeType() == AttributeType_NamespaceAlias )
              {
                Log_EltSerialize ( "Skip NamespaceAlias attr serialization : %s (%x : %s) -> %s (%x)\n",
                    getKeyCache().dumpKey(attr.getKeyId()).c_str(),
                    attr.getKeyId(), attr.getKey().c_str(),
                    getKeyCache().getNamespaceURL(attr.getNamespaceAliasId()),
                    attr.getNamespaceAliasId() );
                continue;
              }
            if ( attr.isBaseType() )
              {
                Log_EltSerialize ( "Attr serialization : %s (%x : %s) -> %x\n",
                    getKeyCache().dumpKey(attr.getKeyId()).c_str(),
                    attr.getKeyId(), attr.getKey().c_str(),
                    attr.getNamespaceId() );
                if ( attr.getNamespaceId() != getKeyCache().getBuiltinKeys().xmlns.ns() )
                  {
                    ensureNamespaceDeclaration ( nodeFlow, attr.getNamespaceId(), false );
                  }
                nodeFlow.newAttribute ( attr.getKeyId(), attr.toString() );
              }
          }
      }
    for ( ElementRef child = getChild() ; child ; child = child.getYounger() )
      {
        child.serialize ( nodeFlow );
      }
    if ( !isRoot )
      nodeFlow.elementEnd ( getKeyId() );
  }
};
