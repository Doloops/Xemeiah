#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/dom/skmapref.h>

#define Log_SKMapHPP Debug

namespace Xem
{
  __INLINE __ui64 SKMapRef::hashString ( const String& str )
  {
    __ui64 h = 0x245;
    for ( const char* c = str.c_str() ; *c ; c++ )
      {
        unsigned char d = (unsigned char)(*c);
        h = d + (h<<6) + (h<<16) - h;
      }
    return h;
  }

  __INLINE SKMapConfig* SKMapRef::getConfig()
  {
    return &(getHeader()->config);
  }

  __INLINE SKMapHeader* SKMapRef::getHeader()
  {
    return getData<SKMapHeader,Read> ();
  }

  __INLINE SKMapType SKMapRef::getSKMapType()
  {
    return (SKMapType) getConfig()->skMapType;
  }

  __INLINE void SKMapRef::checkSKMapType ( SKMapType expectedType )
  {
    if ( ! *this) return;
    if ( getSKMapType() != expectedType )
      {
        SKMapConfig* config = getConfig();
        AttributeSegment* me = getMe<Read> ();
        Bug ( "Invalid SKMap type : expected %x, but I am %x -> %x, me=%x\n",
          expectedType, getSKMapType(), config->skMapType, me->keyId );


        throwException ( InvalidSKMapType, "Invalid SKMap type : expected %x, but I am %x\n",
          expectedType, getSKMapType() );
      }  
  }

  __INLINE void SKMapRef::authorizeHeaderWrite()
  {
    getData<SKMapHeader,Write> ();
  }

  __INLINE SKMapItem* SKMapRef::getHead()
  {
    if ( ! getHeader()->head ) return NULL;
    return getDocumentAllocator().getSegment<SKMapItem,Read> ( getHeader()->head, sizeof(SKMapItem) );
  }

  __INLINE void SKMapRef::authorizeHeadWrite()
  {
    getItem<Write> ( getHeader()->head );
  }

  __INLINE __ui64 SKMapRef::getItemSize ( __ui32 level )
  {
    return sizeof(SKMapItem) + ( level * sizeof(SegmentPtr) );
  
  }

  __INLINE __ui64 SKMapRef::getItemSize ( SKMapItem* item )
  {
    return getItemSize ( item->level );  
  }

  __INLINE SKMapItemPtr SKMapRef::getNewItemPtr ( __ui32 level, SKMapHash hash, SKMapValue value )
  {
    __ui64 allocSize = getItemSize ( level ); // sizeof(SKMapItem) + ( level * sizeof(SegmentPtr) );
#if 1
    AllocationProfile allocProfile = getConfig()->itemAllocProfile;
#else
    AllocationProfile allocProfile = ( level * ( RevisionPage::maxFreeListHeader - 1) ) / getConfig()->maxLevel;
    /*
    Warn ( "level=%u, maxLevel=%u, maxFreeListHeader=%u, allocProfile=%u\n",
	   level, getConfig()->maxLevel, RevisionPage::maxFreeListHeader,
	   allocProfile );
    */
#endif
    SKMapItemPtr itemPtr = getDocumentAllocator().getFreeSegmentPtr ( allocSize, allocProfile );
    SKMapItem* item = getDocumentAllocator().getSegment<SKMapItem,Write> ( itemPtr, allocSize );

    getDocumentAllocator().alter ( item, allocSize );
    item->level = level;
    item->hash = hash;
    item->value = value;
    for ( __ui32 l = 0 ; l <= level ; l++ )
      item->next[l] = NullPtr;
    getDocumentAllocator().protect ( item, allocSize );
    Log_SKMapHPP ( "New item at %llx, level=%x, hash=%llx, value=%llx\n",
        itemPtr, level, hash, value );
    return itemPtr;
  }
  
  
  template<PageCredentials how>
  __INLINE SKMapItem* SKMapRef::getItem ( SKMapItemPtr skMapItemPtr )
  {
    SKMapItem* fake = getDocumentAllocator().getSegment<SKMapItem,Read> ( skMapItemPtr, sizeof(SKMapItem) );
    return getDocumentAllocator().getSegment<SKMapItem,how> ( skMapItemPtr, getItemSize(fake) );
  }
  
  __INLINE void SKMapRef::alterItem ( SKMapItem* skMapItem )
  {
    getDocumentAllocator().alter ( skMapItem, getItemSize ( skMapItem ) );
  }
  
  __INLINE void SKMapRef::protectItem ( SKMapItem* skMapItem )
  {
    getDocumentAllocator().protect ( skMapItem, getItemSize ( skMapItem ) );
  }
 
  /*
   * ******************************************************************************
   * SKMapRef iterator inlines
   */
  __INLINE SKMapConfig* SKMapRef::iterator::getConfig()
  {
    return skMapRef.getConfig();
  }
      
  __INLINE bool SKMapRef::iterator::isPositionned ()
  {
    return (currentItemPtr != NullPtr);
  }

  __INLINE __ui64 SKMapRef::iterator::getItemSize ( SKMapItem* item )
  { return skMapRef.getItemSize ( item ); }
  
  __INLINE SKMapHash SKMapRef::iterator::getHash ( )
  {
    AssertBug ( currentItemPtr, "No current item set !\n" );
    return skMapRef.getItem<Read>(currentItemPtr)->hash;
  }
  
  __INLINE SKMapValue SKMapRef::iterator::getValue ( )
  {
    AssertBug ( currentItemPtr, "No current item set !\n" );
    return skMapRef.getItem<Read>(currentItemPtr)->value;
  }
  

   
};
