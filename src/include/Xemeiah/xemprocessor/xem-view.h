/*
 * xem-view.h
 *
 *  Created on: 10 nov. 2009
 *      Author: francois
 */

#ifndef __XEM_XEMPROCESSOR_XEMVIEW_H_
#define __XEM_XEMPROCESSOR_XEMVIEW_H_


#include <Xemeiah/dom/documentmeta.h>
#include <Xemeiah/xemprocessor/xemprocessor.h>

namespace Xem
{
  class XemProcessor;

  /**
   *
   */
  class XemView : public ElementRef
  {
  protected:
    XemProcessor& xemProcessor;
    __BUILTIN_NAMESPACE_CLASS(xem)& xem;
  public:
    XemView ( const ElementRef& element, XemProcessor& xemProcessor );
    ~XemView () {}


    bool controlScope ( XProcessor& xproc, NodeRef& base );
  };

};


#endif /* __XEM_XEMPROCESSOR_XEMVIEW_H_ */
