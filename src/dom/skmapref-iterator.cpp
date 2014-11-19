#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/dom/skmapref.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_SKMapIter Debug

namespace Xem
{

  SKMapRef::iterator::iterator ( SKMapRef& s )
    : skMapRef ( s )
  {
    reset ();

    AssertBug ( skMapRef, "Null SKMapRef ?\n" );
    
    if ( skMapRef.getHeader()->head != NullPtr )
      {
        currentItemPtr = skMapRef.getHeader()->head;
      }
  }
  
  void SKMapRef::iterator::reset ()
  {
    memset ( &trace, 0, sizeof(trace) );
    currentItemPtr = NullPtr;
  }
  
  SKMapRef::iterator::~iterator ( )
  {
  
  }
  
  void SKMapRef::iterator::setValue ( SKMapValue value )
  {
    SKMapItem * currentItem = skMapRef.getItem<Write> ( currentItemPtr );
    skMapRef.alterItem ( currentItem );
    currentItem->value = value;
    skMapRef.protectItem ( currentItem );
  }
 
  bool SKMapRef::iterator::getNextHash ( SKMapHash& nextHash )
  {
    AssertBug ( currentItemPtr, "No current item set !\n" );
    SKMapItem* currentItem = skMapRef.getItem<Read> ( currentItemPtr );
    if ( currentItem->next[0] == NullPtr ) return false;
    Log_SKMapIter ( "next item at '%llx'\n", currentItem->next[0] );
    SKMapItem* nextItem = skMapRef.getItem<Read> ( currentItem->next[0] );
    
    Log_SKMapIter ( "nextItem at %p, ptr=%llx, hash=%llx, value=%llx\n",
      nextItem, currentItem->next[0], nextItem->hash, nextItem->value );
    nextHash = nextItem->hash;
    return true;
  }
  bool SKMapRef::iterator::getNextValue ( SKMapValue& nextValue )
  {
    AssertBug ( currentItemPtr, "No current item set !\n" );
    SKMapItem* currentItem = skMapRef.getItem<Read> ( currentItemPtr );
    if ( currentItem->next[0] == NullPtr )
        return false;
    SKMapItem* nextItem = skMapRef.getItem<Read> ( currentItem->next[0] ); 
    nextValue = nextItem->value;
    return true;
  }
  
  void SKMapRef::iterator::findClosest ( SKMapHash hash )
  {
    /*
     * Find the closest Item, ie the Item with the highest hash less than or equal to the provided hash
     * This function fills the iterator trace : for each level, the preceding item
     */
    Log_SKMapIter ( "************** FIND HASH %llx ********************\n", hash );

#if 1
    if ( skMapRef.getHeader()->head != NullPtr )
      {
        currentItemPtr = skMapRef.getHeader()->head;
      }
#endif

    /*
     * Cleanup trace
     */
    memset ( trace, 0, sizeof(trace) );
    
    /*
     * First, compare with the head
     */
    SKMapItem* head = skMapRef.getHead();

    if ( head )
      {
        Log_SKMapIter ( "Head : %llx\n", head->hash );
        if ( head->hash == hash )
          { 
            currentItemPtr = skMapRef.getHeader()->head;
            return; 
          }
        if ( head->hash > hash )
          {
            currentItemPtr = NullPtr;
            return;
          }
      }
    else
      {
        currentItemPtr = NullPtr;
        return;
      }
    __ui64 iterations = 0;
    currentItemPtr = skMapRef.getHeader()->head;
#if PARANOID
    AssertBug ( currentItemPtr, "No head !\n" );
#endif
    SKMapItem* currentItem = skMapRef.getItem<Read> ( currentItemPtr );
    
    /*
     * Our current level
     */
    __ui32 level = skMapRef.getConfig()->maxLevel - 1;
    while ( true )
      {
        iterations++;
#if PARANOID
        AssertBug ( currentItem, "No item !\n" );
#endif

        Log_SKMapIter ( "SKMap item=0x%llx (%p) level=%u, item->level=%u, hash=%llx, find=%llx\n",
               currentItemPtr, currentItem, level, 
               currentItem->level,
               currentItem->hash, hash );

        AssertBug ( currentItemPtr % 0x20 == 0, "Not aligned currentItemPtr : %llx\n", currentItemPtr );
        AssertBug ( currentItem->level < getConfig()->maxLevel, "Corrupted SKMap ! level too high %x\n", currentItem->level );

        if ( currentItem->hash == hash )
          {
            Log_SKMapIter ( "iterations %llu\n", iterations );
            AssertBug ( level == 0, "Invalid non-zero level : %x (hash=%llx, find=%llx)\n", 
                level, currentItem->hash, hash );
            return;
          }
#if PARANOID
        AssertBug ( currentItem->hash <= hash, "Item has a higher hash !\n" );
        AssertBug ( level <= currentItem->level, "level %u, item->level %u, iterations %llu\n",
                level, currentItem->level, iterations );
#endif
        if ( ! currentItem->next[level] )
          {
            /*
             * Our current item has no next for the given level
             * Jump to the lower level within the same item
             */
            trace[level] = currentItemPtr;
            Log_SKMapIter ( "Jumping to lower level=%u\n", level );
            if ( level == 0 )
              {
                Log_SKMapIter ( "iterations %llu\n", iterations );
                return;
              }
            level--;
            continue;
          }
        AssertBug ( currentItem->next[level], "No next !\n" );
        
        SKMapItemPtr nextItemPtr = currentItem->next[level];
        SKMapItem* nextItem = skMapRef.getItem<Read> ( nextItemPtr );

        if ( nextItem->hash >= hash )
          {
            /*
             * The next item for the given level has a higher hash, so reduce level within the same item
             */
            trace[level] = currentItemPtr;
            if ( level == 0 )
              {
                if ( nextItem->hash == hash )
                  {
                    currentItemPtr = nextItemPtr;
                  }
                Log_SKMapIter ( "iterations %llu\n", iterations );
                return;
              }
            level--;
            continue;
          }
        /*
         * If we are here, we have a next item with a hash less than or equal to the hash we are looking for
         */
        currentItemPtr = nextItemPtr;
        currentItem = nextItem;
      }
    Bug ( "Shall not be here !\n" );
  }
  
  bool SKMapRef::iterator::insert ( SKMapHash hash, SKMapValue value )
  {
    SKMapHeader* header = skMapRef.getHeader();
    SKMapConfig* config = getConfig();
    if ( ! currentItemPtr ) 
      {
        if ( header->head == NullPtr )
          {
            /** 
             * Simple : we don't have a head, create one.
             */
            currentItemPtr = skMapRef.getNewItemPtr ( config->maxLevel - 1, hash, value );
            // Warn !!! WE ARE STEALING headPtr !
            skMapRef.authorizeHeaderWrite ();
            skMapRef.alterData ();
            header->head = currentItemPtr;
            skMapRef.protectData ();
            
            Log_SKMapIter ( "Set Head hash : '%llx\n", hash );
          }
        else
          {
            /**
             * Ok, we must insert before head.
             * Head has always the highest level possible
             * So recycle the head for this new entry,
             * and create a new Item for the head value.
             */
            SKMapItem* head = skMapRef.getHead();
            AssertBug ( head, "Could not fetch head !\n" );
            AssertBug ( head->hash > hash, "Invalid hash ! head->has=0x%llx, hash=0x%llx\n",
                head->hash, hash );

            __ui32 newLevel = 0;
            while ( newLevel < ( config->maxLevel - 1 ) && ( rand() % 256 <= config->probability[newLevel] ) )
              newLevel++;    
            Log_SKMapIter ( "New level '%u'\n", newLevel );

            /**
             * Create an item for head
             */
            SKMapItemPtr currentItemPtr = skMapRef.getNewItemPtr ( newLevel, head->hash, head->value ); 
            SKMapItem* currentItem = skMapRef.getItem<Write> ( currentItemPtr ); 

            skMapRef.alterItem ( currentItem );
            for ( __ui32 level = 0 ; level <= newLevel ; level++ )
              {
                currentItem->next[level] = head->next[level];
              }
            skMapRef.protectItem ( currentItem );

            /**
             * Ok, now update head
             */
            head = skMapRef.getItem<Write> ( skMapRef.getHeader()->head );
            skMapRef.alterItem ( head );
            head->hash = hash;
            head->value = value;
            for ( __ui32 level = 0 ; level <= newLevel ; level++ )
              {
                head->next[level] = currentItemPtr;
              }
            skMapRef.protectItem ( head );
            
            Log_SKMapIter ( "Update map Head : %llx\n", head->hash );
          }
        return true;
      }
    AssertBug ( getHash() < hash, "Current must be before hash !\n" );

    SKMapHash nextHash;
    if ( getNextHash ( nextHash ) )
      {
        AssertBug ( hash < nextHash, "hash must be before next hash !\n" );
      }
#if 0
    Log_SKMapIter ( "Trace :\n" );
    for ( __ui32 level = 0 ; level < config->maxLevel ; level++ )
      {
        Log_SKMapIter ( "\tlevel=%u, ptr=%llx\n", level, trace[level] );
      }
#endif

    __ui32 newLevel = 0;
    while ( newLevel < config->maxLevel - 1 
        && ( rand() % 256 <= config->probability[newLevel] ) )
      newLevel++;    
    Log_SKMapIter ( "New level '%u'\n", newLevel );
    SKMapItemPtr nextItemPtr = skMapRef.getNewItemPtr ( newLevel, hash, value );
    SKMapItem* nextItem = skMapRef.getItem<Write> ( nextItemPtr );

    for ( __ui32 level = 0 ; level <= newLevel ; level++ )
      {
        if ( trace[level] == 0 )
          {
            Bug ( "Zero trace at level=%d\n", level );
          }
        SKMapItem* itemBefore = skMapRef.getItem<Write> ( trace[level] );
        AssertBug ( itemBefore, "No itemBefore !\n" );
        AssertBug ( itemBefore->hash < hash, "Bug item before has a greater hash !\n" );
        SegmentPtr itemAfterPtr = itemBefore->next[level];
        
        skMapRef.alterItem ( itemBefore );
        itemBefore->next[level] = nextItemPtr;
        skMapRef.protectItem ( itemBefore );

        skMapRef.alterItem ( nextItem );
        nextItem->next[level] = itemAfterPtr;
        skMapRef.protectItem ( nextItem );
      }
    currentItemPtr = nextItemPtr;
    return true;
  }
  
  SKMapRef::iterator& SKMapRef::iterator::operator++ ( int u )
  {
    /**
     * \todo update trace when incrementing SKMapRef::iterator !!!
     */
    SKMapItem* currentItem = skMapRef.getItem<Read> ( currentItemPtr );
    currentItemPtr = currentItem->next[0];
    return *this;
  }
  
  bool SKMapRef::iterator::erase ( )
  {
    if ( ! currentItemPtr )
      {
        NotImplemented ( "May erase head ?\n" );        
      }

    SKMapItem * currentItem = skMapRef.getItem<Read> ( currentItemPtr );
    Log_SKMapIter ( "Erase : currentItemPtr=%llx, level=%x, hash=%llx, value=%llx\n",
      currentItemPtr, currentItem->level, currentItem->hash, currentItem->value );

#ifdef __XEM_DOM_SKMAPREF_DELETION_PARANOID
    for ( __ui32 i = 0 ; i <= currentItem->level ; i++ )
      {
        Log_SKMapIter ( "\tItem next : i=%x : %llx\n", i, currentItem->next[i] );
        if ( currentItem->next[i] )
          {
            SKMapItem * item = skMapRef.getItem<Read> ( currentItem->next[i] );
            Log_SKMapIter ( "\t\tnext level=%x, hash=%llx, value=%llx\n",
              item->level, item->hash, item->value );
            AssertBug ( currentItem->hash < item->hash, "Corrupted SKMap\n" );
          }
      } 
    Log_SKMapIter ( "Trace : \n" );
    for ( __ui32 i = 0 ; i < SKMap_maxLevel ; i++ )
      {
        if ( trace[i] )
          {
            SKMapItem * item = skMapRef.getItem<Read> ( trace[i] );
            Log_SKMapIter ( "\tTrace i=%x : %llx, next level=%x, hash=%llx, value=%llx\n",
              i, trace[i], item->level, item->hash, item->value );
            AssertBug ( item->hash < currentItem->hash, "Corrupted SKMap\n" );
            
            for ( __ui32 k = i ; k <= currentItem->level && k <= item->level ; k++ )
              {
                Log_SKMapIter ( "\t\tnext[%x] -> %llx\n", k, item->next[k] );
                if ( item->next[k] )
                  {
                    SKMapItem* nextItem = skMapRef.getItem<Read> ( item->next[k] );
                    Log_SKMapIter ( "\t\t\tlevel=%x, hash=%llx, value=%llx\n",
                      nextItem->level, nextItem->hash, nextItem->value );
                  }
                if ( item->next[k] != currentItemPtr )
                  {
                    Warn ( "Corrupted SKMap\n" );
                  }
              }
          }
      }
#endif // 1

    if ( skMapRef.getHeader()->head == currentItemPtr )
      {
        /*
         * Here we are, deleting our own head
         * We have to replace our head with the next item, and delete the item
         */
        SKMapItem* head = skMapRef.getHead ();
        
        if ( head->next[0] )
          {
            currentItemPtr = head->next[0];
            SKMapItem* currentItem = skMapRef.getItem<Read> ( currentItemPtr );
            
            skMapRef.authorizeHeadWrite ();

            skMapRef.alterItem ( head );            
            for ( __ui32 level = 0 ; level <= currentItem->level ; level++ )
              {
                AssertBug ( head->next[level] == currentItemPtr, "Diverging head->next for level=%x : head->next=%llx, me=%llx\n",
                  level, head->next[level], currentItemPtr );
                head->next[level] = currentItem->next[level];
              }
            head->hash = currentItem->hash;
            head->value = currentItem->value;
            skMapRef.protectItem ( head );
          }
        else
          {
            Log_SKMapIter ( "Completely deleting the skmapref HEAD !.\n" );
            skMapRef.authorizeHeaderWrite ();
            SKMapHeader* header = skMapRef.getHeader ();
            skMapRef.alterData ();
            header->head = NullPtr;
            skMapRef.protectData ();
          }
      }    
    else
      {    
        for ( __ui32 level = 0 ; level <= currentItem->level ; level++ )
          {
            if ( ! trace[level] )
              {
                Bug ( "Invalid null trace for level=%x !!!\n", level );
              }
            SKMapItem * item = skMapRef.getItem<Write> ( trace[level] );
            AssertBug ( item->level >= level, "Wrong item=%llx trace level=%x for level=%x\n", 
              trace[level], item->level, level );
            AssertBug ( item->next[level] == currentItemPtr, "Wrong next assign from %llx for level=%x, next=%llx, expected %llx\n",
              trace[level], level, item->next[level], currentItemPtr );
            

            skMapRef.alterItem ( item );
            item->next[level] = currentItem->next[level];
            skMapRef.protectItem ( item );
          }
      }      
      
    currentItem = skMapRef.getItem<Write> ( currentItemPtr );
    __ui64 itemSize = skMapRef.getItemSize ( currentItem );

    Log_SKMapIter ( "SKMap : finally : destroying item=%llx, level=%x, size=%llx\n", currentItemPtr, currentItem->level, itemSize );
#if 1 // memset the item
    /*
     * Note : do not use the skMapRef.alterItem(), because we destroy item level information
     */
    skMapRef.getDocumentAllocator().alter ( currentItem, itemSize );
    memset ( currentItem, 0, itemSize );
    skMapRef.getDocumentAllocator().protect ( currentItem, itemSize );
#endif

    skMapRef.getDocumentAllocator().freeSegment ( currentItemPtr, itemSize );
    currentItemPtr = NullPtr;
    return true;
  }
  
};
