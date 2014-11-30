#ifndef __XEM_KERN_JOURNALEDDOCUMENT_H
#define __XEM_KERN_JOURNALEDDOCUMENT_H

#include <Xemeiah/kern/document.h>
#include <Xemeiah/kern/format/journal.h>

namespace Xem
{
  /**
   * Journaled Document interface and functionnalities
   */
  class JournaledDocument : public Document
  {
  protected:
    JournaledDocument ( Store& store, DocumentAllocator& allocator );
    
    virtual JournalHead& getJournalHead () = 0;
    
    virtual bool isJournalEnabled() { return true; }

    /**
     * Applying journal : duplicate element contents
     * @param xproc an XProcessor for DOM events
     * @param target the target element (my element)
     * @param source the source element (from that source document)
     */
    void duplicateElement ( XProcessor& xproc, ElementRef& target, ElementRef& source );

    /**
     * Append a journal item
     */
    void appendJournalItem ( SegmentPtr jItemPtr, JournalItem* jItem );

    /**
     * Alter journal head
     */
    virtual void alterJournalHead () = 0;
    
    /**
     * Protect journal head
     */
    virtual void protectJournalHead () = 0;

    void applyJournalCreateElement(XProcessor& xproc, JournalItem* journalItem, ElementRef& baseElement, ElementRef& sourceAltElement);
  public:
    /**
     * Virtual destructor
     */
    virtual ~JournaledDocument () {}

    /**
     * Append a journal item entry
     */
    void appendJournal ( ElementRef& baseRef, JournalOperation op, ElementRef& altRef, KeyId attributeKeyId );

    /**
     * Apply another JournaledDocument's journal
     * @param xproc an XProcessor to use for Element events
     * @param source the source Document to apply journal from
     */
    void applyJournal ( XProcessor& xproc, JournaledDocument& source );

#if 0 // DEPRECATED
    /**
     * Dump the journal in XML format
     * @param evd the EventHandler to send SAX commands to
     */
    void dumpJournal ( EventHandlerDom& evd );
#endif
  };
};

#endif //  __XEM_KERN_JOURNALEDDOCUMENT_H

