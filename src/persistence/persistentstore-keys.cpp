#include <Xemeiah/persistence/persistentstore.h>
#include <Xemeiah/kern/keycache.h>

#include <Xemeiah/kern/exception.h>

#include <Xemeiah/trace.h>

#include <Xemeiah/kern/format/core_types.h>
#include <Xemeiah/persistence/format/keys.h>

#include <Xemeiah/auto-inline.hpp>
#include <Xemeiah/persistence/auto-inline.hpp>

#define Log_Keys Debug

namespace Xem
{
    /*
     * Load namespaces and keys from an (already formatted) Persistence file.
     * Namespaces and Keys are inserted manually in the keyCache.
     */
    void
    PersistentStore::loadKeysFromStore ()
    {
        if (getSB()->keyPage == 0)
        {
            Log_Keys ( "No keys pages !\n" );
        }
        Log_Keys ( "------------------------ Loading Keys ---------------------------\n" );
        for (AbsolutePageRef<KeyPage> keyPageRef = getAbsolutePage<KeyPage>(getSB()->keyPage); keyPageRef.getPage();
                keyPageRef = getAbsolutePage<KeyPage>(keyPageRef.getPage()->nextPage))
        {
            for (__ui64 idx = 0; idx < KeyPage_keyNumber && keyPageRef.getPage()->keys[idx].id; idx++)
            {
                Log_Keys ( "Load key %x : %s\n", keyPageRef.getPage()->keys[idx].id, keyPageRef.getPage()->keys[idx].name );
                char* name = strdup(keyPageRef.getPage()->keys[idx].name);
                keyCache.localKeyMap.put(keyPageRef.getPage()->keys[idx].id, name);
                keyCache.keysBucket.put(keyPageRef.getPage()->keys[idx].id, name);
            }
        }
        Log_Keys ( "------------------------ Loading Namespaces ---------------------------\n" );
        for (AbsolutePageRef<NamespacePage> namespacePageRef = getAbsolutePage<NamespacePage>(getSB()->namespacePage); namespacePageRef.getPage();
                namespacePageRef = getAbsolutePage<NamespacePage>(namespacePageRef.getPage()->nextPage))
        {
            for (__ui64 idx = 0; idx < NamespacePage_namespaceNumber && namespacePageRef.getPage()->namespaces[idx].namespaceId;
                    idx++)
            {
                Log_Keys ( "Load namespace %x : %s\n",
                        namespacePageRef.getPage()->namespaces[idx].namespaceId, namespacePageRef.getPage()->namespaces[idx].url );
                char* url = strdup(namespacePageRef.getPage()->namespaces[idx].url);
                keyCache.namespaceBucket.put(namespacePageRef.getPage()->namespaces[idx].namespaceId, url);
                keyCache.namespaceMap.put(namespacePageRef.getPage()->namespaces[idx].namespaceId, url);
            }
        }
        Log_Keys ( "------------------------ End of Namespaces Loading ---------------------------\n" );
        bool result = buildKeyCacheBuiltinKeys();
        if ( ! result )
        {
            throwException(PersistenceException, "Could not build key cache !");
        }
    }

    LocalKeyId
    PersistentStore::addKeyInStore (const char* keyName)
    /*
     * Called with the KeyStore key_lock held, and returns with the key_lock held...
     */
    {
        if (strlen(keyName) >= KeySegment::nameLength)
        {
            Bug("KeyName too long : '%s'\n", keyName);
            return 0;
        }
        AbsolutePageRef<KeyPage> keyPageRef(*this);
        if (getSB()->keyPage == NullPage)
        {
            /*
             * No key page, shall create one before
             */
            AbsolutePagePtr keyPagePtr = getFreePagePtr();

            keyPageRef = getAbsolutePage<KeyPage>(keyPagePtr);
            Log_Keys("New keyPage %llx\n", keyPagePtr);
            alterPage(keyPageRef.getPage());
            memset(keyPageRef.getPage(), 0, PageSize);
            protectPage(keyPageRef.getPage());
            lockSB();
            getSB()->keyPage = keyPagePtr;
            unlockSB();
        }
        else
        {
            keyPageRef = getAbsolutePage<KeyPage>(getSB()->keyPage);
            Log_Keys("Use keyPage from SB %llx\n", getSB()->keyPage);
        }

        LocalKeyId nextKeyId = 0;
        for (; keyPageRef.getPage();)
        {
            AssertBug(keyPageRef.getPage(), "Null page for ref %llx\n", keyPageRef.getPagePtr());
            Log_Keys("At page %llx\n", keyPageRef.getPagePtr());

            /*
             * It is asserted that keyPages have no hole, because keys are created continuously,
             * and never deleted.
             */
            for (__ui64 idx = 0; idx < KeyPage_keyNumber; idx++)
            {
                Log_Keys("keyPagePtr=%llx, Page %p, idx=%llx\n", keyPageRef.getPagePtr(), keyPageRef.getPage(), idx);
                if (keyPageRef.getPage()->keys[idx].id == 0)
                {
                    Log_Keys ( "Empty slot %llx at page %p\n", idx, keyPageRef.getPage() );
                    nextKeyId++;

                    alterPage(keyPageRef.getPage());
                    keyPageRef.getPage()->keys[idx].id = nextKeyId;
                    strcpy(keyPageRef.getPage()->keys[idx].name, keyName);
                    protectPage(keyPageRef.getPage());

                    return nextKeyId;
                }
                else
                {
                    nextKeyId = keyPageRef.getPage()->keys[idx].id;
                }
            }
            if (keyPageRef.getPage()->nextPage)
            {
                keyPageRef =getAbsolutePage<KeyPage>(keyPageRef.getPage()->nextPage);
                continue;
            }
            else if (!keyPageRef.getPage()->nextPage)
            {
                Log_Keys ( "Creating keyPage..\n" );
                AbsolutePagePtr nextPagePtr = getFreePagePtr();
                AbsolutePageRef<KeyPage> nextPageRef = getAbsolutePage<KeyPage>(nextPagePtr);

                Log_Keys ( "Created new keyPage=%llx/%p, continuation of %p\n", nextPagePtr, nextPageRef.getPage(), keyPageRef.getPage() );
                alterPage(nextPageRef.getPage());
                memset(nextPageRef.getPage(), 0, PageSize);
                protectPage(nextPageRef.getPage());
                alterPage(keyPageRef.getPage());
                keyPageRef.getPage()->nextPage = nextPagePtr;
                protectPage(keyPageRef.getPage());
                keyPageRef = nextPageRef;
                continue;
            }
            Bug("Shall not be here !!!\n");
        }
        Bug("Shall not be here.\n");
        return 0;
    }

    NamespaceId
    PersistentStore::addNamespaceInStore (const char* namespaceURL)
    /*
     * Called with the KeyStore key_lock held, and returns with the key_lock held...
     */
    {
        AbsolutePageRef<NamespacePage> namespacePageRef(*this);
        __ui32 len = strlen(namespaceURL) + 1;
        if (len > NamespaceSegment::urlLength)
        {
            Bug("Namespace too large : '%s'\n", namespaceURL);
        }
        if (getSB()->namespacePage == 0)
        {
            /*
             *  No namespace page, shall create the first one
             */
            AbsolutePagePtr namespacePagePtr = getFreePagePtr();
            namespacePageRef = getAbsolutePage<NamespacePage>(namespacePagePtr);
            alterPage(namespacePageRef.getPage());
            memset(namespacePageRef.getPage(), 0, PageSize);
            protectPage(namespacePageRef.getPage());
            lockSB();
            getSB()->namespacePage = namespacePagePtr;
            unlockSB();
        }
        else
        {
            namespacePageRef = getAbsolutePage<NamespacePage>(getSB()->namespacePage);
        }

        NamespaceId nextNamespaceId = 1;
        for (; namespacePageRef.getPage();)
        {
            /*
             * It is asserted that namespace pages have no hole, because all namespaces are created
             * continuously, and never deleted.
             */
            if (namespacePageRef.getPage()->nextPage)
            {
                nextNamespaceId = namespacePageRef.getPage()->namespaces[NamespacePage_namespaceNumber - 1].namespaceId + 1;
                namespacePageRef = getAbsolutePage<NamespacePage>(namespacePageRef.getPage()->nextPage);
                continue;
            }
            for (__ui64 idx = 0; idx < NamespacePage_namespaceNumber; idx++)
            {
                if (namespacePageRef.getPage()->namespaces[idx].namespaceId == 0)
                {
                    Log_Keys ( "Empty slot %llx at page %p\n", idx, namespacePageRef.getPage() );
                    alterPage(namespacePageRef.getPage());
                    namespacePageRef.getPage()->namespaces[idx].namespaceId = nextNamespaceId;
                    strcpy(namespacePageRef.getPage()->namespaces[idx].url, namespaceURL);
                    protectPage(namespacePageRef.getPage());
                    return nextNamespaceId;
                }
                nextNamespaceId = namespacePageRef.getPage()->namespaces[idx].namespaceId + 1;
            }

            Log_Keys ( "Creating new namespacePage..\n" );
            AbsolutePagePtr nextPagePtr = getFreePagePtr();
            AbsolutePageRef<NamespacePage> nextPageRef = getAbsolutePage<NamespacePage>(nextPagePtr);
            Log_Keys ( "Created new namespacePage=%llx/%p, continuation of %p\n", nextPagePtr, nextPageRef.getPage(), namespacePageRef.getPage() );
            alterPage(nextPageRef.getPage());
            memset(nextPageRef.getPage(), 0, PageSize);
            protectPage(nextPageRef.getPage());
            alterPage(namespacePageRef.getPage());
            namespacePageRef.getPage()->nextPage = nextPagePtr;
            protectPage(namespacePageRef.getPage());
            namespacePageRef = nextPageRef;
        }
        Bug("Shall not be here.\n");
        return 0;
    }
}
;

