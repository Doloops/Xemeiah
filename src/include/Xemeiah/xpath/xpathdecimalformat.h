#ifndef __XEM_XPATH_XPATHDECIMALFORMAT_H
#define __XEM_XPATH_XPATHDECIMALFORMAT_H

#include <Xemeiah/dom/string.h>

namespace Xem
{
  /**
   * XPath decimal format
   */
  class XPathDecimalFormat
  {
  protected:
    String name;
    /* Used for interpretation of pattern */
    String digit;
    String patternSeparator;
    /* May appear in result */
    String minusSign;
    String infinity;
    String noNumber; /* Not-a-number */
    /* Used for interpretation of pattern and may appear in result */
    String decimalPoint;
    String groupingSeparator;
    String percent;
    String permille;
    String zeroDigit;


    String getDecimalFormat ( KeyId keyId, const char* defaultValue );
    
  public:
    /**
     * Default constructor, implemented in xpath-format-number.cpp
     */
    XPathDecimalFormat ();

    // XPathDecimalFormat ( const ElementRef& eltRef ) : ElementRef (eltRef) {}
    ~XPathDecimalFormat() {}

    String getName() { return name; }
    void setName(String s) { name = s; }

    String getDigit() { return digit; }
    void setDigit(String s) { digit = s; }

    String getPatternSeparator() { return patternSeparator; }
    void setPatternSeparator(String s) { patternSeparator = s; }

    String getMinusSign() { return minusSign; }
    void setMinusSign(String s) { minusSign = s; }

    String getInfinity() { return infinity; }
    void setInfinity(String s) { infinity = s; }

    String getNoNumber() { return noNumber; }
    void setNoNumber(String s) { noNumber = s; }

    String getDecimalSeparator() { return decimalPoint; }
    void setDecimalSeparator(String s) { decimalPoint = s; }

    String getGroupingSeparator() { return groupingSeparator; }
    void setGroupingSeparator(String s) { groupingSeparator = s; }

    String getPercent() { return percent; }
    void setPercent(String s) { percent = s; }

    String getPermille() { return permille; }
    void setPermille(String s) { permille = s; }

    String getZeroDigit() { return zeroDigit; }
    void setZeroDigit(String s) { zeroDigit = s; }

#if 0
    String getName();
    /* Used for interpretation of pattern */
    String getDigit();
    String getPatternSeparator();
    /* May appear in result */
    String getMinusSign();
    String getInfinity();
    String getNoNumber(); /* Not-a-number */
    /* Used for interpretation of pattern and may appear in result */
    String getDecimalSeparator();
    String getGroupingSeparator();
    String getPercent();
    String getPermille();
    String getZeroDigit();
#endif
  };
};

#endif //  __XEM_XPATH_XPATHDECIMALFORMAT_H
