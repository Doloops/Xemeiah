#include <Xemeiah/dom/item.h>

#include <Xemeiah/dom/string.h>

#include <math.h>

#include <Xemeiah/auto-inline.hpp>
/*
 * Default Item behavior
 */

namespace Xem
{
  bool Item::toBool ()
  {
    String str = toString();
    return str.size() != 0;
  }

  inline Integer stringToInteger ( const String& s )
  {
    char* afterConversion;
    Integer result = strtoll ( s.c_str(), &afterConversion, 0 );

    if ( ! afterConversion || *afterConversion == '\0' )
    	return result;
    return (Integer) NAN;
  }

  
  Integer Item::toInteger ()
  {
    return stringToInteger ( toString() );
  }
  
  inline Number stringToNumber ( const String& s )
  {
    if ( s.size() == 0 )
      return NAN;
    char* afterConversion;
    Number result = strtod ( s.c_str(), &afterConversion );
    if ( ! afterConversion )
    	return result;
   
    for ( char* d = afterConversion ; *d ; d++ )
     {
       if ( ! isspace(*d) )
         {
            return NAN;          
         }
     }
     
#if 0 // STRICT CONVERSION
    throwException ( DOMCastException, 
	    "Conversion from String to Number : string '%s' - remaining characters : '%s'\n", 
      s.c_str(), afterConversion );
#endif
    return result;
  }  
  
  Number Item::toNumber ()
  {
    return stringToNumber ( toString() );  
  }

  bool Item::isNaN ()
  {
    return isnan ( toNumber() );
  }

  Number Item::roundNumber ( Number arg )
  {
    if ( isnormal(arg) )
      {
        if ( arg > 0 )
          {
            return floor ( arg + 0.5 );
          }
        else if ( arg == 0 )
          {
            return 0;
          }
        double frac_part, int_part;
        frac_part = modf ( arg, &int_part );

        Number res;
        res = (frac_part < -0.5) ? int_part - 1 : int_part;
        return res;
      }
    return arg;
  }
};

