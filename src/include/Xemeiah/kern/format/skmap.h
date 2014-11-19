#ifndef __XEM_KERN_FORMAT_CORE_TYPES_H
#error Must include <Xemeiah/kern/format/types.h> First !
#endif

#define __XEM_STORE_HAS_SKIPLISTS
#ifdef __XEM_STORE_HAS_SKIPLISTS

#ifndef __XEM_STORE_FORMAT_SKMAP_H
#define __XEM_STORE_FORMAT_SKMAP_H

namespace Xem
{
  /*
   * \file SKMap format.
   * 
   *
   * SKMap is based on the skipList concept.
   * SKMap implements a persistent map<SKMapHash, SKMapList>
   * At the moment, SKMapList is limited to (__ui32) maximum number of results.
   * This is sane.
   */
  
  /**
   * SKMapList is a chunk-based linked-list of SKMapValues ;
   * the values are stored by groups of SKMapList::maxNumber values.
   */
  struct SKMapList
  {
    static const __ui32 maxNumber = 30; 
    SegmentPtr nextList;
    SKMapValue values[maxNumber]; // 30 * 8 = 240 bytes, 
    __ui32 number;
    __ui32 padding;
  }; // Total size : 240 + 4 + 8 = 252 bytes (nearly 256)
    
  typedef SegmentPtr SKMapListPtr;

  /**
   * SKMapItem represents a key (SKMapHash) in the mapping ; SKMapItem has variable size,
   * depending on its level in the skip-list.
   */
  struct SKMapItem
  {
    __ui32 level;
    __ui32 dummy; // For store/mem-alignment
    SKMapHash hash;
    SKMapValue value;
    static const int minimumLevel = 1;
    SegmentPtr next[minimumLevel];
  }; // Size = 4 + (4) + 8 + 8 + 8 = 32, < FreeSegment

  typedef SegmentPtr SKMapItemPtr;

  /*
   * SkipList per-instance Configuration
   */
  /**
   * SKMap_maxLevel is at maximum defined at run-time.
   * Used only to set the probability of each level (coded as a __ui8 : 0 means "unreachable", 255 means 100% chances)
   */
  static const __ui32 SKMap_maxLevel = 64;
  
  /**
   * SKMap can be one of two main types : single associate container (one to one), or multiple (one to many) :
   * - Single containers have their value directly in SKMapItem, 
   * - Multiple have their values in SKMapList referenced in the SKMapItem.
   */
  enum SKMapType
  {
    SKMapType_ElementMap          = 0x01,
    SKMapType_ElementMultiMap     = 0x02,
    SKMapType_IntegerMap          = 0x03,
    SKMapType_Blob                = 0x04,
  };
  
#define SKMapType_QNameList SKMapType_IntegerMap
#define SKMapType_NamespaceList SKMapType_IntegerMap

  /**
   * Settings for SKMap skip-list associative container.
   */
  struct SKMapConfig
  {
    __ui32 skMapType;
    __ui32 maxLevel;
    AllocationProfile itemAllocProfile;
    AllocationProfile listAllocProfile;
    __ui8 probability[SKMap_maxLevel];
  };

  /**
   * SkipList Index Header.
   *
   * SkipLists are always defined as special attributes to elements.
   * So their size must not exceed the maximum size available for
   * attributes (which is very big).
   */
  struct SKMapHeader
  {
    SKMapConfig config;
    SegmentPtr head;
    __ui64 elements;
  };

};

#endif // __XEM_STORE_FORMAT_SKMAP_H
#endif // __XEM_STORE_HAS_SKIPLISTS

