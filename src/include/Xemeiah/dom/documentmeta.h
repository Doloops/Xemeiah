/*
 * documentmeta.h
 *
 *  Created on: 9 nov. 2009
 *      Author: francois
 */

#ifndef __XEM_DOM_DOCUMENTMETA_H_
#define __XEM_DOM_DOCUMENTMETA_H_

#include <Xemeiah/kern/document.h>
#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/dom/domeventmask.h>

// #define __XEM_DOM_DOCUMENTMETA_USE_KEYIDLIST

namespace Xem
{
  class QNameListRef; //< The list of QNames stored as an attribute

  /**
   * Registered Dom Event : each Dom Event registered is represented by an Element into the Document
   */
  class DomEvent : public ElementRef
  {
  protected:
    /**
     * Add an element QName as applicable for this Event
     * @param keyId the QName of the Element to trigger this event for
     */
    void addElementQName ( KeyId keyId );

    /**
     * Add an attribute QName as applicable for this Event
     * @param keyId the QName of the Attribute to trigger this event for
     */
    void addAttributeQName ( KeyId keyId );

    /**
     *
     */
    void addNodeQName ( KeyId attributeKeyId, KeyId keyId );

#ifdef __XEM_DOM_DOCUMENTMETA_USE_KEYIDLIST
    /**
     *
     */
    KeyIdList getNodeQNames ( KeyId attributeKeyId );
#endif
  public:
    /**
     * Constructor : Cast an ElementRef as DomEvent (this will not create a new ElementRef !)
     */
    DomEvent ( const ElementRef& element );

    /**
     * Simple destructor
     */
    ~DomEvent () {}

    /**
     * Set the handle associated with this Event
     * @param handlerId the QName KeyId of the handler hook
     */
    void setHandlerId ( KeyId handlerId );

    /**
     * Get associated handle for this Event
     */
    KeyId getHandlerId ( );

    /**
     * Set the applicable event mask for this Event
     * @param domEventMask the event mask to apply to this event
     */
    void setEventMask ( DomEventMask domEventMask );

    /**
     * Get the applicable event mask for this Event
     */
    DomEventMask getEventMask ();

    /**
     * Add a single QName
     */
    void addQName ( KeyId qnameId );

    /**
     * Add all the QNames found in the following XPath
     */
    void addQNames ( XPath& xpath );

    /**
     * Set an XPath as matching rule
     */
    void setMatchXPath ( XProcessor& xproc, const String& expression );

#ifdef __XEM_DOM_DOCUMENTMETA_USE_KEYIDLIST
    /**
     * Get the list of elements defined
     */
    KeyIdList getElementQNames ();

    /**
     * Get the list of attributes defined
     */
    KeyIdList getAttributeQNames ();
#endif
  };


  /**
   * DomEvents
   */
  class DomEvents : public ElementRef
  {
  protected:
    /**
     * Hash an Event Id based on its type and QName
     * @param domEventType the event type
     * @param qnameId the QName of the Node
     * @return the SKMapHash to register this event for
     */
    static SKMapHash hashEvent ( DomEventType domEventType, KeyId qnameId );

    /**
     * Register a new event
     * @param domEventMask the event mask to apply to this event
     * @param matchXPath an XPath expression to trigger upon
     * @param handlerId the associated handler Id
     */
    DomEvent registerEvent ( DomEventMask domEventMask, XPath& matchXPath, KeyId handlerId );

    /**
     * Add this event to the event map
     * @param eventMap the main map for events
     * @param event build for this event
     * @param mask the event mask to build for
     * @param qnameId the QName of the Node to trigger for
     */
    void buildEventMap ( ElementMultiMapRef& eventMap, DomEvent& event, DomEventMask mask, KeyId qnameId );

    /**
     * Add this event to the event map
     * @param eventMap the main map for events
     * @param event build for this event
     * @param mask the event mask to build for
     * @param qnames the list of QNames to build for
     */
    void buildEventMap ( ElementMultiMapRef& eventMap, DomEvent& event, DomEventMask mask, QNameListRef& qnames );

#ifdef __XEM_DOM_DOCUMENTMETA_USE_KEYIDLIST
    /**
     * Add this event to the event map
     * @param eventMap the main map for events
     * @param event build for this event
     * @param mask the event mask to build for
     * @param qnames the list of QNames to build for
     */
    void buildEventMap ( ElementMultiMapRef& eventMap, DomEvent& event, DomEventMask mask, KeyIdList& qnames );
#endif // __XEM_DOM_DOCUMENTMETA_USE_KEYIDLIST
  public:
    /**
     * DomEvents constructor : ElementRef caster (will not create a new DOM Element !)
     * @param element the ElementRef to cast as DomEvents
     */
    DomEvents ( const ElementRef& element );

    /**
     * DomEvents simple destructor
     */
    ~DomEvents () {}

    /**
     * Create a new Dom Event
     */
    DomEvent createDomEvent ();

    /**
     * Register an event for a single QName
     * @param domEventMask the Event Mask to apply
     * @param matchKeyId the QName of the Node to trigger this event for
     * @param handlerId the Event handler to associate to this event
     * @return the newly created DomEvent Element
     */
    DomEvent registerEvent ( DomEventMask domEventMask, KeyId matchKeyId, KeyId handlerId );

    /**
     * Register an event from a matching XPath expression
     * @param matchXPath the XPath expression to use to trigger this event
     * @param handlerId the Event handler to associate to this event
     * @return the newly created DomEvent Element
     */
    DomEvent registerEvent ( XPath& matchXPath, KeyId handlerId ) DEPRECATED;

    /**
     * Refresh the Event Map, mandatory after a call to registerEvent()
     */
    void buildEventMap ();

    /**
     * Process an event
     * @param xproc the XProcessor to use
     * @param eventType the type of event
     * @param nodeRef the Node (ElementRef or AttributeRef) for which events may be triggered
     */
    void processEvent ( XProcessor& xproc, DomEventType eventType, NodeRef& nodeRef );

  };

  /**
   * DocumentMeta
   */
  class DocumentMeta : public ElementRef
  {
  public:
    /**
     * DocumentMeta Constructor : simple cast to the DocumentMeta class
     */
    DocumentMeta ( const ElementRef& element );

    /**
     * DocumentMeta Destructor : nothing to do
     */
    ~DocumentMeta () {}

    /**
     * Get DomEvents class
     */
    DomEvents getDomEvents ();
  };

};

#endif /* __XEM_DOM_DOCUMENTMETA_H_ */
