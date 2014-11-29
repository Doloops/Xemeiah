/*
 * transactionaldocument.h
 *
 *  Created on: 12 nov. 2009
 *      Author: francois
 */
#ifndef __XEM_KERN_TRANSACTIONALDOCUMENT_H
#define __XEM_KERN_TRANSACTIONALDOCUMENT_H

#include <Xemeiah/kern/format/core_types.h>
#include <Xemeiah/kern/format/domeventtype.h>

namespace Xem
{
  class XProcessor;
  class ElementRef;
  class NodeRef;
  class DocumentMeta;
  class String;

  /**
   * TransactionalDocument is an Interface to handle transactional operations on a document
   */
  class TransactionalDocument
  {
  protected:
    virtual ElementRef getRootElement() = 0;
    virtual DocumentMeta getDocumentMeta () = 0;

    virtual void triggerDomEventDocument ( XProcessor& xproc, DomEventType domEventType );

    virtual void processDomEvent ( XProcessor& xproc, DomEventType eventType, NodeRef& nodeRef ) = 0;
  public:
    /**
     * Persistence Interface API
     */
    virtual bool isWritable () = 0;

    virtual BranchRevId getBranchRevId() = 0;

    virtual bool mayCommit() = 0;

    virtual void commit () = 0;
    virtual void commit ( XProcessor& xproc );

    virtual void reopen () = 0;
    virtual void reopen ( XProcessor& xproc );

    virtual void grantWrite () = 0;
    virtual void grantWrite ( XProcessor& xproc );

    virtual void drop () = 0;
    virtual void drop ( XProcessor& xproc );

    virtual void fork ( const String& branchName, BranchFlags branchFlags ) = 0;
    virtual void fork ( XProcessor& xproc, const String& branchName, BranchFlags branchFlags );

    virtual void fork ( XProcessor& xproc, const String& branchName, const String& branchFlags );

    // virtual void merge ( bool keepBranch ) = 0;
    virtual void merge ( XProcessor& xproc, bool keepBranch ) = 0;

    virtual void scheduleBranchForRemoval () = 0;
    virtual void scheduleRevisionForRemoval () = 0;
    virtual bool isScheduledRevisionRemoval () = 0;
    virtual void appendJournal ( ElementRef& baseRef, JournalOperation op, ElementRef& altRef, KeyId attributeKeyId ) = 0;

    virtual void lockWrite () = 0;
    virtual void unlockWrite () = 0;
    virtual bool isLockedWrite() = 0;

    /**
     * Must return true if we need to lock BranchManager to delete this document (optimization hack)
     */
    virtual bool deletionImpliesLockBranchManager() = 0;
  };

};

#endif /* __XEM_KERN_TRANSACTIONALDOCUMENT_H */
