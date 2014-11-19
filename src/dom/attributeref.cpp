#include <Xemeiah/dom/attributeref.h>
#include <Xemeiah/dom/elementmapref.h>
#include <Xemeiah/dom/integermapref.h>
#include <Xemeiah/dom/blobref.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_Attribute Debug

namespace Xem
{
  AttributeRef::AttributeRef ( Document& _document, AllocationProfile allocProfile, KeyId keyId, AttributeType attributeType, DomTextSize size )
  : NodeRef ( _document )
  {
    setParentElementPtr ( NullPtr );
    __ui64 allocSize = sizeof ( AttributeSegment ) + size;
    AttributePtr attributePtr = getDocumentAllocator().getFreeSegmentPtr ( allocSize, allocProfile );
    setAttributePtr ( attributePtr );
    AttributeSegment * attr = getDocumentAllocator().getSegment<AttributeSegment,Write> ( getAttributePtr() );

    getDocumentAllocator().alter ( attr );
    attr->nextPtr = NullPtr;
    attr->keyId = keyId;
    attr->flag = attributeType;
    AttributeSegment_setSize(attr, size);
    getDocumentAllocator().protect ( attr );
  }

  AttributeRef::AttributeRef ( Document& _document, AllocationProfile allocProfile, KeyId keyId, const char* value )
  : NodeRef ( _document )
  {
    setParentElementPtr ( NullPtr );
    __ui64 size = 0;
    if ( value )
      {
        size = strlen ( value ) + 1;
        if ( size + sizeof(AttributeSegment) >= SegmentSizeMax )
          {
            throwException ( AttributeTooLongException,
                     "Attribute too long : '%lu' >= %lu.\n",
                     (unsigned long) strlen(value), (unsigned long) SegmentSizeMax );
          }
        Log_Attribute ( "size : %llu\n", size );
      }    
    
    __ui64 allocSize = sizeof ( AttributeSegment ) + size;
    AttributePtr attributePtr = getDocumentAllocator().getFreeSegmentPtr ( allocSize, allocProfile );
    setAttributePtr ( attributePtr );
    AttributeSegment * attr = getDocumentAllocator().getSegment<AttributeSegment,Write> ( getAttributePtr() );

    getDocumentAllocator().alter ( attr );
    attr->nextPtr = NullPtr;
    attr->keyId = keyId;
    attr->flag = AttributeType_String;
    AttributeSegment_setSize(attr, size);
    getDocumentAllocator().protect ( attr );

    if ( value )
      {
        char* attrData = getData<char,Write> ();
        alterData ();
        memcpy ( attrData, value, size );
        attrData[size-1] = '\0';
        protectData ();
        Log_Attribute ( "Copied value : '%s'\n", getData<char,Read>() );
        
        updateAttributeFlags ();
      }
  }
  
  void AttributeRef::updateAttributeFlags ()
  {
    AssertBug ( getDocument().isWritable(), "Document is not writable !\n" );
    const char* attrData = getData<char,Read> ();
    
    bool isAVT = false; // (strchr ( attrData, '{' ) != NULL) || ( strchr ( attrData, '}' ) != NULL );
    bool isWhitespace = true;
    
    for ( const char* c = attrData ; *c ; c++ )
      {
        if ( *c == '{' || *c == '}' ) 
          {
            isAVT = true;
          }
        if ( ! isspace(*c) ) 
          {
            isWhitespace = false;
          }
      }

    AttributeSegment * attr = getDocumentAllocator().getSegment<AttributeSegment,Write> ( getAttributePtr() );
    getDocumentAllocator().alter ( attr );
    attr->flag -= ( attr->flag & AttributeFlag_PropertiesMask );
    if ( isAVT )        attr->flag |= AttributeFlag_IsAVT;
    if ( isWhitespace ) attr->flag |= AttributeFlag_IsWhitespace;
    getDocumentAllocator().protect ( attr );
  }

  LocalKeyId AttributeRef::getNamespacePrefix ( NamespaceId nsId, bool recursive )
  {
    return getElement().getNamespacePrefix ( nsId, recursive );
  }

  String AttributeRef::getKey()
  {
    KeyId keyId = getKeyId ();
    if ( ! KeyCache::getNamespaceId ( keyId ) )
      {
        return getKeyCache().getLocalKey ( keyId );
      }
    String s;
    if ( KeyCache::getNamespaceId ( keyId ) == getKeyCache().getBuiltinKeys().xmlns.ns() )
      {
        s = "xmlns:";
      }
    else if ( KeyCache::getNamespaceId ( keyId ) == getKeyCache().getBuiltinKeys().xemint.ns() )
      {
        s = "xemint:";
      }
    else
      {
        ElementRef eltRef = getElement ();
        LocalKeyId prefixId = eltRef.getNamespacePrefix ( KeyCache::getNamespaceId ( keyId ), true );
        if ( prefixId )
          {
            s = getKeyCache().getLocalKey ( prefixId );
            s += ":";
          }
        else
          {
            stringPrintf(s,"ns__%x_:", KeyCache::getNamespaceId(keyId));
          }
      }
    s += getKeyCache().getLocalKey ( KeyCache::getLocalKeyId ( keyId ) );
    return s;
  }

  String AttributeRef::generateVersatileXPath ()
  {
    String xpath = getElement().generateVersatileXPath ();
    xpath += "/@";
    xpath += getKey();
    return xpath;
  }

  String AttributeRef::generateId()
  {
    String transmittable = getElement().generateId ();
    char id[64];
    sprintf ( id, "!%x", getKeyId() );
    transmittable += id;
    return transmittable;
  }
  
  void AttributeRef::rename ( KeyId keyId )
  {
#if PARANOID
    AssertBug ( getAttributePtr(), "Null Attribute Pointer !\n" );
#endif
    AttributeSegment* me = getMe<Write>();
    getDocumentAllocator().alter ( me );
    me->keyId = keyId;
    getDocumentAllocator().protect ( me );
  }

  void AttributeRef::setString ( const char* text )
  {
    AssertBug ( text, "Null text provided !\n" );
    AssertBug ( getType() == AttributeType_String, "Attribute is not a string !\n" );

    ElementRef elt ( *this );
    KeyId keyId = getKeyId ();

    /*
     * We have to delete the existing attribute, and create a new one.
     */
    elt.deleteAttr ( keyId );
    AttributeRef newAttr = elt.addAttr ( keyId, text );

    setParentElementPtr ( newAttr.getParentElementPtr() );
    setAttributePtr ( newAttr.getAttributePtr() );
  }

  void AttributeRef::setString ( const String& text )
  {
    setString ( text.c_str() );
  }

  bool AttributeRef::isBeforeInDocumentOrder ( NodeRef& nRef )
  {
    if ( nRef.isElement() )
    {
        if ( getElement() == nRef.toElement() )
          return false;
        return getElement().isBeforeInDocumentOrder ( nRef.toElement() );
    }
    AttributeRef& aRef = nRef.toAttribute();
    if ( getElement() != aRef.getElement() )
    {
        ElementRef eRef = aRef.getElement();
        return getElement().isBeforeInDocumentOrder ( eRef );
    }

#if PARANOID
    AssertBug ( getElement() == aRef.getElement(), "Shall not be here !\n" );
    AssertBug ( &(getDocument()) == &(nRef.getDocument()), "Shall not be here !\n" );
#endif    
#define __isNamespaceAttr(__attr ) ( (__attr).getType() == AttributeType_NamespaceAlias )
    /*
     * This implements the fact that Data Model requires that namespace nodes precede attribute nodes.
     */
    if ( __isNamespaceAttr(*this) && ! __isNamespaceAttr(aRef) )
        return true;
    if ( ! __isNamespaceAttr(*this) && __isNamespaceAttr(aRef) )
        return false;
    for ( AttributeRef attr = *this ; attr ; attr = attr.getNext() )
        if ( attr == aRef )
            return true;
    return false;
  }

  void AttributeRef::deleteAttributeSegment ()
  {
    getDocumentAllocator().freeSegment ( getAttributePtr(), sizeof(AttributeSegment) + getSize() );
    nodePtr = 0;
  }
      
  void AttributeRef::deleteAttribute ()
  {
    switch ( getAttributeType() )
      {
      case AttributeType_String:
      case AttributeType_NamespaceAlias:
      case AttributeType_XPath:
      case AttributeType_Integer:
      case AttributeType_KeyId:
      case AttributeType_Number:
        deleteAttributeSegment ();
        break;
      case AttributeType_SKMap:
        {
          SKMapHeader* header = getData<SKMapHeader,Read> ();
          switch ( header->config.skMapType )
          {
          case SKMapType_ElementMap:
            {
              ElementMapRef eltMapRef(*this);
              eltMapRef.deleteAttribute();
              break;
            }
          case SKMapType_IntegerMap:
            {
              IntegerMapRef integerMapRef(*this);
              integerMapRef.deleteAttribute();
              break;
            }
          case SKMapType_ElementMultiMap:
            {
              ElementMultiMapRef multiMapRef(*this);
              multiMapRef.deleteAttribute();
              break;
            }
          case SKMapType_Blob:
            {
              BlobRef blobRef(*this);
              blobRef.deleteAttribute ();
              break;
            }
          default:
            NotImplemented ( "SKMap deletion for type=%x !\n",
                header->config.skMapType );
          }
        }
        break;
      default:
        NotImplemented ( "Complex type attribute : %x.\n", getAttributeType() );
      }
  }
};
