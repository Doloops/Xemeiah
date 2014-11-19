#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/dom/skmapref.h>

#define Log_SKMMapHPP Debug

namespace Xem
{

  __INLINE void SKMultiMapRef::multi_iterator::findHash ( SKMapHash hash )
  {
    currentItemPtr = NullPtr;
    currentListPtr = NullPtr;
    currentIndex = 0;
    myHash = hash;
    findList ( );
    if ( *this && getHash() != myHash )
      {
        currentListPtr = NullPtr;
        currentIndex = 0;
      }
  }
  
  __INLINE SKMultiMapRef::multi_iterator::multi_iterator ( SKMultiMapRef& skMultiMap, SKMapHash hash )
  : iterator ( skMultiMap )
  {
    findHash ( hash );
  }

  __INLINE void SKMultiMapRef::multi_iterator::reset ()
  {
    currentListPtr = NullPtr;
    currentIndex = 0;
    myHash = 0;
    iterator::reset ();
  }

  
  __INLINE SKMultiMapRef::multi_iterator::~multi_iterator ()
  {}

  __INLINE void SKMultiMapRef::multi_iterator::findList ( )
  { 
    currentListPtr = NullPtr;
    currentIndex = 0;
    iterator::findClosest ( myHash );
    if ( ! isPositionned() )
        return;
    currentListPtr = (SKMapListPtr) ( iterator::getValue() );
  }
  
  __INLINE bool SKMultiMapRef::multi_iterator::isPositionnedInList ()
  { 
    return ( currentListPtr != NullPtr );
  }
  
  __INLINE SKMapValue SKMultiMapRef::multi_iterator::getValue ()
  { 
#if PARANOID
    AssertBug ( isPositionnedInList(), "Not positionned in list !\n" );
#endif
    SKMapList* currentList = getCurrentList<Read> ();
    Log_SKMMapHPP ( "getValue() : currentList=%p, currentIndex=0x%x, number=%x, maxNumber=%x, value=%llx\n",
        currentList, currentIndex, currentList->number, currentList->maxNumber, currentList->values[currentIndex] );
    return currentList->values[currentIndex];
  }
  

  template<PageCredentials how>
  __INLINE SKMapList* SKMultiMapRef::multi_iterator::getCurrentList ()
  { 
    AssertBug ( currentListPtr != NullPtr, "No list ptr defined !\n" );
    return skMapRef.getDocumentAllocator().getSegment<SKMapList,how> ( currentListPtr );
  }
  

};

