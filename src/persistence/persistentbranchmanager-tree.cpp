#include <Xemeiah/persistence/persistentstore.h>
#include <Xemeiah/persistence/persistentbranchmanager.h>
#include <Xemeiah/persistence/persistentdocument.h>

#include <Xemeiah/kern/volatiledocument.h>
#include <Xemeiah/parser/saxhandler-dom.h>

#include <Xemeiah/auto-inline.hpp>
#include <Xemeiah/persistence/auto-inline.hpp>

namespace Xem
{

#define __formatAttr(__ns,__name,__fmt,...) \
{ \
  char __buff[512]; \
  sprintf ( __buff, __fmt, __VA_ARGS__ ); \
  evd.eventAttr ( __ns, __name, __buff ); \
}    

  Document* PersistentBranchManager::generateBranchesTree ( XProcessor& xproc )
  {
    ElementRef root = xproc.createVolatileDocument(true);
    
    SAXHandlerDom evd ( xproc, root );

    evd.eventElement ( "xem-pers", "store" );
    evd.eventAttr ( "xmlns", "xem-pers", "http://www.xemeiah.org/ns/xem/persistence" );
    SuperBlock* sb = getPersistentStore().getSB();
    evd.eventAttr ( "xem-pers", "magic", sb->magic );
    evd.eventAttr ( "xem-pers", "version", sb->version );
    __formatAttr ( "xem-pers", "segmentSizeMax", "%llu", sb->segmentSizeMax );
    __formatAttr ( "xem-pers", "noMansLand", "%llu", sb->noMansLand );
    __formatAttr ( "xem-pers", "nextBranchId", "%llu", sb->nextBranchId );
    __formatAttr ( "xem-pers", "nextElementId", "%llu", sb->nextElementId );
    __formatAttr ( "xem-pers", "fileLength", "%llu", getPersistentStore().getFileLength() );
    __formatAttr ( "xem-pers", "isHardLimit", "%s", getPersistentStore().isFileLengthIsAHardLimit() ? "true" : "false" );


    evd.eventAttrEnd ();
    
    for ( AbsolutePageRef<BranchPage> branchPageRef = getPersistentStore().getAbsolutePage<BranchPage>(getPersistentStore().getSB()->lastBranch) ; branchPageRef.getPage() ;
  	  branchPageRef = getPersistentStore().getAbsolutePage<BranchPage>(branchPageRef.getPage()->lastBranch) )
      {   
        evd.eventElement ( "xem-pers", "branch" );
        __formatAttr ( "xem-pers", "name", "%s", branchPageRef.getPage()->name );
        __formatAttr ( "xem-pers", "id", "%llu", branchPageRef.getPage()->branchId );
        
        if ( branchPageRef.getPage()->forkedFrom.branchId )
          {
            AssertBug ( branchPageRef.getPage()->forkedFrom.revisionId, "Forked from a null revisionId !\n" );
            __formatAttr ( "xem-pers", "forked-from", "%llu:%llu", _brid ( branchPageRef.getPage()->forkedFrom ) );
          }

        evd.eventAttrEnd ();

#define __onBranchFlag(__flag, __attrName) \
 if ( branchPageRef.getPage()->branchFlags & __flag ) \
 { evd.eventElement ( "xem-pers", "flag" ); __formatAttr ( "xem-pers", "name", "%s", __attrName ); \
   evd.eventAttrEnd(); evd.eventElementEnd ( "xem-pers", "flag" ); }
 
 // __formatAttr ( "xem-pers", __attrName, "%s", "true" )
        if ( branchPageRef.getPage()->branchFlags )
          {
            evd.eventElement ( "xem-pers", "flags" );
            evd.eventAttrEnd ();
                        
            __onBranchFlag ( BranchFlags_MustForkBeforeUpdate, "MustForkBeforeUpdate" );
            __onBranchFlag ( BranchFlags_MayIndexNameIfDuplicate, "MayIndexNameIfDuplicate" );
            __onBranchFlag ( BranchFlags_AutoForked, "AutoForked" );
            __onBranchFlag ( BranchFlags_AutoMerge, "AutoMerge" );
            __onBranchFlag ( BranchFlags_MayNotBeXUpdated, "MayNotBeXUpdated" );
            evd.eventElementEnd ( "xem-pers", "flags" );
          }
        
        for ( AbsolutePageRef<RevisionPage> revPageRef = getPersistentStore().getAbsolutePage<RevisionPage> ( branchPageRef.getPage()->lastRevisionPage ) ;
          revPageRef.getPage() ; revPageRef = getPersistentStore().getAbsolutePage<RevisionPage> ( revPageRef.getPage()->lastRevisionPage ) )
          {
            evd.eventElement ( "xem-pers", "revision" );
            time_t creationTime = (time_t) revPageRef.getPage()->creationTime;
            time_t commitTime = (time_t) revPageRef.getPage()->commitTime;
            __formatAttr ( "xem-pers", "branchRevId", "%llu:%llu", _brid ( revPageRef.getPage()->branchRevId ) );
            __formatAttr ( "xem-pers", "writable", "%s", revPageRef.getPage()->documentAllocationHeader.writable ? "true" : "false" );
            __formatAttr ( "xem-pers", "creationTime", "%s", asctime(localtime(&(creationTime))) );
            __formatAttr ( "xem-pers", "commitTime", "%s", 
                revPageRef.getPage()->commitTime ? asctime(localtime(&(commitTime))) : "(not committed)" );
          
#define __prettyPrintBytesNumber_DCEP(__buff,__bytes,__i,__suff) \
  else if ( __bytes >= 1 << __i ) { sprintf ( __buff, "%llu.%03llu " __suff, __bytes >> __i, (__bytes >> (__i-10)) % (1<<10) ); }
#define __prettyPrintBytesNumber(__buff,__bytes) do { if ( 0 )  {} \
  __prettyPrintBytesNumber_DCEP(__buff,__bytes,30,"GB") \
  __prettyPrintBytesNumber_DCEP(__buff,__bytes,20,"MB") \
  __prettyPrintBytesNumber_DCEP(__buff,__bytes,10,"kB") \
  else { sprintf ( __buff, "%llu b", __bytes ); } } while(0)

#if 0
            char allocedBytes[64];
            __prettyPrintBytesNumber ( allocedBytes, revPage->documentHead.allocedBytes );
            __formatAttr ( "xem-pers", "alloced-bytes", "%s", allocedBytes );
#endif            
            evd.eventAttrEnd ();

            evd.eventElement ( "xem-pers", "owned-pages" );
            __formatAttr ( "xem-pers", "total", "%llu", revPageRef.getPage()->ownedPages );
            evd.eventAttrEnd ();
            for ( __ui64 type = 0 ; type < PageType_Mask ; type++ )		\
              { 
                if ( revPageRef.getPage()->ownedTypedPages[type] )
                  {
                    evd.eventElement ( "xem-pers", "page-type" );
                    __formatAttr ( "xem-pers", "name", "%s", PersistencePageTypeName[type] );
                    __formatAttr ( "xem-pers", "total", "%llu", revPageRef.getPage()->ownedTypedPages[type] );
                    evd.eventAttrEnd ();
                    evd.eventElementEnd ( "xem-pers", "page-type" );
                  }
              }
            evd.eventElementEnd ( "xem-pers", "owned-pages" );
           
#if 0 
            evd.eventElement ( "xem-pers", "journal" );
            evd.eventAttrEnd ();
           
            {
              PersistentDocument* doc = instanciatePersistentDocument ( revPage->revisionAbsolutePagePtr, Document_Read );
              doc->dumpJournal ( evd );         
              doc->release ();
            }            
            
            evd.eventElementEnd ( "xem-pers", "journal" );
#endif
       
            evd.eventElementEnd ( "xem-pers", "revision" );
          }
        evd.eventElementEnd ( "xem-pers", "branch" );
      }
    evd.eventElementEnd ( "xem-pers", "store" );
    return &(root.getDocument());
  }


};

