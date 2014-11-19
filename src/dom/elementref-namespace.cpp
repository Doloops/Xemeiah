#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/dom/attributeref.h>
#include <Xemeiah/kern/namespacealias.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_ElementNS Debug

namespace Xem
{
  AttributeRef ElementRef::addNamespacePrefix ( LocalKeyId prefixId, NamespaceId namespaceId )
  {
    if ( prefixId == getKeyCache().getBuiltinKeys().nons.xmlns() )
      {
        Bug ( "." );
      }
    KeyId keyId = KeyCache::getKeyId(getKeyCache().getBuiltinKeys().xmlns.ns(),prefixId);
    return addNamespaceAlias(keyId,namespaceId);
  }

  AttributeRef ElementRef::addNamespaceAlias ( KeyId keyId, NamespaceId namespaceId )
  {
    AttributeRef attr = findAttr ( keyId, AttributeType_NamespaceAlias );
    if ( attr )
      {
        if ( attr.getNamespaceAliasId() == namespaceId )
          {
            return attr;
          }
#if 0
        throwException ( Exception, "Already have a namespaceAlias declaration for %s (%x) -> %s (%x) , could not set %s (%x) namespace instead.\n", 
            attr.getKey(), keyId, 
            getKeyCache().getNamespaceURL ( attr.getNamespaceAliasId() ), attr.getNamespaceAliasId(),
            getKeyCache().getNamespaceURL ( namespaceId ), namespaceId );
#endif
      }
    else
      {
        Log_ElementNS ( "Insert new namespaceAlias : %x -> %x\n", keyId, namespaceId );
        attr = addAttr ( keyId, AttributeType_NamespaceAlias, sizeof(NamespaceId) );
      }
    NamespaceId* attrData = attr.getData<NamespaceId,Write> ();
    attr.alterData();
    *attrData = namespaceId;
    attr.protectData();
    return attr;
  }

  AttributeRef ElementRef::addNamespaceAlias ( const String& prefix, const String& namespaceURL )
  {
    KeyCache& keyCache = getDocument().getStore().getKeyCache();
    KeyId keyId = keyCache.getKeyId ( keyCache.getBuiltinKeys().xmlns.ns(), prefix.c_str(), true );
    NamespaceId nsId = keyCache.getNamespaceId ( namespaceURL.c_str() );
    return addNamespaceAlias ( keyId, nsId );
  }


  /**
   * \todo Replace this dummy loop by a flag on the ElementFlags.
   */
  bool ElementRef::hasNamespaceAliases ()
  {
    ElementSegment* me = getMe<Read> ();
    return ( me->flags & ElementFlag_HasNamespaceAlias );
#if 0
    for ( AttributeRef attr = getFirstAttr () ; attr ; attr = attr.getNext() )
      if ( attr.getType() == AttributeType_NamespaceAlias )
        return true;
    return false;  
#endif
  }

  NamespaceId ElementRef::getNamespaceAlias ( KeyId keyId, bool recursive )
  {
    for ( ElementRef elt = *this ; elt ; elt = elt.getFather() )
      {
        if ( elt.hasNamespaceAliases() )
          {
            AttributeRef attr = elt.findAttr ( keyId, AttributeType_NamespaceAlias );
            Log_ElementNS ( "findAttr at %s, keyId=%s (%x), recur=%d, attr=%x\n",
                elt.generateVersatileXPath().c_str(),
                getKeyCache().dumpKey(keyId).c_str(), keyId,
                recursive, attr ? attr.getKeyId() : 0 );
            if ( attr )
              return attr.getNamespaceAliasId ();
          }
        if ( ! recursive )
          {
            break;
          }
      }
    return 0;
  }

  NamespaceId ElementRef::getNamespaceIdFromPrefix ( LocalKeyId prefixId, bool recursive )
  {
    return getNamespaceAlias ( getKeyCache().buildNamespaceDeclaration ( prefixId ), recursive );
  }
  
  NamespaceId ElementRef::getDefaultNamespaceId ( bool recursive )
  {
    return getNamespaceAlias ( getKeyCache().getBuiltinKeys().nons.xmlns(), recursive );
  }

  LocalKeyId ElementRef::getNamespacePrefix ( NamespaceId nsId, bool recursive )
  {
    for ( ElementRef elt = *this ; elt ; elt = elt.getFather() )
      {
        if ( ! elt.hasNamespaceAliases() )
          {
            if (! recursive ) break;
            continue;
          }
        for ( AttributeRef attr = elt.getFirstAttr () ; attr ; attr = attr.getNext() )
          if ( attr.getType() == AttributeType_NamespaceAlias )
            {
              if ( attr.getNamespaceAliasId() == nsId )
                {
                  return getKeyCache().getLocalKeyId ( attr.getKeyId() );
                }
            }
        if ( ! recursive ) break;
      }
    return 0;
  }

  LocalKeyId ElementRef::generateNamespacePrefix ( NamespaceId nsId )
  {
    LocalKeyId prefixId = getNamespacePrefix(nsId, true );
    if ( prefixId ) return prefixId;

    char key[64];
    for ( int i = 0 ; i < 65536 ; i++ )
      {
        sprintf ( key, "ns%d", i );
        prefixId = getKeyCache().getKeyId(0, key, true);
        if ( getNamespaceIdFromPrefix ( prefixId ) )
          {
            Log_ElementNS ( "Already taken : prefix %s\n", key );
            continue;
          }
        addNamespacePrefix(prefixId, nsId);
        return prefixId;
      }
    throwException ( Exception, "Could not generate a prefix for namespace %x\n", nsId );
    return 0;
  }

  void ElementRef::copyNamespaceAliases ( ElementRef& source, bool recursive )
  {
    if ( recursive && source.getFather() )
      {
        ElementRef father = source.getFather();
        copyNamespaceAliases(father, recursive);
      }
    for ( AttributeRef attr = source.getFirstAttr() ; attr ; attr = attr.getNext() )
      {
        if ( attr.getAttributeType() != AttributeType_NamespaceAlias )
          continue;
        addNamespaceAlias(attr.getKeyId(), attr.getNamespaceAliasId());
      }
  }
};
