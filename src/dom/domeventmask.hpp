/*
 * domeventmask.cpp
 *
 *  Created on: 9 nov. 2009
 *      Author: francois
 */

#include <Xemeiah/dom/domeventmask.h>
#include <Xemeiah/kern/exception.h>

namespace Xem
{

  __INLINE DomEventMask::DomEventMask ()
  {
    mask = 0;
  }

  __INLINE DomEventMask::DomEventMask ( DomEventType type )
  {
    mask = type;
  }

  __INLINE DomEventMask::DomEventMask ( const DomEventMask& _mask )
  {
    mask = _mask.mask;
  }

  __INLINE DomEventMask::~DomEventMask ()
  {

  }

  __INLINE bool DomEventMask::hasType ( DomEventType type )
  {
    return ((mask & type)==type);
  }

  __INLINE bool DomEventMask::intersects ( DomEventMask other )
  {
    return ( mask & other.mask );
  }

  __INLINE DomEventMask DomEventMask::intersection ( DomEventMask other )
  {
    return DomEventMask(mask & other.mask);
  }

  __INLINE DomEventMask::operator Integer () const
  {
    return mask;
  }

  __INLINE DomEventMask DomEventMask::operator| ( const DomEventMask& other ) const
  {
    return DomEventMask(mask | other.mask);
  }

  static const char* domEventTypeNames[] =
      {
          "CreateElement",            // 0
          "DeleteElement",            // 1
          "CreateAttribute",          // 2
          "BeforeModifyAttribute",    // 3
          "AfterModifyAttribute",     // 4
          "DeleteAttribute",          // 5
          "Unused 6",                 // 6
          "Unused 7",                 // 7
          "Unused 8",                 // 8
          "Unused 9",                 // 9
          "DocumentCommit",           // 10
          "DocumentReopen",           // 11
          "DocumentDrop",             // 12
          "DocumentFork",             // 13
          "DocumentMerge",            // 14
          NULL
      };

  __INLINE void DomEventMask::parse ( const String& maskString )
  {
    mask = 0;

    std::list<String> tokens;
    maskString.tokenize(tokens);

    for ( std::list<String>::iterator iter = tokens.begin() ; iter != tokens.end() ; iter++ )
      {
        int i = 0;
        for ( i = 0 ; domEventTypeNames[i] ; i++ )
          {
            if ( (*iter) == domEventTypeNames[i] )
              {
                mask |= (1 << i);
                break;
              }
          }
        if ( ! domEventTypeNames[i] )
          {
            throwException(Exception,"Invalid DomEventType '%s'", iter->c_str() );
          }
      }
  }

  __INLINE String DomEventMask::toString () const
  {
    String result;
    int i = 0;
    for ( i = 0 ; domEventTypeNames[i] ; i++ )
      {
        if ( mask & (1<<i) )
          {
            if ( result.size() ) result += " ";
            result += domEventTypeNames[i];
          }
      }
    return result;
  }
};
