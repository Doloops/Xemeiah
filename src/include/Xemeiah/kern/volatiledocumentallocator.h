#ifndef __XEM_KERN_VOLATILEDOCUMENTALLOCATOR_H
#define __XEM_KERN_VOLATILEDOCUMENTALLOCATOR_H
#include <Xemeiah/kern/documentallocator.h>

namespace Xem
{
  class Store;
  
  class VolatileDocumentAllocator : public DocumentAllocator
  {
  protected:
    /**
     * Reference to our store
     */
    Store& store;
  public:
    Store& getStore() const { return store; }
  
    VolatileDocumentAllocator ( Store& store );
  
    virtual ~VolatileDocumentAllocator ();
    
    void mapArea ( __ui64 areaIdx );

    void mapDirectPage ( RelativePagePtr relPagePtr );

    BranchRevId getRelativePageBranchRevId ( SegmentPage* segPage );

    RelativePagePtr getFreeRelativePages ( __ui64 askedNumber,  __ui64& allocedNumber, AllocationProfile allocProfile );

    void authorizePageWrite ( RelativePagePtr relPagePtr );

    AllocationProfile getAllocationProfile ( SegmentPtr segmentPtr );
    
  };

};

#endif // __XEM_KERN_VOLATILEDOCUMENTALLOCATOR_H
