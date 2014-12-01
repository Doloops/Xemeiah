#include <Xemeiah/persistence/persistentdocumentallocator.h>
#include <Xemeiah/persistence/persistentbranchmanager.h>
#include <Xemeiah/persistence/pageinfocache.h>
#include <Xemeiah/trace.h>

#include <vector>

#define Log_PageInfoCache Log

namespace Xem
{
    __INLINE AbsolutePageRef<PageInfoPage>
    PageInfoCache::getPageInfoPage (RelativePagePtr relativePagePtr, bool write)
    {
        __ui64 pageInfoIndex = relativePagePtr / PageInfo_pointerNumber;

        Log_PageInfoCache ( "PageInfoCache GET : relativePagePtr=%llx, pageInfoIndex=%llx\n", relativePagePtr, pageInfoIndex);
#ifdef __XEM_PERSISTENCE_PAGEINFOCACHE_VECTOR
        if (pageInfoIndex < pageInfoPageVector.size() && pageInfoPageVector[pageInfoIndex] != NULL)
        {
            Log_PageInfoCache ( "PageInfoCache GET [%llx] : [%llx] => %llx\n", relativePagePtr, pageInfoIndex, pageInfoPageVector[pageInfoIndex]->getPageInfoPageRef().getPagePtr() );
            return pageInfoPageVector[pageInfoIndex]->getPageInfoPageRef().steal();
        }
#else
        PageInfoPageMap::iterator iter = pageInfoPageMap.find(pageInfoIndex);
        if ( iter != pageInfoPageMap.end() && (!write || iter->second->isStolen() ) )
        {
            Log_PageInfoCache ( "PageInfoCache GET [%llx] : [%llx] => %llx\n", relativePagePtr, iter->first, iter->second->getPageInfoPageRef().getPagePtr() );
            return iter->second->getPageInfoPageRef().steal();
        }
#endif

        return AbsolutePageRef<PageInfoPage>(persistentDocumentAllocator.getPersistentStore(), 0);
    }

    __INLINE void
    PageInfoCache::putPageInfoPage (RelativePagePtr relativePagePtr, AbsolutePageRef<PageInfoPage> pageInfoPageRef,
                                    bool write)
    {
        __ui64 pageInfoIndex = relativePagePtr / PageInfo_pointerNumber;

        Log_PageInfoCache ( "PageInfoCache PUT : relativePagePtr=%llx, pageInfoIndex=%llx\n", relativePagePtr, pageInfoIndex);
#ifdef __XEM_PERSISTENCE_PAGEINFOCACHE_VECTOR
        if ( pageInfoPageVector.size() <= pageInfoIndex )
        {
            __ui64 newSize = pageInfoIndex ? (pageInfoIndex * 2 ) : 16;
            pageInfoPageVector.resize(newSize);
        }
        pageInfoPageVector[pageInfoIndex] = new PageInfoPageItem(pageInfoPageRef, write);
#else
        pageInfoPageMap[pageInfoIndex] = new PageInfoPageItem(pageInfoPageRef, write);
#endif
    }

};
