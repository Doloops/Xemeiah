/*
 * documentref.h
 *
 *  Created on: 30 oct. 2009
 *      Author: francois
 */

#ifndef __XEM_KERN_DOCUMENTREF_H_
#define __XEM_KERN_DOCUMENTREF_H_

#include <Xemeiah/kern/document.h>
#include <Xemeiah/trace.h>

#define Log_DocumentRefHPP Debug
namespace Xem
{
  /**
   * (Prototype) Document Reference
   * \todo export this to a dedicated .hpp file
   */
  class DocumentRef
  {
  protected:
    Document& dRef;
    XProcessor& xproc;
    bool autoCommit;
  public:
    DocumentRef ( XProcessor& _xproc, Document& _dRef )
    : dRef(_dRef), xproc(_xproc)
    {
      autoCommit = true; // false;
      Log_DocumentRefHPP ( "New DocumentRef for %p [%llx:%llx], refCount=%llu\n", &dRef, _brid(dRef.getBranchRevId()),
          dRef.getRefCount() );
      AssertBug ( dRef.getRefCount(),
          "DocumentRef created from %p [%llx:%llx] with refCount=%llu\n", &dRef, _brid(dRef.getBranchRevId()),
          dRef.getRefCount() );
    }

    ~DocumentRef ()
    {
      Log_DocumentRefHPP ( "[RELEASE %p]\n", &dRef );
      Log_DocumentRefHPP ( "Release DocumentRef for %p [%llx:%llx], refCount=%llx\n",
          &dRef, _brid(dRef.getBranchRevId()), dRef.getRefCount() );
      if ( dRef.isLockedWrite() )
        {
          AssertBug ( dRef.isWritable(), "Locked but not writable !\n" );
          if ( autoCommit && dRef.getRefCount() == 1 )
            {
              Log_DocumentRefHPP ( "Release DocumentRef for %p [%llx:%llx] : Will commit (autoCommit=%s, refCount=%llu, allocator refCount=%llu).\n",
                  &dRef, _brid(dRef.getBranchRevId()),
                  autoCommit ? "yes" : "no", dRef.getRefCount(), dRef.getDocumentAllocator().getRefCount() );
              dRef.commit(xproc);
            }
          else
            {
              Log_DocumentRefHPP ( "Release DocumentRef for %p [%llx:%llx] : Will not commit (autoCommit=%s, refCount=%llu, allocator refCount=%llu) : only unlockWrite.\n",
                  &dRef, _brid(dRef.getBranchRevId()),
                  autoCommit ? "yes" : "no",
                  dRef.getRefCount(), dRef.getDocumentAllocator().getRefCount() );
              dRef.unlockWrite();
            }
        }
      else
        {
          Log_DocumentRefHPP ( "Release DocumentRef for %p [%llx:%llx], refCount=%llx, no locked write, isWritable()=%s\n",
              &dRef, _brid(dRef.getBranchRevId()), dRef.getRefCount(), dRef.isWritable() ? "true" : "false" );
        }
      AssertBug ( ! dRef.isLockedWrite(), "Did not resume lock write !\n" );

      Store& store = dRef.getStore();

      Document* deletedRef = &dRef;
      store.releaseDocument(deletedRef);
    }

    operator Document& ()
    {
      return dRef;
    }

    Document* operator->()
    {
      return &dRef;
    }

    void lockWrite ()
    {
      AssertBug ( dRef.isWritable(), "Doc not writable !\n" );
      dRef.incrementRefCount();
      dRef.lockWrite();
    }

    void setAutoCommit ( bool _autoCommit )
    {
      autoCommit = _autoCommit;
    }
  };
};

#endif /* __XEM_KERN_DOCUMENTREF_H_ */
