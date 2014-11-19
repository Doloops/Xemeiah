/*
 * xemfsfuseservice-background.cpp
 *
 *  Created on: 1 nov. 2009
 *      Author: francois
 */

#include <Xemeiah/xemfs-fuse/xemfsfuseservice.h>
#include <Xemeiah/kern/documentref.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_FuseBackground Debug

namespace Xem
{
  void XemFSFuseService::initBackgroundFetchThreads ( __ui64 _maxConcurrentThreads )
  {
    unsigned int maxConcurrentThreads = (unsigned int) _maxConcurrentThreads;
    if ( sem_init ( &backgroundFetchSemaphore, 0, maxConcurrentThreads ) )
      {
        throwException ( Exception, "Could not initialize background fetch threads : error=%d:%s\n",
            errno, strerror(errno) );
      }
  }

  class BackgroundFetchThreadArguments
  {
  public:
    BlobRef blobRef;
    String href;
    BackgroundFetchThreadArguments ( BlobRef& _b, const String& _h )
    : blobRef(_b), href(_h)
    {

    }

    ~BackgroundFetchThreadArguments() {}
  };

  void XemFSFuseService::backgroundFetchWindowEvent ( BlobRef& blob )
  {
    Log_FuseBackground ( "[BACKFETCH] Unlock for blob : %s\n", blob.generateVersatileXPath().c_str() );
    Document& doc = blob.getDocument();
    AssertBug ( doc.isWritable(), "Doc not writable !\n" );
    doc.unlockWrite ();
    Log_FuseBackground ( "[BACKFETCH] Unlocked for blob : %s\n", blob.generateVersatileXPath().c_str() );
    usleep ( 10 );

    if ( isStopping() )
      {
        Warn ( "Service is stopping ! We should stop copying something here...\n" );
      }

    Log_FuseBackground ( "[BACKFETCH] Locking for blob : %s\n", blob.generateVersatileXPath().c_str() );
    doc.grantWrite();
    Log_FuseBackground ( "[BACKFETCH] Locked for blob : %s\n", blob.generateVersatileXPath().c_str() );
    AssertBug ( doc.isWritable(), "Doc not writable !\n" );
  }

  void __xem_fuse_unlockBlob ( BlobRef& blob, void* arg )
  {
    AssertBug ( blob, "NULL Blob provided !\n" );
    XemFSFuseService* service = (XemFSFuseService*) arg;
    service->backgroundFetchWindowEvent(blob);
  }

  void XemFSFuseService::backgroundFetchThread ( void* _arg )
  {
    BackgroundFetchThreadArguments* args = (BackgroundFetchThreadArguments*) _arg;

    BlobRef blob = args->blobRef;
    String href = args->href;
    delete ( args );

    sem_wait ( &backgroundFetchSemaphore );

    ElementRef file = blob.getElement();
    DocumentRef doc (getXProcessor(), blob.getDocument());

    doc->grantWrite();

    AssertBug ( doc->isWritable(), "Document not writable !\n" );
    AssertBug ( doc->isLockedWrite(), "Document not locked write !\n" );

    if ( file.hasAttr(xem_fs.contents(), AttributeType_Blob) )
      {
        Bug ( "Already finished copying !\n");
      }
    if ( ! file.hasAttr(xem_fs.contents_temp(), AttributeType_Blob) )
      {
        Bug ( "Lost the race ?\n" );
      }

    blob.readFromFile(href, &__xem_fuse_unlockBlob, this );

    blob.rename ( xem_fs.contents() );
    Log_FuseBackground ( "FUSE-READ : Blob filled Ok from '%s', continuing...\n", href.c_str() );

    doc->commit ();

    Info ( "Finished background fetch for '%s' (size=%llu)",
        href.c_str(), blob.getSize() );
    sem_post ( &backgroundFetchSemaphore );
  }

  void XemFSFuseService::doBackgroundFetch ( ElementRef& file, const String& path )
  {
    if ( file.hasAttr(xem_fs.contents(), AttributeType_Blob) )
      {
        Bug ( "Already finished copying !\n");
      }
    else if ( file.hasAttr(xem_fs.contents_temp(), AttributeType_Blob) )
      {
        Bug ( "Lost the race !\n" );
      }

    Document& doc = file.getDocument();
    doc.incrementRefCount();

#if 0
    if ( ! doc.isWritable() )
      doc.reopen ();
    else if ( ! doc.isLockedWrite() )
      doc.lockWrite();
#endif
    doc.grantWrite();

    if ( file.hasAttr(xem_fs.contents(), AttributeType_Blob)
        || file.hasAttr(xem_fs.contents_temp(), AttributeType_Blob) )
      {
        Warn ( "Lost the race for file : %s!\n", path.c_str() );
        doc.unlockWrite();
        return;
      }

    AssertBug ( doc.isWritable(), "Document not writable !\n" );
    AssertBug ( doc.isLockedWrite(), "Document not locked for write !\n" );

    BlobRef blob = file.addBlob(xem_fs.contents_temp());
    doc.unlockWrite();

    BackgroundFetchThreadArguments* args = new BackgroundFetchThreadArguments(blob, path);
    /*
     * TODO Reimplement Background fetch threads
     */
    NotImplemented ( "Background fetch threads !\n" );
    // startThread( (ThreadFunction)(&XemFSFuseService::backgroundFetchThread), (void*) args );
  }


#ifdef __XEMFS_FUSE_KEEP_DOCUMENT_REFERENCE
  void XemFSFuseService::backgroundCommitThread ( void * arg )
  {
    Info ( "Starting background Commit thread.\n" );
    while ( true )
      {
        Info ( "[BACKGROUND].\n" );
        sleep ( 5 );

        if ( isStopping() )
          {
            Info ( "Quitting background Commit thread.\n" );
            return;
          }

        if ( ! document )
          {
            Info ( "Skipping cause no document.\n" );
            continue;
          }
        if ( document->getRefCount() > 1 )
          {
            Info ( "Skipping cause document refCount=%lx.\n", (unsigned long) document->getRefCount() );
            continue;
          }
        if ( document->isWritable() && document->mayCommit() )
          {
            Info ( "Committing document.\n" );
            document->lockWrite();
            document->commit();
            Info ( "Committed document.\n" );
          }
      }
  }
#else
  void XemFSFuseService::backgroundCommitThread ( void * arg )
  {

  }
#endif
};
