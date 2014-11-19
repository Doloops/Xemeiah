/*
 * domeventmask.h
 *
 *  Created on: 9 nov. 2009
 *      Author: francois
 */

#ifndef __XEM_DOM_DOMEVENTMASK_H
#define __XEM_DOM_DOMEVENTMASK_H

#include <Xemeiah/trace.h>
#include <Xemeiah/kern/format/domeventtype.h>

namespace Xem
{
  class String;

  /**
   * Class DomEventMask : union of zero, one or more DomEventTypes
   */
  class DomEventMask
  {
  protected:
    /**
     * The mask
     */
    DomEventType mask;


  public:
    /**
     * Simple constructor
     */
    INLINE DomEventMask ();

    /**
     * Simple constructor with a DomEventType assigned
     */
    INLINE DomEventMask ( DomEventType type );

    /**
     * Copy constructor
     */
    INLINE DomEventMask ( const DomEventMask& mask );

    /**
     * Simple destructor
     */
    INLINE ~DomEventMask ();

    /**
     * hasType
     */
    INLINE bool hasType ( DomEventType type );

    /**
     * intersects
     */
    INLINE bool intersects ( DomEventMask other );

    /**
     * Compute intersection
     */
    INLINE DomEventMask intersection ( DomEventMask other );

    /**
     * Parse a string to a mask of DomEventType
     */
    INLINE void parse ( const String& maskString );

    /**
     * Serialize value to a space-separated String
     */
    INLINE String toString () const;

    /**
     * Serialize to Integer
     */
    INLINE operator Integer () const;

    /**
     * Union operator
     */
    INLINE DomEventMask operator| ( const DomEventMask& other ) const;
  };

  /**
   * All events that apply to Elements
   */
  static const DomEventMask DomEventMask_Element   =
      DomEventType_CreateElement|DomEventType_DeleteElement;

  /**
   * All events that apply to Attribute
   */
  static const DomEventMask DomEventMask_Attribute =
      DomEventType_CreateAttribute|DomEventType_BeforeModifyAttribute
      |DomEventType_AfterModifyAttribute|DomEventType_DeleteAttribute;

  /**
   * All Persistence-related events that apply at Document level for
   */
  static const DomEventMask DomEventMask_Document =
      DomEventType_DocumentCommit|DomEventType_DocumentReopen|DomEventType_DocumentDrop
      |DomEventType_DocumentFork|DomEventType_DocumentMerge;
};

#endif /* __XEM_DOM_DOMEVENTMASK_H */
