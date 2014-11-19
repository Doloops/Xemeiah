#ifndef __XEMEIAH_PERSISTENCE_JOURNAL_H
#define __XEMEIAH_PERSISTENCE_JOURNAL_H

#include <Xemeiah/kern/format/core_types.h>

namespace Xem
{
  /**
   * List of operations supported by journal keeping
   */
  enum __JournalOperation
  {
    JournalOperation_NoOp            = 0x0,
    JournalOperation_InsertChild     = 0x1,
    JournalOperation_AppendChild     = 0x2,
    JournalOperation_InsertBefore    = 0x3,
    JournalOperation_InsertAfter     = 0x4,
    JournalOperation_UpdateTextNode  = 0x5,
    JournalOperation_Remove          = 0x6,
    JournalOperation_UpdateAttribute = 0x7,
    JournalOperation_DeleteAttribute = 0x8,
    JournalOperation_UpdateBlob      = 0x9,
    JournalOperation_BuildMeta       = 0xa,
    JournalOperation_LastOperation   = 0xb
  };
  
  /**
   * Structure for a JournalItem (entry), 32 bytes size (which fits FreeSegment)
   */
  struct JournalItem
  {
    JournalOperation op;        // 4
    KeyId attributeKeyId;       // 8
    SegmentPtr nextJournalItem; // 16
    ElementId baseElementId;    // 24
    ElementId altElementId;     // 32
  };

  /**
   * Structure for the head Journal
   * The firstAllocedJournalItem and lastAllocedJournalItem represents the Journal list, inherited after a fork()
   * The firstJournalItem represents the head of the journal, resetted after a fork()
   */
  struct JournalHead
  {
    SegmentPtr firstAllocedJournalItem;
    SegmentPtr lastAllocedJournalItem;    
    SegmentPtr firstJournalItem;
  };

  extern const char* JournalOperationLabel [];
};

#endif //  __XEMEIAH_PERSISTENCE_JOURNAL_H
