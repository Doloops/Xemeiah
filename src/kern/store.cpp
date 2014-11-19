#include <Xemeiah/kern/store.h>
#include <Xemeiah/kern/branchmanager.h>
#include <Xemeiah/kern/keycache.h>
#include <Xemeiah/kern/document.h>
#include <Xemeiah/trace.h>
#include <Xemeiah/xprocessor/xprocessorlibs.h>
#include <Xemeiah/kern/servicemanager.h>

#include <Xemeiah/auto-inline.hpp>

#include <list>

#define Log_Store Log

namespace Xem
{

  Store::Store() :
    keyCache(*this),
    volatileAreasMutex("Volatile Areas")
  {
    AssertBug ( sizeof(DocumentHead) <= PageSize, "Revision Page too large : '%llx' !\n", (__ui64) sizeof(DocumentHead) );
    xprocessorLibs = NULL;
    serviceManager = NULL;

#ifdef __XEM_STORE_VOLATILEAREAS_USE_MMAP
    volatileAreasFD = -1;
#endif // __XEM_STORE_VOLATILEAREAS_USE_MMAP
  }

  Store::~Store()
  {
    if (xprocessorLibs)
      delete (xprocessorLibs);
    String_ShowStringStats ();
  }

  void Store::releaseDocument ( Document* doc )
  {
    if ( doc->isLockedWrite() )
      {
        if ( doc->getRole() != "none" )
          Warn ( "Document role='%s' brId=[%llx:%llx] was locked write !\n", doc->getRole().c_str(), _brid(doc->getBranchRevId()) );
        doc->unlockWrite();
      }

    bool deletionImpliesLockBranchManager = doc->deletionImpliesLockBranchManager();
    if ( deletionImpliesLockBranchManager )
      getBranchManager().lockBranchManager();

    doc->lockRefCount();
    AssertBug ( doc->refCount, "Empty refcount !\n" );

    doc->releaseDocumentResources();

    doc->decrementRefCountLockLess();

    Log_Store ("Doc %p, refCount=%llu\n", doc, doc->refCount);
    if ( doc->refCount )
      {
        // doc->housewife();
        doc->unlockRefCount();
        if ( deletionImpliesLockBranchManager )
        {
          getBranchManager().unlockBranchManager();
        }
        return;
      }

    Log_Store ( "Effective document deletion of %p\n", doc );
    doc->unlockRefCount();
    delete ( doc );
    if ( deletionImpliesLockBranchManager )
      getBranchManager().unlockBranchManager();
    return;
  }

  XProcessorLibs&
  Store::getXProcessorLibs()
  {
#if PARANOID
    AssertBug ( xprocessorLibs, "xprocessorLibs not instanciated !\n" );
#endif
    return *xprocessorLibs;
  }

  ServiceManager& Store::getServiceManager()
  {
    if ( ! serviceManager )
      {
        serviceManager = new ServiceManager(*this);
        Log_Store ( "Created new service manager at : %p\n", serviceManager );
      }
    return *serviceManager;
  }

  bool
  Store::buildKeyCacheBuiltinKeys()
  {
    if (!keyCache.buildBuiltinKeys())
      {
        Error ( "Could not build builtin-keys !\n" );
      }
    xprocessorLibs = new XProcessorLibs(*this);
    xprocessorLibs->registerStaticLibs();
    return true;
  }

  void
  Store::housewife()
  {
    stats.showStats();
    Info ( "\tCurrent VolatileAreas : %llu MBytes (%llu areas)\n",
        (volatileAreas.size() * DocumentAllocator::AreaSize) >> 20,
        (__ui64) volatileAreas.size() );
  }
};

