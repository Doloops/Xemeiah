#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/dom/skmapref.h>
#include <Xemeiah/dom/blobref.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_SKMap Debug

namespace Xem
{
  void SKMapRef::initDefaultSKMapConfig ( SKMapConfig& config )
  {
    config.maxLevel = 32;
    config.itemAllocProfile = 0x1c;
    config.listAllocProfile = 0x1d;

    for ( __ui32 level = 0 ; level < config.maxLevel ; level++ )
        config.probability[level] = 128;      
  }

  AttributeRef ElementRef::addSKMap ( KeyId keyId, SKMapType mapType )
  {
    AssertBug ( mapType != SKMapType_Blob, "Shall call addBlob() for that !\n" );
    SKMapConfig config;
    SKMapRef::initDefaultSKMapConfig ( config );
    return addSKMap ( keyId, config, mapType );
  }
  
  AttributeRef ElementRef::addSKMap ( KeyId keyId, SKMapConfig& config, SKMapType mapType )
  {
    DomTextSize skMapHeaderSize = 0;
    if ( mapType == SKMapType_Blob ) skMapHeaderSize = sizeof(BlobHeader);
    else skMapHeaderSize = sizeof(SKMapHeader);
    
    AttributeRef attrRef = addAttr ( keyId, AttributeType_SKMap, skMapHeaderSize );
    SKMapHeader* header = attrRef.getData<SKMapHeader,Write> ();

    attrRef.alterData ();
    memset ( header, 0, skMapHeaderSize );
    header->config = config;
    header->config.skMapType = mapType;
    header->head = NullPtr;
    attrRef.protectData ();

    Log_SKMap( "New skMap at %llx, header=%p\n", attrRef.getAttributePtr(), header );
    return attrRef;
  }

  void SKMapRef::dump ()
  {
    Info ( "Dumping %p\n", this );
    for ( SKMapRef::iterator iter(*this) ; iter ; iter++ )
      {
        Info ( "\tHash=0x%llx, Value=0x%llx\n", iter.getHash(), iter.getValue() );
      }
  
  }

  void SKMapRef::deleteSKMapSingle ()
  {
    SegmentPtr itemPtr = getHeader()->head;
    while ( itemPtr )
      {
        SKMapItem* item = getDocumentAllocator().getSegment<SKMapItem,Read> ( itemPtr, getItemSize((__ui32)0) );
        SegmentPtr nextItemPtr = item->next[0];
        getDocumentAllocator().freeSegment ( itemPtr, getItemSize(item) );
        itemPtr = nextItemPtr;
      }
    deleteAttributeSegment ();

  }

};
