#include <Xemeiah/netstore/netbranchmanager.h>
#include <Xemeiah/netstore/netstore.h>
#include <Xemeiah/netstore/netdocument.h>

#include <Xemeiah/kern/volatiledocument.h>

#include <Xemeiah/persistence/format/revision.h>


#include <Xemeiah/auto-inline.hpp>

namespace Xem
{
  NetBranchManager::NetBranchManager ( NetStore& _netStore )
  : netStore ( _netStore )
  {
  
  }
  
  NetBranchManager::~NetBranchManager ()
  {
  
  }

  const char* documentCreationFlagsLabels[] = 
  {
    "#ERROR", //
    "read",  // DocumentCreationFlags_Read          = 0x01, //< Read with an opportunity to change the document to be writable
    "explicit-read", // DocumentCreationFlags_ExplicitRead  = 0x02, //< Read with non opporutinity to write
    "write", // DocumentCreationFlags_Write         = 0x03, //< Create a writable revision at the end of the branch (branch must not have a writable one !)
    "reuse-write", // DocumentCreationFlags_ReuseWrite    = 0x04, //< Only take the last writable revision, fail if not existent
    "as-revision", // DocumentCreationFlags_AsRevision    = 0x05, //< As the revision is : if it is writable, open it writable, otherwise open it readable
    "unmanaged", // DocumentCreationFlags_UnManaged     = 0x06, //< Not managed by the BranchManager : direct deletion, no cache, no reuse.
    "reset-revision", // DocumentCreationFlags_ResetRevision = 0x07, //< Force to reset the revision
  };
  
  BranchId NetBranchManager::getBranchId ( const String& branchName )
  {
    String params = "branchName=";
    params += branchName;
    String res = netStore.queryString ( "getBranchId", params );
    
    Warn ( "Returned res = '%s'\n", res.c_str() );
    BranchId branchId = strtoull ( res.c_str(), NULL, 0 );
    
    if ( ! branchId )
      {
        throwException ( Exception, "Null branchId !\n" );
      }
    return branchId;      
  }

  void NetBranchManager::renameBranch ( BranchId branchId, const String& branchName ) 
  {
    NotImplemented ( "." );
  }

  BranchId NetBranchManager::createBranch ( const String& branchName, BranchFlags branchFlags )       
  {
    NotImplemented ( "." );
    return 0;
  }

  BranchId NetBranchManager::createBranch ( const String& branchName, BranchRevId forkedFrom, BranchFlags branchFlags )  
  {
    NotImplemented ( "." );
    return 0;
  }

  Document* NetBranchManager::generateBranchesTree () 
  {
    String params;
    return getNetStore().queryXMLDocument ( "generateBranchesTree", params );
  }

  String NetBranchManager::getBranchName ( BranchId branchId ) 
  {
    String params;
    stringPrintf ( params, "branchId=%llu", branchId );
    String branchName = netStore.queryString ( "getBranchName", params );
    
    return branchName;
  }

  RevisionId NetBranchManager::lookupRevision ( BranchId branchId )
  {
    String params;
    stringPrintf ( params, "branchId=%llu", branchId );
    String res = netStore.queryString ( "lookupRevision", params );

    RevisionId revisionId = strtoull ( res.c_str(), NULL, 0 );
    
    if ( ! revisionId )
      {
        throwException ( Exception, "Null revisionId !\n" );
      }
    return revisionId;      
  }

  BranchRevId NetBranchManager::getForkedFrom ( BranchId branchId )  
  {
    String params;
    stringPrintf ( params, "branchId=%llu", branchId );
    String forkedFromStr = netStore.queryString ( "getBranchForkedFrom", params );
    BranchRevId forkedFrom;
    int res = sscanf ( forkedFromStr.c_str(), "%llu:%llu", &(forkedFrom.branchId), &(forkedFrom.revisionId) );
    
    Warn ( "ForkedFrom Parse : '%s' -> '%llu:%llu' (res=%d)\n", forkedFromStr.c_str(), _brid(forkedFrom), res );
    
    return forkedFrom;
  }

  void NetBranchManager::deleteBranch ( BranchInfo* branchInfo )
  {
    NotImplemented ( "Deleting branch.\n" );
  }

  Document* NetBranchManager::createDocument ( BranchId branchId, RevisionId revisionId, DocumentCreationFlags flags ) 
  {
    String role = "none";
    return createDocument ( branchId, revisionId, flags, role );
  }

  Document* NetBranchManager::createDocument ( BranchId branchId, RevisionId revisionId, DocumentCreationFlags flags, const String& role ) 
  {
// bool queryData ( const String& action, const String& arguments, void*& buff, __ui64 buffSize );
    lockBranchManager ();
    
    void* buff;
    __ui64 buffSize;
    
    if ( flags == DocumentCreationFlags_ExplicitRead || flags == DocumentCreationFlags_Read )
      {
        RevisionId cacheRevId = revisionId;
        if ( cacheRevId == 0 )
          {
            cacheRevId = lookupRevision ( branchId );
          }
        Document* doc = getDocumentFromCache ( branchId, cacheRevId, flags, role );
        if ( doc )
          {
            unlockBranchManager ();
            return doc;
          }
      }
    
    String params;
    stringPrintf ( params, "branchId=%llx&revisionId=%llx&flags=%x&mode=%s&role=%s", branchId, revisionId, flags, documentCreationFlagsLabels[flags], role.c_str() );
   
    if ( ! netStore.queryData ( "createDocument", params, buff, buffSize ) )
      {
        Bug ( "." );
      }
    
    // AssertBug ( buffSize == Document::AreaSize, "Invalid revisionPage size !\n" );
    
    Warn ( "buffSize=%llu\n", buffSize );
    
    RevisionPage* revisionPage = (RevisionPage*) buff;


    DocumentHead* dH = &(revisionPage->documentHead);
    Warn ( "Revision at %p : DocumentHead : writable=0x%llx, ownedPages=0x%llx, allocedBytes=0x%llx, firstReservedId=%llx,\n"
           "\tlastReservedId=0x%llx, rootElementPtr=0x%llx, elements=0x%llx, nextRelativePagePtr=%llx\n",
           revisionPage,
           dH->writable, dH->ownedPages, dH->allocedBytes, dH->firstReservedElementId,
           dH->lastReservedElementId, dH->rootElementPtr, dH->elements, dH->nextRelativePagePtr );
    
    Warn ( "Revision (at %p) : '%llx:%llx'\n", revisionPage, _brid(revisionPage->branchRevId) );
    
    NetDocument* document = new NetDocument ( getNetStore(), revisionPage, flags, role );
    // document->setRole ( role );

    BranchRevId nullForkedFrom = {0, 0};
    BranchInfo* branchInfo = referenceBranch ( revisionPage->branchRevId.branchId, nullForkedFrom );
  
    AssertBug ( branchInfo, "Invalid zero branchInfo !\n" );
  
    branchInfo->referenceDocument ( document );
    putDocumentInCache ( branchInfo, document );
  
    unlockBranchManager ();
    return document;
  }

  void NetBranchManager::scheduleBranchForRemoval ( BranchId branchId ) 
  {
    NotImplemented ( "." );
  }

};

