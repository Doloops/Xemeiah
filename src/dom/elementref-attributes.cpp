/*
 * elementref-attributes.cpp
 * ElementRef functions dealing with attribute handling
 *
 *  Created on: 15 déc. 2009
 *      Author: francois
 */

#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/dom/attributeref.h>
#include <Xemeiah/dom/qnamelistref.h>
#include <Xemeiah/dom/namespacelistref.h>
#include <Xemeiah/xpath/xpath.h>
#include <Xemeiah/kern/format/journal.h>
#include <Xemeiah/dom/elementmapref.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_Element_Attrs Debug
#define Log_Element_GetAttrAsKeyId Debug

namespace Xem
{
  /*
   * Attribute Functions
   */
  AttributeRef ElementRef::addAttr (KeyId keyId, AttributeType type, DomTextSize size )
  {
    if ( ! getDocument().isWritable() )
      {
        Error ( "Document '%s' not writable !\n", getDocument().getDocumentTag().c_str() );
        Bug ( "." );
        throwException ( Exception, "Document '%s' not writable !\n", getDocument().getDocumentTag().c_str() );
      }
#if PARANOID
    AssertBug ( getDocument().isWritable(), "Document not writable !\n" );
#endif
    Log_Element_Attrs ( "addAttr elt=0x%llx, keyId=0x%x, type=0x%x, size=%lu (0x%lx), ctxt=%p, wr=%d\n",
        getElementId(), keyId, type, (unsigned long) size, (unsigned long) size,
        &(getDocument()), getDocument().isWritable() );

    if ( AttributeRef attr = findAttr ( keyId, type ) )
      {
        throwException ( Exception, "Element [%llx:%x:%s] : Already have an attribute %x:type=%x -> %s ! Element is : %s\n",
          getElementId(), getKeyId(), getKey().c_str(), keyId, type, attr.getKey().c_str(),
          generateVersatileXPath().c_str() );
      }

    AttributeRef finalAttr ( getDocument(), getAllocationProfile(), keyId, type, size );
    addAttr ( finalAttr );
    return finalAttr;
  }

  AttributeRef ElementRef::addAttr ( KeyId keyId, const char* value )
  {
    Log_Element_Attrs ( "Add attr '%x'='%s'\n", keyId, value );
    if ( keyId == getKeyCache().getBuiltinKeys().nons.xmlns() )
      {
        return addNamespaceAlias ( keyId, getKeyCache().getNamespaceId ( value ) );
      }
    if ( getKeyCache().getNamespaceId ( keyId ) == getKeyCache().getBuiltinKeys().xmlns.ns() )
      {
        Log_Element_Attrs ( "This attribute is a namespace declaration ! key=%x:'%s', value='%s'\n",
            keyId, getKeyCache().dumpKey(keyId).c_str(), value );
        return addNamespaceAlias ( keyId, getKeyCache().getNamespaceId ( value ) );
      }
    AttributeRef attr = findAttr ( keyId, AttributeType_String );
    if ( attr )
      {
        /*
         * \todo recode attribute reuse !
         */
        deleteAttr ( keyId );
      }

    AttributeRef newAttr ( getDocument(), getAllocationProfile(), keyId, value );
    addAttr ( newAttr );
    return newAttr;
  }

  AttributeRef ElementRef::addAttr ( KeyId keyId, const String& value )
  {
    Log_Element_Attrs ( "Add Attr string at %p/%p %s\n", &value, value.c_str(), value.c_str() );
    return addAttr ( keyId, value.c_str() );
  }

  AttributeRef ElementRef::addAttr ( XProcessor& xproc, KeyId keyId, const String& value )
  {
    Log_Element_Attrs ( "Add Attr string at %p/%p %s\n", &value, value.c_str(), value.c_str() );
    AttributeRef attr = findAttr ( keyId );
    bool modified = false;
    if ( attr )
      {
        modified = true;
        Log_Element_Attrs ( "Modify attr : '%s' => '%s'\n", attr.toString().c_str(), value.c_str() );

        eventAttribute(xproc,DomEventType_BeforeModifyAttribute,attr);
      }
    attr = addAttr ( keyId, value.c_str() );
    if ( modified )
      eventAttribute(xproc,DomEventType_AfterModifyAttribute,attr);
    else
      eventAttribute(xproc,DomEventType_CreateAttribute,attr);
    return attr;
  }

  AttributeRef ElementRef::copyAttribute ( AttributeRef& attrRef )
  {
    AttributeRef newAttr(getDocument());
    switch ( attrRef.getAttributeType() )
    {
    case AttributeType_NamespaceAlias:
      return addNamespaceAlias(attrRef.getKeyId(), attrRef.getNamespaceAliasId());
    case AttributeType_String:
    case AttributeType_Number:
    case AttributeType_Integer:
    case AttributeType_XPath:
    case AttributeType_KeyId:
      {
        newAttr = AttributeRef ( getDocument(), getAllocationProfile(), attrRef.getKeyId(), attrRef.getAttributeType(), attrRef.getSize() );
        addAttr ( newAttr );
        newAttr.alterData();
        memcpy ( newAttr.getData<void,Write>(), attrRef.getData<void,Read>(), attrRef.getSize() );
        newAttr.protectData();
        if ( attrRef.getAttributeType() == AttributeType_String )
          newAttr.updateAttributeFlags();
        Log_Element_Attrs ( "Copied attr(keyId=%x,key=%s,type=%x) size=%x\n",
            newAttr.getKeyId(), newAttr.getKey().c_str(), newAttr.getAttributeType(),
            newAttr.getSize() );
        return newAttr;
      }
    case AttributeType_SKMap:
      {
        SKMapHeader* header = attrRef.getData<SKMapHeader,Read> ();
        switch ( header->config.skMapType )
        {
        case SKMapType_ElementMap:
        case SKMapType_IntegerMap:
        case SKMapType_ElementMultiMap:
        case SKMapType_Blob:
        default:
          NotImplemented ( "Copying SKMap type %x\n", header->config.skMapType );
        }
      }
    default:
      NotImplemented("copyContents() for attribute type : %x\n", attrRef.getAttributeType() );
    }
    return newAttr;
  }

  AttributeRef ElementRef::addAttrAsInteger ( KeyId keyId, Integer value, AttributeType type )
  {
    AttributeRef attrRef = findAttr(keyId,type);
    if ( ! attrRef ) attrRef = addAttr ( keyId, type, sizeof(Integer) );
    Integer* valPtr = attrRef.getData<Integer,Write> ();
    attrRef.alterData ();
    *valPtr = value;
    attrRef.protectData ();

    Log_Element_Attrs ( "At '%s' : add integer %s (%x, type=%d) = %llx\n",
                        generateVersatileXPath().c_str(), getKeyCache().dumpKey(keyId).c_str(), keyId, type,
                        (unsigned long long) value );

#if PARANOID
    AssertBug ( *valPtr == value, "Diverging ! value=%lld, valPtr=%lld\n",
        value, *valPtr );
    Log_Element_Attrs ( "keyId=%x, value=%lld, ValPtr = %lld\n", keyId, value, *valPtr );
#endif
    return attrRef;
  }

  AttributeRef ElementRef::addAttrAsKeyId ( KeyId keyId, KeyId valueKeyId )
  {
    return addAttrAsInteger ( keyId, valueKeyId, AttributeType_KeyId );
  }

  AttributeRef ElementRef::addAttrAsQName ( KeyId keyId, KeyId qnameId )
  {
    String qname;
    if ( KeyCache::getNamespaceId(qnameId) )
      {
        LocalKeyId prefixId = generateNamespacePrefix(KeyCache::getNamespaceId(qnameId));
        Log_Element_Attrs ( "Generated prefixId=%x (%s) for namespace %s (handlerId=%s %x)\n",
            prefixId, getKeyCache().dumpKey(prefixId).c_str(),
            getKeyCache().getNamespaceURL(KeyCache::getNamespaceId(qnameId)),
            getKeyCache().dumpKey(qnameId).c_str(), qnameId );
        qname = getKeyCache().getKey(prefixId, KeyCache::getLocalKeyId(qnameId));
      }
    else
      {
        qname = getKeyCache().getLocalKey(qnameId);
      }
    AttributeRef newAttr = addAttr(keyId, qname);
    addAttrAsKeyId(keyId, qnameId);
#if PARANOID
    if ( getAttrAsKeyId(keyId) != qnameId )
      {
        throwException ( Exception, "Could not store qname %x (%s)\n", qnameId, qname.c_str() );
      }
#endif
    return newAttr;
  }

  AttributeRef ElementRef::addAttrAsElementId ( KeyId keyId, ElementId i )
  {
    return addAttrAsInteger ( keyId, i );
  }

  Number ElementRef::getAttrAsNumber ( KeyId keyId )
  {
    AttributeRef attrNumber = findAttr ( keyId, AttributeType_Number );
    if ( attrNumber ) return attrNumber.toNumber ();

    AttributeRef attrStr = findAttr ( keyId, AttributeType_String );
    if ( ! attrStr )
      {
        throwException ( Exception, "Could not get attribute '%x'\n", keyId );
      }
    Number n = atof ( attrStr.toString().c_str() );
    return n;
  }

  Integer ElementRef::getAttrAsInteger ( KeyId keyId )
  {
    AttributeRef attrInteger = findAttr ( keyId, AttributeType_Integer );
    if ( attrInteger ) return attrInteger.toInteger ();

    Bug ( "Element %s : keyId=%s (%x), type=%d, no integer value (has index=%s)\n",
                generateVersatileXPath().c_str(), getKeyCache().dumpKey(keyId).c_str(),
                keyId, attrInteger.getType(), findAttr(keyId) ? "yes" : "no" );

    String s = getAttr ( keyId );

    return s.toInteger ();
  }

  ElementId ElementRef::getAttrAsElementId ( KeyId keyId )
  {
    return getAttrAsInteger(keyId);
  }

  KeyId ElementRef::getAttrAsKeyId ( KeyId attrKeyId )
  {
    AttributeRef attr = findAttr ( attrKeyId, AttributeType_KeyId );
    if ( attr )
      {
        return attr.toInteger();
      }
    attr = findAttr ( attrKeyId, AttributeType_String );
    if ( !attr )
      {
        throwException ( Exception, "No attribute named '%s' at element : %s\n",
            getKeyCache().dumpKey(attrKeyId).c_str(), generateVersatileXPath().c_str() );
      }

    if ( !attr.toString().size() )
      return 0;
    Log_Element_GetAttrAsKeyId ( "getAttrAsKeyId for '%s'\n", attr.generateVersatileXPath().c_str() );
    return getKeyCache().getKeyIdWithElement ( *this, attr.toString(), false );
  }

  KeyId ElementRef::getAttrAsKeyId ( XProcessor& xproc, KeyId attrKeyId )
  {
    AttributeRef attr = findAttr ( attrKeyId, AttributeType_KeyId );
    if ( attr )
      {
        return attr.toInteger();
      }
    Log_Element_Attrs ( "At item=%s, keyId=%s", generateVersatileXPath().c_str(),
        getKeyCache().dumpKey(attrKeyId).c_str() );
    String contents = getEvaledAttr ( xproc, attrKeyId );

    return getKeyCache().getKeyIdWithElement ( *this, contents.c_str(), false );
#if 0
    if ( strchr ( contents.c_str(), ':' ) )
      {
        return getKeyCache().getKeyIdWithElement ( this, contents.c_str() );
      }

    if ( ! contents.size() ) return 0;
    KeyId keyId = getKeyCache().getKeyId ( 0, contents.c_str(), true );

    if ( ! keyId )
      {
        throwException ( Exception, "Invalid zero keyId, from contents='%s' !\n", contents.c_str() );
      }
    keyId = getKeyCache().getKeyId ( getNamespaceAlias ( getKeyCache().getBuiltinKeys().nons.xmlns(), true ), keyId );
    return keyId;
#endif
  }

  KeyIdList ElementRef::getAttrAsKeyIdList ( KeyId attrKeyId, bool useDefaultNamespace )
  {
    String value = getAttr(attrKeyId);
    KeyIdList qnames;

    std::list<String> tokens;
    value.tokenize ( tokens );
    for ( std::list<String>::iterator iter = tokens.begin() ; iter != tokens.end() ; iter++ )
      {
        String qname = *iter;
        KeyId keyId = getKeyCache().getKeyIdWithElement ( *this, qname, useDefaultNamespace );
        Log_Element_Attrs ( "Parsed : '%s' -> '%x'\n", qname.c_str(), keyId );
        qnames.push_back(keyId);
      }
    return qnames;
  }


  QNameListRef ElementRef::getAttrAsQNameList ( KeyId attrKeyId )
  {
    static const bool useDefaultNamespace = false;
    AttributeRef attr = findAttr(attrKeyId, AttributeType_QNameList );
    if ( attr ) return attr;
    attr = findAttr(attrKeyId, AttributeType_String);
    if ( ! attr )
      {
        // throwException(Exception, "Could not find attribute '%s' !", getKeyCache().dumpKey(attrKeyId).c_str() );
        return attr;
      }

    QNameListRef qnameList = QNameListRef::createQNameListRef(*this, attrKeyId);
    String qnames = attr.toString();
    std::list<String> tokens;
    qnames.tokenize(tokens);
    for ( std::list<String>::iterator token = tokens.begin() ; token != tokens.end() ; token++ )
      {
        KeyId keyId = getKeyCache().getKeyIdWithElement ( *this, *token, useDefaultNamespace );
        qnameList.put(keyId);
      }
    return qnameList;
  }

  void ElementRef::addQNameInQNameList ( KeyId attrKeyId, KeyId qnameId )
  {
    AttributeRef attrString = findAttr(attrKeyId, AttributeType_String);
    if ( ! attrString )
      {
        attrString = addAttr ( attrKeyId, "" );
      }
    QNameListRef qnameList = getAttrAsQNameList(attrKeyId);
    if ( qnameList.has(qnameId) )
      {
        /*
         * Nothing to do, qname is already part of the list
         */
        return;
      }
    qnameList.put(qnameId);

    /*
     * Now, update the String-version of the QName List
     */
    LocalKeyId prefixId = 0;
    if ( KeyCache::getNamespaceId(qnameId) )
      {
        prefixId = getNamespacePrefix(KeyCache::getNamespaceId(qnameId),true);
        if ( !prefixId )
          {
            prefixId = generateNamespacePrefix(KeyCache::getNamespaceId(qnameId));
            Assert ( prefixId, "Could not get prefixId ?\n" );
            Log_Element_Attrs ( "Generated prefixId=%x (%s) for namespace %s (keyId=%s %x)\n",
                prefixId, getKeyCache().dumpKey(prefixId).c_str(),
                getKeyCache().getNamespaceURL(KeyCache::getNamespaceId(qnameId)),
                getKeyCache().dumpKey(qnameId).c_str(), qnameId );
          }
      }
    const char* qname = NULL;
    if ( prefixId )
      qname = getKeyCache().getKey(prefixId, KeyCache::getLocalKeyId(qnameId));
    else
      qname = getKeyCache().getLocalKey(KeyCache::getLocalKeyId(qnameId));
    String qnames = attrString.toString();
    qnames += " ";
    qnames += qname;
    Log_Element_Attrs ( "Set String : %s = '%s'\n", attrString.getKey().c_str(), qnames.c_str() );
    deleteAttr(attrKeyId, AttributeType_String);
    addAttr(attrKeyId,qnames);
  }

  NamespaceListRef ElementRef::getAttrAsNamespaceList ( KeyId namespaceListKeyId, AttributeRef& sourceAttr )
  {
    NamespaceListRef nsList = findAttr(namespaceListKeyId,AttributeType_NamespaceList);
    if ( nsList ) return nsList;

    std::list<String> tokens;
    sourceAttr.toString().tokenize(tokens);

    Log_Element_Attrs ( "Build : String(%s) => NamespaceList(%s)\n",
        sourceAttr.toString().c_str(),
        getKeyCache().dumpKey(namespaceListKeyId).c_str() );

    Log_Element_Attrs( "Prefixes : %s\n", sourceAttr.toString().c_str() );

    nsList = addSKMap(namespaceListKeyId,SKMapType_NamespaceList);

    for ( std::list<String>::iterator token = tokens.begin() ; token != tokens.end() ; token++ )
      {
        KeyId keyId = 0;
        if ( *token == "#default" )
          {
            keyId = getKeyCache().getBuiltinKeys().nons.xmlns();
          }
        else
          {
            KeyId nsKeyId = getKeyCache().getKeyId ( 0, token->c_str(), true );
            AssertBug ( nsKeyId, "Could not get namespace prefixId %x:'%s'\n", nsKeyId, token->c_str() );
            keyId = getKeyCache().getKeyId ( getKeyCache().getBuiltinKeys().xmlns.ns(), nsKeyId );
          }
        NamespaceId nsId = getNamespaceAlias(keyId, true);
        Log_Element_Attrs ( "getAttrAsNamespaceList : %s => %s\n", getKeyCache().dumpKey(keyId).c_str(), getKeyCache().getNamespaceURL(nsId) );
        nsList.put(nsId);
      }
    return nsList;
  }

  void ElementRef::addNamespaceInNamespaceList ( KeyId namespaceListKeyId, KeyId source, NamespaceId nsId )
  {
    Bug ( "addNamespaceInNamespaceList NOT IMPLEMENTED.\n" );
    NotImplemented ( "addNamespaceInNamespaceList" );
  }

  String ElementRef::getAttr ( KeyId keyId )
  {
    AttributeRef attr = findAttr ( keyId, AttributeType_String );
    if ( attr ) return attr.toString ();

    throwDOMException ( "Element '%s' (keyId=%x, elementId=0x%llx) : could not find attr %s ('%x')\n",
        getKey().c_str(), getKeyId(), getElementId(),
        getKeyCache().dumpKey(keyId).c_str(), keyId );
    return String();
  }

  String ElementRef::getEvaledAttr ( XProcessor& xproc, KeyId keyId )
  {
    Log_Element_Attrs ( "*********** Get Evaled Attr elt=%s, keyId=%x *************\n",
      getKey().c_str(), keyId );
    AttributeRef baseAttr = findAttr ( keyId, AttributeType_String );

    Log_Element_Attrs ( "Found attr %llx/%llx\n", baseAttr.getParentElementPtr(), baseAttr.getAttributePtr() );

    if ( ! baseAttr )
      {
#if 0
        Log_Element_Attrs ( "At getEvaledAttr : element '%s' has no attribute '%s'\n",
               getKey().c_str(), getDocument().getStore().getKeyCache().dumpKey ( keyId ).c_str() );
        for ( AttributeRef attr = getFirstAttr () ; attr ; attr = attr.getNext() )
          {
            Log_Element_Attrs ( "\tAttribute : '%s' (keyId=%x) (type=0x%x) = '%s'\n",
                attr.getKey().c_str(), attr.getKeyId(), attr.getType(), attr.toString().c_str() );
          }
#endif
        return String();
      }

    Log_Element_Attrs ( "****** Found attr '%s' -> '%s' **********\n", baseAttr.getKey().c_str(), baseAttr.toString().c_str() );
    if ( ! baseAttr.isAVT() )
      {
        return baseAttr.toString ();
      }
    XPath attrXPath(xproc, *this, keyId, true);
    return attrXPath.evalString ();
  }


  bool ElementRef::deleteAttr ( KeyId keyId )
  {
    bool res = false;
    while ( deleteAttr ( keyId, 0 ) ) res = true;
    return res;
  }

  bool ElementRef::deleteAttr ( KeyId keyId, AttributeType attrType )
  {
    AttributeRef attr = getFirstAttr ();
    if ( ! attr ) return false;
    AttributeRef last ( getDocument(), NullPtr, NullPtr );

    Log_Element_Attrs ( "deleteAttr keyId=%x, attrType=%x\n", keyId, attrType );

    while ( attr )
      {
        if ( attr.getKeyId() == keyId && ( ! attrType || ( attr.getType() == attrType ) ) )
          break;
        last = attr;
        attr = attr.getNext ();
      }
    if ( ! attr )
      {
        return false;
      }

    if ( attr == getFirstAttr() )
      {
        ElementSegment* me = getMe<Write> ();
        getDocumentAllocator().alter ( me );
        __checkElementFlag_HasAttributesAndChildren ( me->flags );
        me->attributesAndChildren.attrPtr = attr.getMe<Read>()->nextPtr;
        getDocumentAllocator().protect ( me );
      }
    else
      {
        AttributeSegment* lastSeg = last.getMe<Write>();
        getDocumentAllocator().alter ( lastSeg );
        lastSeg->nextPtr = attr.getMe<Read>()->nextPtr;
        getDocumentAllocator().protect ( lastSeg );
      }
    attr.deleteAttribute();
    return true;
  }

  void ElementRef::deleteAttributes ( XProcessor& xproc )
  {
    ElementSegment* me = getMe<Write>();
    __checkElementFlag_HasAttributesAndChildren ( me->flags );
    AttributePtr attrPtr;

    /**
     * First, iterate to trigger all DeleteAttribute events
     */
    for ( attrPtr = me->attributesAndChildren.attrPtr ; attrPtr ; )
      {
        AttributeRef attr ( getDocument(), getElementPtr(), attrPtr );
        attrPtr = attr.getMe<Read>()->nextPtr;

        if ( attr.isBaseType() )
          {
            eventAttribute(xproc,DomEventType_DeleteAttribute,attr);
          }
      }

    /**
     * Then, iterate again, but to really delete all attributes
     */
    attrPtr = me->attributesAndChildren.attrPtr;

    while ( attrPtr )
      {
        AttributeRef attr ( getDocument(), getElementPtr(), attrPtr );
        SegmentPtr nextAttrPtr = attr.getMe<Read>()->nextPtr;
        attr.deleteAttribute ();
        attrPtr = nextAttrPtr;
      }
  }
};
