#include <Xemeiah/kern/format/journal.h>

#include <Xemeiah/kern/journaleddocument.h>
#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/dom/attributeref.h>
#include <Xemeiah/dom/blobref.h>
#include <Xemeiah/dom/documentmeta.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_Journal Debug
#define Log_Journal_Apply Debug
#define Warn_Journal_Apply Warn

namespace Xem
{
  const char* JournalOperationLabel [] =
  {
    "NoOp",
    "InsertChild",
    "AppendChild",
    "InsertBefore",
    "InsertAfter",
    "UpdateTextNode",
    "Remove",
    "UpdateAttribute",
    "DeleteAttribute",
    "UpdateBlob",
    "BuildMeta",
    "Invalid : LastOperation"
  };

  JournaledDocument::JournaledDocument ( Store& store, DocumentAllocator& allocator )
  : Document(store, allocator)
  {

  }

  void JournaledDocument::appendJournalItem ( SegmentPtr journalItemPtr, JournalItem* journalItem )
  {
    AssertBug ( isWritable(), "Document not writable !\n" );
    AssertBug ( isLockedWrite(), "Document not locked write !\n" );

    if ( getJournalHead().lastAllocedJournalItem )
      {
        JournalItem* previousJournalItem = getDocumentAllocator().getSegment<JournalItem,Write>
          ( getJournalHead().lastAllocedJournalItem );
        getDocumentAllocator().alter ( previousJournalItem );
        previousJournalItem->nextJournalItem = journalItemPtr;
        getDocumentAllocator().protect ( previousJournalItem );
      }
    
    alterJournalHead ();

    if ( ! getJournalHead().firstAllocedJournalItem )
      getJournalHead().firstAllocedJournalItem = journalItemPtr;
    
    if ( ! getJournalHead().firstJournalItem )
      getJournalHead().firstJournalItem = journalItemPtr;
      
    getJournalHead().lastAllocedJournalItem = journalItemPtr;
    
    protectJournalHead ();
  
  }
  
  void JournaledDocument::appendJournal ( ElementRef& baseRef, JournalOperation op, ElementRef& altRef, KeyId attributeKeyId )
  {
    AssertBug ( isWritable(), "JournaledDocument is not writable !\n" );
    AssertBug ( isLockedWrite(), "JournaledDocument is not locked write !\n" );

    Log_Journal ( "[JOURNAL] [brid:%llx:%llx], op=%d:'%s', baseRef=%llx:%s, alt=%llx:%s, attr=%x:%s\n",
      _brid(getBranchRevId()),
      op, JournalOperationLabel[op], baseRef.getElementId(), baseRef.getKey().c_str(),
      altRef ? altRef.getElementId() : 0,
      altRef ? altRef.getKey().c_str() : "(none)", 
      attributeKeyId,
      attributeKeyId ? getKeyCache().dumpKey(attributeKeyId).c_str() : "(none)" );

    if ( ! isJournalEnabled() )
    {
        Log_Journal ( "[JOURNAL] Journal disabled\n");
        return;
    }


    SegmentPtr journalItemPtr = getDocumentAllocator().getFreeSegmentPtr ( sizeof(JournalItem), 0 );
    
    JournalItem* journalItem = getDocumentAllocator().getSegment<JournalItem,Write> ( journalItemPtr );
    
    getDocumentAllocator().alter ( journalItem );
    
    journalItem->nextJournalItem = NullPtr;
    journalItem->op = op;
    journalItem->baseElementId = baseRef.getElementId ();
    journalItem->altElementId = altRef ? altRef.getElementId () : 0;
    journalItem->attributeKeyId = attributeKeyId;
    
    getDocumentAllocator().protect ( journalItem );
    
    appendJournalItem ( journalItemPtr, journalItem );
  }
  
  void JournaledDocument::applyJournal ( XProcessor& xproc, JournaledDocument& source )
  {
    SegmentPtr journalItemPtr = source.getJournalHead().firstJournalItem;

    while ( journalItemPtr )
      {
        JournalItem* journalItem = source.getDocumentAllocator().getSegment<JournalItem,Read> ( journalItemPtr );
        journalItemPtr = journalItem->nextJournalItem;

        __JournalOperation op = (__JournalOperation) journalItem->op;

        Log_Journal ( "[JOURNAL-APPLY] op=%d:'%s', baseRef=%llx, alt=%llx, attr=%x\n",
          op, JournalOperationLabel[op], journalItem->baseElementId,
          journalItem->altElementId, journalItem->attributeKeyId );
        
        ElementRef baseElement ( *this );
        try
          {
            baseElement = getElementById ( journalItem->baseElementId );
          }
        catch ( Exception * e )
          {
            detailException ( e, "Could not apply journal op=%d:'%s', because baseElement %llx does not exist in target Document.\n",
                op, JournalOperationLabel[op], journalItem->baseElementId );
            Warn ( "Caught exception : %s\n", e->getMessage().c_str() );
            continue;
            throw ( e );
          }
        ElementRef sourceAltElement ( source );
        bool recursive = false;
        
        if ( journalItem->altElementId && op != JournalOperation_Remove )
          {
            try
              {
                sourceAltElement = source.getElementById ( journalItem->altElementId );
              }
            catch ( Exception* e )
              {
                if ( 1 ) // Be more relax about target elements we can not find, they may be deleted
                  {
                    delete ( e );
                    continue;                  
                  }
                detailException ( e, "Could not apply journal op=%d:'%s', because altElement %llx does not exist in source Document.\n",
                    op, JournalOperationLabel[op], journalItem->altElementId );
                throw ( e );
              }
            if ( ! sourceAltElement )
              {
                Warn_Journal_Apply ( "[JOURNAL-APPLY] Could not get source alt element, aborting this item.\n" );
                continue;
              }
          }
        
        switch ( op )
        {
        case JournalOperation_InsertChild:
        {
          ElementRef newChild = createElement ( baseElement, sourceAltElement.getKeyId(), journalItem->altElementId );
          baseElement.insertChild ( newChild );
          if ( recursive )
            duplicateElement ( xproc, newChild, sourceAltElement );
          Log_Journal_Apply ( "[JOURNAL] Appended '%s'\n", sourceAltElement.generateVersatileXPath().c_str() );
          newChild.eventElement ( xproc, DomEventType_CreateElement );
          break;
        }
        case JournalOperation_AppendChild:
        {
          ElementRef newChild = createElement ( baseElement, sourceAltElement.getKeyId(), journalItem->altElementId );
          baseElement.appendLastChild ( newChild );
          if ( recursive )
            duplicateElement ( xproc, newChild, sourceAltElement );
          Log_Journal_Apply ( "[JOURNAL] Appended '%s'\n", sourceAltElement.generateVersatileXPath().c_str() );
          newChild.eventElement ( xproc, DomEventType_CreateElement );
          break;
        }
        case JournalOperation_InsertBefore:
        {
          ElementRef newChild = createElement ( baseElement, sourceAltElement.getKeyId(), journalItem->altElementId );
          baseElement.insertBefore ( newChild );
          if ( recursive )
            duplicateElement ( xproc, newChild, sourceAltElement );
          newChild.eventElement ( xproc, DomEventType_CreateElement );
          break;
        }
        case JournalOperation_InsertAfter:
        {
          ElementRef newChild = createElement ( baseElement, sourceAltElement.getKeyId(), journalItem->altElementId );
          baseElement.insertAfter ( newChild );
          if ( recursive )
            duplicateElement ( xproc, newChild, sourceAltElement );
          newChild.eventElement ( xproc, DomEventType_CreateElement );
          break;
        }
        case JournalOperation_Remove:
        {
          baseElement.deleteElement ( xproc );
          break;
        }
        case JournalOperation_UpdateTextNode:
        {
          baseElement.setText ( sourceAltElement.getText() );
          break;
        }
        case JournalOperation_UpdateAttribute:
        {
          Log_Journal_Apply ( "[JOURNAL-APPLY] Journal op=%x, attr=%x, next=0x%llx, base=0x%llx, alt=0x%llx\n",
              journalItem->op, journalItem->attributeKeyId, journalItem->nextJournalItem, journalItem->baseElementId, 
              journalItem->altElementId );
          if ( ! sourceAltElement )
            {
              Warn ( "[JOURNAL-APPLY] Could not apply Journal op=%x, attr=%x, next=0x%llx, base=0x%llx, alt=0x%llx\n",
                journalItem->op, journalItem->attributeKeyId, journalItem->nextJournalItem, journalItem->baseElementId,
                journalItem->altElementId );
              continue;
              throwException ( Exception, "No sourceAltElement for UpdateAttribute !!!\n" );
            }
          AttributeRef attrRef = sourceAltElement.findAttr ( journalItem->attributeKeyId, AttributeType_String );
          if ( attrRef )
            {
              baseElement.addAttr ( xproc, journalItem->attributeKeyId, attrRef.toString() );
            }
          else
            {
              /*
               * Try to find the attribute in source element, based only on its KeyId
               * It is asserted that the source is String, and alternate AttributeTypes are only derivative forms.
               */
              Log_Journal_Apply ( "[JOURNAL-APPLY] Attribute not found ! op=%x, attr=%x, next=0x%llx, base=0x%llx, alt=0x%llx\n",
                  journalItem->op, journalItem->attributeKeyId, journalItem->nextJournalItem, journalItem->baseElementId, 
                  journalItem->altElementId );
              AttributeRef foundAttr = AttributeRef(sourceAltElement.getDocument());
              for ( AttributeRef sourceAttr = sourceAltElement.getFirstAttr() ; sourceAttr ; sourceAttr = sourceAttr.getNext() )
                {
                  if ( sourceAttr.getKeyId() == journalItem->attributeKeyId )
                    {
                      Log_Journal_Apply ( "[JOURNAL-APPLY] Source has attr type=%x\n",
                          sourceAttr.getAttributeType() );
                      if ( foundAttr )
                        {
                          throwException ( Exception, "Duplicate multiple-type attribute %s (type=%x, exists=%x)!\n",
                              sourceAttr.generateVersatileXPath().c_str(),
                              sourceAttr.getAttributeType(), foundAttr.getAttributeType() );
                        }
                      foundAttr = sourceAttr;
                    }
                }
              if ( foundAttr )
                {
                  if ( foundAttr.getAttributeType() == AttributeType_XPath )
                    {
                      Log_Journal_Apply ( "[JOURNAL-APPLY] Update XPath attr %s (%s)\n",
                          foundAttr.getKey().c_str(),
                          getKeyCache().dumpKey(foundAttr.getKeyId()).c_str());
                    }
                  baseElement.copyAttribute(foundAttr);
                }
              else
                {
                  throwException ( Exception, "[JOURNAL-APPLY] Attribute not found ! op=%x, attr=%x (%s), next=0x%llx, base=0x%llx, alt=0x%llx, base=%s\n",
                    journalItem->op, journalItem->attributeKeyId,
                    getKeyCache().dumpKey(journalItem->attributeKeyId).c_str(),
                    journalItem->nextJournalItem, journalItem->baseElementId,
                    journalItem->altElementId, sourceAltElement.generateVersatileXPath().c_str() );
                }
            }
          break;
        }
        case JournalOperation_UpdateBlob:
        {
          KeyId blobNameId = journalItem->attributeKeyId;
          BlobRef sourceBlob = sourceAltElement.findAttr ( blobNameId, AttributeType_SKMap );
          BlobRef targetBlob = baseElement.findAttr ( blobNameId, AttributeType_SKMap );
          if ( ! targetBlob )
            {
              targetBlob = baseElement.addBlob ( blobNameId );
            }
          targetBlob.updateBlobFrom ( sourceBlob );          
          break;
        }
        case JournalOperation_BuildMeta:
        {
          Log_Journal_Apply ( "Updating Meta EventMap ...\n" );
          getDocumentMeta().getDomEvents().buildEventMap();
          Log_Journal_Apply ( "Updating Meta EventMap ... OK\n" );
          break;
        }
#if 0
        case JournalOperation_AddMeta:
        {
          DomEvent newEvent = getDocumentMeta().getDomEvents().createDomEvent();
          Log_Journal_Apply ( "Copy DomEvent contents : (%s)\n",  baseElement.generateVersatileXPath().c_str() );
          newEvent.copyContents(xproc, baseElement, false);
          Log_Journal_Apply ( "Copy DomEvent contents : done\n" );
          getDocumentMeta().getDomEvents().buildEventMap();
          Bug ( "." );
          Warn_Journal_Apply ( "[JOURNAL-APPLY] Stupid JournalOperation_AddMeta !\n" );
          break;
        }
#endif
        case JournalOperation_DeleteAttribute:
        case JournalOperation_NoOp:
        case JournalOperation_LastOperation:
          Warn_Journal_Apply ( "[JOURNAL-APPLY] Invalid/NotImplemented no-op journal item %d:%s !\n", op, JournalOperationLabel[op] );
          continue;
        }

        SegmentPtr copiedJournalItemPtr = getDocumentAllocator().getFreeSegmentPtr ( sizeof(JournalItem), 0 );
        JournalItem* copiedJournalItem = getDocumentAllocator().getSegment<JournalItem,Write> ( copiedJournalItemPtr );
        getDocumentAllocator().alter ( copiedJournalItem );
        memcpy ( copiedJournalItem, journalItem, sizeof(JournalItem) );
        copiedJournalItem->nextJournalItem = NullPtr;
        getDocumentAllocator().protect ( copiedJournalItem );
        
        appendJournalItem ( copiedJournalItemPtr, copiedJournalItem );
        Log_Journal_Apply ( "[JOURNAL-APPLY] : Item applied ok.\n" );
      }
  }
  
  void JournaledDocument::duplicateElement ( XProcessor& xproc, ElementRef& target, ElementRef& source )
  {
    Warn ( "duplicateElement : DEPRECATED !\n" );
#if 0
    if ( ! source.isRegularElement() )
    {
      target.setText ( source.getText() );
      return;
    }
    for ( AttributeRef attr = source.getFirstAttr() ; attr ; attr = attr.getNext() )
      {
        AttributeRef targetAttr = target.addAttr ( attr.getKeyId(), attr.toString() );
        target.eventAttribute ( xproc, DomEventType_CreateAttribute, targetAttr );
      }
    for ( ElementRef sourceChild = source.getChild() ; sourceChild ; sourceChild = sourceChild.getYounger() )
      {
        ElementRef targetChild ( target.getDocument() );
        if ( sourceChild.isRegularElement() )
          targetChild = createElement ( target, sourceChild.getKeyId(), sourceChild.getElementId() );
        else
          targetChild = createTextualNode ( target, sourceChild.getKeyId(), sourceChild.getElementId() );

        target.appendLastChild ( targetChild );
        duplicateElement ( xproc, targetChild, sourceChild );
        targetChild.eventElement ( xproc, DomEventType_CreateElement );
      }
#endif
  }  

#if 0 // DEPRECATED
  void JournaledDocument::dumpJournal ( EventHandlerDom& evd )
  {
#define __formatAttr(__ns,__name,__fmt,...) \
{ \
  char __buff[512]; \
  sprintf ( __buff, __fmt, __VA_ARGS__ ); \
  evd.eventAttr ( __ns, __name, __buff ); \
}    
    SegmentPtr jItemPtr = getJournalHead().firstJournalItem;
    
    while ( jItemPtr )
      {
        JournalItem* jItem = getDocumentAllocator().getSegment<JournalItem,Read> ( jItemPtr );
        jItemPtr = jItem->nextJournalItem;
      
        evd.eventElement ( "xem-pers", "item" );
        
        __formatAttr ( "xem-pers", "opId", "%u", jItem->op );
        __formatAttr ( "xem-pers", "op", "%s", JournalOperationLabel[jItem->op] );
        __formatAttr ( "xem-pers", "baseElementId", "0x%llx", jItem->baseElementId );
        
        if ( jItem->altElementId )
          __formatAttr ( "xem-pers", "altElementId", "0x%llx", jItem->altElementId );
          
        if ( jItem->attributeKeyId )
          __formatAttr ( "xem-pers", "attributeKeyId", "%x", jItem->attributeKeyId );
        
        evd.eventAttrEnd ();
        evd.eventElementEnd ( "xem-pers", "item" );
      }
#undef __formatAttr  
  }
#endif
};

