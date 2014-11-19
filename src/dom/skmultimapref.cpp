#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/dom/skmapref.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_SKMMap Debug

namespace Xem
{
  bool
  SKMultiMapRef::multi_iterator::insert(SKMapValue value)
  {
    Log_SKMMap ( "insert : myHash=%llx, value=%llx\n", myHash, value );
    if (!isPositionned() || iterator::getHash() != myHash)
      {
        currentListPtr = getDocumentAllocator().getFreeSegmentPtr
            (sizeof(SKMapList), getConfig()->listAllocProfile);
        SKMapList* currentList = getCurrentList<Write> ();
        Log_SKMMap ( "New list for hash=%llx, value=%llx, currentList=0x%llx (%p)\n",
            myHash, value, currentListPtr, currentList );
        getDocumentAllocator().alter(currentList);
        memset(currentList, 0, sizeof(SKMapList));
        currentList->values[0] = value;
        currentList->number = 1;
        getDocumentAllocator().protect(currentList);
        iterator::insert(myHash, currentListPtr);
        return true;
      }
    AssertBug ( currentListPtr, "No current list defined !\n" );
    AssertBug ( iterator::getHash() == myHash, "Invalid hash !\n" );

    SKMapList* currentList = getCurrentList<Read> ();
    while (currentList->nextList != NullPtr)
      {
        currentListPtr = currentList->nextList;
        currentList = getCurrentList<Read> ();
      }
    AssertBug ( currentList->number <= currentList->maxNumber,
        "List number out of range : list=%p, number=0x%x, maxNumber=0x%x\n",
        currentList, currentList->number, currentList->maxNumber );
    AssertBug ( currentList->number > 0,
        "List number out of range : list=%p, number=0x%x, maxNumber=0x%x\n",
        currentList, currentList->number, currentList->maxNumber ); Log_SKMMap ( "Append at currentList=%p, number=0x%x\n",
        currentList, currentList->number );
    /**
     * Now promote the list Read-Write
     */
    currentList = getCurrentList<Write> ();
    if (currentList->number == currentList->maxNumber)
      {
        SKMapListPtr newListPtr = getDocumentAllocator().getFreeSegmentPtr
            ( sizeof(SKMapList), getConfig()->listAllocProfile);
        SKMapList* newList = getDocumentAllocator().getSegment<SKMapList, Write> (newListPtr);

        Log_SKMMap ( "New list at 0x%llx (%p) after 0x%llx (%p)\n",
            newListPtr, newList, currentListPtr, currentList );
        getDocumentAllocator().alter(newList);
        memset(newList, 0, sizeof(SKMapList));
        newList->values[0] = value;
        newList->number = 1;
        getDocumentAllocator().protect(newList);
        getDocumentAllocator().alter(currentList);
        currentList->nextList = newListPtr;
        getDocumentAllocator().protect(currentList);
        return true;
      }
    getDocumentAllocator().alter(currentList);
    currentList->values[currentList->number] = value;
    currentList->number++;
    getDocumentAllocator().protect(currentList);

    Log_SKMMap ( "insert() : currentList=%p, number=%x, maxNumber=%x, value=%llx\n",
        currentList, currentList->number, currentList->maxNumber, currentList->values[currentList->number - 1] );

    return true;
  }

  SKMultiMapRef::multi_iterator&
  SKMultiMapRef::multi_iterator::operator++(int u)
  {
    AssertBug ( currentListPtr, "No currentList set !\n" );
    SKMapList* currentList = getCurrentList<Read> ();
    AssertBug ( currentIndex < currentList->maxNumber, "Invalid index !\n" );
    AssertBug ( currentList->number <= currentList->maxNumber, "Number out of range : list=%p, number=%x, maxNumber=%x\n",
        currentList, currentList->number, currentList->maxNumber ); Log_SKMMap ( "op : currentIndex=0x%x, currentList=%p, currentList->number=0x%x, currentList->maxNumber=0x%x\n",
        currentIndex, currentList, currentList->number, currentList->maxNumber );
    currentIndex++;
    if (currentIndex >= currentList->number)
      {
        currentListPtr = currentList->nextList;
        currentIndex = 0;
      }
    return *this;
  }

  bool
  SKMultiMapRef::multi_iterator::erase(SKMapValue value)
  {
    AssertBug ( currentListPtr != NullPtr, "No list ptr defined !\n" );

    SegmentPtr lastListPtr = NullPtr;

    while (currentListPtr)
      {
        SKMapList* currentList = getCurrentList<Read> ();

        for (__ui32 idx = 0; idx < currentList->number; idx++)
          {
            if (currentList->values[idx] == value)
              {
                currentList = getCurrentList<Write> ();

                if (currentList->number > 1)
                  {
                    /*
                     * We only have to split left all values
                     */
                    getDocumentAllocator().alter(currentList);
                    for (; idx < currentList->number - 1; idx++)
                      currentList->values[idx] = currentList->values[idx + 1];
                    currentList->values[currentList->number - 1] = 0xdeadbeef; // Clear all
                    currentList->number--;
                    getDocumentAllocator().protect(currentList);
                  }
                else
                  {
                    /**
                     * Check that we have a predecessor
                     */
                    if (lastListPtr)
                      {
                        /*
                         * Update the previous pointer to the next one, and destroy the list
                         */
                        SKMapList* lastList = getDocumentAllocator().getSegment<SKMapList, Write> (lastListPtr);
                        getDocumentAllocator().alter(lastList);
                        lastList->nextList = currentList->nextList;
                        getDocumentAllocator().protect(lastList);

                        getDocumentAllocator().freeSegment(currentListPtr,sizeof(SKMapList));
                        currentListPtr = currentList->nextList;
                        currentIndex = 0;
                        return true;
                      }
                    else if (currentList->nextList)
                      {
                        /*
                         * We dont have any list before, but we have after. Make it the head of the list in SKMap.
                         */
                        iterator::setValue(currentList->nextList);
                        getDocumentAllocator().freeSegment(currentListPtr,sizeof(SKMapList));
                        currentListPtr = currentList->nextList;
                        currentIndex = 0;
                      }
                    else
                      {
                        /*
                         * We dont have any list before nor after, so we just have to dump the iterator
                         */
                        getDocumentAllocator().freeSegment(currentListPtr,sizeof(SKMapList));
                        iterator::erase();

                        currentListPtr = NullPtr;
                        currentIndex = 0;
                      }
                  }
                return true;
              }

          }

        lastListPtr = currentListPtr;
        currentListPtr = currentList->nextList;
        currentList = getCurrentList<Read> ();
      }

    Warn ( "Could not remove value ref %llx\n", value );
    return false;
  }

  bool
  SKMultiMapRef::multi_iterator::erase()
  {
    NotImplemented ( "SKMultiMapRef::multi_iterator::erase ().\n" );
    return false;
  }

  /**
   * Delete item list
   */
  void SKMultiMapRef::deleteSKMapList ( SegmentPtr listPtr )
  {
    while ( listPtr )
      {
        SKMapList* list = getDocumentAllocator().getSegment<SKMapList,Read> ( listPtr );
        SegmentPtr nextListPtr = list->nextList;
        getDocumentAllocator().freeSegment(listPtr,sizeof(SKMapList));
        listPtr = nextListPtr;
      }
  }

  /**
   * deleteAttribute()
   */
  void SKMultiMapRef::deleteAttribute()
  {
    SegmentPtr itemPtr = getHeader()->head;
    while ( itemPtr )
      {
        SKMapItem* item = getDocumentAllocator().getSegment<SKMapItem,Read> ( itemPtr, getItemSize((__ui32)0) );
        SegmentPtr nextItemPtr = item->next[0];

        SegmentPtr listPtr = item->value;
        deleteSKMapList ( listPtr );
        getDocumentAllocator().freeSegment ( itemPtr, getItemSize(item) );

        itemPtr = nextItemPtr;
      }
    deleteAttributeSegment ();
  }

}
;

