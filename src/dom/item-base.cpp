#include <Xemeiah/dom/item-base.h>

#include <Xemeiah/dom/string.h>

#include <Xemeiah/auto-inline.hpp>

#include <math.h>

#define Log_ItemBase Debug

namespace Xem
{
  template<> Item::ItemType ItemImpl<bool>::getItemType() const { return Type_Bool; }
  template<> Item::ItemType ItemImpl<Integer>::getItemType() const { return Type_Integer; }
  template<> Item::ItemType ItemImpl<Number>::getItemType() const { return Type_Number; }
  
  template<> bool ItemImpl<bool>::toBool() { return contents; }
  template<> bool ItemImpl<Integer>::toBool() { return contents != 0; }
  template<> bool ItemImpl<Number>::toBool() { return contents != 0 && ! isnan(contents); }

  template<> String ItemImpl<bool>::toString() { return String ( contents ? "true" : "false" ); }
  template<> String ItemImpl<Integer>::toString() 
  {
    if ( contents == IntegerInfinity )
      {
        return String ( "NaN" );
      }
    char* buff = (char*) malloc ( 64 );
    sprintf ( buff, "%lld", contents );
    Log_ItemBase ( "Integer to string : '%s' (n=%.32f)\n", buff, (Number) contents );
    return stringFromAllocedStr ( buff );
  }
  
  template<> String ItemImpl<Number>::toString() 
  {
    Log_ItemBase ( "value : (lf) %.32lf (lg) %lg\n", contents, contents );

    if ( isnan ( contents ) ) 
      {
        return String ( "NaN" );
      }

    char* buff = (char*) malloc ( 128 );


    double integer_part;
    double fractional_part = modf ( contents, &integer_part );

    if ( fractional_part == 0 )
      {
        sprintf ( buff, "%ld", (long int) integer_part );
      }
    else
      {
        char format[32];
        for ( int i = 1 ; i < 64 ; i++ )
          {
            sprintf ( format, "%%.%df", i );
            sprintf ( buff, format, contents );
            double res = atof(buff);
            Log_ItemBase ( "i=%d, format=[%s], buff[%s], res=%.32f\n", i, format, buff, res );
            if ( res == contents )
              break;
          }
      }
    Log_ItemBase ( "Number (%.32lf) to string : '%s'\n", contents, buff );
    return stringFromAllocedStr ( buff );
  }

  template<> Number ItemImpl<bool>::toNumber()     { return contents ? 1 : 0; }
  template<> Number ItemImpl<Integer>::toNumber()  
  { 
    Log_ItemBase ( "Integer to Number.\n" );
    return contents; 
  }
  template<> Number ItemImpl<Number>::toNumber()   { return contents; }

  template<> Integer ItemImpl<bool>::toInteger() { return contents ? 1 : 0; }
  template<> Integer ItemImpl<Integer>::toInteger() { return contents; }
  template<> Integer ItemImpl<Number>::toInteger() 
  { 
    if ( !isnormal(contents) && contents != 0 ) return IntegerInfinity;
    
    Number rounded = roundNumber ( contents );
    Log_ItemBase ( "Number to Integer '%.32f' -> '%.32f' -> '%lld'.\n",
        contents, rounded, (Integer) lrint(rounded) );
    return lrint(rounded); 
  }
};
