#include <Xemeiah/netstore/netdocument.h>
#include <Xemeiah/netstore/netstore.h>

#include <Xemeiah/dom/elementref.h>

#include <Xemeiah/auto-inline.hpp>

namespace Xem
{
  NetDocument::NetDocument ( NetStore& netStore, RevisionPage* _revisionPage, DocumentCreationFlags _documentCreationFlags, const String& _role )
  : Document ( netStore )
  {
    documentCreationFlags = _documentCreationFlags;
    setRole ( _role );
    
    revisionPage = _revisionPage;
    documentHead = &(revisionPage->documentHead );
    freeSegmentsHeader = &(revisionPage->freeSegmentsHeader);

    rootElementPtr = documentHead->rootElementPtr;
    
    areasAlloced = ( documentHead->nextRelativePagePtr >> InAreaBits ) + 1;

    Info ( "Creating :  [%llu:%llu], areas=%llu, revPage=%p\n", _brid(getBranchRevId()), areasAlloced, revisionPage );
    areas = (void**) realloc ( areas, sizeof(void*) * ( areasAlloced ) );
    for ( __ui64 areaIdx = 0 ; areaIdx < areasAlloced ; areaIdx++ )
      areas[areaIdx] = NULL;

    ElementRef root = getRootElement ();
    if ( root && root.hasAttr ( store.getKeyCache().builtinKeys.xemint.element_map(), AttributeType_SKMap ) )
      {
        isIndexed = true;
      }
  }

  NetDocument::~NetDocument ()
  {
    Warn ( "Deleting : [%llu:%llu]\n", _brid(getBranchRevId()) );
    for ( __ui64 areaIdx = 0 ; areaIdx < areasAlloced ; areaIdx++ )
      {
        if ( areas[areaIdx] )
          {
            getStore().releaseVolatileArea ( areas[areaIdx] );
          }
      }
    free ( areas );
  }
  
  void NetDocument::release ()
  {
    Warn ( "Releasing : [%llu:%llu]\n", _brid(getBranchRevId()) );
    getStore().getBranchManager().releaseDocument ( this );  
  }
  
  void NetDocument::mapArea ( __ui64 areaIdx )
  {
    if ( areaIdx >= areasAlloced )
      {
        NotImplemented ( "Out of bounds.\n" );
      }
    if ( areas[areaIdx] )
      {
        Bug ( "Already mapped !\n" );
      }

    if ( getRole() == "none" )
      {
        Bug ( "." );
      }
  
    NetStore& netStore = dynamic_cast<NetStore&> (getStore());
    __ui64 buffSize;
  
    String params;
    stringPrintf ( params, "role=%s&branchId=%llx&revisionId=%llx&area=0x%llx&length=0x%llx", 
        getRole().c_str(),
        revisionPage->branchRevId.branchId,
        revisionPage->branchRevId.revisionId, 
        areaIdx, AreaSize );
   
    void* buff;
    if ( ! netStore.queryData ( "fetchDocumentArea", params, buff, buffSize ) )
      {
        Bug ( "." );
      }
    AssertBug ( buffSize == AreaSize, "Invalid return buffSize=%llx\n", buffSize );
    
    areas[areaIdx] = buff;
  }
  
  void NetDocument::authorizePageWrite ( RelativePagePtr relPagePtr )
  {
    Warn ( "authorizePageWrite, relPagePtr = '%llx'\n", relPagePtr );
  }
    
  RelativePagePtr NetDocument::getFreeRelativePages ( __ui64 askedNumber,  __ui64& allocedNumber, AllocationProfile allocProfile )
  {
    NotImplemented ( "." );
    return 0;
  }
  
  AllocationProfile NetDocument::getAllocationProfile ( SegmentPtr segmentPtr )
  {
    NotImplemented ( "." );
    return 0;
  }

  BranchRevId NetDocument::getBranchRevId()
  {
    return revisionPage->branchRevId;
  }

  bool NetDocument::commit () { NotImplemented ( "no commit.\n" ); return false; }
  bool NetDocument::reopen () 
  { 
    // NotImplemented ( "no reopen.\n" ); return false;
    BranchRevId initialBranchRevId = getBranchRevId();
    void* buff;
    __ui64 buffSize;

    String params;
    stringPrintf ( params, "branchId=%llx&revisionId=%llx&flags=%x&role=%s", getBranchRevId().branchId, getBranchRevId().revisionId, 
        getDocumentCreationFlags(), getRole().c_str() );
   
    NetStore& netStore = dynamic_cast<NetStore&> (getStore());
    if ( ! netStore.queryData ( "reopenDocument", params, buff, buffSize ) )
      {
        throwException ( Exception, "Could not reopen document !\n" );
      }
    AssertBug ( buffSize == sizeof(RevisionPage), "Invalid buffSize\n" );

    memcpy ( revisionPage, buff, sizeof(RevisionPage) );
    getStore().releaseVolatileArea ( buff );
    
    documentHead = &(revisionPage->documentHead );
    freeSegmentsHeader = &(revisionPage->freeSegmentsHeader);
      
    getStore().getBranchManager().updateDocumentReference ( this, initialBranchRevId, getBranchRevId() );
    
    return true;
  }
  
  bool NetDocument::fork ( String& branchName, BranchFlags branchFlags ) { NotImplemented ( "no fork.\n" ); return false; }
  bool NetDocument::merge () { NotImplemented ( "no merge.\n" ); return false; }
  __ui32 NetDocument::getFirstFreeSegmentOffset ( RelativePagePtr relPagePtr ) { NotImplemented ( "." ); return 0; }
  void NetDocument::setFirstFreeSegmentOffset ( RelativePagePtr relPagePtr, __ui32 offset ) { NotImplemented ( "." ); }
  
};
