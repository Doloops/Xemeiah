#include <Xemeiah/kern/store.h>
#include <Xemeiah/kern/keycache.h>
#include <Xemeiah/kern/namespacealias.h>
#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/dom/attributeref.h>
#include <Xemeiah/trace.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_Keys Debug

namespace Xem
{
    KeyCache::KeyCache (Store& __s) :
            store(__s), writeMutex("KeyCache Write")
    {
        builtinKeys = NULL;
    }

    KeyCache::~KeyCache ()
    {
        unloadKeys();
    }

    void
    KeyCache::unloadKeys ()
    {
        keysBucket.clear();
        namespaceBucket.clear();
    }

    LocalKeyId
    KeyCache::buildNamespacePrefix (NamespaceAlias& nsAlias, NamespaceId nsId)
    {
        char prefixKey[256];
        for (__ui32 id = 0; id < 256; id++)
        {
            sprintf(prefixKey, "ns%d", id);
            LocalKeyId prefixId = getKeyId(0, prefixKey, true);
            if (nsAlias.getNamespaceIdFromPrefix(prefixId))
            {
                continue;
                NotImplemented(
                        "For namespaceId=%x, could not assign key '%s' (id %x) because it is already mapped to namespaceId=%x\n",
                        nsId, prefixKey, prefixId, nsAlias.getNamespaceIdFromPrefix(prefixId));
            }
            Log_Keys ( "Building prefix : associating prefix='%s' (%x) to nsId %x (%s)\n",
                    prefixKey, prefixId, nsId, getNamespaceURL ( nsId ) );
            return prefixId;
        }
        Bug("Could not generate a prefix for anonymous NamespaceAlias nsId=%x\n", nsId);
        return 0;
    }

    KeyId
    KeyCache::buildNamespaceDeclaration (LocalKeyId prefixId)
    {
        return getKeyId(getBuiltinKeys().xmlns.ns(), prefixId);
    }

    NamespaceId
    KeyCache::getNamespaceId (const char* namespaceURL)
    {
        if (!*namespaceURL)
        {
            return 0;
        }
        NamespaceId nsId = namespaceBucket.get(namespaceURL);
        if (!nsId)
        {
            nsId = store.addNamespaceInStore(namespaceURL);

            Log_Keys ( "[NEW-NSALIAS] Create namespace alias %x to '%s'\n", nsId, namespaceURL );
            namespaceMap.put(nsId, strdup(namespaceURL));
            namespaceBucket.put(nsId, namespaceMap.get(nsId));
        }
        return nsId;
    }

    const char*
    KeyCache::getNamespaceURL (NamespaceId nsId)
    {
        if (nsId == 0)
            return "";
        return namespaceMap.get(nsId);
    }

    void
    KeyCache::parseKey (const String& name, LocalKeyId& prefixId, LocalKeyId& localKeyId)
    {
        if (!name.c_str() || !name.c_str()[0])
        {
            throwException(Exception, "Empty key !\n");
        }
        const char* localKey = strchr(name.c_str(), ':');
        if (!localKey)
        {
            prefixId = 0;
            localKeyId = getKeyId(0, name.c_str(), true);
            if (localKeyId == 0)
                throwException(InvalidKeyException, "Empty key !\n");
            return;
        }
        char* nsKey = strdup(name.c_str());
        char* column = strchr(nsKey, ':');
        *column = '\0';
        column++;
        if (strchr(column, ':'))
        {
            free(nsKey);
            throwException(InvalidKeyException, "Invalid multi-column key '%s'\n", name.c_str());
        }

        try
        {
            prefixId = getKeyId(0, nsKey, true);
            localKeyId = getKeyId(0, column, true);
        }
        catch (Exception* e)
        {
            free(nsKey);
            throw(e);
        }
        free(nsKey);

        if ((prefixId == 0) || (localKeyId == 0))
        {
            throwException(Exception, "Invalid key !\n");
        }
    }

    KeyId
    KeyCache::getKeyId (NamespaceId nsId, const char* keyName, bool create)
    {
        Log_Keys ( "Getting key '%s', create=%s\n", keyName, create ? "yes" : "no" );
        AssertBug(keyName, "Null keyName provided !\n");

        const char* suffix = strchr(keyName, ':');
        if (suffix != NULL)
        {
            keyName = suffix + 1;
        }

//    if ( strncmp(keyName, "xmlns:", 6) == 0 )
//    {
//        keyName = &(keyName[6]);
//    }

        /*
         * Check that keyName is really well formated
         * TODO : compute Hash at the same time
         */
#if 1 // PARANOID
        if (('A' <= *keyName && *keyName <= 'Z') || ('a' <= *keyName && *keyName <= 'z') || (*keyName == '_'))
        {
        }
        else
        {
            throwException(InvalidKeyException, "Invalid key '%s'\n", keyName);
        }

        for (const char* c = keyName; *c; c++)
        {
            if (('A' <= *c && *c <= 'Z') || ('a' <= *c && *c <= 'z') || ('0' <= *c && *c <= '9') || (*c == '-')
                    || (*c == '_') || (*c == '.'))
                continue;
            Error("Invalid key '%s'\n", keyName);
            Bug(".");
            throwException(InvalidKeyException, "Invalid key '%s'\n", keyName);
        }

#endif
        LocalKeyId localKeyId = getFromCache(keyName);
        if (!localKeyId)
        {
            localKeyId = store.addKeyInStore(keyName);
            char* name = strdup(keyName);
            localKeyMap.put(localKeyId, name);
            keysBucket.put(localKeyId, name);
        }
        return getKeyId(nsId, localKeyId);
    }

    KeyId
    KeyCache::getKeyIdWithElement (const ElementRef& _parsingFromElementRef, const String& keyName,
                                   bool useDefaultNamespace)
    {
        ElementRef parsingFromElementRef = _parsingFromElementRef;
        LocalKeyId localKeyId;
        LocalKeyId prefixKeyId;

        parseKey(keyName, prefixKeyId, localKeyId);

        NamespaceId nsId = 0;
        if (!prefixKeyId)
        {
            if (useDefaultNamespace)
                nsId = parsingFromElementRef.getDefaultNamespaceId();
        }
        else if (prefixKeyId == getBuiltinKeys().nons.xmlns())
        {
            nsId = getBuiltinKeys().xmlns.ns();
        }
        else
        {
            KeyId declarationId = getKeyId(getBuiltinKeys().xmlns.ns(), prefixKeyId);
            nsId = parsingFromElementRef.getNamespaceAlias(declarationId);
            if (!nsId)
            {
                Error("Could not resolve namespace for prefix : '%s', at element '%s'\n", keyName.c_str(),
                      parsingFromElementRef.generateVersatileXPath().c_str());
                throwException(UnresolvableNamespaceException,
                               "Could not resolve namespace for prefix : '%s', at element '%s'\n", keyName.c_str(),
                               parsingFromElementRef.generateVersatileXPath().c_str());
            }
        }
        return getKeyId(nsId, localKeyId);
    }

    String
    KeyCache::dumpKey (KeyId keyId)
    {
        String key;
        if (getNamespaceId(keyId))
        {
            key += "(";
            key += getNamespaceURL(getNamespaceId(keyId));
            key += "):";
        }
        key += getLocalKey(getLocalKeyId(keyId));
        return key;
    }

    void
    KeyCache::forAllKeys (void
    (*func) (void* arg, LocalKeyId localKeyId, const char* keyName),
                          void* arg)
    {
        LocalKeyId highest = localKeyMap.getLastKey();
        for (LocalKeyId localKeyId = 1; localKeyId <= highest; localKeyId++)
        {
            const char* val = localKeyMap.get(localKeyId);
            if (!val)
                continue;
            func(arg, localKeyId, val);
        }
    }

    void
    KeyCache::forAllNamespaces (void
    (*func) (void* arg, NamespaceId nsId, const char* url),
                                void* arg)
    {
        NamespaceId highest = namespaceMap.getLastKey();
        for (NamespaceId nsId = 1; nsId <= highest; nsId++)
        {
            const char* val = namespaceMap.get(nsId);
            if (!val)
                continue;
            func(arg, nsId, val);
        }
    }

    /*
     * *********************** HTML Key functions **********************
     */
    bool
    KeyCache::isHTMLKeyIn (KeyId keyId, const char** qnames)
    {
        if (getNamespaceId(keyId))
            return false;
        String lower = stringToLowerCase(getLocalKey(keyId));
        for (const char** qname = qnames; *qname; qname++)
        {
            if (lower == *qname)
                return true;
        }
        return false;
    }

    bool
    KeyCache::isHTMLKeyHTML (KeyId keyId)
    {
        static const char* qnames[] =
            { "html", NULL };
        return isHTMLKeyIn(keyId, qnames);
    }

    bool
    KeyCache::isHTMLKeyHead (KeyId keyId)
    {
        static const char* qnames[] =
            { "head", NULL };
        return isHTMLKeyIn(keyId, qnames);
    }

    bool
    KeyCache::isHTMLKeyDisableTextProtect (KeyId keyId)
    {
        static const char* qnames[] =
            { "script", "style", NULL };
        return isHTMLKeyIn(keyId, qnames);
    }

    bool
    KeyCache::isHTMLKeyDisableAttributeValue (KeyId keyId)
    {
        static const char* qnames[] =
            { "checked", "selected", NULL };
        return isHTMLKeyIn(keyId, qnames);
    }

    bool
    KeyCache::isHTMLKeySkipIndent (KeyId keyId)
    {
        static const char* qnames[] =
            { "th", "td", "a", NULL };
        return isHTMLKeyIn(keyId, qnames);
    }

    bool
    KeyCache::isHTMLKeySkipMarkupClose (KeyId keyId)
    {
        static const char* qnames[] =
            { "area", "base", "basefont", "br", "col", "frame", "hr", "img", "input", "isindex", "link", "meta",
                    "param", NULL };
        return isHTMLKeyIn(keyId, qnames);
    }

    bool
    KeyCache::isHTMLKeyURIAttribute (KeyId eltKeyId, KeyId attrKeyId)
    {
        // a/href,area/href,link/href,img/src,img/longdesc,img/usemap,object/classid,object/codebase,object/data,object/usemap,q/cite
        // blockquote/cite,ins/cite,del/cite,form/action,input/src,input/usemap,head/profile,base/href,script/src,script/for
        struct QNamePair
        {
            const char* elt;
            const char* attr;
        };
        static const QNamePair qnames[] =
            {
                { "a", "href" },
                { "area", "href" },
                { "link", "href" },
                { "img", "src" },
                { "img", "longdesc" },
                { "img", "usemap" },
                { "img", "onclick" },
                { "object", "classid" },
                { "object", "codebase" },
                { "object", "data" },
                { "object", "usemap" },
                { "q", "cite" },
                { "blockquote", "cite" },
                { "ins", "cite" },
                { "del", "cite" },
                { "form", "action" },
                { "input", "src" },
                { "input", "usemap" },
                { "head", "profile" },
                { "base", "href" },
                { "script", "src" },
                { "script", "for" },
                { NULL, NULL } };
        if (getNamespaceId(eltKeyId))
            return false;
        if (getNamespaceId(attrKeyId))
            return false;
        String lowerElt = stringToLowerCase(getLocalKey(eltKeyId));
        String lowerAttr = stringToLowerCase(getLocalKey(attrKeyId));
        for (const QNamePair* qn = qnames; qn->elt; qn++)
        {
            if (lowerElt == qn->elt && lowerAttr == qn->attr)
            {
                return true;
            }
        }
        return false;
    }
}
;

