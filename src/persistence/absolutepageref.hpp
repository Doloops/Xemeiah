#ifndef __XEM_PERSISTENCE_ABSOLUTEPAGEREF_H
#include <Xemeiah/persistence/absolutepageref.h>
#include <Xemeiah/persistence/persistentstore.h>
#endif // __XEM_PERSISTENCE_ABSOLUTEPAGEREF_H

#define Log_APR(...) Debug("[APR]" __VA_ARGS__)

namespace Xem
{
    template<typename T>
        __INLINE
        AbsolutePageRef<T>::AbsolutePageRef (const PersistentStore& persistentStore)
        {
            AssertBug(&persistentStore != NULL, "Should not be instantiated with NULL persistentStore here !\n");
            this->persistentStore = (PersistentStore*) &persistentStore;
            this->pagePtr = NullPage;
            this->page = NULL;
        }

    template<typename T>
        __INLINE
        AbsolutePageRef<T>::AbsolutePageRef (PersistentStore* persistentStore, const AbsolutePagePtr pagePtr)
        {
            AssertBug(persistentStore != NULL, "Should not be instantiated with NULL persistentStore here !\n");
            this->persistentStore = persistentStore;
            this->pagePtr = pagePtr;
            this->page = NULL;
        }

    template<typename T>
        __INLINE
        AbsolutePageRef<T>::AbsolutePageRef (const PersistentStore& persistentStore, const AbsolutePagePtr pagePtr)
        {
            AssertBug(&persistentStore != NULL, "Should not be instantiated with NULL persistentStore here !\n");
            this->persistentStore = (PersistentStore*) &persistentStore;
            this->pagePtr = pagePtr;
            this->page = NULL;
        }
    /**
     * This is called stealing a reference
     */
    template<typename T>
        __INLINE
        AbsolutePageRef<T>::AbsolutePageRef (PersistentStore* persistentStore, T* page, const AbsolutePagePtr pagePtr)
        {
            AssertBug(persistentStore == NULL, "Should be instantiated with NULL persistentStore here !\n");
            AssertBug(page != NULL, "Should not be instantiated with NULL page here !\n");
            this->persistentStore = persistentStore;
            this->page = page;
            this->pagePtr = pagePtr;
        }

    template<typename T>
        __INLINE
        AbsolutePageRef<T>::AbsolutePageRef (const AbsolutePageRef& pageRef)
        {
            AssertBug(pageRef.persistentStore != NULL || pageRef.page != NULL,
                    "Invalid NULL pageRef : persistentStore=%p, pagePtr=%llx, page=%p !\n",
                    pageRef.persistentStore, pageRef.pagePtr, pageRef.page);
            this->persistentStore = pageRef.persistentStore;
            this->pagePtr = pageRef.pagePtr;
            if (this->persistentStore != NULL)
            {
                this->page = NULL;
            }
            else
            {
                this->page = pageRef.page;
            }
            Log_APR("Ref by constructor(const&)  : pagePtr=%llx\n", pageRef.pagePtr);
        }

    template<typename T>
        __INLINE
        AbsolutePageRef<T>::~AbsolutePageRef ()
        {
            if (persistentStore != NULL && page != NULL)
            {
                Log_APR("Release page : pagePtr=%llx\n", pagePtr);
                persistentStore->__releasePage(pagePtr);
            }
        }

    template<typename T>
        __INLINE
        T*
        AbsolutePageRef<T>::getPage ()
        {
            if (page == NULL)
            {
                AssertBug(persistentStore != NULL, "Null persistentStore provided here !");
                Log_APR("Access to page : pagePtr=%llx\n", pagePtr);
                page = (T*) persistentStore->__getAbsolutePage(pagePtr);
            }
            return page;
        }

    template<typename T>
        __INLINE
        AbsolutePageRef<T>&
        AbsolutePageRef<T>::operator= (const AbsolutePageRef& pageRef)
        {
            if (persistentStore != NULL && page != NULL)
            {
                Log_APR("Because ref by copy : Release page : pagePtr=%llx\n", pagePtr);
                persistentStore->__releasePage(pagePtr);
            }
            this->persistentStore = pageRef.persistentStore;
            this->pagePtr = pageRef.pagePtr;
            if (persistentStore != NULL)
            {
                this->page = NULL;
            }
            else
            {
                this->page = pageRef.page;
            }
            Log_APR("Ref by copy : store=%p, pagePtr=%llx, page=%p (from %p)\n", persistentStore, pagePtr, page,
                    pageRef.page);
            return *this;
        }

    template<typename T>
        __INLINE
        AbsolutePageRef<T>
        AbsolutePageRef<T>::steal ()
        {
            T* page = getPage();
            AssertBug(page, "Could not get page for pagePtr=%llx\n", pagePtr);
            return AbsolutePageRef(NULL, page, pagePtr);
        }
}
