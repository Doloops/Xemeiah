#include <Xemeiah/xpath/xpathparser.h>
#include <Xemeiah/trace.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_XPathParser_Store Debug

namespace Xem
{
  __ui64 XPathParser::getParsedSize ()
  {
    __ui64 size = sizeof(XPathSegment) + parsedSteps.size() * sizeof(XPathStep) + nextResourceOffset;
    return size;
  }
  
  void XPathParser::packParsed ( char* data, __ui64 segmentSize )
  {
    /** 
     * Pack our stuff in the data pointer
     * 1- Pack Segment header
     */
    __ui64 offset = 0;
    XPathSegment* xpathSegment = (XPathSegment*) &(data[offset]);
    xpathSegment->firstStep = firstStep;
    xpathSegment->nbStepId = parsedSteps.size();
    offset += sizeof(XPathSegment);

    /**
     * 2- Pack Steps
     */
    for ( XPathStepId stepId = 0 ; stepId < parsedSteps.size() ; stepId++ )
      {
        memcpy ( &(data[offset]), getStep(stepId), sizeof(XPathStep) );
        offset += sizeof(XPathStep);
      }

    /**
     * 3- Pack Resources
     */
    if ( nextResourceOffset )
      {
        Log_XPathParser_Store ( "Saving resources from %p, nextResourceOffset=%llu, offset=%llu\n",
          resourceBlock, nextResourceOffset, offset );
        memcpy ( &(data[offset]), resourceBlock, nextResourceOffset );
        offset += nextResourceOffset;
      }

    AssertBug ( offset == segmentSize, "Computed offset=%llu differs from segmentSize=%llu !\n", offset, segmentSize );
  }

  XPathSegment* XPathParser::getPackedParsed ()
  {
    __ui64 size = getParsedSize ();
    if ( packedSegment ) return packedSegment;
    packedSegment = (XPathSegment*) malloc ( size );
    packParsed ( (char*) packedSegment, size );
    return packedSegment;
  }

  AttributeRef XPathParser::saveToStore ( ElementRef& elementRef, KeyId attrKeyId )
  {
    Log_XPathParser_Store ( "Save to store : eltRef=%llx/%x:%s attr=%x (ctxt=%p wr=%d)\n",
	      elementRef.getElementId(), elementRef.getKeyId(),
	      elementRef.getKey().c_str(), attrKeyId,
	      &(elementRef.getDocument()), elementRef.getDocument().isWritable() );

    AssertBug ( elementRef.getDocument().isWritable(),"Could not save to store, document not writable : "
        "eltRef=%llx/%x:%s attr=%x (ctxt=%p wr=%d)\n",
        elementRef.getElementId(), elementRef.getKeyId(),
        elementRef.getKey().c_str(), attrKeyId,
        &(elementRef.getDocument()), elementRef.getDocument().isWritable() );
#if PARANOID
//	  AssertBug ( xpathSegment, "Can not store a NULL XPath !\n" );
#endif
    /**
     * Compute segment size (i.e. Steps and Resource pointers)
     */
    __ui64 segmentSize = getParsedSize ();

    Log_XPathParser_Store ( "Total code size : %llu bytes (0x%llx)\n", segmentSize, segmentSize );

    if ( segmentSize > SegmentSizeMax ) { NotImplemented ( "Very large parsed XPath !\n"); }
    
    /**
     * Acquire an Attribute with the appropriate size.
     */
    AttributeRef attrRef = elementRef.addAttr ( attrKeyId, AttributeType_XPath, segmentSize );
    char * data = (char*) attrRef.getData<void,Write> ();
    Log_XPathParser_Store ( "xpath data at %p\n", data );

    attrRef.alterData ();

    packParsed ( data, segmentSize );

    attrRef.protectData ();
    return attrRef;
  }


};
