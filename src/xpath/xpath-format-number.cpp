/*
 * Taken from numbers.c: Implementation of the XSLT number functions present in libxslt1.1
 *
 * Reference:
 *   http://www.w3.org/TR/1999/REC-xslt-19991116
 *
 *  Copyright (C) 2001-2002 Daniel Veillard.  All Rights Reserved.
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
  * of this software and associated documentation files (the "Software"), to deal
  * in the Software without restriction, including without limitation the rights
  * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  * copies of the Software, and to permit persons to whom the Software is fur-
  * nished to do so, subject to the following conditions:
  *
  * The above copyright notice and this permission notice shall be included in
  * all copies or substantial portions of the Software.
  *
  * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FIT-
  * NESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
  * DANIEL VEILLARD BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
  * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CON-
  * NECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
  *
  * Except as contained in this notice, the name of Daniel Veillard shall not
  * be used in advertising or otherwise to promote the sale, use or other deal-
  * ings in this Software without prior written authorization from him.
 *
 * Contacts :
 * daniel@veillard.com
 * Bjorn Reese <breese@users.sourceforge.net>
 */

#include <Xemeiah/kern/utf8.h>
#include <Xemeiah/xpath/xpath.h>
#include <Xemeiah/xpath/xpathdecimalformat.h>
#include <Xemeiah/xprocessor/xprocessor.h>
#include <Xemeiah/dom/nodeset.h>

#include <Xemeiah/auto-inline.hpp>

#include <math.h>

/**
 * \file Freestyle re-implementation of libxslt1.1 numbering
 */

#define Log_XPathFormatNumber Debug

namespace Xem
{
#define __evalArgNumber(__idx)						\
  NodeSet __res##__idx; Number res##__idx;				\
    evalStep ( __res##__idx, node, step->functionArguments[__idx] ); \
    res##__idx = __res##__idx.toNumber ( );

#define __evalArgStr(__idx)						\
  NodeSet __res##__idx; String res##__idx;				\
  evalStep ( __res##__idx, node, step->functionArguments[__idx] ); \
  res##__idx = __res##__idx.toString ( );

  String
  xsltFormatNumberConversion(XPathDecimalFormat& decimalFormat, String& format,
      Number number);

  void
  XPath::evalFunctionFormatNumber(__XPath_Functor_Args)
  {
    __evalArgNumber(0);
    __evalArgStr(1);

    KeyId decimalFormatId = 0;
    if (step->functionArguments[2] != XPathStepId_NULL)
      {
        decimalFormatId = evalStepAsKeyId(node, step->functionArguments[2]);
        if (!decimalFormatId)
          throwXPathException ( "Invalid decimal-format name !\n" );
      }

    XPathDecimalFormat decimalFormat = getXProcessor().getXPathDecimalFormat(
        decimalFormatId);
    String res = xsltFormatNumberConversion(decimalFormat, res1, res0);
    result.setSingleton(res);
  }

  /**
   * This structure is stricly static to the current file.
   */
  struct XPathFormatNumberInfo
  {
    int integer_hash; /* Number of '#' in integer part */
    int integer_digits; /* Number of '0' in integer part */
    int frac_digits; /* Number of '0' in fractional part */
    int frac_hash; /* Number of '#' in fractional part */
    int group; /* Number of chars per display 'group' */
    int multiplier; /* Scaling for percent or permille */
    bool add_decimal; /* Flag for whether decimal point appears in pattern */
    bool is_multiplier_set; /* Flag to catch multiple occurences of percent/permille */
    bool is_negative_pattern;/* Flag for processing -ve prefix/suffix */
  };

  typedef char xmlChar;

  int
  xmlXPathIsInf(Number number)
  {
    return isinf(number);
  }

  int
  xmlXPathIsNaN(Number number)
  {
    return isnan(number);
  }

  int
  xsltUTF8Charcmp(const char* f, const char* s)
  {
    return strncmp(f, s, 1);
  }

  int
  xsltUTF8Charcmp(const char* f, const String& s)
  {
    return xsltUTF8Charcmp(f, s.c_str());
  }

  int
  xmlUTF8Strloc(const char* s, const char* c)
  {
    const char* s2 = strstr(s, c);
    if (!s2)
      return -1;
    return ((unsigned long) s2 - (unsigned long) s);
  }

  int
  xmlUTF8Strloc(const char* s, const String& c)
  {
    return xmlUTF8Strloc(s, c.c_str());
  }

  int
  xmlUTF8Strloc(const String& s, const String& c)
  {
    return xmlUTF8Strloc(s.c_str(), c.c_str());
  }

  int
  xmlStrlen(const String& s)
  {
    return s.size();
  }

  int
  xsltUTF8Size(const unsigned char* utf)
  {
    if (utf == NULL)
      return -1;
    return utf8CharSize(*utf);
  }

  int
  xsltUTF8Size(const char* utf)
  {
    return xsltUTF8Size((const unsigned char*) utf);
  }

  int
  xmlStrncmp(const char* s1, const char* s2, int i)
  {
    return strncmp(s1, s2, i);
  }

  int
  xmlStrncmp(const char* s1, const String& s2, int i)
  {
    return xmlStrncmp(s1, s2.c_str(), i);
  }

  int
  xmlStrncmp(const String& s1, const String& s2, int i)
  {
    return xmlStrncmp(s1.c_str(), s2.c_str(), i);
  }

  char*
  xmlUTF8Strpos(char* s, int i)
  {
    return &(s[i]);
  }

  char*
  xmlUTF8Strpos(const String& s, int i)
  {
    return xmlUTF8Strpos((char*) s.c_str(), i);
  }

  int
  xmlCopyCharMultiByte(char* p, int i)
  {
    Bug ( "." );
    return 0;
  }

  int
  xmlCopyCharMultiByte(char* p, const String& s)
  {
    int i = 0;
    for (const char* c = s.c_str(); *c; c++)
      {
        *p = *c;
        p++;
        i++;
      }
    return i;
  }

  int
  xsltGetUTF8Char(const char * utf, int * len)
  {
    Bug ( "." );
    return 0;
  }

  int
  xsltGetUTF8Char(const String& utf, int * len)
  {
    return xsltGetUTF8Char(utf.c_str(), len);
  }

#define SYMBOL_QUOTE		('\'')

#define IS_SPECIAL(_decimalFormat,_letter)			\
    ((xsltUTF8Charcmp((_letter), (_decimalFormat).getZeroDigit()) == 0)	    ||	\
     (xsltUTF8Charcmp((_letter), (_decimalFormat).getDigit()) == 0)	    ||	\
     (xsltUTF8Charcmp((_letter), (_decimalFormat).getDecimalSeparator()) == 0)  ||	\
     (xsltUTF8Charcmp((_letter), (_decimalFormat).getGroupingSeparator()) == 0)	    ||	\
     (xsltUTF8Charcmp((_letter), (_decimalFormat).getPatternSeparator()) == 0))

  static int
  xsltFormatNumberPreSuffix(XPathDecimalFormat& decimalFormat,
      const char** format, XPathFormatNumberInfo& info)
  {
    int count = 0; /* will hold total length of prefix/suffix */
    int len;

    Log_XPathFormatNumber ( "At format '%s'\n", *format );

    while (1)
      {
        /*
         * prefix / suffix ends at end of string or at
         * first 'special' character
         */
        if (**format == 0)
          return count;

        /* if next character 'escaped' just count it */
        if (**format == SYMBOL_QUOTE)
          {
            if (*++(*format) == 0)
              return -1;
          }
        else if (IS_SPECIAL(decimalFormat, *format))
          return count;
        /*
         * else treat percent/per-mille as special cases,
         * depending on whether +ve or -ve
         */
        else
          {
            /*
             * for +ve prefix/suffix, allow only a
             * single occurence of either
             */
            if (xsltUTF8Charcmp(*format, decimalFormat.getPercent()) == 0)
              {
                if (info.is_multiplier_set)
                  return -1;
                info.multiplier = 100;
                info.is_multiplier_set = true;
              }
            else if (xsltUTF8Charcmp(*format, decimalFormat.getPermille()) == 0)
              {
                if (info.is_multiplier_set)
                  return -1;
                info.multiplier = 1000;
                info.is_multiplier_set = true;
              }
          }

        if ((len = xsltUTF8Size(*format)) < 1)
          return -1;
        count += len;
        *format += len;
      }
    return -1;
  }

  static String
  xsltNumberFormatDecimal(Number number, const String& digit_zero, int width,
      int digitsPerGroup, const String& groupingCharacter,
      int groupingCharacterLen)
  {
    /*
     * This used to be
     *  xmlChar temp_string[sizeof(double) * CHAR_BIT * sizeof(xmlChar) + 4];
     * which would be length 68 on x86 arch.  It was changed to be a longer,
     * fixed length in order to try to cater for (reasonable) UTF8
     * separators and numeric characters.  The max UTF8 char size will be
     * 6 or less, so the value used [500] should be *much* larger than needed
     */
    xmlChar temp_string[500];
    xmlChar *pointer;
    xmlChar temp_char[6];
    int i;
    int val;
    int len;

    /* Build buffer from back */
    pointer = &temp_string[sizeof(temp_string)] - 1; /* last char */
    *pointer = 0;
    i = 0;
    while (pointer > temp_string)
      {
        if ((i >= width) && (fabs(number) < 1.0))
          break; /* for */
        if ((i > 0) && (groupingCharacterLen) && (digitsPerGroup > 0) && ((i
            % digitsPerGroup) == 0))
          {
            if (pointer - groupingCharacterLen < temp_string)
              {
                i = -1; /* flag error */
                break;
              }
            pointer -= groupingCharacterLen;
            xmlCopyCharMultiByte(pointer, groupingCharacter);
          }
        val = digit_zero.c_str()[0] + (int) fmod(number, 10.0);
        if (val < 0x80)
          { /* shortcut if ASCII */
            if (pointer <= temp_string)
              { /* Check enough room */
                i = -1;
                break;
              }
            *(--pointer) = val;
          }
        else
          {
            /* 
             * Here we have a multibyte character.  It's a little messy,
             * because until we generate the char we don't know how long
             * it is.  So, we generate it into the buffer temp_char, then
             * copy from there into temp_string.
             */
            len = xmlCopyCharMultiByte(temp_char, val);
            if ((pointer - len) < temp_string)
              {
                i = -1;
                break;
              }
            pointer -= len;
            memcpy(pointer, temp_char, len);
          }
        number /= 10.0;
        ++i;
      }
    if (i < 0)
      throwException ( Exception, "Internal buffer size exceeeeeeded !\n" );
#if 0
    xsltGenericError(xsltGenericErrorContext,
        "xsltNumberFormatDecimal: Internal buffer size exceeded");
    xmlBufferCat(buffer, pointer);
#endif
    Log_XPathFormatNumber ( "pointer at %p\n", pointer );
    Log_XPathFormatNumber ( "pointer contents : %s\n", pointer );
    return stringFromAllocedStr(strdup(pointer));
  }

  /**
   * xsltFormatNumberConversion:
   * @self: the decimal format
   * @format: the format requested
   * @number: the value to format
   * @result: the place to ouput the result
   *
   * format-number() uses the JDK 1.1 DecimalFormat class:
   *
   * http://java.sun.com/products/jdk/1.1/docs/api/java.text.DecimalFormat.html
   *
   * Structure:
   *
   *   pattern    := subpattern{;subpattern}
   *   subpattern := {prefix}integer{.fraction}{suffix}
   *   prefix     := '\\u0000'..'\\uFFFD' - specialCharacters
   *   suffix     := '\\u0000'..'\\uFFFD' - specialCharacters
   *   integer    := '#'* '0'* '0'
   *   fraction   := '0'* '#'*
   *
   *   Notation:
   *    X*       0 or more instances of X
   *    (X | Y)  either X or Y.
   *    X..Y     any character from X up to Y, inclusive.
   *    S - T    characters in S, except those in T
   *
   * Special Characters:
   *
   *   Symbol Meaning
   *   0      a digit
   *   #      a digit, zero shows as absent
   *   .      placeholder for decimal separator
   *   ,      placeholder for grouping separator.
   *   ;      separates formats.
   *   -      default negative prefix.
   *   %      multiply by 100 and show as percentage
   *   ?      multiply by 1000 and show as per mille
   *   X      any other characters can be used in the prefix or suffix
   *   '      used to quote special characters in a prefix or suffix.
   *
   * Returns a possible XPath error
   */
  String
  xsltFormatNumberConversion(XPathDecimalFormat& decimalFormat, String& format,
      Number number)
  {
    String result;
    String prefix, suffix, nprefix, nsuffix;
    const char* the_format;

    int prefix_length, suffix_length = 0, nprefix_length, nsuffix_length;
    double scale;
    int j, len = 0;
    int self_grouping_len;
    XPathFormatNumberInfo format_info;
    /* 
     * delayed_multiplier allows a 'trailing' percent or
     * permille to be treated as suffix 
     */
    int delayed_multiplier = 0;
    /* flag to show no -ve format present for -ve number */
    char default_sign = 0;

    if (format.size() <= 0)
      {
        throwException ( Exception, "Invalid null-size format !\n" );
      }
    switch (xmlXPathIsInf(number))
      {
    case -1:
      result += decimalFormat.getMinusSign();
      /* no-break on purpose */
    case 1:
      result += decimalFormat.getInfinity();
      return result;
    default:
      if (xmlXPathIsNaN(number))
        {
          result = decimalFormat.getNoNumber();
          return result;
        }
      }

    format_info.integer_hash = 0;
    format_info.integer_digits = 0;
    format_info.frac_digits = 0;
    format_info.frac_hash = 0;
    format_info.group = -1;
    format_info.multiplier = 1;
    format_info.add_decimal = false;
    format_info.is_multiplier_set = false;
    format_info.is_negative_pattern = false;

    the_format = (char*) format.c_str();

    /*
     * First we process the +ve pattern to get percent / permille,
     * as well as main format 
     */
    prefix = the_format;
    prefix_length = xsltFormatNumberPreSuffix(decimalFormat, &the_format,
        format_info);
    if (prefix_length < 0)
      {
        throwException ( Exception, "Could not call presuffix()\n" );
      }

    Log_XPathFormatNumber ( ": presuffix : prefix_length=%d\n", prefix_length );

    /* 
     * Here we process the "number" part of the format.  It gets 
     * a little messy because of the percent/per-mille - if that
     * appears at the end, it may be part of the suffix instead 
     * of part of the number, so the variable delayed_multiplier 
     * is used to handle it 
     */
    self_grouping_len = xmlStrlen(decimalFormat.getGroupingSeparator());
    while ((*the_format != 0) && (xsltUTF8Charcmp(the_format,
        decimalFormat.getDecimalSeparator()) != 0) && (xsltUTF8Charcmp(
        the_format, decimalFormat.getPatternSeparator()) != 0))
      {

        if (delayed_multiplier != 0)
          {
            format_info.multiplier = delayed_multiplier;
            format_info.is_multiplier_set = true;
            delayed_multiplier = 0;
          }
        if (xsltUTF8Charcmp(the_format, decimalFormat.getDigit()) == 0)
          {
            if (format_info.integer_digits > 0)
              {
                throwException ( Exception, "Got spurious number of digits ?\n" );
              }
            format_info.integer_hash++;
            if (format_info.group >= 0)
              format_info.group++;
          }
        else if (xsltUTF8Charcmp(the_format, decimalFormat.getZeroDigit()) == 0)
          {
            format_info.integer_digits++;
            if (format_info.group >= 0)
              format_info.group++;
          }
        else if ((self_grouping_len > 0) && (!xmlStrncmp(the_format,
            decimalFormat.getGroupingSeparator(), self_grouping_len)))
          {
            /* Reset group count */
            format_info.group = 0;
            the_format += self_grouping_len;
            continue;
          }
        else if (xsltUTF8Charcmp(the_format, decimalFormat.getPercent()) == 0)
          {
            if (format_info.is_multiplier_set)
              {
                throwException ( Exception, "Multiplier already set ?\n" );
              }
            delayed_multiplier = 100;
          }
        else if (xsltUTF8Charcmp(the_format, decimalFormat.getPermille()) == 0)
          {
            if (format_info.is_multiplier_set)
              {
                throwException ( Exception, "Multiplier already set ?\n" );
              }
            delayed_multiplier = 1000;
          }
        else
          break; /* while */

        if ((len = xsltUTF8Size(the_format)) < 1)
          {
            throwException ( Exception, "Could not fwd format ?\n" );
          }
        the_format += len;
      }

    /* We have finished the integer part, now work on fraction */
    if (xsltUTF8Charcmp(the_format, decimalFormat.getDecimalSeparator()) == 0)
      {
        format_info.add_decimal = true;
        the_format += xsltUTF8Size(the_format); /* Skip over the decimal */
      }

    while (*the_format != 0)
      {
        if (xsltUTF8Charcmp(the_format, decimalFormat.getZeroDigit()) == 0)
          {
            if (format_info.frac_hash != 0)
              {
                throwException ( Exception, "Frac_hash already set on the_format=%s\n", the_format );
              }
            format_info.frac_digits++;
          }
        else if (xsltUTF8Charcmp(the_format, decimalFormat.getDigit()) == 0)
          {
            format_info.frac_hash++;
          }
        else if (xsltUTF8Charcmp(the_format, decimalFormat.getPercent()) == 0)
          {
            if (format_info.is_multiplier_set)
              {
                throwException ( Exception, "Multiplier already set !\n" );
              }
            delayed_multiplier = 100;
            if ((len = xsltUTF8Size(the_format)) < 1)
              {
                throwException ( Exception, "Null size ?\n" );
              }
            the_format += len;
            continue; /* while */
          }
        else if (xsltUTF8Charcmp(the_format, decimalFormat.getPermille()) == 0)
          {
            if (format_info.is_multiplier_set)
              {
                throwException ( Exception, "Multiplier already set !\n" );
              }
            delayed_multiplier = 1000;
            if ((len = xsltUTF8Size(the_format)) < 1)
              {
                throwException ( Exception, "Null size ?\n" );
              }
            the_format += len;
            continue; /* while */
          }
        else if (xsltUTF8Charcmp(the_format,
            decimalFormat.getGroupingSeparator()) != 0)
          {
            break; /* while */
          }
        if ((len = xsltUTF8Size(the_format)) < 1)
          {
            throwException ( Exception, "Null size ?\n" );
          }
        the_format += len;
        if (delayed_multiplier != 0)
          {
            format_info.multiplier = delayed_multiplier;
            delayed_multiplier = 0;
            format_info.is_multiplier_set = true;
          }
      }

    /* 
     * If delayed_multiplier is set after processing the 
     * "number" part, should be in suffix 
     */
    if (delayed_multiplier != 0)
      {
        the_format -= len;
        delayed_multiplier = 0;
      }
    suffix = the_format;
    suffix_length = xsltFormatNumberPreSuffix(decimalFormat, &the_format,
        format_info);
    if ((suffix_length < 0) || ((*the_format != 0) && (xsltUTF8Charcmp(
        the_format, decimalFormat.getPatternSeparator()) != 0)))
      {
        throwException ( Exception, "Something wild : suffix_length=%d, the_format=%s, patternSeparator=%s.\n",
            suffix_length, the_format, decimalFormat.getPatternSeparator().c_str() );
      }

    /*
     * We have processed the +ve prefix, number part and +ve suffix.
     * If the number is -ve, we must substitute the -ve prefix / suffix
     */
    if (number < 0)
      {
        /*
         * Note that j is the number of UTF8 chars before the separator,
         * not the number of bytes! (bug 151975)
         */
        j = xmlUTF8Strloc(format, decimalFormat.getPatternSeparator());
        if (j < 0)
          {
            /* No -ve pattern present, so use default signing */
            default_sign = 1;
          }
        else
          {
            /* Skip over pattern separator (accounting for UTF8) */
            the_format = xmlUTF8Strpos(format, j + 1);
            /* 
             * Flag changes interpretation of percent/permille 
             * in -ve pattern 
             */
            format_info.is_negative_pattern = true;
            format_info.is_multiplier_set = false;

            Log_XPathFormatNumber ( "calling for '-ve' at %s\n", the_format );
            /* First do the -ve prefix */
            nprefix = the_format;
            nprefix_length = xsltFormatNumberPreSuffix(decimalFormat,
                &the_format, format_info);
            if (nprefix_length < 0)
              {
                throwException ( Exception, "Invalid size for nprefix !\n" );
              }
            Log_XPathFormatNumber ( "called for '-ve' at [%s], nprefix_length=%d\n", the_format, nprefix_length );
            while (*the_format != 0)
              {
                if ((xsltUTF8Charcmp(the_format, decimalFormat.getPercent())
                    == 0) || (xsltUTF8Charcmp(the_format,
                    decimalFormat.getPermille()) == 0))
                  {
                    if (format_info.is_multiplier_set)
                      {
                        throwException ( Exception, "Multiplier aready set !\n" );
                      }
                    format_info.is_multiplier_set = true;
                    delayed_multiplier = 1;
                  }
                else if (IS_SPECIAL(decimalFormat, the_format))
                  delayed_multiplier = 0;
                else
                  break; /* while */
                if ((len = xsltUTF8Size(the_format)) < 1)
                  {
                    throwException ( Exception, "Error while forwarding string.\n" );
                  }
                the_format += len;
              }
            if (delayed_multiplier != 0)
              {
                format_info.is_multiplier_set = false;
                the_format -= len;
              }

            /* Finally do the -ve suffix */
            if (*the_format != 0)
              {
                nsuffix = the_format;
                nsuffix_length = xsltFormatNumberPreSuffix(decimalFormat,
                    &the_format, format_info);
                if (nsuffix_length < 0)
                  {
                    throwException ( Exception, "Invalid nsuffix length !\n" );
                  }
              }
            else
              nsuffix_length = 0;
            if (*the_format != 0)
              {
                throwException ( Exception, "Invalid things after format !\n" );
              }
            /*
             * Here's another Java peculiarity:
             * if -ve prefix/suffix == +ve ones, discard & use default
             */
            if ((nprefix_length != prefix_length) || (nsuffix_length
                != suffix_length) || ((nprefix_length > 0) && (xmlStrncmp(
                nprefix, prefix, prefix_length) != 0)) || ((nsuffix_length > 0)
                && (xmlStrncmp(nsuffix, suffix, suffix_length) != 0)))
              {
                prefix = nprefix;
                prefix_length = nprefix_length;
                suffix = nsuffix;
                suffix_length = nsuffix_length;
                Log_XPathFormatNumber ( "post : prefix_length=%d, suffix_length=%d\n", prefix_length, suffix_length );
              } /* else {
               default_sign = 1;
               }
               */
          }
      }

    /**
     * Now outputing error !
     */
#if 0     
    if (found_error != 0)
      {
        xsltTransformError(NULL, NULL, NULL,
            "xsltFormatNumberConversion : "
            "error in format string '%s', using default\n", format);
        default_sign = (number < 0.0) ? 1 : 0;
        prefix_length = suffix_length = 0;
        format_info.integer_hash = 0;
        format_info.integer_digits = 1;
        format_info.frac_digits = 1;
        format_info.frac_hash = 4;
        format_info.group = -1;
        format_info.multiplier = 1;
        format_info.add_decimal = TRUE;
      }
#endif

    /* Ready to output our number.  First see if "default sign" is required */
    if (default_sign != 0)
      {
        // xmlBufferAdd(buffer, decimalFormat.minusSign, xsltUTF8Size(decimalFormat.minusSign));
        result += decimalFormat.getMinusSign();
      }

    Log_XPathFormatNumber ( "prefix='%s', , prefix_length=%d\n", prefix.c_str(), prefix_length );
    if (prefix_length)
      {
        /* Put the prefix into the buffer */
        result += prefix.substr(0, prefix_length);
      }

    /* Next do the integer part of the number */
    number = fabs(number) * (double) format_info.multiplier;
    scale = pow(10.0,
        (double) (format_info.frac_digits + format_info.frac_hash));
    number = floor((scale * number + 0.5)) / scale;

    if (decimalFormat.getGroupingSeparator().size())
      {
        len = xmlStrlen(decimalFormat.getGroupingSeparator());
        result += xsltNumberFormatDecimal(floor(number),
            decimalFormat.getZeroDigit(), format_info.integer_digits,
            format_info.group, decimalFormat.getGroupingSeparator(), len);
      }
    else
      {
        result += xsltNumberFormatDecimal(floor(number),
            decimalFormat.getZeroDigit(), format_info.integer_digits,
            format_info.group, String(","), 1);
      }

    /* Special case: java treats '.#' like '.0', '.##' like '.0#', etc. */
    if ((format_info.integer_digits + format_info.integer_hash
        + format_info.frac_digits == 0) && (format_info.frac_hash > 0))
      {
        ++format_info.frac_digits;
        --format_info.frac_hash;
      }

    /* Add leading zero, if required */
    if ((floor(number) == 0) && (format_info.integer_digits
        + format_info.frac_digits == 0))
      {
        result += decimalFormat.getZeroDigit();
      }

    /* Next the fractional part, if required */
    if (format_info.frac_digits + format_info.frac_hash == 0)
      {
        if (format_info.add_decimal)
          result += decimalFormat.getDecimalSeparator();
      }
    else
      {
        number -= floor(number);
        if ((number != 0) || (format_info.frac_digits != 0))
          {
            result += decimalFormat.getDecimalSeparator();
            number = floor(scale * number + 0.5);
            for (j = format_info.frac_hash; j > 0; j--)
              {
                if (fmod(number, 10.0) >= 1.0)
                  break; /* for */
                number /= 10.0;
              }
            result += xsltNumberFormatDecimal(floor(number),
                decimalFormat.getZeroDigit(), format_info.frac_digits + j, 0,
                0, 0);
          }
      }
    /* Put the suffix into the buffer */
    if (suffix_length)
      {
        result += suffix.substr(0, suffix_length);
      }
    return result;
  }

}
;

