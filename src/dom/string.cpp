#include <Xemeiah/dom/string.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_StringCompare Debug // String comparator operations


// #define __XEM_STRING_COMPARATOR_SKIP_RETURN_CHARACTERS // Option : while sorting strings, skip return characters.
#define __XEM_STRING_COMPARATOR_TRIM_SPACES // Option : trim string by the left

namespace Xem
{
#ifdef __XEM_STRING_STATS
  StringStats_Nb String_numberOfMalloc = 0;
  StringStats_Nb String_totalMalloc = 0;
  StringStats_Nb String_numberOfStrdup = 0;
  StringStats_Nb String_numberOfFree = 0;
  StringStats_Nb String_numberOfRealloc = 0;
  
  void String_ShowStringStats()
  {
    Info ( "String : %lu mallocs (total %lu), %lu strdups, %lu free, %lu realloc\n",
      String_numberOfMalloc, String_totalMalloc, String_numberOfStrdup, String_numberOfFree, String_numberOfRealloc );
  
  }
  
  void __String_CheckStrdup ( const char* s )
  {
    // Warn ( "Strdup (%p) [%s]\n", s, s );

//    Bug ( "." );
  
  }  
#endif // __XEM_STRING_STATS

  void
  String::tokenize(std::list<String>& list, char separator ) const
  {
#if 1
    if ( !c_str() || !*c_str() ) return;
    const char* current = c_str();
    StringSize currentSz = 0;
    Debug ( "Tokenize '%s'\n", c_str() );
#define _doPush() \
    if ( currentSz ) \
    { \
      String ns; \
      ns._malloc(currentSz+1); \
      memcpy(ns.buff, current, currentSz); \
      ns.buff[currentSz] = 0; \
      Debug ( "Push : '%s'\n", ns.c_str() ); \
      list.push_back(ns); \
    }
    for ( const char* s = c_str() ; *s ; s++ )
      {
        if ( separator != '\0' ? separator == *s : isspace(*s) )
          {
            _doPush();
            current = s;
            current++;
            currentSz = 0;
            continue;
          }
        currentSz++;
      }
    _doPush();
#undef _doPush
#else
    String current;
    if (!c_str())
      return;
    for (const char* s = c_str(); *s; s++)
      {
        if ( separator != '\0' ? separator == *s : isspace(*s) )
          {
            if (current.size())
              list.push_back(current);
            current = "";
            continue;
          }
        current += *s;
      }
    if (current.size())
      list.push_back(current);
#endif
  }

  void tokensToTokenList ( std::list<String*>& list, const String& tokens )
  {
    String expr;
    bool sep = false;
    for ( const char* c = tokens.c_str() ; *c ; c++ )
      {
        switch ( *c )
        {
        case ' ': 
          if ( sep ) 
            {
              list.push_back ( new String(expr) );
              expr = "";
              sep = false;
              break;
            }
        default:
          expr += *c;
          sep = true;
        }
      }
    if ( expr.size() )
      list.push_back ( new String(expr) );
  }

  int stringComparator ( const char* left, const char* right, bool caseOrderUpperFirst )
  {
    AssertBug ( left && right, "Provided a null string !\n" );

    const char* l = left;
    const char* r = right;

    Log_StringCompare ( "Comparing '%s' with '%s'\n", left, right );
#ifdef __XEM_STRING_COMPARATOR_TRIM_SPACES
    while ( *l )
      {
        if ( isspace(*l) )
          l++;
        else
          break;
      }

    while ( *r )
      {
        if ( isspace(*r) )
          r++;
        else
          break;
      }

    Log_StringCompare ( "[POST-TRIM] Comparing '%s' with '%s'\n", l, r );
#endif
    int returnIfEquals = 0;
    while ( true )
      {
        Log_StringCompare ( "l=%p (%d:%c), r=%p (%d:%c)\n", l, *l, *l, r, *r, *r );
        if ( !*l )
          return *r ? -1 : returnIfEquals;
        if ( !*r )
          return *l ? 1: returnIfEquals;
#ifdef __XEM_STRING_COMPARATOR_SKIP_RETURN_CHARACTERS
        if ( *l == '\n' || *l == '\r' )
          {
            Log_StringCompare ( "Skipping left space\n" );
            l++;
            continue;
          }
        if ( *r == '\n' || *r == '\r' )
          {
            Log_StringCompare ( "Skipping right space\n" );
            r++;
            continue;
          }
#endif
        if ( *l == *r )
         {
            l++; r++;
            continue;
         }
        Log_StringCompare ( "POST-FILTER : l=%p (%d:%c), r=%p (%d:%c)\n", l, *l, *l, r, *r, *r );

        if ( isalpha(*l) && isalpha(*r) )
          {
            if ( toupper(*l) == toupper(*r) )
              {
                /**
                 * Here we know that :
                 * Both characters are alphanumeric, and are the same case-insensitively.
                 * We must continue to process strings, but we prepare the end of the string
                 */
                bool isOrderingCorrect = caseOrderUpperFirst ? isupper(*l) : islower(*l);

                Log_StringCompare ( "Same chars : caseOrderUpperFirst=%d, orderingCorrect=%d\n", caseOrderUpperFirst, isOrderingCorrect );

                if ( ! isOrderingCorrect )
                  {
                    returnIfEquals = 1;
                  }
                else if ( returnIfEquals == 0 )
                  {
                    returnIfEquals = -1;
                  }
                /**
                 * Ordering is preserved, continue comparing.
                 */
                l++; r++;
                continue;
              }
            else
              {
                /**
                 * Left and right are different characters (case-insensitively)
                 * Compare them as uppers.
                 */
                return ( toupper(*l) < toupper(*r) ) ? -1 : 1;
              }
          }
        return *l < *r ? -1 : 1;
      }
    Bug ( "Shall never be here !\n" );
    return 42;
  }

};
