#include <Xemeiah/xsl/xsl-numbering.h>
#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/xpath/xpath.h>
#include <Xemeiah/nodeflow/nodeflow-stream.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_XSLNumbering Debug

namespace Xem
{
  XSLNumbering::XSLNumbering ()
  {
    
  
  }

  XSLNumbering::~XSLNumbering ()
  {
    
  
  }

  int xsltUTF8Size(const char* utf);


  void XSLNumbering::parse ( const String& expression, char groupingSeparator, __ui32 groupingSize )
  {
    // allocateXPathSegment ();      
    init ();
    
    __ui32 intConversionSz = 0;
    const char* intConversionStart;
    __ui32 preTokenSz = 0;
    const char* preToken = NULL;
    
    XPathStepId conversionStepId = allocStep ();
    firstStep = conversionStepId;
    
    XPathStep* conversionStep = getStep ( conversionStepId );
    
    conversionStep->action = XPathFunc_XSLNumbering;
    int numberOfArguments = 0;
    for ( int arg = 0 ; arg < XPathStep_MaxFuncCallArgs ; arg++ )
      conversionStep->functionArguments[arg] = XPathStepId_NULL;

//      throwXPathException ( "Expression too complex, too much converters.\n" ); 
    XPathStepId preTokenStepId = XPathStepId_NULL;      
#define addStepConversion( __id, __step ) \
    if ( numberOfArguments == XPathStep_MaxFuncCallArgs ) \
      throwXSLFormatNumberException ( "Too complex.\n" ); \
    conversionStep->functionArguments[numberOfArguments++] = __id; \
    __step->xslNumberingFormat.preToken = preTokenStepId; preTokenStepId = XPathStepId_NULL;

#define preTokenToStep() \
   if ( preTokenSz ) \
     { \
       Log_XSLNumbering ( "End of pretoken sz=%u / %s\n", preTokenSz, preToken ); \
       char buff[256]; \
       memcpy ( buff, preToken, preTokenSz ); \
       buff[preTokenSz] = '\0'; \
       preTokenSz = 0; \
       preTokenStepId = allocStepResource ( buff ); \
     }


#define intConversionToStep() \
  if ( intConversionSz ) \
    { \
      XPathStepId stepId = allocStep (); \
      XPathStep* step = getStep ( stepId ); \
      step->action = XPathFunc_XSLNumbering_IntegerConverter; \
      step->xslNumberingFormat.integerConversion.precision = intConversionSz; \
      step->xslNumberingFormat.integerConversion.groupingSeparator = groupingSeparator; \
      step->xslNumberingFormat.integerConversion.groupingSize = groupingSize; \
      intConversionSz = 0; \
      addStepConversion ( stepId, step ); \
    }

    for ( const char* f = expression.c_str() ; *f ; f++ )
      {
        Log_XSLNumbering ( "f=%c (%x), intConversionSz=%u, preTokenSz=%u, xsltUTF8Size=%d\n",
              *f, (unsigned char)*f, intConversionSz, preTokenSz, xsltUTF8Size(f) );
        switch ( *f )
        {
        case '0':
        case '1':
          if ( intConversionSz )
            {
              intConversionSz++;
              break;
            }
          preTokenToStep ();
          intConversionSz = 1;
          intConversionStart = f;
          break;
        case 'A':
        case 'a':
        case 'i':
        case 'I':
          intConversionToStep()
          preTokenToStep ();
          {
            XPathStepId stepId = allocStep ();
            XPathStep* step = getStep ( stepId );
            step->action = XPathFunc_XSLNumbering_SingleCharacterConverter;
            step->xslNumberingFormat.singleCharacter = *f;
            addStepConversion ( stepId, step );
          }
          break;
        default:
          if ( xsltUTF8Size(f) > 1 )
            {
              throwException ( Exception, "NotImplemented : multi-char in numbering !\n" );
            }
          intConversionToStep();
          if ( preTokenSz == 0 )
            {
              preToken = f;
              preTokenSz = 1;
              break;              
            }
          preTokenSz++;
        };
      }

    intConversionToStep();
    preTokenToStep ();

    if ( numberOfArguments == XPathStep_MaxFuncCallArgs )
      throwXSLFormatNumberException ( "Too complex.\n" );
    conversionStep->functionArguments[numberOfArguments++] = preTokenStepId;

  }

  String XPath::evalNumbering ( std::list<Integer>& positions )
  {
    AssertBug ( xpathSegment->firstStep != XPathStepId_NULL, "First step is null !\n" );
    XPathStep* rootStep = getStep ( xpathSegment->firstStep );
    AssertBug ( rootStep->action == XPathFunc_XSLNumbering, "Invalid action %x\n", rootStep->action );

    // logXPath ();  

    char number[256];
    char* n = number;
    int currentToken = 0;
    
    bool skipPreToken = false;
    
    int lastConversionToken = 0; // When currentToken == lastConversionToken, we do not have any separator left.
    for ( int i = 0 ; i < XPathStep_MaxFuncCallArgs && rootStep->functionArguments[i] != XPathStepId_NULL ; i++ )
      {
        XPathStep* step = getStep ( rootStep->functionArguments[i] );
        if ( step->action != XPathAxis_Resource )
          lastConversionToken = i;
        if ( i == XPathStep_MaxFuncCallArgs )
          break;
      }
    
    Log_XSLNumbering ( "lastConversionToken = %u\n", lastConversionToken );
    
    for ( std::list<Integer>::iterator iter = positions.begin () ; iter != positions.end() ; iter++ )
      {        
        XPathStep* currentStep = getStep ( rootStep->functionArguments[currentToken] );
        Log_XSLNumbering ( "Step %i, currentStep->action=%x\n", currentToken, currentStep->action );
        AssertBug ( currentStep->action != XPathAxis_Resource, "Invalid resource action in middle of nowhere. Action=0x%x\n", currentStep->action );
        if ( skipPreToken )
          {
            *n = '.';
            n++;
          }
        else if ( currentStep->xslNumberingFormat.preToken != XPathStepId_NULL )
          {
            XPathStep* preTokenStep = getStep ( currentStep->xslNumberingFormat.preToken );            
            for ( const char* resource = getResource ( preTokenStep->resource ) ;
              *resource ; resource++ )
              {
                *n = *resource;
                n++;
              }
          }
        AssertBug ( currentStep->action != XPathAxis_Resource, "Invalid step.\n");

        if ( currentStep->action == XPathFunc_XSLNumbering_IntegerConverter )
          {
            __ui8 precision = currentStep->xslNumberingFormat.integerConversion.precision;
            __ui8 groupingSize = currentStep->xslNumberingFormat.integerConversion.groupingSize;
            if ( groupingSize == 255 || *iter < 10LL )
              {
                int res = sprintf ( n, "%.*llu", precision, *iter  );
                n = &(n[res]);
              }
            else
              {
                Integer pos = *iter;
                __ui8 groupingSeparator = currentStep->xslNumberingFormat.integerConversion.groupingSeparator;
                /*
                 * digits is the number of characters (exponent=log10(pos));
                 * divider is the current divider
                 */
                Integer divider = 1;
                Integer digits = 1;
                while ( true )
                  {
                    if ( divider * 10 > pos )
                      break;
                    divider *= 10;
                    digits ++;
                  }
                Log_XSLNumbering ( "[GROUP] pos=%lld, divider=%lld, digits=%lld\n", pos, divider, digits );
                while ( true )
                  {
                    Integer digit = pos / divider;
                    Log_XSLNumbering ( "[GROUP] digit=%lld, digits=%lld\n", digit, digits );
                    AssertBug ( digit < 10, "Invalid digit %lld\n", digit );
                    *n = (char)digit + '0'; n++;
                    pos -= ( digit * divider );
                    if ( divider == 1 ) break;
                    divider /= 10;
                    digits --;
                    if ( digits && ( digits % groupingSize == 0 ) )
                      {
                        *n = groupingSeparator;
                        n++;
                      }
                  }
                *n = '\0';
                Log_XSLNumbering ( "[GROUP] number=%s\n", number );
              }
          }
        else if ( currentStep->action == XPathFunc_XSLNumbering_SingleCharacterConverter )
          {
            unsigned char op = currentStep->xslNumberingFormat.singleCharacter;
            switch ( op )
              {

              case 'I':
              case 'i':
                {
                  Integer i = *iter;
                  if ( i  == 0 )
                    {
                      *n = '0';
                      n++;
                      break;
                    }
                  static const char* majRomChar = "IVXLCDM";
                  static const char* minRomChar = "ivxlcdm";
                  
                  static const char* majRomPreChar = "?IIXXCC";
                  static const char* minRomPreChar = "?iixxcc";
                  const char* romPreChar = majRomPreChar;
                  
                  const char* romChar = majRomChar;
                  
                  if ( op == 'i' )
                    {
                      romChar = minRomChar;
                      romPreChar = minRomPreChar;
                    }
                  static const Integer romVal[] = { 1, 5, 10, 50, 100, 500, 1000 }; 
                  static const Integer romPreVal[] = { 0, 4, 9, 40, 90, 400, 900 };
                  int romNb = 6;
                  while ( i > 0 )
                    {
                      Log_XSLNumbering ( "i=%lld, romNb=%d (val=%lld) (preval=%lld)\n", i, romNb, romVal[romNb], romPreVal[romNb] );
                      if ( i >= romVal[romNb] )
                        {
                          *n = romChar[romNb];
                          n++;
                          i -= romVal[romNb];
                          continue;
                        }
                      if ( i >= romPreVal[romNb] )
                       {
                         *n = romPreChar[romNb];
                         n++;
                         *n = romChar[romNb];
                         n++;
                         i -= romPreVal[romNb];
                       }
                      romNb --;
                    }
                  *n = '\0';
                  Log_XSLNumbering ( "i=%lld -> %s\n", *iter, number );
                }
                break;
              case 'A':
              case 'a':
                {
                  char c = op;
                  Integer i = *iter;
                  if ( i == 0 )
                    {
                      break;
                    }
                  Log_XSLNumbering ( "Transforming i=%lld\n", i );
                  i--;
                  while ( true )
                    {
                      Integer remains = i % 26;
                      Integer quotient = i / 26;
                      Log_XSLNumbering ( "i=%lld, q=%lld, r=%lld\n", i, quotient, remains );
                      AssertBug ( i == quotient * 26 + remains, "i=%lld, q=%lld, r=%lld\n", 
                        i, quotient, remains );
                      if ( quotient )
                        {
                          *n = (quotient-1) + c;
                          n++;
                        }
                      *n = remains + c;
                      n++;
                      if ( quotient > 26 )
                        {
                          NotImplemented ( "Number too high to be transformed to an alphanumeric representation : %llu.\n", *iter );
                          i = quotient;
                        }
                      else
                        break;
                    }
                }
                break;
              default:
                throwXSLFormatNumberException ( "Invalid single-character %c\n", op );
              }
          }
        else
          {
            throwXSLFormatNumberException ( "Invalid converter : %x\n", currentStep->action );
          }
        /*
         * We did not go to the last conversion operator, progress
         */
        if ( currentToken < lastConversionToken ) 
          currentToken ++;
        /*
         * We have gone to the last operator (currentToken == lastConversionToken), so we wont progress
         * If the last converter is also the first, we must skip the preToken and use '.' instead.
         */
        else if ( currentToken == 0 )
          {
            skipPreToken = true;
          }
      }
    Log_XSLNumbering ( "At end : lastConversonToken=%u, arg=%u, arg[+1]=%u\n",
        lastConversionToken,
        rootStep->functionArguments[lastConversionToken],
        rootStep->functionArguments[lastConversionToken+1] );
    if ( rootStep->functionArguments[lastConversionToken+1] != XPathStepId_NULL )
      {
        XPathStep* currentStep = getStep ( rootStep->functionArguments[lastConversionToken+1] );
        AssertBug ( currentStep->action == XPathAxis_Resource, "Invalid end of step" );
        for ( const char* resource = getResource ( currentStep->resource ) ;
          *resource ; resource++ )
          {
            *n = *resource;
            n++;
          }
      }
      
    n[0] = '\0';
    
    Log_XSLNumbering ( "(sz=%lu) -> '%s'\n", (unsigned long) positions.size(), number );
    return stringFromAllocedStr ( strdup(number) );
  }
  
  void XSLProcessor::xslFormatNumber ( ElementRef& item, std::list<Integer>& positions )
  {
    if ( positions.size() == 0 )
      return;
      
    if ( item.hasAttr(xsl.format()) )
      {
        AttributeRef xpathAttr = item.findAttr(xslimpl.parsed_number_format(), AttributeType_XPath );
        if ( xpathAttr )
          {
            XPath xpath ( getXProcessor(), xpathAttr );
            String result = xpath.evalNumbering ( positions );
            getNodeFlow().appendText ( result, false );
          }
        else
          {
            String format = item.getAttr(xsl.format());
            bool isAVT = false;
            if ( strchr ( format.c_str(), '{' ) )
              {
                isAVT = true;
                format = item.getEvaledAttr ( getXProcessor(), xsl.format());
              }
            XSLNumbering xslNumbering;

            char groupingSeparator = '?';
            __ui8 groupingSize = 255;
            if ( item.hasAttr ( xsl.grouping_separator() ) && item.hasAttr ( xsl.grouping_size() ) )
              {
                const char* groupingSeparatorStr = item.getAttr ( xsl.grouping_separator() ).c_str();
                groupingSeparator = groupingSeparatorStr[0];
                if ( groupingSeparatorStr[1] )
                  {
                    NotImplemented ( "Unsupported : multiple-character groupingSeparator '%s'\n", groupingSeparatorStr );
                  }
                groupingSize = atoi ( item.getAttr ( xsl.grouping_size() ).c_str() );
                if ( groupingSize == 0 )
                  {
                    Warn ( "xsl:number : invalid grouping-size '%s'\n", item.getAttr ( xsl.grouping_size() ).c_str() );
                    groupingSize = 255;
                  }
              }
            Log_XSLNumbering ( "Parsing : format='%s', grouping sep='%c', size='%u', isAVT=%d\n",
              format.c_str(), groupingSeparator, groupingSize, isAVT );
            xslNumbering.parse ( format, groupingSeparator, groupingSize );
      
            if ( ! isAVT )
              {
                if ( item.getDocument().isWritable() )
                  {
                    xslNumbering.saveToStore ( item, xslimpl.parsed_number_format() );
                  }
              }
            
            String result = XPath(getXProcessor(), xslNumbering).evalNumbering ( positions );
            getNodeFlow().appendText ( result, false );
          }
        return;
      }
    
    
    if ( positions.size() > 1 )
      {
        throwXSLFormatNumberException ( "Not implemented : multiple positions, no format !\n" );      
      }

    char number[64];
    sprintf ( number, "%lld", positions.front() );
    getNodeFlow().appendText ( number, true );    
  }

  void XSLProcessor::xslFormatNumber ( ElementRef& item, Integer value )
  {
    std::list<Integer> list;
    list.push_back ( value );
    xslFormatNumber ( item, list );
  }

  
  void XSLProcessor::xslInstructionNumber ( __XProcHandlerArgs__ )
  {
    if ( item.hasAttr(xsl.value()) )
      {
        XPath valueXPath ( getXProcessor(), item, xsl.value() );
        NodeSet result; valueXPath.eval ( result );
        Integer value = result.toInteger ();
        xslFormatNumber ( item, value );
        return;
      }
    String level;
    if ( item.hasAttr(xsl.level()) )
      level = item.getEvaledAttr ( getXProcessor(), xsl.level() );
    else
      level = "single";

    // We don't have a value, so we have to compute using position
    
    // First, compute the from Element
    ElementRef from = getCurrentNode().getElement().getRootElement();
    if ( item.hasAttr(xsl.from()) )
      {
        XPath fromXPath ( getXProcessor(), item, xsl.from() );
        for ( ElementRef ancestor = getCurrentNode().getElement().getFather() ; ancestor ; ancestor = ancestor.getFather() )
          {
            if ( fromXPath.matches ( ancestor ) )
              {
                from = ancestor;
                break;
              }
          }
      }
          
    if ( level == "single" )
      {
        Integer pos = 1;
        if ( item.hasAttr(xsl.count()) )
          {
            XPath countXPath ( getXProcessor(), item, xsl.count() );
            bool found = false;
            for ( ElementRef ancestor = getCurrentNode().getElement() ; 
                ancestor ; ancestor = ancestor.getFather() )
              {
                if ( countXPath.matches ( ancestor ) )
                  {
                    for ( ElementRef elder = ancestor.getElder() ; elder ; elder = elder.getElder() )
                      {
                        if ( countXPath.matches ( elder ) )
                          pos++;
                      }
                    found = true;
                    break;
                  }
                if ( ancestor == from ) break;
              }
            if ( ! found ) return;
          }
        else
          {
            pos = 1;
            for ( ElementRef ancestor = getCurrentNode().getElement() ; 
                ancestor ; ancestor = ancestor.getFather() )
              {
                if ( ancestor.getKeyId() == getCurrentNode().getKeyId() )
                  {
                    for ( ElementRef elder = ancestor.getElder() ; elder ; elder = elder.getElder() )
                      {
                        if ( elder.getKeyId() == getCurrentNode().getKeyId() )
                          pos++;
                      }
                  }
                if ( ancestor == from ) break;                  
              }
          }
        xslFormatNumber ( item, pos );
      }
    else if ( level == "multiple" )
      {
        std::list<Integer> positions;
        if ( item.hasAttr(xsl.count()) )
          {
            XPath countXPath ( getXProcessor(), item, xsl.count() );
            for ( ElementRef ancestor = getCurrentNode().getElement() ; 
                ancestor != from ; ancestor = ancestor.getFather() )
              {
                if ( countXPath.matches ( ancestor ) )
                  {
                    Integer pos = 1;
                    for ( ElementRef elder = ancestor.getElder() ; elder ; elder = elder.getElder() )
                      {
                        if ( countXPath.matches ( elder ) )
                          pos++;
                      }
                    positions.push_front ( pos );
                  }
              }
          }
        else
          {
            for ( ElementRef ancestor = getCurrentNode().getElement() ; 
                ancestor != from ; ancestor = ancestor.getFather() )
              {
                if ( ancestor.getKeyId() == getCurrentNode().getKeyId() )
                  {
                    Integer pos = 1;
                    for ( ElementRef elder = ancestor.getElder() ; elder ; elder = elder.getElder() )
                      {
                        if (  elder.getKeyId() == getCurrentNode().getKeyId()  )
                          pos++;
                      }
                    positions.push_front ( pos );
                  }
              }              
          }
        xslFormatNumber ( item, positions );
      }
    else if ( level == "any" )
      {
        ElementRef current = getCurrentNode().getElement();

        Log_XSLNumbering ( "Any from '%s'\n", current.generateVersatileXPath().c_str() );
        Integer pos = 0;

#define __count(__elt)  \
  if ( countXPath )  \
    { \
      if ( countXPath.matches ( __elt ) ) \
        { \
          pos++; \
          Log_XSLNumbering ( "Counted %s as it matches '%s'\n", __elt.getKey().c_str(), countXPath.getExpression() ); \
        } \
    } \
  else if ( __elt.getKeyId() == getCurrentNode().getKeyId() ) \
    { \
      pos++; \
    }
#define __checkTermination(__elt) \
  if ( fromXPath ) \
    { \
      if ( fromXPath.matches ( __elt ) ) \
        { \
          Log_XSLNumbering ( "Current=%s matches 'from' clause, quitting.\n", \
            __elt.generateVersatileXPath().c_str() ); \
          goto end_of_loop;   \
        } \
    }
        XPath countXPath ( getXProcessor() );
        if ( item.hasAttr(xsl.count()) ) countXPath.init ( item, xsl.count() );

        XPath fromXPath ( getXProcessor() );
        if ( item.hasAttr(xsl.from()) ) fromXPath.init ( item, xsl.from() );

        for ( ElementRef ancestor = current ; ancestor ; ancestor = ancestor.getFather() )
          {
            Log_XSLNumbering ( "pos=%llu, ancestor=%s\n", pos, ancestor.generateVersatileXPath().c_str() );
            __checkTermination ( ancestor );
            __count ( ancestor );
            if ( ancestor.getElder() )
              {
                ElementRef origin = ancestor.getFather();
                ElementRef current = ancestor.getElder();
                while ( true )
                  {
                    Log_XSLNumbering ( "pos=%llu, current=%s / origin=%s\n", pos,
                        current.generateVersatileXPath().c_str(), origin.generateVersatileXPath().c_str() );
                    if ( current.getLastChild() )
                      {
                        current = current.getLastChild();
                        continue;
                      }
                    while ( ! current.getElder() )
                      {
                        __checkTermination ( current );
                        __count ( current );
                        current = current.getFather();
                        if ( !current )
                          break;
                        if ( current == origin )
                          break;
                        __checkTermination ( current );
                      }
                    if ( !current || current == origin )
                      break;
                    if ( ! current.getElder() ) break;
                    __count ( current );
                    current = current.getElder();
                  }
              
              }
          }
end_of_loop:        
        xslFormatNumber ( item, pos );
      }
    else
      {
        throwXSLException ( "Invalid level for xsl:number : '%s'\n", level.c_str() );
      }
  }
  
  
};


