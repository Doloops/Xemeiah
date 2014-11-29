/*
 * transactionlessdocument.h
 *
 *  Created on: 15 nov. 2009
 *      Author: francois
 */

#ifndef __XEM_KERN_TRANSACTIONLESSDOCUMENT_H
#define __XEM_KERN_TRANSACTIONLESSDOCUMENT_H

#include <Xemeiah/kern/document.h>

#define Log_TransactionlessDoc Debug

namespace Xem
{
  /**
   * Transactionless Document is a Document which does not support Transactions
   */
  class TransactionlessDocument : public Document
  {
  protected:
    TransactionlessDocument( Store& _store, DocumentAllocator& _documentAllocator )
    : Document(_store, _documentAllocator) {}

    virtual ~TransactionlessDocument() {}
  public:
    virtual BranchRevId getBranchRevId() { BranchRevId nullBrId = {0, 0}; Log_TransactionlessDoc ( "no BranchRevId.\n" ); return nullBrId; }

    virtual void commit () { Warn ( "no commit.\n" ); }
    virtual bool mayCommit() { Warn ( "no commit.\n"); return false; }
    virtual void reopen () { Warn ( "no reopen.\n" ); }
    virtual void grantWrite () { Warn ( "no grandWrite.\n" ); }
    virtual void drop () { Warn ( "no drop.\n" ); }
    virtual void fork ( const String& branchName, BranchFlags branchFlags ) { Warn ( "no fork.\n" ); }
    virtual void merge ( XProcessor& xproc, bool keepBranch ) { Warn ( "no merge.\n" ); }

    virtual void scheduleBranchForRemoval () { Bug ( "no scheduleBranchForRemoval.\n" ); }
    virtual void scheduleRevisionForRemoval () { Bug ( "no scheduleRevisionForRemoval.\n" ); }
    virtual bool isScheduledRevisionRemoval () { Bug ( "no isScheduledRevisionRemoval.\n "); return false; }
    virtual void appendJournal ( ElementRef& baseRef, JournalOperation op, ElementRef& altRef, KeyId attributeKeyId )
    { Log_TransactionlessDoc ( "no appendJournal.\n" ); }

    virtual void lockWrite () { Log_TransactionlessDoc ( "no lockWrite.\n" ) ; }
    virtual void unlockWrite () { Log_TransactionlessDoc ( "no lockWrite.\n" ) ; }
    virtual bool isLockedWrite() { Log_TransactionlessDoc ( "no lockWrite.\n" ); return isWritable(); }

    /**
     * Must return true if we need to lock BranchManager to delete this document (optimization hack)
     * No, we don't need to
     */
    virtual bool deletionImpliesLockBranchManager() { return false; }
  };

};

#endif /* __XEM_KERN_TRANSACTIONLESSDOCUMENT_H */
