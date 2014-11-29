#include <Xemeiah/persistence/persistentdocumentallocator.h>
#include <Xemeiah/persistence/allocationstats.h>
#include <Xemeiah/kern/format/dom.h>
#include <Xemeiah/kern/format/blob.h>
#include <Xemeiah/kern/format/journal.h>
#include <Xemeiah/kern/format/skmap.h>
#include <Xemeiah/persistence/format/revision.h>
#include <Xemeiah/persistence/format/pages.h>

#include <Xemeiah/persistence/pageinfoiterator.h>

#include <Xemeiah/auto-inline.hpp>
#include <Xemeiah/persistence/auto-inline.hpp>

namespace Xem
{
#define Log_Check Info
#define Log_Check_L Debug // Very verbose
#define Log_Check_Journal Debug // Very verbose
#define Log_Check_Page Log

#define CheckError(__text,...) do { fprintf ( stderr, "[CHECK][ERROR][Rev=%llx:%llx]" __text, _brid(getBranchRevId()), __VA_ARGS__ );  \
        stats.addError("[Rev=%llx:%llx]" __text, _brid(getBranchRevId()), __VA_ARGS__); } while (0)
#define CheckInfo(__text,...) fprintf ( stderr, "[CHECK][Rev=%llx:%llx]" __text, _brid(getBranchRevId()), __VA_ARGS__ );

#if 1
#define CheckLog(...)
#else
#define CheckLog(...) fprintf ( stderr, "[CHECK]" __VA_ARGS__ );
#endif

#define AssertError(__cond,...) if ( ! (__cond) ) CheckError ( __VA_ARGS__ )

    bool
    PersistentDocumentAllocator::checkIndirectionPage (RelativePagePtr relPagePtr, AbsolutePagePtr absPagePtr,
                                                       PageType pageType, bool isStolen, void* arg)
    {
        AllocationStats* pStats = (AllocationStats*) arg;
        pStats->referencePage("In Revision/Indirection", revisionPageRef.getPage()->branchRevId, relPagePtr,
                              absPagePtr & PagePtr_Mask, pageType, __isStolen(absPagePtr) | isStolen);
        return true;
    }

    bool
    PersistentDocumentAllocator::checkSegmentPage (RelativePagePtr relPagePtr, PageInfo& pageInfo, PageType pageType,
                                                   bool isStolen, void* arg)
    {
        AssertBug(pageType, "Invalid non-typed page !\n");
        AbsolutePagePtr absPagePtr = pageInfo.absolutePagePtr;
        AllocationStats* pStats = (AllocationStats*) arg;

        if (isStolen)
            absPagePtr |= PageFlags_Stolen;
        if (pageType == PageType_Revision)
        {
            return true;
        }
        pStats->referencePage("In Revision", revisionPageRef.getPage()->branchRevId, relPagePtr,
                              absPagePtr & PagePtr_Mask, pageType, __isStolen(absPagePtr));
        return true;
    }

    void
    PersistentDocumentAllocator::checkPages (AllocationStats& stats)
    {
        forAllIndirectionPages(&PersistentDocumentAllocator::checkIndirectionPage,
                               &PersistentDocumentAllocator::checkSegmentPage, &stats, false, false);
        Log_Check_Page ( "[After Check : alloc=%llx, map=%llx, chunkMapSize=%llx]\n",
                areasAlloced, areasMapped, getPersistentStore().getChunkMapSize() );
    }

    void
    PersistentDocumentAllocator::checkRelativePages (AllocationStats& stats)
    {
#ifdef __XEM_PERSISTENCE_CHECK_BUILD_BRANCHPAGETABLE      
        /*
         * Checking the branch page table
         *
         */
        AllocationStats::BranchPageTable* brTable = stats.getBranchPageTable();
        BranchRevId myBrId = getBranchRevId();
        for (RelativePagePtr relPagePtr = PageSize; relPagePtr < getDocumentAllocationHeader().nextRelativePagePtr;
                relPagePtr += PageSize)
        {
            // Log_Check_Page ( "At relPagePtr = %llx\n", relPagePtr );
            AllocationStats::BranchPageTable::iterator rpiter = brTable->find(relPagePtr);
            if (rpiter == brTable->end())
            {
                CheckError("Invalid relPagePtr=%llx\n", relPagePtr);
                continue;
            }
            AllocationStats::RelativePageInfos& rpi = rpiter->second;
            if (rpi.revInfos.size() == 0)
            {
                CheckError("Invalid null rpvi for relPagePtr=%llx\n", relPagePtr);
            }
            Log_Check_Page ( "Page %llx has %lu rpvi\n", relPagePtr, (unsigned long) rpi.revInfos.size() );

            AbsolutePagePtr myAbsPagePtr = NullPage;
            AbsolutePagePtr lastNotStolenAbsPagePtr = NullPage;
            BranchRevId lastNotStolenBranchRevId =
                { 0, 0 };
            bool stolen = false, found = false, resolved = false;

            for (std::list<AllocationStats::RelativePageRevInfos>::iterator rpviter = rpi.revInfos.begin();
                    rpviter != rpi.revInfos.end(); rpviter++)
            {
                AllocationStats::RelativePageRevInfos& rpvi = *rpviter;
                Log_Check_Page ( "\trelPagePtr=%llx, brId=%llx:%llx, abs=%llx, stolen=%s\n",
                        relPagePtr, _brid(rpvi.brId), rpvi.absPagePtr, rpvi.stolen ? "true" : "false" );

                if ( bridcmp(myBrId, rpvi.brId) == 0)
                {
                    Log_Check_Page ( "\t\tFound my version !\n" );
                    myAbsPagePtr = rpvi.absPagePtr;
                    found = true;
                    stolen = rpvi.stolen;
                    if ( ! stolen )
                    {
                        resolved = true;
                    }
                }
                else if ( ! rpvi.stolen )
                {
                    lastNotStolenAbsPagePtr = rpvi.absPagePtr;
                    lastNotStolenBranchRevId = rpvi.brId;
                    resolved = true;
                }
            }
            Log_Check_Page("Final, myAbsPagePtr=%llx, lastNotStolenAbsPagePtr=%llx (brid=%llx:%llx), stolen=%d, found=%d, resolved=%d\n",
                    myAbsPagePtr, lastNotStolenAbsPagePtr, _brid(lastNotStolenBranchRevId), stolen, found, resolved);
            if (!found)
            {
                CheckError("Cound not find relPagePtr=%llx\n", relPagePtr);
            }
            if (stolen && (myAbsPagePtr != lastNotStolenAbsPagePtr))
            {
                CheckError("Diverging absPagePtr whereas stolen : my=%llx, rpvi(%llx:%llx)=%llx\n", myAbsPagePtr,
                           _brid(lastNotStolenBranchRevId), lastNotStolenAbsPagePtr);
            }
            if (!resolved)
            {
                CheckError("Unresolved relPagePtr=%llx\n", relPagePtr);
                for (std::list<AllocationStats::RelativePageRevInfos>::iterator rpviter = rpi.revInfos.begin();
                        rpviter != rpi.revInfos.end(); rpviter++)
                {
                    AllocationStats::RelativePageRevInfos& rpvi = *rpviter;
                    CheckError("\trelPagePtr=%llx, brId=%llx:%llx, abs=%llx, stolen=%s\n", relPagePtr, _brid(rpvi.brId),
                               rpvi.absPagePtr, rpvi.stolen ? "true" : "false");

                }
            }
        }
#endif //  __XEM_PERSISTENCE_CHECK_BUILD_BRANCHPAGETABLE
    }

    void
    PersistentDocumentAllocator::checkPageInfos (AllocationStats& stats, __ui64 *nbPagesPerAllocationProfile, __ui64& totalPages)
    {
        mapMutex.lock();
        for (PageInfoIterator iter(*this); iter; iter++)
        {
            RelativePagePtr relPagePtr = iter.first();
            if ((relPagePtr / PageSize) % 8 == 0)
            {
                if (relPagePtr)
                fprintf( stderr, "\n");
                CheckInfo("rel:%8llx : ", relPagePtr);
            }

            PageInfo pageInfo = iter.second();

#ifdef __XEM_PERSISTENTDOCUMENTALLOCATOR_HAS_PAGEINFOPAGEPTRTABLE
            PageInfoPagePtr cachePtr = 0;
            PageInfoPagePtrTable::iterator iter = pageInfoPtrTable.find(relPagePtr);
            if ( iter != pageInfoTable.end() )
            {
                cachePtr = iter->second;
            }
#endif // __XEM_PERSISTENTDOCUMENTALLOCATOR_HAS_PAGEINFOPAGEPTRTABLE
            fprintf( stderr, "%8llx%c ", pageInfo.absolutePagePtr,
            bridcmp(getBranchRevId(), pageInfo.branchRevId) ? '*' : ' ');

            // idx++;
            totalPages++;

            if (pageInfo.allocationProfile >= getDocumentAllocationHeader().nbAllocationProfiles)
            {
                CheckError("Invalid allocation profile for page %llx, from brid [%llx:%llx], allocation profile=%x\n",
                relPagePtr, _brid(pageInfo.branchRevId), pageInfo.allocationProfile);
            }
            else
            {
                nbPagesPerAllocationProfile[pageInfo.allocationProfile]++;
            }
        }
        mapMutex.unlock();
        fprintf( stderr, "\n");

    }

    void
    PersistentDocumentAllocator::checkContents (AllocationStats& stats)
    {
        RevisionPage* revisionPage = revisionPageRef.getPage();
        CheckInfo(
                "Checking revision contents for revisionPage=%p, revision=[%llx:%llx], nextRelativePagePtr=%llx, documentHeadPtr=%llx, " "indirection[firstPage=%llx, level=%x]\n",
                revisionPage, _brid(getBranchRevId()), getNextRelativePagePtr(), revisionPage->documentHeadPtr,
                revisionPage->indirection.firstPage, revisionPage->indirection.level);

        __ui64 chunkSize = sizeof(FreeSegment);
        RelativePagePtr nextRelativePagePtr = getNextRelativePagePtr();
        AssertBug(nextRelativePagePtr % chunkSize == 0,
                  "Invalid nextRelativePagePtr=%llx, not a multiple of chunkSize=%llx\n", nextRelativePagePtr,
                  chunkSize);

        __ui64 *nbPagesPerAllocationProfile = (__ui64 *) malloc(sizeof(__ui64) * getDocumentAllocationHeader().nbAllocationProfiles );
        __ui64 totalPages = 0;
        memset(nbPagesPerAllocationProfile, 0, sizeof(__ui64) * getDocumentAllocationHeader().nbAllocationProfiles );

        checkPageInfos(stats, nbPagesPerAllocationProfile, totalPages);

        CheckInfo("Total pages per profile : (total=%llu, nbFreeListHeaders=%x)\n", totalPages,
                  getDocumentAllocationHeader().nbAllocationProfiles);
        for (AllocationProfile allocProfile = 0; allocProfile < getDocumentAllocationHeader().nbAllocationProfiles;
                allocProfile++)
        {
            if (nbPagesPerAllocationProfile[allocProfile])
            {
                CheckInfo("\tprofile %x : %llu pages (%llu %%).\n", allocProfile,
                          nbPagesPerAllocationProfile[allocProfile],
                          (nbPagesPerAllocationProfile[allocProfile] * 100) / totalPages);
            }
        }

        __ui64 nbChunks = nextRelativePagePtr / chunkSize;
        Log_Check ( "\tContents : nbChunks=%llx, chunckSize=%llx\n", nbChunks, chunkSize );

        __ui64 sizeofChunk = 1;

        unsigned char* chunks = (unsigned char*) malloc(sizeofChunk * nbChunks);
        CheckInfo(
                "\tContents : alloced %llu MBytes (0x%llx bytes) at %p for 0x%llx chunks (revision has %llx pages, %llu MBytes)\n",
                (sizeofChunk * nbChunks) >> 20, sizeofChunk * nbChunks, chunks, nbChunks,
                nextRelativePagePtr >> InPageBits, nextRelativePagePtr >> 20);

        memset(chunks, '?', sizeofChunk * nbChunks);

        enum SegmentType
        {
            SegmentType_AttributeHead = 'A',
            SegmentType_Attribute = 'a',
            SegmentType_BlobHead = 'B',
            SegmentType_Blob = 'b',
            SegmentType_DocumentHeadHead = 'D',
            SegmentType_DocumentHead = 'd',
            SegmentType_ElementHead = 'E',
            SegmentType_Element = 'e',
            SegmentType_FreePageHead = 'P',
            SegmentType_FreePage = 'p',
            SegmentType_FreeSegmentHead = 'F',
            SegmentType_FreeSegment = 'f',
            SegmentType_JournalItemHead = 'J',
            SegmentType_JournalItem = 'j',
            SegmentType_ReservedHead = 'R',
            SegmentType_Reserved = 'r',
            SegmentType_SKMapItemHead = 'I',
            SegmentType_SKMapItem = 'i',
            SegmentType_SKMapListHead = 'L',
            SegmentType_SKMapList = 'l',
            SegmentType_TextualContentsHead = 'T',
            SegmentType_TextualContents = 't'
        };

        __ui64 nbChunksPerType[256];
        memset(nbChunksPerType, 0, sizeof(nbChunksPerType));

#define __isEmptySegment(__type,__offset,__size,__error) \
    do { \
      __ui64 __idx = __offset / chunkSize; \
      __error = false; \
      if ( (__offset) % chunkSize ) \
      { CheckError ( "Invalid ptr=%llx not aligned to chunkSize %llx, remains=%llx.\n", __offset, chunkSize, (__offset) % chunkSize ); }\
      else if ( __idx >= nbChunks ) { CheckError ( "Invalid ptr=%llx out of bounds !\n", __offset ); } \
      else if ( chunks[__idx] != '?' ) \
      { CheckError ( "Chunk %llx already set as %c, now saying type=%c\n", __offset, chunks[__idx], (unsigned char)__type##Head ); __error = true; } \
      else { \
        for ( __ui64 __s = 1 ; __s * chunkSize < __size ; __s++ ) \
          { \
            if ( chunks[__idx+__s] != '?' ) { CheckError ( "Chunk %llx already set at offset=%llx, as %c instead of %c\n", __offset, __s*chunkSize, chunks[__idx+__s],  (unsigned char)__type ); __error = true; } \
          } \
        } \
      if ( __error ) { erroneousPages[__offset & PagePtr_Mask] = true; }  \
    } while (0)

#define __markSegment(__type, __offset, __size) \
    do { \
      __ui64 __idx = __offset / chunkSize; \
      bool error = false; \
      if ( (__offset) % chunkSize ) \
      { CheckError ( "Invalid ptr=%llx not aligned to chunkSize %llx, remains=%llx.\n", __offset, chunkSize, (__offset) % chunkSize ); }\
      else if ( __idx >= nbChunks ) { CheckError ( "Invalid ptr=%llx out of bounds !\n", __offset ); } \
      else if ( chunks[__idx] != '?' ) \
      { CheckError ( "Chunk %llx already set as %c, now saying type=%c\n", __offset, chunks[__idx], (unsigned char)__type##Head ); error = true; } \
      else { \
        chunks[__idx] = (unsigned char)__type##Head; identifiedSize += chunkSize; nbChunksPerType[__type##Head]++; \
        for ( __ui64 __s = 1 ; __s * chunkSize < __size ; __s++ ) \
          { \
            if ( chunks[__idx+__s] != '?' ) { CheckError ( "Chunk %llx already set at offset=%llx, as %c instead of %c\n", __offset, __s*chunkSize, chunks[__idx+__s],  (unsigned char)__type ); error = true; } \
            else { chunks[__idx+__s] = (unsigned char)__type; identifiedSize += chunkSize; nbChunksPerType[__type]++; } \
          } \
        } \
      if ( error ) { erroneousPages[__offset & PagePtr_Mask] = true; }  \
    } while (0)

#define __isValidPtr(__offset) \
  ( (__offset % chunkSize == 0 ) && ( (__offset) / chunkSize ) < nbChunks )

#define __checkIsValidElementPtr(__offset) \
    do { \
      AssertBug ( (__offset) % chunkSize == 0, "Invalid ptr=%llx not aligned to chunkSize %llx, remains=%llx.\n", __offset, chunkSize, (__offset) % chunkSize ); \
      __ui64 __idx = __offset / chunkSize; \
      if ( __idx >= nbChunks ) \
      { CheckError ( "Invalid ptr=%llx out of bounds !\n", __offset ); } \
      else if ( chunks[__idx] != (unsigned char) SegmentType_ElementHead ) \
      { CheckError ( "Invalid ptr=%llx, not an element : type='%c' !\n", __offset, chunks[__idx] ); } \
    } while(0)

#define __dumpPageContents(__relPagePtr) \
  do { __ui64 index = __relPagePtr / chunkSize;  \
    /* PageInfo pageInfo; getPageInfo ( __relPagePtr, pageInfo ); */ \
    Lock __lock(mapMutex); \
    __ui64 __index; \
    AbsolutePageRef<PageInfoPage> __pageInfoPageRef = doGetPageInfoPage(__relPagePtr, __index, false); \
    PageInfo& pageInfo = getPageInfo(__pageInfoPageRef, __index); \
    CheckLog ( "--> Dumping page contents - rel=%llx (abs=%llx, brid=[%llx:%llx], allocProfile=%x, firstFreeInPage=%x)", \
      __relPagePtr, pageInfo.absolutePagePtr, _brid(pageInfo.branchRevId), pageInfo.allocationProfile, pageInfo.firstFreeSegmentInPage ); \
    for ( __ui64 i = 0 ; i < PageSize/chunkSize ; i++ ) \
      { \
        if ( i % 64 == 0 ) \
        { \
          fprintf ( stderr, "\n" ); \
          CheckLog ( "%llx :", __relPagePtr + i*chunkSize );\
        } \
        if (i % 8 == 0 ) \
          fprintf ( stderr, " " );\
        fprintf ( stderr, "%c", chunks[index+i] );\
      } \
    fprintf ( stderr, "\n" );\
  } while ( 0 )  

        std::map<RelativePagePtr, bool> erroneousPages;
        std::list<SegmentPtr> skMapList;

        __ui64 identifiedSize = 0;

        __markSegment(SegmentType_Reserved, 0ULL, sizeof(FreeSegment));
        __markSegment(SegmentType_DocumentHead, revisionPage->documentHeadPtr, sizeof(DocumentHead));

        /*
         * Identify JournalItem segments as a single-linked-list
         */
        CheckInfo("Identifying JournalItem : alloced[first=%llx,last=%llx], first=%llx\n",
                  getRevisionPage()->journalHead.firstAllocedJournalItem,
                  getRevisionPage()->journalHead.lastAllocedJournalItem,
                  getRevisionPage()->journalHead.firstJournalItem);

        SegmentPtr journalItemPtr = getRevisionPage()->journalHead.firstAllocedJournalItem;

        bool identifiedFirstJournalItemPtr = false;
        if (getRevisionPage()->journalHead.firstJournalItem == NullPtr)
            identifiedFirstJournalItemPtr = true;

        while (journalItemPtr)
        {
            Log_Check_Journal ( "\tjournalItemPtr=%llx\n", journalItemPtr );
            if (journalItemPtr == getRevisionPage()->journalHead.firstJournalItem)
                identifiedFirstJournalItemPtr = true;
            __markSegment(SegmentType_JournalItem, journalItemPtr, sizeof(JournalItem));
            JournalItem* journalItem = getSegment<JournalItem, Read>(journalItemPtr);

            Log_Check_Journal ( "\t\top=%d, baseRef=%llx, alt=%llx, attr=%x\n",
                    journalItem->op, journalItem->baseElementId,
                    journalItem->altElementId, journalItem->attributeKeyId );

            if (journalItem->op >= JournalOperation_LastOperation)
            {
                CheckError("Invalid journal operation 0x%x (at %llx)\n", journalItem->op, journalItemPtr);
            }

            if (journalItem->nextJournalItem == NullPtr)
            {
                if (journalItemPtr != getRevisionPage()->journalHead.lastAllocedJournalItem)
                {
                    CheckError(
                            "Corrupted journal list end : last journal entry in revision is %llx, but list ends at %llx\n",
                            getRevisionPage()->journalHead.lastAllocedJournalItem, journalItemPtr);
                }
            }
            journalItemPtr = journalItem->nextJournalItem;
        }
        if (!identifiedFirstJournalItemPtr)
        {
            CheckError("\firstJournalItem=%llx is not part of the journal list !!\n",
                       getRevisionPage()->journalHead.firstJournalItem);
        }

        /*
         * Identify all free segments of all profiles, all size levels
         */
        CheckInfo("Identifying free segments from %x profiles\n", getDocumentAllocationHeader().nbAllocationProfiles);

        for (AllocationProfile profile = 0; profile < getDocumentAllocationHeader().nbAllocationProfiles; profile++)
        {
            FreeSegmentsLevelHeader* fslh = getFreeSegmentsLevelHeader(profile);
            Log_Check_L ( "Profile=%x, fslh at %p\n", profile, fslh );
#if 0
            Log_Check_L ( "\tfreePage=%llx, freeOffset=%llx\n", fslh->freePage, fslh->freeOffset );
            if ( fslh->freePage != NullPage )
            {
                __markSegment ( SegmentType_FreePage, (fslh->freePage + fslh->freeOffset), (PageSize - fslh->freeOffset) );
            }
#endif
            for (__ui32 level = 0; level < fslh->levelNumber; level++)
            {
                Log_Check_L ( "\tlevel=%x, firstFreeHole=%llx\n", level, fslh->firstFreeHole[level] );
                __ui64 nbInChain = 0;
                RelativePagePtr holePagePtr = NullPage;
                for (SegmentPtr holePtr = fslh->firstFreeHole[level]; holePtr;)
                {
                    FreeSegment* freeSegment = getSegment<FreeSegment, Read>(holePtr);
                    Log_Check_L ( "FS prof=%x, level=%x, nbInChain=%llx, holePtr=%llx, holePagePtr=%llx, size=%llx\n",
                            profile, level, nbInChain, holePtr, holePagePtr, freeSegment->size );

                    if (freeSegment->dummy != FreeSegment_dummy)
                    {
                        CheckError(
                                "Invalid freeSegment ! profile=%x, level=%x, nbInChain=%llx, holePtr=%llx, stored in %llx, dummy=%x\n",
                                profile, level, nbInChain, holePtr, holePagePtr, freeSegment->dummy);
                        erroneousPages[holePtr & PagePtr_Mask] = true;
                        if (holePagePtr)
                            erroneousPages[holePagePtr] = true;
                        break;
                    }

                    bool error;
                    __isEmptySegment(SegmentType_FreeSegment, holePtr, freeSegment->size, error);
                    if (error)
                    {
                        CheckError(
                                "Invalid FreeSegment ! profile=%x, level=%x, nbInChain=%llx, holePtr=%llx, stored in %llx, dummy=%x, size=%llx, next=%llx\n",
                                profile, level, nbInChain, holePtr, holePagePtr, freeSegment->dummy, freeSegment->size,
                                freeSegment->next);
                        erroneousPages[holePtr & PagePtr_Mask] = true;
                        if (holePagePtr)
                            erroneousPages[holePagePtr] = true;
                        break;
                    }
                    __markSegment(SegmentType_FreeSegment, holePtr, freeSegment->size);
                    holePagePtr = holePtr & PagePtr_Mask;
                    holePtr = freeSegment->next;
                    nbInChain++;
                }
            }
        }

        /*
         * Identify all elements and their attributes
         */
        DocumentHead* documentHead = getSegment<DocumentHead, Read>(revisionPage->documentHeadPtr,
                                                                    sizeof(DocumentHead));
        ElementPtr ptr = documentHead->rootElementPtr;

        bool isOnMetaElementRoot = false; //<

        __ui64 attributesPerType[AttributeType_Mask + 1];
        memset(attributesPerType, 0, sizeof(attributesPerType));

        __ui64 attributeSizesPerType[AttributeType_Mask + 1];
        memset(attributeSizesPerType, 0, sizeof(attributeSizesPerType));

        __ui64 attributeMaxSizesPerType[AttributeType_Mask + 1];
        memset(attributeMaxSizesPerType, 0, sizeof(attributeMaxSizesPerType));

        Log_Check_L ( "Checking elements tree from root %llx\n", ptr );
        while (true)
        {
            AssertBug(ptr, "Invalid NULL currentElt.\n");
            if (!__isValidPtr(ptr))
            {
                CheckError("Invalid ptr=%llx, out of bounds.\n", ptr);
                break;
            }
            // Log_Check_L ( "At ptr : 0x%llx\n", ptr );
            // housewife ();
            __markSegment(SegmentType_Element, ptr, sizeof(ElementSegment));
            ElementSegment* eltSeg = getSegment<ElementSegment, Read>(ptr);
            if (eltSeg->id == 0)
            {
                CheckError("Invalid elementId=%llx  for element at %llx\n", eltSeg->id, ptr);
            }
            // Log_Check_L ( "Elt ptr=%llx, id=%llx, key=%x\n", ptr, eltSeg->id, eltSeg->keyId );
            if (eltSeg->flags & ElementFlag_HasAttributesAndChildren)
            {
                for (SegmentPtr attrPtr = eltSeg->attributesAndChildren.attrPtr; attrPtr;)
                {
                    if (!__isValidPtr(attrPtr))
                    {
                        CheckError("Invalid attrPtr=%llx, exiting attribute check for currentElt=%llx at %llx\n",
                                   attrPtr, eltSeg->id, ptr);
                        break;
                    }
                    AttributeSegment* attrSeg = getSegment<AttributeSegment, Read>(attrPtr);
                    // __ui64 size = sizeof(AttributeSegment) + attrSeg->size;

                    __ui64 size = sizeof(AttributeSegment) + AttributeSegment_getSize(attrSeg);
                    __markSegment(SegmentType_Attribute, attrPtr, size);
                    attributesPerType[attrSeg->flag & AttributeType_Mask]++;
                    attributeSizesPerType[attrSeg->flag & AttributeType_Mask] += size;

                    if (attributeMaxSizesPerType[attrSeg->flag & AttributeType_Mask] < size)
                    {
                        attributeMaxSizesPerType[attrSeg->flag & AttributeType_Mask] = size;
                    }

                    switch (attrSeg->flag & AttributeType_Mask)
                    {
                        case AttributeType_String:
                        case AttributeType_NamespaceAlias:
                        case AttributeType_XPath:
                        case AttributeType_Integer:
                        case AttributeType_Number:
                        case AttributeType_KeyId:
                            break;
                        case AttributeType_SKMap:
                            Log_Check_L ( "\tSKMap found for element %llx (key %x), attr %llx (key %x)\n",
                                    ptr, eltSeg->keyId, attrPtr, attrSeg->keyId );
                            skMapList.push_back(attrPtr);
                            break;
                        default:
                            CheckError(
                                    "\tInvalid type attribute %x for attr=%llx , exiting attribute check for currentElt=%llx at %llx\n",
                                    attrSeg->flag, attrPtr, eltSeg->id, ptr);
                            attrSeg = NULL;
                    }
                    if (!attrSeg)
                        break;
                    attrPtr = attrSeg->nextPtr;
                }
                if (eltSeg->attributesAndChildren.childPtr)
                {
                    ElementSegment* childSeg = getSegment<ElementSegment, Read>(eltSeg->attributesAndChildren.childPtr);
                    AssertError(childSeg->fatherPtr == ptr,
                                "Invalid father ptr from element=%llx, child=%llx, his father is =%llx\n", ptr,
                                eltSeg->attributesAndChildren.childPtr, childSeg->fatherPtr);
                    AssertError(childSeg->elderPtr == NullPtr,
                                "Invalid elder ptr from element=%llx, child=%llx, his elder is=%llx\n", ptr,
                                eltSeg->attributesAndChildren.childPtr, childSeg->elderPtr);

                    ptr = eltSeg->attributesAndChildren.childPtr;
                    continue;
                }
            }
            else if (eltSeg->flags & ElementFlag_HasTextualContents)
            {
                if (eltSeg->textualContents.size <= eltSeg->textualContents.shortFormatSize)
                {

                }
                else
                {
                    __markSegment(SegmentType_TextualContents, eltSeg->textualContents.contentsPtr,
                                  eltSeg->textualContents.size);
                }
            }
            else
            {
                CheckError("Element has invalid flag mask %x\n", eltSeg->flags);

            }

            while (!eltSeg->youngerPtr)
            {
                ptr = eltSeg->fatherPtr;
                if (ptr == documentHead->rootElementPtr)
                    break;
                if (ptr == documentHead->metaElementPtr)
                    break;

                if (!ptr)
                {
                    eltSeg = NULL;
                    break;
                }
                eltSeg = getSegment<ElementSegment, Read>(ptr);
            }
            if (ptr == documentHead->rootElementPtr)
            {
                ptr = documentHead->metaElementPtr;
                isOnMetaElementRoot = true;
                Log_Check ( "Selecting metaElementPtr at 0x%llx\n", ptr );
                if (!ptr)
                {
                    break;
                }
                continue;
            }
            if (ptr == documentHead->metaElementPtr)
            {
                AssertError(isOnMetaElementRoot, "Not selected meta tree ! ptr=0x%llx\n", ptr);
                break;
            }
            if (!ptr)
                break;
            if (!eltSeg->youngerPtr)
                break;

            if (!__isValidPtr(eltSeg->youngerPtr))
            {
                CheckError("Invalid eltSeg->youngerPtr=%llx\n", eltSeg->youngerPtr);
                break;
            }
            ElementSegment* youngerSeg = getSegment<ElementSegment, Read>(eltSeg->youngerPtr);
            AssertError(youngerSeg->fatherPtr == eltSeg->fatherPtr,
                        "Invalid fathers between me=%llx and younger=%llx : mine is %llx, younger is %llx !\n", ptr,
                        eltSeg->youngerPtr, eltSeg->fatherPtr, youngerSeg->fatherPtr);
            AssertBug(youngerSeg->elderPtr == ptr, "Invalid elder link !\n");
            ptr = eltSeg->youngerPtr;
        }

        /*
         * Dump information on usage per AttributeType
         */
        for (AttributeType type = 0; type < AttributeType_Mask + 1; type++)
        {
            if (attributesPerType[type])
            {
                CheckInfo("\tAttributeType %x : %llu attributes, total %llu bytes, avg %llu bytes per attr, max %llu\n",
                          type, attributesPerType[type], attributeSizesPerType[type],
                          attributeSizesPerType[type] / attributesPerType[type], attributeMaxSizesPerType[type]);
            }
        }

        /*
         * Identify SKMap contents. This is done separately because some SKMaps store ElementPtrs we should check
         */
        if (skMapList.size())
        {
            Log_Check ( "Checking %lu skMaps from skMapList\n", (unsigned long) skMapList.size() );
            for ( std::list<SegmentPtr>::iterator iter = skMapList.begin(); iter != skMapList.end(); iter++ )
            {
                // housewife ();
                SegmentPtr skMapListPtr = *iter;
                SKMapHeader* skMapHeader = getSegment<SKMapHeader,Read> ( skMapListPtr + sizeof(AttributeSegment) );
                bool single = false;
                bool error = false;
                bool valueIsElement = false;
                bool blob = false;
                switch ( skMapHeader->config.skMapType )
                {
                    case SKMapType_ElementMap:
                    single = true; valueIsElement = true; break;
                    case SKMapType_ElementMultiMap:
                    single = false; valueIsElement = true; break;
                    case SKMapType_IntegerMap:
                    single = true; valueIsElement = false; break;
                    case SKMapType_Blob:
                    single = true; valueIsElement = false; blob = true; break;
                    default:
                    CheckError ( "Invalid or not-implemented skMapType=%x\n", skMapHeader->config.skMapType );
                    error = true;
                }
                if ( error ) break;

                __ui64 nbItems = 0;
                __ui64 nbLists = 0;
                __ui64 nbElements = 0;
                __ui64 lastBlobStart = 0;
                __ui64 lastBlobOffset = 0;
                __ui64 blobSize = 0;
                if ( blob )
                {
                    BlobHeader* blobHeader = getSegment<BlobHeader,Read> ( skMapListPtr + sizeof(AttributeSegment) );
                    blobSize = blobHeader->totalSize;
                    CheckInfo ( "Blob (0x%llx) : size 0x%llx (%llu)\n", skMapListPtr, blobSize, blobSize );
                }

                for ( SKMapItemPtr itemPtr = skMapHeader->head; itemPtr; )
                {
                    nbItems++;
                    if ( ! __isValidPtr(itemPtr))
                    {
                        CheckError("Invalid itemPtr %llx\n", itemPtr );
                        break;
                    }
                    SKMapItem* item = getSegment<SKMapItem,Read> ( itemPtr, sizeof(SKMapItem) );
                    __ui64 size = sizeof(SKMapItem) + ( item->level * sizeof(SegmentPtr) );
                    __markSegment ( SegmentType_SKMapItem, itemPtr, size );

                    if ( single )
                    {
                        if ( valueIsElement )
                        {
                            __checkIsValidElementPtr ( (item->value ) );
                        }
                        else if ( blob )
                        {
                            if ( itemPtr == skMapHeader->head )
                            {
                                AssertError ( item->hash == 0, "Invalid blob first hash '0x%llx'\n", item->hash );
                            }
                            else
                            {
                                __ui64 pieceSize = item->hash - lastBlobStart;
                                CheckLog ( "\tBlob Piece [0x%llx:0x%llx] => 0x%llx\n", lastBlobStart, item->hash, lastBlobOffset );
                                if ( item->hash >= blobSize )
                                {
                                    AssertError ( item->value == 0, "Segment has value after for item->hash=0x%llx (size=0x%llx)\n", item->hash, blobSize );
                                    AssertError ( item->next[0] == 0, "Segment has a next value ! item->hash=0x%llx (size=0x%llx)\n",
                                            item->hash, blobSize );
                                    CheckLog ( "\t** Blob end.\n" );
                                }
                                if ( lastBlobOffset )
                                {
                                    __markSegment ( SegmentType_Blob, lastBlobOffset, pieceSize );
                                }
                            }
                            lastBlobStart = item->hash;
                            lastBlobOffset = item->value;
                        }
                    }
                    else
                    {
                        for ( SKMapListPtr listPtr = item->value; listPtr; )
                        {
                            nbLists++;
                            if ( ! __isValidPtr(listPtr) )
                            {
                                CheckError("Invalid listPtr=%llx\n", listPtr );
                                break;
                            }
                            SKMapList* list = getSegment<SKMapList,Read> ( listPtr, sizeof(SKMapList) );
                            __markSegment ( SegmentType_SKMapList, listPtr, sizeof(SKMapList) );
                            for ( __ui32 idx = 0; idx < list->number; idx++ )
                            {
                                nbElements++;
                                if ( valueIsElement )
                                {
                                    if ( ! __isValidPtr ( list->values[idx] ) )
                                    {
                                        CheckError ( "Invalid ptr=%llx for list=%llx, idx=%x\n",
                                                list->values[idx], listPtr, idx );
                                        break;
                                    }
                                    __checkIsValidElementPtr ( ( list->values[idx] ) );
                                }
                                CheckLog ( "SKMMap=%llx, hash=%llx, list=%llx, idx=%x, value=%llx\n",
                                        skMapListPtr, item->hash, listPtr, idx, list->values[idx] );
                            }
                            listPtr = list->nextList;
                        }
                    }
                    itemPtr = item->next[0];
                } // Iterating over items
                CheckInfo ( "SKMMap=%llx (type %x): items=%llu, lists=%llu\n", skMapListPtr, skMapHeader->config.skMapType, nbItems, nbLists );
                if ( nbElements )
                {
                    CheckInfo ( "\tnbElements=%llu, %llu elements per list\n", nbElements, nbElements / nbLists );
                }
            }
        }

        /*
         * Check the viability of the firstFreeInPage offset
         */
        CheckInfo( "Checking firstFreeSegmentOffset(), nextRelativePagePtr=%llx\n", nextRelativePagePtr);
        for (RelativePagePtr relPagePtr = NullPage; relPagePtr < nextRelativePagePtr; relPagePtr += PageSize)
        {
            __ui32 freeOffset = getFirstFreeSegmentOffset(relPagePtr);
            if (freeOffset >= PageSize)
            {
                CheckLog ( "\tPage %llx has no firstFreeSegmentOffset\n", relPagePtr );
                continue;
            }
            SegmentPtr freeSegmentPtr = relPagePtr + (SegmentPtr) freeOffset;

            bool error = false;
            while (freeSegmentPtr)
            {
                if (freeSegmentPtr % chunkSize)
                {
                    CheckError("\tInvalid non-aligned free segment %llx\n", freeSegmentPtr);
                    error = true;
                    break;
                }
                __ui64 idx = freeSegmentPtr / chunkSize;
                if (idx >= nbChunks)
                {
                    CheckError("\tInvalid free segment %llx, idx=%llx (nbChunks=%llx)\n", freeSegmentPtr, idx,
                               nbChunks);
                    error = true;
                    break;
                }
                if (chunks[idx] != 'F')
                {
                    CheckError("\tInvalid free segment %llx, identified as '%c'\n", freeSegmentPtr, chunks[idx]);
                    error = true;
                    break;
                }
                FreeSegment* freeSeg = getSegment<FreeSegment, Read>(freeSegmentPtr);
                if (freeSeg->dummy != FreeSegment_dummy)
                {
                    CheckError("Invalid dummy ! seg=%llx, dummy=%x\n", freeSegmentPtr, freeSeg->dummy);
                    error = true;
                    break;
                }CheckLog ( "\tNext : %x\n", freeSeg->nextInPage );
                if (freeSeg->nextInPage >= PageSize)
                    break;
                if (freeSeg->nextInPage <= freeOffset)
                {
                    CheckError("\tPage=%llx : Invalid cycling or not-sorted nextInPage=%x, previous=%x\n", relPagePtr,
                               freeSeg->nextInPage, freeOffset);
                    error = true;
                    break;
                }
                freeOffset = freeSeg->nextInPage;
                freeSegmentPtr = relPagePtr + (SegmentPtr) freeOffset;
            }
            if (error)
                continue;CheckLog ( "\tPage %llx OK.\n", relPagePtr );
        }

        /*
         * Check that all bytes in the revision have been identified
         */
        if (identifiedSize != nextRelativePagePtr)
        {
            CheckError("Identified size=%llu bytes (0x%llx) on %llu (0x%llx) allocated -> %llu%% found.\n",
                       identifiedSize, identifiedSize, nextRelativePagePtr, nextRelativePagePtr,
                       (100 * identifiedSize) / nextRelativePagePtr);
        }

        /*
         * Dump errnoneous pages
         */
        if (erroneousPages.size())
        {
            CheckError("Found %lu erroneous pages.\n", (unsigned long ) erroneousPages.size());
            for (std::map<RelativePagePtr, bool>::iterator iter = erroneousPages.begin(); iter != erroneousPages.end();
                    iter++)
            {
                __ui64 relPagePtr = iter->first;

                try
                {
                    __ui64 index;
                    AbsolutePageRef<PageInfoPage> pageInfoPageRef = doGetPageInfoPage(relPagePtr, index, false);
                    PageInfo& pageInfo = getPageInfo(pageInfoPageRef, index);
                    AbsolutePagePtr absPagePtr = pageInfo.absolutePagePtr & PagePtr_Mask;
                    PageFlags pageFlags = pageInfo.absolutePagePtr & PageFlags_Mask;
                    CheckError("\tErroneous page %llx, absPagePtr=%llx, pageFlags=%llx\n", relPagePtr, absPagePtr,
                               pageFlags);
#if 0
                    __ui64 pageIndex = absPagePtr >> InPageBits;
                    if ( pageIndex >= stats.pageTableSize )
                    {
                        CheckError ( "\tInvalid absPagePtr=%llx out of bounds !\n", absPagePtr );
                    }
                    else
                    {
                        AllocationStats::PageStats* pStats = &(stats.pageTable[pageIndex]);
                        stats.dumpPageReferences ( pStats );
                    }
#endif                  
                }
                catch (Exception* e)
                {
                    CheckError("\tCould not get pageInfo for relPagePtr=%llx\n", relPagePtr);
                }
#if 0 // def __XEM_PERSISTENCE_CHECK_BUILD_BRANCHPAGETABLE
                AllocationStats::BranchPageTable::iterator rpiter = brTable->find ( relPagePtr );
                if (rpiter == brTable->end() )
                {
                    CheckError ( "Invalid relPagePtr=%llx\n", relPagePtr );
                    continue;
                }
                AllocationStats::RelativePageInfos* rpi = rpiter->second;
                if ( rpi->revInfos.size() == 0 )
                {
                    CheckError ( "Invalid null rpvi for relPagePtr=%llx\n", relPagePtr );
                }
                for ( std::list<AllocationStats::RelativePageRevInfos*>::iterator rpviter = rpi->revInfos.begin();
                        rpviter != rpi->revInfos.end(); rpviter++ )
                {
                    AllocationStats::RelativePageRevInfos* rpvi = *rpviter;
                    CheckError ( "\trelPagePtr=%llx, brId=%llx:%llx, abs=%llx, stolen=%s\n",
                            relPagePtr, _brid(rpvi->brId), rpvi->absPagePtr, rpvi->stolen ? "true" : "false" );
                }
#endif // __XEM_PERSISTENCE_CHECK_BUILD_BRANCHPAGETABLE
                __dumpPageContents(relPagePtr);
            }
        }

        __ui64 unknownChunkStart = 0;
        __ui64 nbUnknownChunks = 0;
        __ui64 nbUnknownSegments = 0;
        __ui64 nbKnownChunks = 0;
        Log_Check ( "Checking unknown chunks...\n" );

        bool isInUnknown = false;
        __ui64 nbUnknownPerLevel[FreeSegmentsLevelHeader::levelNumber];
        memset(nbUnknownPerLevel, 0, sizeof(nbUnknownPerLevel));

        /*
         * Dump all erroneous chunks
         */
        for (__ui64 idx = 0; idx < nbChunks; idx++)
        {
            unsigned char chunkType = chunks[idx];
            if (chunkType != '?')
            {
                nbKnownChunks++;
                if (isInUnknown)
                {
                    CheckError("Unknown area [%08llx,%08llx] size=%08llx\n", unknownChunkStart * chunkSize,
                               idx * chunkSize, (idx - unknownChunkStart) * chunkSize);
                    nbUnknownSegments++;
                    isInUnknown = false;
                    __ui64 size = (idx - unknownChunkStart) * chunkSize;
                    __ui32 level;
                    computeFreeLevel(level, size);
                    nbUnknownPerLevel[level]++;
                    __dumpPageContents(((unknownChunkStart * chunkSize) & PagePtr_Mask));
                }
            }
            else
            {
                nbUnknownChunks++;
                if (!isInUnknown)
                {
                    isInUnknown = true;
                    unknownChunkStart = idx;
                }
            }
        }
        /*
         * Append tail if we are still on an unknown chuhnk
         */
        if (isInUnknown)
        {
            nbUnknownSegments++;
            __dumpPageContents(((unknownChunkStart * chunkSize) & PagePtr_Mask));
        }

        /*
         * Dump unknown chunks sorted by size level
         */
        for (__ui32 level = 0; level < FreeSegmentsLevelHeader::levelNumber; level++)
        {
            if (nbUnknownPerLevel[level] == 0)
                continue;
            CheckError("\tUnknown per level : level=%x, number=%llu (0x%llx)\n", level, nbUnknownPerLevel[level],
                       nbUnknownPerLevel[level]);
        }

        /*
         * Dump statistics : chunck usage per type (head and body)
         */
        for (unsigned char typeHead = 'A'; typeHead <= 'Z'; typeHead++)
        {
            unsigned char type = typeHead - 'A' + 'a';
            if (nbChunksPerType[typeHead])
            {
                CheckInfo("typeHead %c : %llu (0x%llx) chunks\n", typeHead, nbChunksPerType[typeHead],
                          nbChunksPerType[typeHead]);
            }
            if (nbChunksPerType[type])
            {
                CheckInfo("type     %c : %llu (0x%llx) chunks\n", type, nbChunksPerType[type], nbChunksPerType[type]);
            }

        }

        /*
         * Remind that some chunks are not identified
         */
        if (nbUnknownChunks)
        {
            CheckError("Checking unknown chunks : %llu unknown chunks found, in %llu unknown segments.\n",
                       nbUnknownChunks, nbUnknownSegments);
        }

#ifdef __XEM_PERSISTENTDOCUMENT_CHECK_DUMP_ALL_PAGES
        /*
         * Dump all page contents
         */
        for ( RelativePagePtr relPagePtr = NullPage; relPagePtr < nextRelativePagePtr; relPagePtr+= PageSize )
        {
            __dumpPageContents ( relPagePtr );
        }
#endif

        Log_Check ( "Known chunks : %llu of %llu chunks known -> 0x%llx bytes of 0x%llx allocated.\n",
                nbKnownChunks, nbChunks, nbKnownChunks * chunkSize, nbChunks * chunkSize );

        free(chunks);
    }
}
;

