#ifndef __XEM_PERSISTENCE_ABSOLUTEPAGEREF_H
#define __XEM_PERSISTENCE_ABSOLUTEPAGEREF_H

#include <Xemeiah/trace.h>

#include <Xemeiah/kern/format/core_types.h>
#include <Xemeiah/kern/format/document_head.h>
#include <Xemeiah/persistence/format/superblock.h>
#include <Xemeiah/persistence/format/revision.h>
#include <Xemeiah/persistence/format/pages.h>

namespace Xem
{
    template<typename T>
        class AbsolutePageRef
        {
            friend class PersistentStore;
            friend class PersistentBranchManager;
            friend class PersistentDocumentAllocator;
            friend class PageInfoIterator;
        private:
            PersistentStore* persistentStore;
            AbsolutePagePtr pagePtr;
            T* page;
        protected:
            AbsolutePageRef (const PersistentStore& persistentStore);

            AbsolutePageRef (PersistentStore* persistentStore, const AbsolutePagePtr pagePtr);

            AbsolutePageRef (const PersistentStore& persistentStore, const AbsolutePagePtr pagePtr);

            /**
             * This is called stealing a reference
             */
            AbsolutePageRef (PersistentStore* persistentStore, T* page, const AbsolutePagePtr pagePtr);
        public:
            INLINE
            AbsolutePageRef (const AbsolutePageRef& pageRef);

            INLINE
            ~AbsolutePageRef ();

            INLINE
            AbsolutePagePtr
            getPagePtr () const
            {
                return pagePtr;
            }

            INLINE
            T*
            getPage ();

            INLINE
            AbsolutePageRef&
            operator= (const AbsolutePageRef& pageRef);

            INLINE
            AbsolutePageRef steal();
        };
}
#endif // __XEM_PERSISTENCE_ABSOLUTEPAGEREF_H
