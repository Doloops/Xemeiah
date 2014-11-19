#if 0 // DEPRECATED

#ifndef __XEM_SUBDOCUMENT_H
#define __XEM_SUBDOCUMENT_H

#include <Xemeiah/kern/document.h>

namespace Xem
{
  /**
   * SubDocument provides a chroot() mechanism on an existing Document ; 
   * the SubDocument created have the same contents, but the root element is changed.
   */
  class SubDocument : public Document
  {
  protected:
    Document& fatherDocument;
    
    bool mayIndex () { return fatherDocument.mayIndex(); }

    void mapArea ( __ui64 areaIdx ) { return fatherDocument.mapArea ( areaIdx ); }

    BranchRevId getRelativePageBranchRevId ( SegmentPage* segPage );

    RelativePagePtr getFreeRelativePages ( __ui64 askedNumber,  __ui64& allocedNumber, AllocationProfile allocProfile )
    { return fatherDocument.getFreeRelativePages ( askedNumber, allocedNumber, allocProfile ); }

    void authorizePageWrite ( RelativePagePtr relPagePtr )
    { return fatherDocument.authorizePageWrite ( relPagePtr ); }
    
    AllocationProfile getAllocationProfile ( SegmentPtr segmentPtr )
    { return fatherDocument.getAllocationProfile ( segmentPtr ); }
  public:
    SubDocument ( ElementRef& foreignRoot );
    ~SubDocument ();
  
  
  };


};

#endif // __XEM_SUBDOCUMENT_H

#endif
