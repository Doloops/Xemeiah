#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/dom/attributeref.h>
#include <Xemeiah/kern/format/journal.h>

#define Log_Element Debug

/*
 * Element Constructors and Destructor
 */
namespace Xem
{
    /*
     * Element Stuff
     */
    __INLINE ElementPtr
    ElementRef::getElementPtr () const
    {
        return nodePtr;
    }

    __INLINE void
    ElementRef::setElementPtr (ElementPtr ePtr)
    {
        nodePtr = ePtr;
#ifdef __XEM_ELEMENTREF_CACHE
        __me_cache = NULL;
#endif
    }

    template<PageCredentials how>
        __INLINE ElementSegment*
        ElementRef::getMe ()
        {
#if PARANOID
            AssertBug ( getElementPtr(), "Null pointer for element !\n" );
#endif

            ElementSegment* __me = getDocumentAllocator().getSegment<ElementSegment, how>(getElementPtr(),
                                                                                          sizeof(ElementSegment));

#if PARANOID
            AssertBug ( __me, "Null segment for pointer %llx\n", getElementPtr() );
#endif
            return __me;
        }

    __INLINE
    ElementRef::ElementRef (Document& _document) :
            NodeRef(_document)
    {
        Log_Element ( "New (empty) ref=%p\n", this );
        setElementPtr( NullPtr);
    }

    __INLINE
    ElementRef::ElementRef (Document& _document, SegmentPtr __ptr) :
            NodeRef(_document)
    {
        Log_Element ( "New ElementRef=%p, document=%p, ptr=%llx\n", this, &document, __ptr );
        setElementPtr(__ptr);
    }

    __INLINE
    ElementRef::ElementRef (NodeRef& nodeRef) :
            NodeRef(nodeRef.getDocument())
    {
        Log_Element ( "New ElementRef=%p document=%p from noderef=%p\n", this, &document, &nodeRef );
        if (nodeRef.isElement())
        {
            setElementPtr(nodeRef.toElement().getElementPtr());
        }
        else
        {
            setElementPtr(nodeRef.toAttribute().getParentElementPtr());
        }
    }

    __INLINE
    ElementRef::ElementRef (ElementRef* brother) :
            NodeRef(brother->getDocument())
    {
        setElementPtr(brother->getElementPtr());
        Log_Element ( "New ElementRef=%p, brother %p, document=%p, ptr=%llx\n",
                this, brother, &document, getElementPtr() );
    }

    __INLINE
    ElementRef::ElementRef (const ElementRef& brother) :
            NodeRef(brother.getDocument())
    {
        setElementPtr(brother.getElementPtr());
        Log_Element ( "New ElementRef=%p, document=%p, from brother=%p, ptr=%llx\n",
                this, &document, &brother, getElementPtr() );
    }

    __INLINE
    ElementRef::ElementRef (const AttributeRef& attrRef) :
            NodeRef(attrRef.getDocument())
    {
#if PARANOID
        AssertBug ( attrRef.getAttributePtr() && attrRef.getParentElementPtr(),
                "Invalid attribute ptr %llx:%llx to build an elementRef !\n",
                attrRef.getAttributePtr(), attrRef.getParentElementPtr() );
#endif
        setElementPtr(attrRef.getParentElementPtr());
    }

    __INLINE
    ElementRef::~ElementRef ()
    {
        Log_Element ( "delete ref=%p\n", this );
    }

    /*
     * Operators
     */
    __INLINE ElementRef&
    ElementRef::operator= (const ElementRef& eRef)
    {
        Log_Element ( "operator=\n" );
#if PARANOID
        if ( &document != &(eRef.document ) )
        {
            Bug ( "Yet : Document changing for an elementRef\n" );
        }
#endif
        setElementPtr(eRef.getElementPtr());
        return *this;
    }

    __INLINE bool
    ElementRef::operator== (const ElementRef& eRef)
    {
        if (&document != &(eRef.document))
            return false;
        return getElementPtr() == eRef.getElementPtr();
    }

    __INLINE
    ElementRef::operator bool () const
    {
        return getElementPtr() != NullPtr;
    }

    /*
     * Conversion & Affectation Operators
     *
     */
    __INLINE KeyId
    ElementRef::getKeyId ()
    {
        return getMe<Read>()->keyId;
    }

    __INLINE String
    ElementRef::getKey ()
    {
        KeyId keyId = getKeyId();
        if (KeyCache::getNamespaceId(keyId))
        {
            LocalKeyId prefixId = getNamespacePrefix(KeyCache::getNamespaceId(keyId), true);
            String s;
            if (prefixId == getKeyCache().getBuiltinKeys().nons.xmlns() || prefixId == 0)
            {
                s += "(";
                s += getKeyCache().getNamespaceURL(KeyCache::getNamespaceId(keyId));
                s += ")";
            }
            else
            {
                s += getKeyCache().getLocalKey(prefixId);
            }
            s += ":";
            s += getKeyCache().getLocalKey(KeyCache::getLocalKeyId(keyId));
            return s;
        }
        return getKeyCache().getLocalKey(keyId);
    }

    __INLINE NamespaceId
    ElementRef::getNamespaceId ()
    {
        return getKeyCache().getNamespaceId(getKeyId());
    }

    /*
     * Element flags
     */
    __INLINE void
    ElementRef::setFlag (ElementFlag flag, bool value)
    {
        AssertBug(flag >= ElementFlag_DisableOutputEscaping, "Invalid value for flag.\n");
        ElementSegment* me = getMe<Read>();
        ElementFlag myFlag = me->flags;
        if (((myFlag & flag) == flag) == value)
        {
            /**
             * The flag is properly set
             */
            return;
        }
        if (value)
        {
            myFlag |= flag;
        }
        else
        {
            myFlag -= (myFlag & flag);
        }
        me = getMe<Write>();
        getDocumentAllocator().alter(me);
        me->flags = myFlag;
        getDocumentAllocator().protect(me);
    }

    __INLINE bool
    ElementRef::hasFlag (ElementFlag flag)
    {
        AssertBug(flag >= ElementFlag_DisableOutputEscaping, "Invalid value for flag.\n");
        ElementSegment* me = getMe<Read>();
        ElementFlag myFlag = me->flags;
        return ((myFlag & flag) == flag);
    }

    __INLINE void
    ElementRef::setDisableOutputEscaping (bool escaping)
    {
        AssertBug(isText(), "ElementRef not a text !\n");
        setFlag(ElementFlag_DisableOutputEscaping, escaping);
    }

    __INLINE bool
    ElementRef::getDisableOutputEscaping ()
    {
        AssertBug(isText(), "ElementRef not a text !\n");
        return hasFlag(ElementFlag_DisableOutputEscaping);
    }

    __INLINE void
    ElementRef::setWrapCData (bool wrap)
    {
        AssertBug(isText(), "ElementRef not a text !\n");
        setFlag(ElementFlag_WrapCData, wrap);
    }

    __INLINE bool
    ElementRef::getWrapCData ()
    {
        AssertBug(isText(), "ElementRef not a text !\n");
        return hasFlag(ElementFlag_WrapCData);
    }

    /*
     * Element Accessors
     *
     */
#if PARANOID
#define __elementref_accessor(_ptr)		\
  AssertBug ( getElementPtr(), "No Pointer defined !\n" );	\
  return ElementRef ( document, getMe<Read>()->_ptr )
#else
#define __elementref_accessor(_ptr)		\
  return ElementRef ( document, getMe<Read>()->_ptr )
#endif
    //  return getStore().getElement<Read> ( getRev(), ptr )->_ptr
    __INLINE ElementRef
    ElementRef::getElder ()
    {
        __elementref_accessor(elderPtr);
    }
    __INLINE ElementRef
    ElementRef::getYounger ()
    {
        __elementref_accessor(youngerPtr);
    }
#undef __elementref_accessor

    __INLINE ElementRef
    ElementRef::getFather ()
    {
        if (getElementPtr() == getDocument().rootElementPtr)
        {
            return ElementRef(getDocument());
        }
        ElementRef father( document, getMe<Read>()->fatherPtr);
#ifdef __XEM_DOM_NODEREF_CHROOT_USING_XEMINT_ROOT
        if ( 0 && father && father.getKeyId() == getKeyCache().getBuiltinKeys().xemint.root())
        {
            return ElementRef(getDocument(), 0);
        }
#endif // __XEM_DOM_NODEREF_CHROOT_USING_XEMINT_ROOT
        return father;
    }

    __INLINE ElementRef
    ElementRef::getChild ()
    {
        AssertBug(getElementPtr(), "Invalid ptr !\n");
        ElementSegment* me = getMe<Read>();
        if (!__doesElementRefHasAttributesAndChildren(me->flags))
        {
            return ElementRef(getDocument());
        }
        __checkElementFlag_HasAttributesAndChildren(me->flags);
        return ElementRef(document, me->attributesAndChildren.childPtr);
    }

    __INLINE ElementRef
    ElementRef::getLastChild ()
    {
        AssertBug(getElementPtr(), "Invalid ptr !\n");
        ElementSegment* me = getMe<Read>();
        if (!__doesElementRefHasAttributesAndChildren(me->flags))
        {
            return ElementRef(getDocument());
        }
        __checkElementFlag_HasAttributesAndChildren(me->flags);
        return ElementRef(getDocument(), me->attributesAndChildren.lastChildPtr);
    }

    __INLINE AttributeRef
    ElementRef::getFirstAttr ()
    {
        ElementSegment* me = getMe<Read>();
        if (!__doesElementRefHasAttributesAndChildren(me->flags))
        {
            return AttributeRef(getDocument());
        }
        __checkElementFlag_HasAttributesAndChildren(me->flags);
        return AttributeRef(getDocument(), getElementPtr(), me->attributesAndChildren.attrPtr);
    }

    __INLINE bool
    ElementRef::hasAttr (KeyId keyId, AttributeType type)
    {
        return (bool) findAttr(keyId, type);
    }

    __INLINE bool
    ElementRef::hasAttr (KeyId keyId)
    {
        return (bool) findAttr(keyId);
    }

    __INLINE AttributeRef
    ElementRef::findAttr (KeyId _keyId, AttributeType type)
    {
        Log_Element ( "getAttr keyId=%x type=%x\n", _keyId, type );
        if (_keyId == 0)
            return AttributeRef(document);
        AttributeSegment* attr = NULL;
        ElementSegment* me = getMe<Read>();
        if (!__doesElementRefHasAttributesAndChildren(me->flags))
            return AttributeRef(getDocument());

        // __checkElementFlag_HasAttributesAndChildren ( me->flags );
        __checkElementFlag_HasAttributesAndChildren(me->flags);
        SegmentPtr attrPtr = me->attributesAndChildren.attrPtr;

        KeyId keyId = _keyId;
        if (KeyCache::getNamespaceId(keyId) == KeyCache::getNamespaceId(me->keyId))
        {
            keyId = KeyCache::getLocalKeyId(keyId);
        }

        while (attrPtr)
        {
            attr = getSegment<AttributeSegment, Read>(attrPtr);
            Log_Element ( "At attr keyId=%x, ptr=%llx, next=%llx, flag=%x, search[keyId=%x, type=%x]\n",
                    attr->keyId, attrPtr, attr->nextPtr, attr->flag, keyId, type );

            if (attr->keyId == keyId || attr->keyId == _keyId)
            {
                if ((type & (attr->flag & AttributeType_Mask)) == type)
                {
                    Log_Element ( "** return %llx\n", attrPtr );
                    return AttributeRef(document, getElementPtr(), attrPtr);
                }
            }
            attrPtr = attr->nextPtr;
        }
        Log_Element ( "** return NULL\n" );
        return AttributeRef(document, NullPtr, NullPtr);
    }

    /*
     * Textual contents setting
     */
    __INLINE void
    ElementRef::setText (const char* text, DomTextSize textSize)
    {
        AssertBug(isText() || isPI() || isComment(), "Not a text element !\n");

        ElementSegment* me = getMe<Write>();
        AssertBug(me->flags & ElementFlag_HasTextualContents, "Invalid flags %x\n", me->flags);

        if (me->textualContents.size > me->textualContents.shortFormatSize)
        {
            // We shall reuse a bit here... Nevermind...
            getDocumentAllocator().freeSegment(me->textualContents.contentsPtr, me->textualContents.size);
        }

        if (textSize <= me->textualContents.shortFormatSize)
        {
            getDocumentAllocator().alter(me);
            me->textualContents.size = textSize;
            memcpy(me->textualContents.contents, text, textSize);

            getDocumentAllocator().protect(me);
        }
        else
        {
            SegmentPtr contentsPtr = getDocumentAllocator().getFreeSegmentPtr(textSize, getAllocationProfile());

            char* textSegment = getDocumentAllocator().getSegment<char, Write>(contentsPtr, textSize);
            getDocumentAllocator().alter(textSegment, textSize);
            memcpy(textSegment, text, textSize);
            getDocumentAllocator().protect(textSegment, textSize);

            getDocumentAllocator().alter(me);
            me->textualContents.size = textSize;
            me->textualContents.contentsPtr = contentsPtr;
            getDocumentAllocator().protect(me);
        }
        getDocument().appendJournal(*this, JournalOperation_UpdateTextNode, *this, 0);
    }

    __INLINE void
    ElementRef::setText (const char* text)
    {
        DomTextSize textSize = strlen(text) + 1;
        setText(text, textSize);
    }

    __INLINE bool
    ElementRef::isText ()
    {
        return getKeyId() == getKeyCache().getBuiltinKeys().xemint.textnode();
    }

    __INLINE bool
    ElementRef::isComment ()
    {
        return getKeyId() == getKeyCache().getBuiltinKeys().xemint.comment();
    }

    __INLINE bool
    ElementRef::isPI ()
    {
        return KeyCache::getNamespaceId(getKeyId()) == getKeyCache().getBuiltinKeys().xemint_pi.ns();
    }

    __INLINE bool
    ElementRef::isRegularElement ()
    {
        return (!isText() && !isComment() && !isPI());
    }

    /**
     * \todo Optimize this with a flag on Element to know if it's a whitespace or not.
     */
    __INLINE bool
    ElementRef::isWhitespace ()
    {
        AssertBug(isText(), "Element is not a text() !\n");
        bool isWs = true;
        for (const char* c = getText().c_str(); *c; c++)
            if (!isspace(*c))
            {
                isWs = false;
                break;
            }
        return isWs;
    }

    __INLINE String
    ElementRef::getPIName ()
    {
        AssertBug(isPI(), "Element is not a PI !\n");
        const char* piName = getKeyCache().getLocalKey(KeyCache::getLocalKeyId(getKeyId()));
        return String(piName);
    }

    __INLINE String
    ElementRef::getText ()
    {
        ElementSegment* me = getMe<Read>();

        AssertBug(me->flags & ElementFlag_HasTextualContents, "Invalid flags %x\n", me->flags);

        if (me->textualContents.size <= me->textualContents.shortFormatSize)
        {
            return String(me->textualContents.contents);
        }
        const char* contents = getDocumentAllocator().getSegment<char, Read>(me->textualContents.contentsPtr,
                                                                             me->textualContents.size);
        return String(contents);
    }

    INLINE AllocationProfile
    ElementRef::getAllocationProfile ()
    {
#if PARANOID  
        AssertBug ( getElementPtr(), "Null pointer in getAllocationProfile !\n" );
#endif
        return getDocumentAllocator().getAllocationProfile(getElementPtr());
    }

    /**
     * Modifiers
     */
    __INLINE void
    ElementRef::addAttr (AttributeRef& attr)
    {
#if PARANOID
        AssertBug ( &document == &(attr.document), "AttributeRef and ElementRef do not have the same document !\n" );
        AssertBug ( attr.getParentElementPtr() == NullPtr || attr.getParentElementPtr() == getElementPtr(),
                "AttributeRef already added to another ElementRef \n" );
        AssertBug ( attr.getAttributePtr() != NullPtr, "AttributeRef not defined !\n" );
#endif
        for (AttributeRef myAttr = getFirstAttr(); myAttr; myAttr = myAttr.getNext())
        {
            if (myAttr.getKeyId() == attr.getKeyId() && myAttr.getAttributeType() == attr.getAttributeType())
            {
                NotImplemented("Duplicate attributes : key=%x:%s, type=%x, attribute=%s\n", myAttr.getKeyId(),
                               myAttr.getKey().c_str(), myAttr.getAttributeType(),
                               myAttr.generateVersatileXPath().c_str());
            }
        }
        ElementSegment* me = getMe<Write>();
        __checkElementFlag_HasAttributesAndChildren(me->flags);

        if (attr.getAttributeType() == AttributeType_NamespaceAlias && !(me->flags & ElementFlag_HasNamespaceAlias))
        {
            getDocumentAllocator().alter(me);
            me->flags |= ElementFlag_HasNamespaceAlias;
            getDocumentAllocator().protect(me);
        }

        SegmentPtr nextAttrPtr = NullPtr;
        if (me->attributesAndChildren.attrPtr == NullPtr)
        {
            getDocumentAllocator().alter(me);
            me->attributesAndChildren.attrPtr = attr.getAttributePtr();
            getDocumentAllocator().protect(me);
        }
        else
        {
            SegmentPtr lastAttrPtr = me->attributesAndChildren.attrPtr;
            AttributeSegment* lastAttrSeg = getDocumentAllocator().getSegment<AttributeSegment, Read>(lastAttrPtr);
            while (lastAttrSeg->nextPtr != NullPtr)
            {
                lastAttrPtr = lastAttrSeg->nextPtr;
                lastAttrSeg = getDocumentAllocator().getSegment<AttributeSegment, Read>(lastAttrPtr);
            }
            getDocumentAllocator().authorizeWrite(lastAttrPtr, lastAttrSeg);
            getDocumentAllocator().alter(lastAttrSeg);
            lastAttrSeg->nextPtr = attr.getAttributePtr();
            getDocumentAllocator().protect(lastAttrSeg);
        }
        AttributeSegment * attrSeg = getDocumentAllocator().getSegment<AttributeSegment, Read>(attr.getAttributePtr());
        getDocumentAllocator().alter(attrSeg);
        attrSeg->nextPtr = nextAttrPtr;
        getDocumentAllocator().protect(attrSeg);
        attr.setParentElementPtr(getElementPtr());

        Log_Element ( "Added attr on elementRef=%llx:%x (%llx, ref at %p), attr=%x (elt=%llx,attr=%llx, ref at %p)\n",
                getElementId(), getKeyId(), getElementPtr(), this,
                attr.getKeyId(), attr.getParentElementPtr(), attr.getAttributePtr(), &attr );
        if (attr.isBaseType())
        {
            getDocument().appendJournal(*this, JournalOperation_UpdateAttribute, *this, attr.getKeyId());
        }
    }

    __INLINE KeyId
    ElementRef::__getNS (KeyId keyId)
    {
        return KeyCache::getKeyId(getNamespaceId(), KeyCache::getLocalKeyId(keyId));
    }
}
;

#undef Log_Element
