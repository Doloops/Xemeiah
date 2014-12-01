#ifndef __XEM_PERSISTENCE_PAGEINFOCACHE_H
#define __XEM_PERSISTENCE_PAGEINFOCACHE_H

#include <Xemeiah/kern/format/core_types.h>

#define __XEM_PERSISTENCE_PAGEINFOCACHE_VECTOR

#include <map>
#include <vector>

namespace Xem
{
    class PersistentDocumentAllocator;

    class PageInfoPageItem
    {
    private:
        AbsolutePageRef<PageInfoPage> pageInfoPageRef;
        bool stolen;
    public:
        PageInfoPageItem(const AbsolutePageRef<PageInfoPage>& _pageInfoPageRef, bool _stolen)
        : pageInfoPageRef(_pageInfoPageRef)
        {
            stolen = _stolen;
        }

        AbsolutePageRef<PageInfoPage>& getPageInfoPageRef()
        {
            return pageInfoPageRef;
        }
        bool isStolen()
        {
            return stolen;
        }
    };

    typedef std::vector<PageInfoPageItem*> PageInfoPageVector;
    typedef std::map<RelativePagePtr, PageInfoPageItem*> PageInfoPageMap;

    class PageInfoCache
    {
        friend class PersistentDocumentAllocator;
    private:
        PersistentDocumentAllocator& persistentDocumentAllocator;
#ifdef __XEM_PERSISTENCE_PAGEINFOCACHE_VECTOR
        PageInfoPageVector pageInfoPageVector;
#else
        PageInfoPageMap pageInfoPageMap;
#endif
    protected:
        PageInfoCache(PersistentDocumentAllocator& persistentDocumentAllocator);
        ~PageInfoCache();
    public:
        void flush();

        INLINE AbsolutePageRef<PageInfoPage>
        getPageInfoPage (RelativePagePtr relativePagePtr, bool write );

        INLINE void
        putPageInfoPage( RelativePagePtr relativePagePtr, AbsolutePageRef<PageInfoPage> pageInfoPageRef, bool write);
    };

}

#endif // PAGEINFOCACHE
