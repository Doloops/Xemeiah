#include <Xemeiah/kern/document.h>
#include <Xemeiah/dom/documentmeta.h>

#define Log_DocumentHPP Debug

namespace Xem
{
    __INLINE DocumentHead&
    Document::getDocumentHead ()
    {
#if PARANOID
        AssertBug ( documentHeadPtr, "Null DocumentHeadPtr !\n" );
#endif
        return *(getDocumentAllocator().getSegment<DocumentHead, Read>(documentHeadPtr, sizeof(DocumentHead)));
    }

    __INLINE ElementId
    Document::getFreeElementId ()
    {
#if 0
        if ( !isIndexed ) return 0;
#endif

        ElementId elementId;
        // LockMutex ( Alloc );
        AssertBug(getDocumentHead().firstReservedElementId <= getDocumentHead().lastReservedElementId,
                  "Reserved elements out of bounds.\n");
        if (getDocumentHead().firstReservedElementId == getDocumentHead().lastReservedElementId)
        {
            ElementId firstId, lastId;
            store.reserveElementIds(firstId, lastId);

            alterDocumentHead();
            getDocumentHead().firstReservedElementId = firstId;
            getDocumentHead().lastReservedElementId = lastId;
            protectDocumentHead();
        }
        AssertBug(getDocumentHead().firstReservedElementId < getDocumentHead().lastReservedElementId,
                  "Reserved elements did not work.\n");
        alterDocumentHead();
        getDocumentHead().firstReservedElementId++;
        protectDocumentHead();
        elementId = getDocumentHead().firstReservedElementId;

        Log_DocumentHPP ( "New elementId=0x%llx\n", elementId );
        return elementId;
    }

    __INLINE RoleId
    Document::getRoleId ()
    {
        return roleId;
    }

    __INLINE void
    Document::setRoleId (RoleId _roleId)
    {
        AssertBug(_roleId, "Null roleId provided !\n");
        AssertBug(roleId, "Null roleId stored !\n");
        AssertBug(roleId == getKeyCache().getBuiltinKeys().nons.none(),
                  "Document already has a role %s (%x) set, could not set %s (%x) !\n",
                  getStore().getKeyCache().dumpKey(roleId).c_str(), roleId,
                  getStore().getKeyCache().dumpKey(_roleId).c_str(), _roleId);
        roleId = _roleId;
    }

    __INLINE String
    Document::getRole ()
    {
        return String(getKeyCache().getLocalKey(roleId));
    }

    __INLINE void
    Document::setRole (const String& role)
    {
        KeyId keyId = getKeyCache().getKeyId(0, role.c_str(), true);
        AssertBug(keyId, "Null keyId provided !\n");
        roleId = KeyCache::getLocalKeyId(keyId);
    }

    __INLINE void
    Document::processDomEvent (XProcessor& xproc, DomEventType eventType, NodeRef& nodeRef)
    {
        Log ("XProc=%p, eventType=%x, nodeRef=%s\n", &xproc, eventType, nodeRef.getKey().c_str());
        DocumentMeta documentMeta = getDocumentMeta();
        if (!documentMeta.getChild())
        {
            return;
        }
        documentMeta.getDomEvents().processEvent(xproc, eventType, nodeRef);
    }
}
;

