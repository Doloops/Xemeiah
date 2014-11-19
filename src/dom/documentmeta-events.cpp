/*
 * documentmeta-events.cpp
 *
 *  Created on: 9 nov. 2009
 *      Author: francois
 */

#include <Xemeiah/dom/documentmeta.h>
#include <Xemeiah/dom/childiterator.h>
#include <Xemeiah/dom/attributeref.h>
#include <Xemeiah/dom/elementmapref.h>
#include <Xemeiah/dom/qnamelistref.h>
#include <Xemeiah/xprocessor/xprocessor.h>
#include <Xemeiah/kern/journaleddocument.h>
#include <Xemeiah/xpath/xpathparser.h>
#include <Xemeiah/auto-inline.hpp>

#define __xemint getKeyCache().getBuiltinKeys().xemint

#define Log_DME Log

namespace Xem
{
    DomEvent::DomEvent (const ElementRef& element) :
            ElementRef(element)
    {
        AssertBug(*this, "Element not defined !\n");
        AssertBug(getKeyId() == __xemint.dom_event(), "Element is not a document dom event !");

    }

    void
    DomEvent::setHandlerId (KeyId handlerId)
    {
        if (!KeyCache::getNamespaceId(handlerId)) // && ! getNamespacePrefix(KeyCache::getNamespaceId(handlerId),true) )
        {
            throwException(Exception, "No namespace defined for this handler !\n");
        }
        addAttrAsQName(__xemint.handler(), handlerId);
    }

    KeyId
    DomEvent::getHandlerId ()
    {
        return getAttrAsKeyId(__xemint.handler());
    }

    void
    DomEvent::setEventMask (DomEventMask domEventMask)
    {
        String eventNames = domEventMask.toString();
        addAttr(__xemint.event_names(), eventNames);
#if 0
        addAttrAsInteger(__xemint.event_mask(), domEventMask);
#endif
    }

    DomEventMask
    DomEvent::getEventMask ()
    {
        DomEventMask mask;
        mask.parse(getAttr(__xemint.event_names()));
        return mask;
#if 0
        if ( hasAttr(__xemint.event_mask(),AttributeType_Integer) )
        {
            mask = getAttrAsInteger(__xemint.event_mask());
            AttributeRef attrRef = addAttr(__xemint.event_names(),mask.toString());
            getDocument().appendJournal(*this,JournalOperation_UpdateAttribute,*this,__xemint.event_names());
        }
#endif
    }

#ifdef __XEM_DOM_DOCUMENTMETA_USE_KEYIDLIST
    void DomEvent::addNodeQName ( KeyId attributeKeyId, KeyId keyId )
    {
        KeyIdList qnamesList = getNodeQNames ( attributeKeyId );
        qnamesList.push_back(keyId);
        Log_DME ( "Add : %x\n", keyId );
        if ( KeyCache::getNamespaceId(keyId) && ! getNamespacePrefix(KeyCache::getNamespaceId(keyId),true) )
        {
            LocalKeyId prefixId = generateNamespacePrefix(KeyCache::getNamespaceId(keyId));
            Assert ( prefixId, "Could not get prefixId ?\n" );
            Log_DME ( "Generated prefixId=%x (%s) for namespace %s (keyId=%s %x)\n",
                    prefixId, getKeyCache().dumpKey(prefixId).c_str(),
                    getKeyCache().getNamespaceURL(KeyCache::getNamespaceId(keyId)),
                    getKeyCache().dumpKey(keyId).c_str(), keyId );
        }

        String qnames; bool first = true;
        for ( KeyIdList::iterator iter = qnamesList.begin(); iter != qnamesList.end(); iter++ )
        {
            if ( first ) first = false;
            else qnames += " ";

            KeyId qnameId = *iter;
            Log_DME ( "Serialize : %x\n", qnameId );
            if ( KeyCache::getNamespaceId(qnameId) )
            {
                LocalKeyId prefixId = getNamespacePrefix(KeyCache::getNamespaceId(qnameId),true);
                AssertBug ( prefixId, "No prefixId provided !\n" );

                qnames += getKeyCache().getKey(prefixId, KeyCache::getLocalKeyId(qnameId));
            }
            else
            {
                qnames += getKeyCache().getLocalKey(qnameId);
            }
        }
        Log_DME ( "Serialize qnames list : '%s'\n", qnames.c_str( ) );
        addAttr ( attributeKeyId, qnames );
    }

    KeyIdList DomEvent::getNodeQNames ( KeyId attributeKeyId )
    {
        if ( ! findAttr ( attributeKeyId, AttributeType_String ) )
        {
            return KeyIdList();
        }
        return getAttrAsKeyIdList(attributeKeyId);
    }
#else
    void
    DomEvent::addNodeQName (KeyId attributeKeyId, KeyId qnameId)
    {
        addQNameInQNameList(attributeKeyId, qnameId);
    }
#endif

    void
    DomEvent::addElementQName (KeyId keyId)
    {
        return addNodeQName( __xemint.element_qnames(), keyId);
    }

    void
    DomEvent::addAttributeQName (KeyId keyId)
    {
        return addNodeQName( __xemint.attribute_qnames(), keyId);
    }

    void
    DomEvent::addQNames (XPath& xpath)
    {
        XPath::XPathFinalSteps finalSteps;
        xpath.buildFinalSteps(finalSteps);

        for (XPath::XPathFinalSteps::iterator iter = finalSteps.begin(); iter != finalSteps.end(); iter++)
        {
            Log_DME ( "Pushing QName : %x\n", iter->keyId );
            if (iter->elementOrAttribute)
                addAttributeQName(iter->keyId);
            else
                addElementQName(iter->keyId);
            for (std::list<KeyId>::iterator attrIter = iter->predicateAttributeIds.begin();
                    attrIter != iter->predicateAttributeIds.end(); attrIter++)
            {
                addAttributeQName(*attrIter);
            }
        }
    }

    void
    DomEvent::setMatchXPath (XProcessor& xproc, const String& expression)
    {
        addAttr( __xemint.match(), expression);
        XPath matchXPath(xproc, *this, __xemint.match());
        addQNames(matchXPath);
    }

    void
    DomEvent::addQName (KeyId qnameId)
    {
        DomEventMask domEventMask = getEventMask();
        if (domEventMask.intersects(DomEventMask_Element))
        {
            addElementQName(qnameId);
        }
        if (domEventMask.intersects(DomEventMask_Attribute))
        {
            addAttributeQName(qnameId);
        }
    }

#ifdef __XEM_DOM_DOCUMENTMETA_USE_KEYIDLIST
    KeyIdList DomEvent::getElementQNames ()
    {
        return getNodeQNames ( __xemint.element_qnames() );
    }

    KeyIdList DomEvent::getAttributeQNames ()
    {
        return getNodeQNames ( __xemint.attribute_qnames() );
    }
#endif

    /*
     * ***********************************************************************************************
     * DomEvents part
     * ***********************************************************************************************
     */
    DomEvents::DomEvents (const ElementRef& element) :
            ElementRef(element)
    {
        AssertBug(*this, "Element not defined !\n");
        AssertBug(getKeyId() == __xemint.dom_events(), "Element is not a document dom events !");
    }

    DomEvent
    DomEvents::createDomEvent ()
    {
        if (!getDocument().isLockedWrite())
        {
            throwException(Exception, "Document not set writable !\n");
        }
        DomEvent event = getDocument().createElement(*this, __xemint.dom_event());
        appendChild(event);
        return event;
    }

    DomEvent
    DomEvents::registerEvent (DomEventMask domEventMask, XPath& matchXPath, KeyId handlerId)
    {
        DomEvent domEvent = createDomEvent();
        domEvent.setEventMask(domEventMask);
        domEvent.addQNames(matchXPath);
        domEvent.setHandlerId(handlerId);
        matchXPath.copyTo(domEvent, __xemint.match());
        getDocument().appendJournal(domEvent, JournalOperation_UpdateAttribute, domEvent, __xemint.match());
        return domEvent;
    }

    DomEvent
    DomEvents::registerEvent (XPath& matchXPath, KeyId handlerId)
    {
        return registerEvent(DomEventMask_Element | DomEventMask_Attribute, matchXPath, handlerId);
    }

    DomEvent
    DomEvents::registerEvent (DomEventMask domEventMask, KeyId matchKeyId, KeyId handlerId)
    {
        DomEvent domEvent = createDomEvent();
        domEvent.setEventMask(domEventMask);
        domEvent.setHandlerId(handlerId);
        domEvent.addQName(matchKeyId);
        return domEvent;
    }

    SKMapHash
    DomEvents::hashEvent (DomEventType domEventType, KeyId qnameId)
    {
        return (((SKMapHash) domEventType) << 32) + ((SKMapHash) qnameId);
    }

    void
    DomEvents::buildEventMap (ElementMultiMapRef& eventMap, DomEvent& event, DomEventMask mask, KeyId qnameId)
    {
        for (int i = 0; i < 32; i++)
        {
            DomEventType type = (1 << i);
            if (!mask.hasType(type))
                continue;

            SKMapHash hash = hashEvent(type, qnameId);

            /*
             * This fetching is important, because it allows event to store the EventId as KeyId
             */
            KeyId handlerId = event.getHandlerId();
            if (!handlerId)
            {
                throwException(Exception, "No handlerId defined !\n");
            }
            Log_DME ( "Associating :\n\tevent=%x (%s)\n\t+ qname=%x (%s)\n\thash=%llx => handler=%x (%s)\n",
                    type, DomEventMask(type).toString().c_str(),
                    qnameId, getKeyCache().dumpKey(qnameId).c_str(),
                    hash,
                    event.getHandlerId(),
                    getKeyCache().dumpKey(event.getHandlerId()).c_str() );
            eventMap.put(hash, event);
        }
    }

    void
    DomEvents::buildEventMap (ElementMultiMapRef& eventMap, DomEvent& event, DomEventMask mask, QNameListRef& qnames)
    {
        if (!qnames)
            return;
        for (QNameListRef::iterator iter(qnames); iter; iter++)
        {
            KeyId qnameId = iter.getKeyId();
            buildEventMap(eventMap, event, mask, qnameId);
        }
    }

#ifdef __XEM_DOM_DOCUMENTMETA_USE_KEYIDLIST
    void DomEvents::buildEventMap ( ElementMultiMapRef& eventMap, DomEvent& event, DomEventMask mask, KeyIdList& qnames )
    {
        for ( KeyIdList::iterator iter = qnames.begin(); iter != qnames.end(); iter++ )
        {
            KeyId qnameId = *iter;
            buildEventMap(eventMap, event, mask, qnameId);
        }
    }
#endif

    void
    DomEvents::buildEventMap ()
    {
        if (findAttr(__xemint.event_map(), AttributeType_SKMap))
        {
            if (!deleteAttr(__xemint.event_map(), AttributeType_SKMap))
            {
                Bug("Could not delete this attribute ?\n");
            }
        }
        ElementMultiMapRef eventMap = addSKMap(__xemint.event_map(), SKMapType_ElementMultiMap);
        for (ChildIterator child(*this); child; child++)
        {
            DomEvent event = child;
            DomEventMask mask = event.getEventMask();

            if ((Integer) mask == 0)
            {
                throwException(Exception, "Empty mask for event=%s\n", event.generateVersatileXPath().c_str());
            }
            Log_DME ( "Event names : '%s'\n", mask.toString().c_str() );

            if (event.findAttr( __xemint.match(), AttributeType_String)
                    && !event.findAttr( __xemint.match(), AttributeType_XPath))
            {
                Log_DME ( "Saving XPath compiled format for %s\n", event.generateVersatileXPath().c_str() );
                XPathParser matchXPath(event, __xemint.match());
                matchXPath.saveToStore(event, __xemint.match());
            }

#ifdef __XEM_DOM_DOCUMENTMETA_USE_KEYIDLIST
            KeyIdList elementQNames = event.getElementQNames();
            KeyIdList attributeQNames = event.getAttributeQNames();
#else
            QNameListRef elementQNames = event.getAttrAsQNameList(__xemint.element_qnames());
            QNameListRef attributeQNames = event.getAttrAsQNameList(__xemint.attribute_qnames());
#endif

            buildEventMap(eventMap, event, mask.intersection(DomEventMask_Element), elementQNames);
            buildEventMap(eventMap, event, mask.intersection(DomEventMask_Attribute), attributeQNames);

            if (mask.intersects(DomEventMask_Document))
            {
                buildEventMap(eventMap, event, mask.intersection(DomEventMask_Document), __xemint.root());
            }
            else
            {
                if (elementQNames.empty() && attributeQNames.empty())
                {
                    throwException(Exception, "EventMask : not document, but no QName for element nor attribute !\n");
                }
            }
        }
        ElementRef nullRef(getDocument());
        getDocument().appendJournal(*this, JournalOperation_BuildMeta, nullRef, 0);
    }

    void
    DomEvents::processEvent (XProcessor& xproc, DomEventType domEventType, NodeRef& nodeRef)
    {
        ElementMultiMapRef eventMap = findAttr(__xemint.event_map(), AttributeType_SKMap);
        if (!eventMap)
        {
            Log_DME ( "No EventMap for role=%s, uri=%s, tag=%s\n",
                    getDocument().getRole().c_str(),
                    getDocument().getDocumentURI().c_str(),
                    getDocument().getDocumentTag().c_str() );
            return;
        }

        KeyId qnameId = nodeRef.getKeyId();
        if (!KeyCache::getNamespaceId(qnameId) && nodeRef.isAttribute())
        {
            qnameId = KeyCache::getKeyId(nodeRef.getElement().getNamespaceId(), qnameId);
        }
        SKMapHash hash = hashEvent(domEventType, qnameId);

        Log_DME ( "------------------------------------------------------\n" );
        Log_DME ( "Event for eventType=%x (%s), qnameId=%x, hash=%llx, nodeRef=%s\n",
                domEventType, DomEventMask(domEventType).toString().c_str(),
                qnameId, hash, nodeRef.generateVersatileXPath().c_str() );

        for (ElementMultiMapRef::multi_iterator iter(eventMap, hash); iter; iter++)
        {
            DomEvent event = eventMap.get(iter);

            if (!event)
            {
                Log_DME ( "No event for eventType=%x (%s), qnameId=%x, nodeRef=%s\n",
                        domEventType, DomEventMask(domEventType).toString().c_str(),
                        qnameId, nodeRef.generateVersatileXPath().c_str() );
                return;
            }
            AttributeRef matchAttr = event.findAttr(__xemint.match(), AttributeType_XPath);

            ElementRef elementRef = nodeRef.getElement();
            bool useElementRef = false;

            if (matchAttr)
            {
                XPath matchXPath(xproc, matchAttr);
                if (!matchXPath.matches(nodeRef))
                {
                    Log_DME ( "Does not match directly !\n" );
                    if (nodeRef.isAttribute() && matchXPath.matches(elementRef))
                    {
                        Log_DME ( "Attribute Element matches Element XPath !\n" );
                        useElementRef = true;
                    }
                    else
                    {
                        Log_DME ( "Attribute element does not match (%s) either... (elementRef=%s)\n",
                                matchXPath.getExpression(), elementRef.generateVersatileXPath().c_str() );
                        continue;
                    }
                }
            }
#if PARANOID
            else
            {
                AssertBug ( ! event.findAttr(__xemint.match(),AttributeType_String),
                        "Event %s has an XPath in String format, but none in XPath format !\n",
                        event.generateVersatileXPath().c_str() );
            }
#endif
            KeyId handlerId = event.getHandlerId();

            Log_DME ( "Found event %s for eventType=%x (%s), qnameId=%x, nodeRef=%s, handlerId=%x (%s)\n",
                    event.generateVersatileXPath().c_str(),
                    domEventType, DomEventMask(domEventType).toString().c_str(),
                    qnameId, nodeRef.generateVersatileXPath().c_str(),
                    handlerId, getKeyCache().dumpKey(handlerId).c_str() );

            XProcessor::DomEventHandler handler = xproc.getDomEventHandler(handlerId);
            if (!handler)
            {
                Warn("Could not resolve DomEventHandler %x (%s)\n", handlerId, getKeyCache().dumpKey(handlerId).c_str());
                throwException(Exception, "Could not resolve DomEventHandler %x (%s)\n", handlerId,
                               getKeyCache().dumpKey(handlerId).c_str());
            }
            (handler.module->*handler.hook)(domEventType, event, useElementRef ? elementRef : nodeRef);
        }
        Log_DME ( "------------------------------------------------------\n" );
    }
}
;
