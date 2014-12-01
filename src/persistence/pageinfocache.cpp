#include <Xemeiah/persistence/persistentdocumentallocator.h>
#include <Xemeiah/persistence/persistentbranchmanager.h>
#include <Xemeiah/persistence/pageinfocache.h>
#include <Xemeiah/trace.h>

#include <Xemeiah/auto-inline.hpp>
#include <Xemeiah/persistence/auto-inline.hpp>

namespace Xem
{
    PageInfoCache::PageInfoCache (PersistentDocumentAllocator& _persistentDocumentAllocator) :
            persistentDocumentAllocator(_persistentDocumentAllocator)
    {

    }

    PageInfoCache::~PageInfoCache ()
    {

    }

    void
    PageInfoCache::flush ()
    {

    }
}
