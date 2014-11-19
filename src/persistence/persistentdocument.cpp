#include <Xemeiah/persistence/persistentdocument.h>
#include <Xemeiah/persistence/persistentbranchmanager.h>
#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/dom/skmapref.h>
#include <Xemeiah/trace.h>

#include <Xemeiah/auto-inline.hpp>
#include <Xemeiah/persistence/auto-inline.hpp>

#define Log_PDoc Debug

// #define __XEM_PERSISTANCE_ENABLE_QUICKMERGE

namespace Xem
{
  PersistentDocument::PersistentDocument ( Store& __s, PersistentDocumentAllocator& persistentDocumentAllocator, DocumentOpeningFlags flags )
    : JournaledDocument (__s, persistentDocumentAllocator ) 
  { 
    documentOpeningFlags = flags;
    documentHeadPtr = NullPtr;
    
    acknowledgedPersistentDocumentDeletion = false;
    if ( documentOpeningFlags == DocumentOpeningFlags_UnManaged )
      acknowledgedPersistentDocumentDeletion = true;
      
    scheduledRevisionRemoval = false;
    scheduledDocumentRelease = false;
    
    isIndexed = false;

    doInitialize ();
  }
 
  PersistentDocument::~PersistentDocument ( )
  {
    AssertBug ( acknowledgedPersistentDocumentDeletion,
        "Persistent Document %p deletion has not been acknowledged by PersistentDocument !!!\n",
        this );
    Log_PDoc ( "[PERSISTENT-DOCUMENT] Delete %p\n", this );
  }

  JournalHead& PersistentDocument::getJournalHead ()
  {
    return getPersistentDocumentAllocator().getRevisionPage()->journalHead;
  }

  void PersistentDocument::doInitialize ()
  {
    /**
     * Check if we have a documentHead
     */
    if ( getPersistentDocumentAllocator().getRevisionPage()->documentHeadPtr == NullPtr )
      {
        Log_PDoc ( "Creating new DocumentHead for [%llu:%llu]\n", _brid(getBranchRevId()) );
        documentHeadPtr = getDocumentAllocator().getFreeSegmentPtr ( sizeof(DocumentHead), 0 );
        
        getPersistentDocumentAllocator().alterRevisionPage();
        getPersistentDocumentAllocator().getRevisionPage()->documentHeadPtr = documentHeadPtr;
        getPersistentDocumentAllocator().protectRevisionPage();
        
        alterDocumentHead();
        getDocumentHead().rootElementPtr = NullPtr;
        getDocumentHead().metaElementPtr = NullPtr;
        getDocumentHead().firstReservedElementId = 0;
        getDocumentHead().lastReservedElementId = 0;
        getDocumentHead().elements = 0;
        protectDocumentHead();
      }

    documentHeadPtr = getPersistentDocumentAllocator().getRevisionPage()->documentHeadPtr;
      
    if ( documentHeadPtr >= getPersistentDocumentAllocator().getDocumentAllocationHeader().nextRelativePagePtr )
      {
        throwException ( Exception, "Invalid documentHeadPtr %llx (nextRelativePagePtr=%llx)\n",
            documentHeadPtr, getPersistentDocumentAllocator().getDocumentAllocationHeader().nextRelativePagePtr );
      }

    if ( getDocumentHead().rootElementPtr )
      {
        if ( getDocumentHead().rootElementPtr >= getPersistentDocumentAllocator().getDocumentAllocationHeader().nextRelativePagePtr )
          {
            throwException ( Exception, "Invalid getDocumentHead().rootElementPtr %llx (nextRelativePagePtr=%llx)\n",
                getDocumentHead().rootElementPtr,
                getPersistentDocumentAllocator().getDocumentAllocationHeader().nextRelativePagePtr );
          }
        rootElementPtr = getDocumentHead().rootElementPtr;
      }
    else
      {
        createRootElement ();
      }

    ElementRef root = getRootElement ();
    if ( root && root.hasAttr ( store.getKeyCache().getBuiltinKeys().xemint.element_map(), AttributeType_SKMap ) )
      {
        isIndexed = true;
      }
  }
  
  void PersistentDocument::releaseDocumentResources ()
  {
    getPersistentStore().getPersistentBranchManager().assertIsLocked();

    AssertBug ( ! isLockedWrite(), "Shall not be locked write !\n" );

    if ( isLockedWrite() )
      {
        Log_PDoc ( "Document %p [%llx:%llx] : unlock write at releaseDocumentResources()\n",
            this, _brid(getBranchRevId()) );
        unlockWrite();
      }
    AssertBug ( refCount, "Zero refCount ?\n" );
    if ( refCount == 1 )
      {
        /*
         * releaseDocument is allowed to change document refCount
         */
        getStore().getBranchManager().releaseDocument ( this );
      }
    AssertBug ( refCount, "Zero refCount ?\n" );
    if ( refCount == 1 )
      {
        acknowledgedPersistentDocumentDeletion = true;
      }
    else
      {
        Log_PDoc ( "After releaseDocumentResources(this=%p,brid=[%llx:%llx]), refCount=%llx, will not delete this document.\n" ,
            this, _brid(getBranchRevId()), refCount );
      }
  }

  void PersistentDocument::scheduleRevisionForRemoval ()
  {
    scheduledRevisionRemoval = true;
    scheduleDocumentRelease ();
  }
  
  void PersistentDocument::scheduleBranchForRemoval ()
  {
    scheduleDocumentRelease ();
    scheduleRevisionForRemoval ();
    getStore().getBranchManager().scheduleBranchForRemoval ( getBranchRevId().branchId );
  }

  void PersistentDocument::scheduleDocumentRelease ()
  {
    scheduledDocumentRelease = true;
  }

  bool PersistentDocument::isScheduledRevisionRemoval () 
  { 
    return scheduledRevisionRemoval; 
  }
    
  bool PersistentDocument::mayIndex ()
  {
    return true;
    Log_PDoc ( "Element Indexation is disabled !\n" );
    return false;
  }

  void PersistentDocument::housewife ()
  {
    Log_PDoc ( "Calling [HOUSEWIFE] with refCount=%llx\n", refCount );
    if ( refCount == 1 )
      {
        getDocumentAllocator().housewife();
      }
    return;
  }

  void PersistentDocument::merge ( XProcessor& xproc, bool keepBranch )
  {
    if ( keepBranch )
      {
        NotImplemented ( "Merge [%llx:%llx] : not implemented : keepBranch=true\n", _brid(getBranchRevId()) );
      }
    AssertBug ( ! isWritable(), "[MERGE] : Could not merge writable document [%llx:%llx] !\n",
      _brid(getBranchRevId()) );

    BranchRevId forkedFrom = getStore().getBranchManager().getForkedFrom ( getBranchRevId().branchId );
    
    Log_PDoc ( "[MERGE] : At document [%llx:%llx] : forked from [%llx:%llx], merging to branchId=%llx\n",
      _brid(getBranchRevId()), _brid(forkedFrom), forkedFrom.branchId );
    
    if ( ! forkedFrom.branchId || ! forkedFrom.revisionId )
      {
        throwException ( Exception, "Invalid forkedFrom [%llx:%llx] for document [%llx:%llx]\n",
          _brid(forkedFrom), _brid(getBranchRevId()) );
      }
    
    Document* _mergeDocument = getStore().getBranchManager().openDocument ( forkedFrom.branchId, 0, DocumentOpeningFlags_Write );
    PersistentDocument* mergeDocument = dynamic_cast<PersistentDocument*> ( _mergeDocument );

    mergeDocument->incrementRefCount();

    AssertBug ( mergeDocument, "Merge document is null !\n" );
    AssertBug ( mergeDocument->isWritable(), "I have been provided a non-writable document !!!\n" );
    AssertBug ( mergeDocument->getRefCount(), "RefCount is null ???\n" );

    BranchRevId mergeBranchRevId = mergeDocument->getBranchRevId();

    AssertBug ( mergeBranchRevId.branchId == forkedFrom.branchId, "Diverging branches in merge !!\n" );

#ifdef __XEM_PERSISTANCE_ENABLE_QUICKMERGE
    if ( mergeBranchRevId.revisionId == forkedFrom.revisionId + 1 && getBranchRevId().revisionId == 1)
      {
        BranchRevId forkedFrom = getStore().getBranchManager().getForkedFrom ( getBranchRevId().branchId );
        AssertBug ( mergeDocument->getBranchRevId().branchId == forkedFrom.branchId, "Not a quickMerge !\n" );
        AssertBug ( mergeDocument->getBranchRevId().revisionId == forkedFrom.revisionId + 1, "Not a quickMerge !\n" );

        Warn ( "------------------------ QUICK MERGE : MERGE TO [%llx:%llx] ---------------------\n",
            _brid(mergeBranchRevId) );
        
        getPersistentDocumentAllocator().quickMerge ( mergeDocument->getPersistentDocumentAllocator() );

        Warn ( "------------------------ QUICK MERGE : COMMITTING [%llx:%llx] ---------------------\n",
            _brid(mergeBranchRevId) );

        mergeDocument->commit ();

        mergeDocument->getPersistentDocumentAllocator().flushInMemCaches ();
        mergeDocument->getPersistentDocumentAllocator().unmapAllAreas ();        

        // mergeDocument->release ();
        getStore().releaseDocument(mergeDocument);

        Info ( "[MERGE] : QUICK MERGE : [%llx:%llx] : forked from [%llx:%llx], merging to [%llx:%llx] ok !\n",
          _brid(getBranchRevId()), _brid(forkedFrom), _brid(mergeBranchRevId) );
  
#ifdef __XEM_PERSISTENCE_QUICKMERGE_EXTRA_CHECK
        PersistentDocument* mergeDocument2 = (PersistentDocument*)
            getStore().getBranchManager().openDocument ( forkedFrom.branchId, forkedFrom.revisionId+1, DocumentOpeningFlags_Read );
        Warn ( "------------------------ QUICK MERGE : CHECK MERGED CONTENTS ---------------------\n" );
        mergeDocument2->checkContents ();
#endif
        return;
      }
#endif // __XEM_PERSISTANCE_ENABLE_QUICKMERGE
    
    Warn ( "[MERGE] : Can't do QUICK MERGE. Applying journal from [%llx:%llx] : forked from [%llx:%llx], merging to [%llx:%llx] ok !\n",
          _brid(getBranchRevId()), _brid(forkedFrom), _brid(mergeBranchRevId) );
    
    try
      {
        mergeDocument->applyJournal ( xproc, *this );
        mergeDocument->commit ( xproc );
      }
    catch ( Exception * e )
      {
        detailException ( e, "Could not merge branch from [%llx:%llx] to [%llx:%llx]\n",
          _brid(getBranchRevId()), _brid(mergeDocument->getBranchRevId()) );
        mergeDocument->scheduleRevisionForRemoval ();
        // mergeDocument->release ();
        getStore().releaseDocument(mergeDocument);
        throw ( e );        
      }  

    Log_PDoc ( "[MERGE] : At document [%llx:%llx] : forked from [%llx:%llx], merging to [%llx:%llx] ok !\n",
      _brid(getBranchRevId()), _brid(forkedFrom), _brid(mergeDocument->getBranchRevId()) );

    getStore().releaseDocument(mergeDocument);
  }
  
  void PersistentDocument::commit ()
  {
    Bug ( "Forbidden call to commit() : shall call commit(XProcessor&) !\n" );
  }

  void PersistentDocument::commit ( XProcessor& xproc )
  {
    getPersistentDocumentAllocator().commit ();

    BranchFlags branchFlags = getBranchFlags ();

    if ( ( branchFlags & BranchFlags_AutoForked )
        && ( branchFlags & BranchFlags_AutoMerge ) )
      {
        merge (xproc, false);
        scheduleBranchForRemoval ();
      }
  }
  
  void PersistentDocument::drop ()
  {
    Warn ( "NotImplemented : dropping a persistent document.\n" );
    return;
    NotImplemented ( "dropping a persistent document.\n" );
  }

  bool PersistentDocument::mayCommit()
  {
    return getPersistentDocumentAllocator().mayCommit();
  }

  void PersistentDocument::dropAllPages ()
  {
    getPersistentDocumentAllocator().dropAllPages ();
  
  }
  
  void PersistentDocument::fork ( String& branchName, BranchFlags branchFlags )
  {
    if ( getDocumentOpeningFlags() == DocumentOpeningFlags_ExplicitRead )
      {
        Bug ( "forking a document which has been marked DocumentOpeningFlags_ExplicitRead !\n" );
      }
    AssertBug ( !isWritable() && !isLockedWrite(), "Document is writable or locked write !\n" );

    BranchRevId initialBranchRevId = getBranchRevId ();

    getPersistentDocumentAllocator().fork ( branchName, branchFlags );
    
    /**
     * Update reference
     */
    getStore().getBranchManager().updateDocumentReference ( this, initialBranchRevId, getBranchRevId() );
  }

  void PersistentDocument::reopen ()
  {
    getPersistentStore().getPersistentBranchManager().assertIsUnlocked();
    getPersistentStore().getPersistentBranchManager().assertBranchUnlockedForWrite(getBranchRevId().branchId);

    if ( getDocumentOpeningFlags() == DocumentOpeningFlags_ExplicitRead )
      {
        throwException ( Exception, "reopenning a document which has been marked DocumentOpeningFlags_ExplicitRead !\n" );
      }
    if ( getBranchFlags() & BranchFlags_MayNotBeXUpdated )
      {
        throwException ( Exception, "Document (tag=%s)(role=%s) [%llu:%llu], branch '%s' may not be XUpdated !\n",
          getDocumentTag().c_str(), getRole().c_str(), _brid(getBranchRevId()), getBranchName().c_str() );
      }
    
    BranchRevId initialBranchRevId = getBranchRevId ();

    getPersistentDocumentAllocator().reopen ( getBranchFlags() );

    if ( initialBranchRevId.branchId != getBranchRevId().branchId )
      {
        /**
         * Update document reference
         */
        getStore().getBranchManager().updateDocumentReference ( this, initialBranchRevId, getBranchRevId() );
      }
  }

  void PersistentDocument::grantWrite ()
  {
    if ( getDocumentOpeningFlags() == DocumentOpeningFlags_ExplicitRead )
      {
        throwException ( Exception, "reopenning a document which has been marked DocumentOpeningFlags_ExplicitRead !\n" );
      }

    if ( isLockedWrite() )
      {
        AssertBug ( isWritable(), "Doc is locked for write, but not writable ?\n" );
        return;
      }

    if ( isWritable() )
      {
        lockWrite();
        if ( isWritable() )
          return;
      }
    reopen ();
  }

  void PersistentDocument::lockWrite()
  {
    AssertBug ( isWritable(), "Will not lock write : document is not writable !\n" );
    getPersistentStore().getPersistentBranchManager().lockBranchForWrite(getBranchRevId().branchId);

    if ( ! isWritable() )
      {
        unlockWrite();
        Warn ( "After lockWrite(), document role=%s, brid=[%llx:%llx] is no longer writable !\n",
          getRole().c_str(), _brid(getBranchRevId()));
      }
  }

  void PersistentDocument::unlockWrite()
  {
    getPersistentStore().getPersistentBranchManager().unlockBranchForWrite(getBranchRevId().branchId);
  }

  bool PersistentDocument::isLockedWrite()
  {
    return getPersistentStore().getPersistentBranchManager().isBranchLockedForWrite(getBranchRevId().branchId);
  }
};

