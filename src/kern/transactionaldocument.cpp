/*
 * transactionaldocument.cpp
 *
 *  Created on: 15 nov. 2009
 *      Author: francois
 */

#include <Xemeiah/kern/transactionaldocument.h>
#include <Xemeiah/dom/domeventmask.h>
#include <Xemeiah/dom/elementref.h>

#include <Xemeiah/auto-inline.hpp>

namespace Xem
{

  void TransactionalDocument::triggerDomEventDocument ( XProcessor& xproc, DomEventType domEventType )
  {
    ElementRef root = getRootElement();
    processDomEvent ( xproc, domEventType, root );
  }

  void TransactionalDocument::commit ( XProcessor& xproc )
  {
    commit ();
    triggerDomEventDocument(xproc, DomEventType_DocumentCommit);
  }

  void TransactionalDocument::reopen ( XProcessor& xproc )
  {
    reopen ();
    triggerDomEventDocument(xproc, DomEventType_DocumentReopen);
  }

  void TransactionalDocument::grantWrite ( XProcessor& xproc )
  {
    if ( isWritable() )
      {
        if ( !isLockedWrite() )
          lockWrite();
        return;
      }
    reopen ( xproc );
  }

  void TransactionalDocument::drop ( XProcessor& xproc )
  {
    triggerDomEventDocument(xproc, DomEventType_DocumentDrop);
    drop ();
  }

  void TransactionalDocument::fork ( XProcessor& xproc, String& branchName, BranchFlags branchFlags )
  {
    fork ( branchName, branchFlags );
    triggerDomEventDocument(xproc, DomEventType_DocumentFork);
  }

#if 0
  void TransactionalDocument::merge ( XProcessor& xproc, bool keepBranch )
  {
    merge ( keepBranch );
    triggerDomEventDocument(xproc, DomEventType_DocumentMerge);
  }
#endif
};
