#include <Xemeiah/kern/document.h>
#include <Xemeiah/kern/store.h>
#include <Xemeiah/kern/branchmanager.h>
#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/dom/elementmapref.h>
#include <Xemeiah/dom/documentmeta.h>
#include <Xemeiah/trace.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_RefCount Debug
#define Log_Document Log

namespace Xem
{
    Document::Document (Store& _store, DocumentAllocator& _documentAllocator) :
            store(_store), documentAllocator(_documentAllocator), refCountMutex("RefCount", this)
    {
        documentHeadPtr = NullPtr;
        boundDocumentAllocator = NULL;

        isIndexed = false;

        rootElementPtr = NullPtr;

        roleId = store.getKeyCache().getBuiltinKeys().nons.none();

        refCount = 0;
        unparsedEntitiesMap = NULL;
    }

    Document::~Document ()
    {
        Log_Document ( "Deleting Document ! this=%p, boundDocumentAllocator=%p\n", this, boundDocumentAllocator );
        if (boundDocumentAllocator)
        {
            AssertBug(boundDocumentAllocator->getRefCount(), "Null refCount !\n");
            boundDocumentAllocator->decrementRefCount(*this);
            if (boundDocumentAllocator->getRefCount() == 0)
            {
                delete (boundDocumentAllocator);
            }
        }
#if 0
        Info ( "At Document Destruction for '%s' - showStats : \n", getDocumentTag().c_str() );
        stats.showStats ();
#endif
        AssertBug(refCount == 0, "%p: Destroying while refCount=%llx\n", this, refCount);
        if (unparsedEntitiesMap)
            delete (unparsedEntitiesMap);
    }

    void
    Document::bindDocumentAllocator (DocumentAllocator* allocator)
    {
        allocator->incrementRefCount(*this);
        boundDocumentAllocator = allocator;
    }

    void
    Document::lockRefCount ()
    {
        refCountMutex.lock();
    }

    void
    Document::unlockRefCount ()
    {
        refCountMutex.unlock();
    }

    void
    Document::assertRefCountLocked ()
    {
        refCountMutex.assertLocked();
    }

    void
    Document::incrementRefCount ()
    {
        lockRefCount();
        refCount++;
        Log_RefCount ( "incrementRefCount(this=%p, refCount=%llx)\n", this, refCount );
        unlockRefCount();
    }

    void
    Document::decrementRefCount ()
    {
        lockRefCount();
        AssertBug(refCount, "Invalid zero refCount for document %p (role='%s')\n", this, getRole().c_str());
        refCount--;
        Log_RefCount ( "decrementRefCount(this=%p, refCount=%llx)\n", this, refCount );
        unlockRefCount();
    }

    void
    Document::incrementRefCountLockLess ()
    {
        refCountMutex.assertLocked();
        refCount++;
        Log_RefCount ( "LOCKLESS decrementRefCount(this=%p, refCount=%llx)\n", this, refCount );
    }

    void
    Document::decrementRefCountLockLess ()
    {
        refCountMutex.assertLocked();
        AssertBug(refCount, "Invalid zero refCount for document %p (role='%s')\n", this, getRole().c_str());
        refCount--;
        Log_RefCount ( "LOCKLESS decrementRefCount(this=%p, refCount=%llx)\n", this, refCount );
    }

    bool
    Document::isReferenced ()
    {
        bool res;
        lockRefCount();
        res = refCount != 0;
        unlockRefCount();
        return res;
    }

    void
    Document::housewife ()
    {
        Log_Document ( "Document::housewife : Nothing to do in default Document implementation !\n" );
    }

#if 0
    void Document::release ()
    {
#ifdef __XEM_DOCUMENT_HAS_REFCOUNT
        decrementRefCount ();
        if ( isReferenced() ) return;
#endif 
        delete ( this );
    }
#endif

    void Document::authorizeWriteDocumentHead()
    {
        getDocumentAllocator().authorizeWrite(documentHeadPtr, sizeof(DocumentHead));
    }

    bool
    Document::createRootElement ()
    {
        AssertBug(rootElementPtr == NullPtr, "Root Element already defined !\n");

        ElementRef nullElement = ElementRef(*this);
        ElementRef rootElement = createElement(nullElement, store.getKeyCache().getBuiltinKeys().xemint.root(),
                                               getFreeElementId());

        rootElementPtr = rootElement.getElementPtr();

        authorizeWriteDocumentHead();
        alterDocumentHead();
        getDocumentHead().rootElementPtr = rootElementPtr;
        getDocumentHead().elements = 1;
        protectDocumentHead();

        if (mayIndex())
        {
            isIndexed = true;
            SKMapConfig config;
            config.maxLevel = 32;

            config.itemAllocProfile = 0x1e;
            config.listAllocProfile = 0x1f;

            for (__ui32 level = 0; level < config.maxLevel; level++)
                config.probability[level] = 128;

            rootElement.addSKMap(store.getKeyCache().getBuiltinKeys().xemint.element_map(), config,
                                 SKMapType_ElementMultiMap);

            /*
             * The root element was created with isIndexed=false, so we have to index it now.
             */
            indexElementById(rootElement);
        }
        rootElement.addNamespaceAlias(store.getKeyCache().getBuiltinKeys().xml.defaultPrefix(),
                                      store.getKeyCache().getBuiltinKeys().xml.ns());
        return true;
    }

    ElementRef
    Document::getRootElement ()
    {
        AssertBug(rootElementPtr, "Null Document !\n");
        return ElementRef(*this, rootElementPtr);
    }

    String
    Document::getDocumentTag ()
    {
        String tag;
        String uri = getDocumentURI();
        if (uri.c_str() && *uri.c_str())
        {
            stringPrintf(tag, "document(\"%s\")", uri.c_str());
        }
        else if (getBranchRevId().branchId)
        {
            stringPrintf(tag, "document(\"branch:%llx:%llx\",\"%s\")", _brid(getBranchRevId()), getRole().c_str());
        }
        else
        {
            stringPrintf(tag, "volatile-document(%p,%s)", this, getRole().c_str());
        }
        return tag;
    }

    String
    Document::getDocumentURI ()
    {
        if (!getRootElement().hasAttr(getStore().getKeyCache().getBuiltinKeys().xemint.document_uri()))
            return String();
        return getRootElement().getAttr(getStore().getKeyCache().getBuiltinKeys().xemint.document_uri());
    }

    String
    Document::getDocumentBaseURI ()
    {
        if (!getRootElement().hasAttr(getStore().getKeyCache().getBuiltinKeys().xemint.document_base_uri()))
            return String();
        return getRootElement().getAttr(getStore().getKeyCache().getBuiltinKeys().xemint.document_base_uri());
    }

    void
    Document::setDocumentURI (const String& uri)
    {
        char* baseURI = strdup(uri.c_str());
        char* lastSeparator = strrchr(baseURI, '/');
        if (lastSeparator)
        {
            lastSeparator++;
            *lastSeparator = '\0';
        }
        else
        {
            free(baseURI);
            baseURI = NULL;
        }
        ElementRef rootElement = getRootElement();

        Log_Document ( "setDocumentURI : tag='%s', uri='%s', baseURI='%s'\n",
                getDocumentTag().c_str(), uri.c_str(), baseURI );

        rootElement.addAttr(getStore().getKeyCache().getBuiltinKeys().xemint.document_uri(), uri);

        rootElement.addAttr(getStore().getKeyCache().getBuiltinKeys().xemint.document_base_uri(),
                            baseURI ? baseURI : "");

        if (baseURI)
            free(baseURI);
    }

    /*
     * *************************** Element Creation ************************
     */

    ElementRef
    Document::createElement (ElementRef& fromElement, KeyId keyId)
    {
        return createElement(fromElement, keyId, isIndexed ? getFreeElementId() : 0);
    }

    ElementRef
    Document::createElement (ElementRef& fromElement, KeyId keyId, ElementId elementId)
    {
        AssertBug ( isWritable(), "Document not writable !\n" );
        AssertBug ( isLockedWrite(), "Document not locked write !\n" );

        if ( keyId == getKeyCache().getBuiltinKeys().xemint.textnode() )
        {
            return createTextualNode(fromElement, keyId, elementId);
        }


        AssertBug ( keyId != getKeyCache().getBuiltinKeys().xemint.textnode(), "Invalid keyId '%s'\n", getKeyCache().dumpKey(keyId).c_str());

        AllocationProfile allocProfile =
                fromElement ? getDocumentAllocator().getAllocationProfile(fromElement, keyId) : 0;
        ElementPtr eltPtr = getDocumentAllocator().getFreeSegmentPtr(sizeof(ElementSegment), allocProfile);
        ElementSegment* eltSeg = getDocumentAllocator().getSegment<ElementSegment, Write>(eltPtr);

        getDocumentAllocator().alter(eltSeg);
        memset(eltSeg, 0, sizeof(ElementSegment));

        eltSeg->keyId = keyId;
        eltSeg->flags = ElementFlag_HasAttributesAndChildren;
        eltSeg->id = elementId;

        getDocumentAllocator().protect(eltSeg);

        ElementRef eltRef(*this, eltPtr);

        indexElementById(eltRef);

        return eltRef;
    }

    ElementRef
    Document::createTextualNode (ElementRef& fromElement, KeyId keyId, ElementId elementId)
    {
        AllocationProfile allocProfile =
                fromElement ? getDocumentAllocator().getAllocationProfile(fromElement, keyId) : 0;
        ElementPtr eltPtr = getDocumentAllocator().getFreeSegmentPtr(sizeof(ElementSegment), allocProfile);
        ElementSegment* eltSeg = getDocumentAllocator().getSegment<ElementSegment, Write>(eltPtr);

        getDocumentAllocator().alter(eltSeg);
        memset(eltSeg, 0, sizeof(ElementSegment));

        eltSeg->keyId = keyId;
        eltSeg->flags = ElementFlag_HasTextualContents;
        eltSeg->id = elementId;
        eltSeg->textualContents.size = 0;
        getDocumentAllocator().protect(eltSeg);

        ElementRef eltRef(*this, eltPtr);

        indexElementById(eltRef);

        return eltRef;
    }

    ElementRef
    Document::createTextNode (ElementRef& fromElement, const char* text)
    {
        ElementRef newChild = createTextualNode(fromElement, getKeyCache().getBuiltinKeys().xemint.textnode(),
                                                isIndexed ? getFreeElementId() : 0);
        newChild.setText(text);
        return newChild;
    }

    ElementRef
    Document::createTextNode (ElementRef& fromElement, const String& text)
    {
        return createTextNode(fromElement, text.c_str());
    }

    ElementRef
    Document::createCommentNode (ElementRef& fromElement, const char* comment)
    {
        ElementRef newChild = createTextualNode(fromElement, getKeyCache().getBuiltinKeys().xemint.comment(),
                                                isIndexed ? getFreeElementId() : 0);
        newChild.setText(comment);
        return newChild;
    }

    ElementRef
    Document::createPINode (ElementRef& fromElement, const char* piName, const char* piContents)
    {
        KeyId keyId = KeyCache::getKeyId(getKeyCache().getBuiltinKeys().xemint_pi.ns(),
                                         getKeyCache().getKeyId(0, piName, true));
        ElementRef newChild = createTextualNode(fromElement, keyId, isIndexed ? getFreeElementId() : 0);
        newChild.setText(piContents);
        return newChild;
    }

    /*
     * ******************************* Element Indexing **********************
     */
    inline SKMapHash
    elementId2SKMapHash (ElementId elementId)
    {
        return ~(elementId >> 8);
    }

    void
    Document::indexElementById (ElementRef& eltRef)
    {
        if (!isIndexed)
            return;

        ElementRef root = getRootElement();

        AssertBug(isLockedWrite(), "Document not locked write.\n");

        ElementMultiMapRef eltMap = root.findAttr(store.getKeyCache().getBuiltinKeys().xemint.element_map(),
                                                  AttributeType_SKMap);
        if (!eltMap)
        {
            throwException(Exception, "This context is not indexed ! getElementById() is illegal here.\n");
        }
        ElementId elementId = eltRef.getElementId();
        AssertBug(elementId, "Zero elementId !\n");
        SKMapHash hash = elementId2SKMapHash(elementId);
        Log_Document ( "skm : index elementId=%llx with hash=%llx\n", elementId, hash );
        eltMap.put(hash, eltRef);
    }

    void
    Document::unIndexElementById (ElementRef& eltRef)
    {
        if (!isIndexed)
            return;

        ElementRef root = getRootElement();
        ElementMultiMapRef eltMap = root.findAttr(store.getKeyCache().getBuiltinKeys().xemint.element_map(),
                                                  AttributeType_SKMap);
        AssertBug(eltMap, "Document is not indexed !\n");

        SKMapHash hash = elementId2SKMapHash(eltRef.getElementId());

        eltMap.remove(hash, eltRef);
    }

    ElementRef
    Document::getElementById (ElementId elementId)
    {
        if (!isIndexed)
        {
            throwException(
                    Exception,
                    "This document (this=%p, tag='%s', role='%s') is not indexed ! getElementById(elementId=%llx) is illegal here.\n",
                    this, getDocumentTag().c_str(), getRole().c_str(), elementId);
        }

        ElementRef root = getRootElement();

        ElementMultiMapRef eltMap = root.findAttr(store.getKeyCache().getBuiltinKeys().xemint.element_map(),
                                                  AttributeType_SKMap);
        AssertBug(eltMap, "Document is not indexed !\n");

        SKMapHash hash = elementId2SKMapHash(elementId);
        __ui64 i = 0;
        (void) i;
        Log_Document ( "elementId=%llx, hash=%llx\n", elementId, hash );
        for (ElementMultiMapRef::multi_iterator iter(eltMap, hash); iter; iter++)
        {
            Log_Document ( "Iterating to find elementId=%llx, hash=%llx, value=%llx, i=%llx\n",
                    elementId, hash, iter.getValue(), i++ );
            AssertBug(iter.getValue(), "Invalid value : '0x%llx\n", iter.getValue());
            ElementRef eltRef = eltMap.get(iter);
            if (eltRef.getElementId() == elementId)
                return eltRef;
        }

        throwException(Exception, "This document (this=%p, tag='%s', role='%s') has no elementId=%llx.\n", this,
                       getDocumentTag().c_str(), getRole().c_str(), elementId);
        return ElementRefNull;
    }

    ElementRef
    Document::getElementById (const String& nodeIdStr)
    {
        return getElementById(nodeIdStr.c_str());
    }

    ElementRef
    Document::getElementById (const char* nodeIdStr)
    {
        NotImplemented("Document complex getElementById(%s) not implemented.\n", nodeIdStr);
#if 0
        char role[128];
        ElementId elementId;
        if ( ! ElementRef::parseElementId ( nodeIdStr, role, elementId ) )
        {
            Warn ( "Invalid NodeId Format '%s'\n", nodeIdStr );
            return NullPtr;
        }
        if ( strcmp(getRole().c_str(),role) != 0 )
        {
            throwException ( Exception, "Invalid roles : said '%s', mine is '%s'\n", role, getRole().c_str() );
        }
        return getElementById ( elementId );
#endif
        return ElementRefNull;
    }

    ElementMultiMapRef
    Document::getKeyMapping (KeyId keyNameId)
    {
        ElementRef root = getRootElement();
        return ElementMultiMapRef(root.findAttr(keyNameId, AttributeType_SKMap));
    }

    ElementMultiMapRef
    Document::createKeyMapping (KeyId keyNameId, SKMapConfig& config)
    {
        ElementRef root = getRootElement();
        AttributeRef mapRef = root.addSKMap(keyNameId, config, SKMapType_ElementMultiMap);
        return ElementMultiMapRef(mapRef);
    }

    ElementMultiMapRef
    Document::createKeyMapping (KeyId keyNameId)
    {
        SKMapConfig config;
        SKMapRef::initDefaultSKMapConfig(config);
        return createKeyMapping(keyNameId, config);
    }

    ElementRef
    Document::getMetaElement (bool create)
    {
        if (!getDocumentHead().metaElementPtr && create)
        {
            ElementRef root = getRootElement();
            ElementRef metaElement = createElement(root, getKeyCache().getBuiltinKeys().xemint.document_meta());
            ElementPtr metaElementPtr = metaElement.getElementPtr();

            authorizeWriteDocumentHead();
            alterDocumentHead();
            getDocumentHead().metaElementPtr = metaElementPtr;
            protectDocumentHead();
        }
        ElementRef metaElement(*this, getDocumentHead().metaElementPtr);
        return metaElement;
    }

    DocumentMeta
    Document::getDocumentMeta ()
    {
        return getMetaElement(true);
    }

    /*
     * Misc
     */
    void
    Document::setUnparsedEntity (const String& entityName_, const String& entityValue_)
    {
        if (unparsedEntitiesMap == NULL)
        {
            unparsedEntitiesMap = new UnparsedEntitiesMap();
        }
        String entityName = stringFromAllocedStr(strdup(entityName_.c_str()));
        String entityValue = stringFromAllocedStr(strdup(entityValue_.c_str()));
        (*unparsedEntitiesMap)[entityName] = entityValue;
    }

    String
    Document::getUnparsedEntity (const String& entityName)
    {
        if (!unparsedEntitiesMap)
            return String("");
        UnparsedEntitiesMap::iterator iter = unparsedEntitiesMap->find(entityName);
        if (iter == unparsedEntitiesMap->end())
            return String("");
        return iter->second;
    }
}
;
