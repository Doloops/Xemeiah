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
        PageInfoPage* pageInfoPage;
        __ui64 idx;
        bool write;

    public:
        PageInfoIterator (PersistentDocumentAllocator& _allocator, RelativePagePtr _relPagePtr = 0, bool _write = false) :
                allocator(_allocator)
        {
            relPagePtr = _relPagePtr;
            write = _write;

            pageInfoPage = NULL;
            idx = 0;

            if (!allocator.doGetPageInfoPage(relPagePtr, pageInfoPage, idx, write))
            {
                Bug("Could not get page info !\n");
            }
        }

        PageInfoIterator&
        operator++ (int i)
        {
            idx++;
            relPagePtr += PageSize;
            if (idx == PageInfo_pointerNumber && relPagePtr < allocator.getNextRelativePagePtr())
            {
                if (!allocator.doGetPageInfoPage(relPagePtr, pageInfoPage, idx, write))
                {
                    Bug("Could not get page info !\n");
                }
            }
            return *this;
        }

        RelativePagePtr
        first ()
        {
            return relPagePtr;
        }

        PageInfo&
        second ()
        {
            AssertBug(relPagePtr < allocator.getNextRelativePagePtr(), "OOB !\n");
            AssertBug(pageInfoPage, "NULL pageInfoPage !\n");
            return pageInfoPage->pageInfo[idx];
        }

        operator bool ()
        {
            return (relPagePtr < allocator.getNextRelativePagePtr());
        }
    };

}
#endif // __XEM_PERSISTENCE_PAGEINFOITERATOR_H
