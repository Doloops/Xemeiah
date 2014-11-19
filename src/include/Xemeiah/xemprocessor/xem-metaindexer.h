/*
 * xem-metaindexer.h
 *
 *  Created on: 10 nov. 2009
 *      Author: francois
 */

#ifndef __XEM_XEMPROCESSOR_METAINDEXER_H_
#define __XEM_XEMPROCESSOR_METAINDEXER_H_

#include <Xemeiah/dom/documentmeta.h>
#include <Xemeiah/xemprocessor/xemprocessor.h>

namespace Xem
{
  class XemProcessor;

  /**
   *
   */
  class MetaIndexer : public ElementRef
  {
  protected:
    XemProcessor& xemProcessor;
    __BUILTIN_NAMESPACE_CLASS(xem)& xem;
  public:
    MetaIndexer ( const ElementRef& element, XemProcessor& xemProcessor );
    ~MetaIndexer () {}

    void setXPath ( XProcessor& xproc, XPathParser& matchXPath, XPathParser& useXPath, XPathParser& scopeXPath );
    void build ( XProcessor& xproc );
    void eval ( XProcessor& xproc, NodeSet& result, ElementRef& baseElement, NodeSet& args );

    ElementMultiMapRef getScope ( XProcessor& xproc, NodeRef& base, bool create );
  };

};

#endif /* __XEM_XEMPROCESSOR_METAINDEXER_H_ */
