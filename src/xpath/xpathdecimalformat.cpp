#include <Xemeiah/xpath/xpathdecimalformat.h>

#include <Xemeiah/auto-inline.hpp>

namespace Xem
{
#if 0
  String XPathDecimalFormat::getDecimalFormat ( KeyId keyId, const char* defaultValue )
  {
    if ( ! (*this) ) return String(defaultValue);
    AttributeRef val = findAttr ( keyId );
    if ( ! val ) return String(defaultValue);
    return val.toString ();
  }
#endif

  XPathDecimalFormat::XPathDecimalFormat ()
  {
    digit = "#";
    patternSeparator = ";";
    decimalPoint = ".";
    groupingSeparator = ",";
    percent = "%";

    /* UTF-8 for 0x2030 */
    static const char __permille[4] = {0xe2, 0x80, 0xb0, 0};
    permille = __permille;
    zeroDigit = "0";
    minusSign = "-";
    infinity = "Infinity";
    noNumber = "NaN";
  }

#if 0
#define __builtin_xsl getKeyCache().getBuiltinKeys().xsl

  String XPathDecimalFormat::getName()
  {
    return getDecimalFormat ( /* __builtin_xsl.name() */ 0, "(default)" );
  } 
  
  String XPathDecimalFormat::getDigit()
  {
    return getDecimalFormat ( /* __builtin_xsl.digit() */ 0, "#" );
  }
  
  String XPathDecimalFormat::getPatternSeparator()
  {
    return getDecimalFormat ( /* __builtin_xsl.pattern_separator() */ 0, ";" );
  }
  
  String XPathDecimalFormat::getMinusSign()
  {
    return getDecimalFormat ( /* __builtin_xsl.minus_sign() */ 0, "-" );
  }
  
  String XPathDecimalFormat::getInfinity()
  {
    return getDecimalFormat ( /* __builtin_xsl.infinity() */ 0, "Infinity" );
  }
  
  String XPathDecimalFormat::getNoNumber()
  {
    return getDecimalFormat ( /* __builtin_xsl.NaN() */ 0, "NaN" );
  }
  
  String XPathDecimalFormat::getDecimalSeparator()
  {
    return getDecimalFormat ( /* __builtin_xsl.decimal_separator() */ 0, "." );
  }
  
  String XPathDecimalFormat::getGroupingSeparator()
  {
    return getDecimalFormat ( /* __builtin_xsl.grouping_separator() */ 0, "," );
  }
  
  String XPathDecimalFormat::getPercent()
  {
    return getDecimalFormat ( /* __builtin_xsl.percent() */ 0, "%" );
  }
  
  String XPathDecimalFormat::getPermille()
  {
    /* UTF-8 for 0x2030 */
    static const char __permille[4] = {0xe2, 0x80, 0xb0, 0};
    return getDecimalFormat ( /* __builtin_xsl.per_mille() */ 0, __permille );
  }
  
  String XPathDecimalFormat::getZeroDigit()
  {
    return getDecimalFormat ( /* __builtin_xsl.zero_digit() */ 0, "0" );
  }
#endif
};

