#ifndef __XEM_XSL_NUMBERING_H
#define __XEM_XSL_NUMBERING_H

#include <Xemeiah/xsl/xslprocessor.h>
#include <Xemeiah/xpath/xpathparser.h>

namespace Xem
{
  /**
   * XSLNumbering : stores the xsl:format attribute of an xsl:number element as an XPath expression
   * \todo xsl:number must be more integrated to XPath expressions.
   */
  class XSLNumbering : public XPathParser
  {
  protected:
  
  public:
    XSLNumbering ();
    ~XSLNumbering ();
    
    void parse ( const String& format, char groupingSeparator, __ui32 groupingSize );
  };

  void xslInstructionNumber ( __XProcHandlerArgs__ );
};

#endif // __XEM_XSL_NUMBERING_H

