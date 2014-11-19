#ifndef __XEM_NETSTORE_NETDOCUMENT_H
#define __XEM_NETSTORE_NETDOCUMENT_H

#include <Xemeiah/kern/document.h>
#include <Xemeiah/persistence/cachebranchmanager.h>

#include <Xemeiah/persistence/format/revision.h>

namespace Xem
{
  class NetStore;
  class NetDocument : public Document
  {
  protected:
    friend class NetStore;
    friend class NetBranchManager;
    
    DocumentCreationFlags documentCreationFlags;
    RevisionPage* revisionPage;
    
    NetDocument ( NetStore& _netStore, RevisionPage* _revisionPage, DocumentCreationFlags _documentCreationFlags, const String& _role );

    virtual ~NetDocument ();

    virtual bool mayIndex () { return true; }
    virtual void mapArea ( __ui64 areaIdx );
    virtual RelativePagePtr getFreeRelativePages ( __ui64 askedNumber,  __ui64& allocedNumber, AllocationProfile allocProfile );
    virtual AllocationProfile getAllocationProfile ( SegmentPtr segmentPtr );
    virtual void authorizePageWrite ( RelativePagePtr relPagePtr );
    virtual DocumentCreationFlags getDocumentCreationFlags() { return documentCreationFlags; }
    virtual bool isScheduledRevisionRemoval () { Warn ( "no isScheduledRevisionRemoval.\n "); return false; }

    virtual __ui32 getFirstFreeSegmentOffset ( RelativePagePtr relPagePtr );
    virtual void setFirstFreeSegmentOffset ( RelativePagePtr relPagePtr, __ui32 offset );

  public:

    virtual BranchRevId getBranchRevId();
    virtual bool commit ();
    virtual bool reopen ();
    virtual bool fork ( String& branchName, BranchFlags branchFlags );
    virtual bool merge ();
    
    virtual void release ();
  };
};

#endif // __XEM_NETSTORE_NETDOCUMENT_H
