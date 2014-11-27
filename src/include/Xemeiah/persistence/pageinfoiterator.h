#ifndef __XEM_PERSISTENCE_PAGEINFOITERATOR_H
#define __XEM_PERSISTENCE_PAGEINFOITERATOR_H

#include <Xemeiah/persistence/persistentdocumentallocator.h>

namespace Xem
{
    /**
     * Iterates over all known segment pages
     */
    class PageInfoIterator
    {
    protected:
        PersistentDocumentAllocator& allocator;
        RelativePagePtr relPagePtr;
        AbsolutePageRef<PageInfoPage> pageInfoPageRef;
        __ui64 idx;
        bool write;

    public:
        PageInfoIterator (PersistentDocumentAllocator& _allocator, RelativePagePtr _relPagePtr = 0, bool _write = false) :
            allocator(_allocator), pageInfoPageRef(_allocator.getPersistentStore())
        {
            relPagePtr = _relPagePtr;
            write = _write;

            idx = 0;

            pageInfoPageRef = allocator.doGetPageInfoPage(relPagePtr, idx, write);
            AssertBug(pageInfoPageRef.getPage(), "Could not get page info !\n");
        }

        PageInfoIterator&
        operator++ (int i)
        {
            idx++;
            relPagePtr += PageSize;
            if (idx == PageInfo_pointerNumber && relPagePtr < allocator.getNextRelativePagePtr())
            {
                pageInfoPageRef = allocator.doGetPageInfoPage(relPagePtr, idx, write);
                AssertBug(pageInfoPageRef.getPage(), "Could not get page info !\n");
            }
            return *this;
        }

        RelativePagePtr
        first ()
        {
            return relPagePtr;
        }

        PageInfo
        second ()
        {
            AssertBug(relPagePtr < allocator.getNextRelativePagePtr(), "OOB !\n");
            AssertBug(pageInfoPageRef.getPage(), "NULL pageInfoPage !\n");
            return pageInfoPageRef.getPage()->pageInfo[idx];
        }

        operator bool ()
        {
            return (relPagePtr < allocator.getNextRelativePagePtr());
        }
    };

}
#endif // __XEM_PERSISTENCE_PAGEINFOITERATOR_H
