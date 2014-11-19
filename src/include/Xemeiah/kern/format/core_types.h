#ifndef __XEM_KERN_FORMAT_CORE_TYPES_H
#define __XEM_KERN_FORMAT_CORE_TYPES_H

#include <time.h>

namespace Xem
{
#define InPageBits 12
#define PageSize (((__ui64)1)<<InPageBits)

#define TotalPointerBits 64

#define MaximumPage (((__ui64)1)<<(TotalPointerBits-InPageBits))
#define PagePtr_Mask ((MaximumPage-1)<<InPageBits)
#define SegmentPtr_Mask (PageSize-1)

  typedef unsigned long long __ui64;
  typedef unsigned int __ui32;
  typedef unsigned short int __ui16;
  typedef unsigned char __ui8;

  typedef long long int Integer;
  typedef double Number;
  
  typedef __ui64 BranchId;
  typedef __ui64 RevisionId;
  /**
   * KeyIds are defined 32bits only, because there may not be 4G different
   * keys in the whole universe. 
   * At least, 4G > 36^6, so we could store more than all 6-character 
   * (lowcase alpha + digital) random combinations of keys...
   *
   * In order to simplify the keyId to nsKeyId mappings, and reduce volume
   * in Store, the 32 bits of the key are splitted in two parts.
   * The first (32-KeyId_LocalBits) bits are for the namespace id,
   * The last (KeyId_LocalBits) bits are for the local key id.
   *
   * As a result, the KeyId, a __ui32 value, is structured as a couple of __ui16 values.
   * But it may not be defined symetrically, taking for example 10 bits for namespace and 22 bits for local part.
   */
  typedef __ui32 KeyId;

  typedef __ui16 NamespaceId;
  typedef __ui16 LocalKeyId;
  typedef __ui16 NSAliasId;
#define KeyId_LocalBits (16)
#define KeyId_LocalMask ( (1<<KeyId_LocalBits) - 1 )
  
  /**
   * ElementId is stored in the ElementSegment as well.
   */
  typedef __ui64 ElementId;


  /**
   * Big Fat Notice :
   *
   * AbsolutePagePtr : absolute page pointers represent the effective offset
   * between the beginning of the store file to beginning of the page described.
   * As a result, AbsolutePagePtr are *always* aligned to PageSize.
   *
   * Relative For Segment Pages, SegmentPtrs are made up of two different parts :
   * [Relative Page Pointer] [In-page segment pointer]
   * 63 ..             .. 12 11 ..                .. 0 (bits, for 64-12 config.)
   *
   * As a result, RelativePagePtr is *always* aligned to PageSize, and in-page
   * segment pointer is *always* < PageSize.
   */
  typedef __ui64 AbsolutePagePtr;
  typedef AbsolutePagePtr IndirectionPagePtr;
  typedef AbsolutePagePtr PageInfoPagePtr;  
  typedef __ui64 RelativePagePtr;

  typedef AbsolutePagePtr BranchPagePtr;
  typedef AbsolutePagePtr RevisionPagePtr;

  typedef __ui64 SegmentPtr;

  typedef __ui64 PageType;

  /**
   * Unique Branch-Revision Id
   * The BranchId is the unique id for branches, and the revisionId
   * is incremented within a branch for all revisions created in the branch.
   */
  struct BranchRevId
  {
    BranchId branchId;
    RevisionId revisionId;
  };
  
  
  /**
   * Branch Flags
   */
  typedef __ui64 BranchFlags;
 
  static const BranchFlags BranchFlags_None                      = 0x00; //< No flag
  static const BranchFlags BranchFlags_MustForkBeforeUpdate      = 0x01; //< Option : we have to use fork() to create writable documents on this branch
  static const BranchFlags BranchFlags_MayIndexNameIfDuplicate   = 0x02; //< Option : at creation time, may alter provided branch name to grant unicity
  static const BranchFlags BranchFlags_AutoForked                = 0x04; //< Option : set the branch as automatically forked
  static const BranchFlags BranchFlags_AutoMerge                 = 0x08; //< Option : set the branch to automatically merge at commit
  static const BranchFlags BranchFlags_MayNotBeXUpdated          = 0x10; //< Option : forbid XUpdate to modify this branch
  static const BranchFlags BranchFlags_CanForkAtReopen           = 0x20; //< Option : we can use fork() in reopen() if necessary
  
#define _brid(brevid) brevid.branchId, brevid.revisionId
#define bridcmp(brid1,brid2)				\
  ( ( (brid1).branchId < (brid2).branchId) ? -2 :		\
    ( (brid1).branchId > (brid2).branchId ) ? 2 :		\
    ( (brid1).revisionId < (brid2).revisionId ) ? -1 :		\
    ( (brid1).revisionId > (brid2).revisionId) ? 1 : 0 )

  /**
   * Segments and pages allocation profiles
   */
  typedef __ui32 AllocationProfile;

  struct ElementSegment;

  typedef __ui64 SKMapHash;
  typedef __ui64 SKMapValue;
  struct SKMapConfig;
  
  /**
   * Operation of a Journal Item
   */
  typedef __ui32 JournalOperation;

  /**
   * Pointer to an Element
   */
  typedef SegmentPtr ElementPtr;

  /**
   * Pointer to an Attribute
   */
  typedef SegmentPtr AttributePtr;
  
  /**
   * Size of text contents, for both attributes and text(), comment() or pi() nodes
   * Size is at least limited by the Dom AreaSize and SegmentSizeMax, so __ui32 shall be enough
   * If more contents must be stored, we may use the Blob mechanism instead
   */
  typedef __ui32 DomTextSize;
};


#endif // __XEM_KERN_FORMAT_CORE_TYPES_H
