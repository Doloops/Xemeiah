#define Log_AttributeHPP Debug

#include <math.h>

namespace Xem
{
  __INLINE AttributePtr AttributeRef::getAttributePtr() const
  {
    return nodePtr;
  }

  __INLINE void AttributeRef::setAttributePtr ( AttributePtr _ptr )
  {
    nodePtr = _ptr;
  }

  __INLINE ElementPtr AttributeRef::getParentElementPtr() const
  {
    return parentElementPtr__;
  }

  __INLINE void AttributeRef::setParentElementPtr ( ElementPtr _ePtr )
  {
    parentElementPtr__ = _ePtr;  
  }

  __INLINE AttributeRef::AttributeRef ( Document& _document ) 
    : NodeRef ( _document )
  {
    Log_AttributeHPP ( "New attr at %p\n", this );
    setParentElementPtr ( NullPtr );
    setAttributePtr ( NullPtr );
  }
  
  __INLINE AttributeRef::AttributeRef 
  ( Document& _document, ElementPtr __parentElementPtr, AttributePtr __attributePtr ) 
    : NodeRef ( _document )
  {
    setParentElementPtr ( __parentElementPtr );
    setAttributePtr ( __attributePtr );
    Log_AttributeHPP ( "New attr at %p\n", this );
  }
  
  __INLINE AttributeRef::AttributeRef ( const AttributeRef& __a )
    : NodeRef ( __a.document )
  {
    setParentElementPtr ( __a.getParentElementPtr() );
    setAttributePtr ( __a.getAttributePtr() );
    Log_AttributeHPP ( "New attr at %p\n", this );
  }
  
  __INLINE AttributeRef::AttributeRef ( ElementRef& __p )
    : NodeRef ( __p.getDocument() )
  {
    setParentElementPtr ( __p.getFirstAttr().getParentElementPtr () );
    setAttributePtr ( __p.getFirstAttr().getAttributePtr() );
    Log_AttributeHPP ( "New attr at %p\n", this );
    if ( (*this) && !isBaseType() )
      {
        setAttributePtr ( getNext().getAttributePtr () );
      }
  }

  __INLINE AttributeRef::~AttributeRef ( )
  {
    Log_AttributeHPP ( "Delete attr at %p\n", this );
  }

  __INLINE KeyId AttributeRef::getKeyId() 
  { 
    return getMe<Read>()->keyId; 
  }

  __INLINE NamespaceId AttributeRef::getNamespaceId ()
  {
    return getKeyCache().getNamespaceId ( getKeyId() );
  }

  __INLINE LocalKeyId AttributeRef::getLocalKeyId()
  {
    return getKeyCache().getLocalKeyId ( getKeyId() );
  }

  __INLINE const char* AttributeRef::getKey( NamespaceAlias& nsAlias )
  { 
    return getKeyCache().getKey ( nsAlias, getKeyId() ); 
  }

  __INLINE AttributeType AttributeRef::getType() 
  { 
    return getMe<Read>()->flag & AttributeType_Mask; 
  }

  __INLINE bool AttributeRef::isAVT() 
  { 
    return getMe<Read>()->flag & AttributeFlag_IsAVT; 
  }

  __INLINE bool AttributeRef::isWhitespace() 
  { 
    return getMe<Read>()->flag & AttributeFlag_IsWhitespace; 
  }

  __INLINE ElementRef AttributeRef::getElement ()
  {
    AssertBug ( *this, "Null attribute !\n" );
    return ElementRef ( *this );
  }  
  
  __INLINE AttributeRef::operator bool()  const
  { 
#if PARANOID
    if ( ! getParentElementPtr() )
      {
        AssertBug ( ! getAttributePtr(),
              "ElementPtr not defined, but attribute defined !\n" );
      }
#endif
    return getAttributePtr() != NullPtr; 
  }

  __INLINE bool AttributeRef::isBaseType()
  { 
    return __isAttributeBaseType(getType() ); 
  }



  __INLINE AttributeRef& AttributeRef::operator= 
  ( const AttributeRef& aRef )
  {
    AssertBug ( &document == &(aRef.document), "Attribute assignation : documents do not match !\n" );
    setParentElementPtr ( aRef.getParentElementPtr () );
    setAttributePtr ( aRef.getAttributePtr () );
    return *this;
  }

  __INLINE AttributeRef AttributeRef::getNext()
  { 
    ElementPtr parentPtr = getParentElementPtr();
    AttributeSegment* me = getMe<Read>();
    AttributePtr nextPtr = me->nextPtr;
    return AttributeRef(document, parentPtr, nextPtr );
  }

  __INLINE bool AttributeRef::operator== ( const AttributeRef& attr )
  {
    if ( &document != &(attr.document) ) return false;
    return ( ( getParentElementPtr() == attr.getParentElementPtr() )
	     && ( getAttributePtr() == attr.getAttributePtr() ) );
  }

  __INLINE DomTextSize AttributeRef::getSize()
  {
    return AttributeSegment_getSize(getMe<Read>());
  }

  template<PageCredentials how>
  __INLINE AttributeSegment* AttributeRef::getMe()
  {
#if PARANOID
    AssertBug ( getAttributePtr(), "Null Attribute Pointer !\n" );
#endif
    return getDocumentAllocator().getSegment<AttributeSegment,how> ( getAttributePtr() );
  }

  template<typename T, PageCredentials how>
  __INLINE T* AttributeRef::getData() 
  {
#if PARANOID
    if ( how == Write )
      {
        AssertBug ( getDocument().isLockedWrite(), "Document shall be locked write !\n" );
      }
#endif
    __ui64 size = getSize();
    void* me = getDocumentAllocator().getSegment<void,how> ( getAttributePtr(), sizeof(AttributeSegment) + size );
    return (T*)(((__ui64)me)+sizeof(AttributeSegment));
  }

  __INLINE void AttributeRef::alterData ()
  {
#if PARANOID
    AssertBug ( getDocument().isLockedWrite(), "Document shall be locked write !\n" );
#endif
    __ui64 size = getSize();
    void* me = getDocumentAllocator().getSegment<void,Write> ( getAttributePtr(), sizeof(AttributeSegment) + size );
    getDocumentAllocator().alter ( me, sizeof(AttributeSegment) + size );
  }

  __INLINE void AttributeRef::protectData ()
  {
    __ui64 size = getSize();
    void* me = getDocumentAllocator().getSegment<void,Read> ( getAttributePtr(), sizeof(AttributeSegment) + size );
    getDocumentAllocator().protect ( me, sizeof(AttributeSegment) + size );
  }

  __INLINE String AttributeRef::toString ()
  {
    AssertBug ( (*this), "Getting string from a null attribute !\n" );

    switch ( getAttributeType() )
      {
      case AttributeType_String:
        return String(getData<char,Read>());
      case AttributeType_NamespaceAlias:
        Log_AttributeHPP ( "********* GOT NAMESPACE ID 0x%x, url '%s' **********\n",
          *(getData<NamespaceId,Read>()), getKeyCache().getNamespaceURL(*(getData<NamespaceId,Read>())) );
        return String(getKeyCache().getNamespaceURL(*(getData<NamespaceId,Read>())));
      case AttributeType_Integer: 
        {
          String s;
          stringPrintf(s, "%lld", toInteger() );
          return s;
        }
      case AttributeType_XPath: return String("[XPath]");
      case AttributeType_SKMap: return String("[SKMap]");
      case AttributeType_KeyId: return String("[KeyId]");
      default:
        NotImplemented ( "Complex type attributes : type=%x.\n", getAttributeType() );
      }
    return String ("Unhandled type");
  }
  
  __INLINE Number AttributeRef::toNumber ()
  {
    switch ( getAttributeType() )
    {
    case AttributeType_String:
      return Item::toNumber ();
    case AttributeType_Integer:
      return (Number) toInteger();
    default:
      NotImplemented ( "Number for flag = %x\n", getAttributeType() );  
    }
    return NAN;  
  }

  __INLINE Integer AttributeRef::toInteger ()
  {
    switch ( getAttributeType() )
    {
    case AttributeType_KeyId:
    case AttributeType_Integer:
      return *(getData<Integer,Read>());
    default:
      return toString().toInteger();      
    }
    Bug ( "Shall not be here !\n" );
    return 0;
  }

  __INLINE NamespaceId AttributeRef::getNamespaceAliasId ()
  {
    AssertBug ( getType() == AttributeType_NamespaceAlias, "Attribute is not a namespace alias !\n" );
    return *(getData<NamespaceId,Read> ());
  }
};
