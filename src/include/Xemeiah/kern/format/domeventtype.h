/*
 * domeventtype.h
 *
 *  Created on: 9 nov. 2009
 *      Author: francois
 */

#ifndef __XEM_KERN_FORMAT_DOMEVENT_H_
#define __XEM_KERN_FORMAT_DOMEVENT_H_

#include <Xemeiah/kern/format/core_types.h>

namespace Xem
{
  /**
   * Type for DomEventType
   */
  typedef __ui32 DomEventType;

  /**
   * Macro assignment for DomEventType
   * DomEventType may be set as a Mask, so design in to be ORable
   */
  #define DomEventType_CreateElement         (1 << 0)
  #define DomEventType_DeleteElement         (1 << 1)
  #define DomEventType_CreateAttribute       (1 << 2)
  #define DomEventType_BeforeModifyAttribute (1 << 3)
  #define DomEventType_AfterModifyAttribute  (1 << 4)
  #define DomEventType_DeleteAttribute       (1 << 5)

  #define DomEventType_DocumentCommit        (1 << 10)
  #define DomEventType_DocumentReopen        (1 << 11)
  #define DomEventType_DocumentDrop          (1 << 12)
  #define DomEventType_DocumentFork          (1 << 13)
  #define DomEventType_DocumentMerge         (1 << 14)
};

#endif /* __XEM_KERN_FORMAT_DOMEVENT_H_ */
