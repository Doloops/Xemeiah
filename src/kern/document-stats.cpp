#include <Xemeiah/kern/document.h>

#include <Xemeiah/auto-inline.hpp>

namespace Xem
{
  Document::Stats::Stats ()
  {
    numberOfXPathParsed = 0;
    numberOfXPathInstanciated = 0;    
  }
  
  Document::Stats::~Stats ()
  {

  }
  
  void Document::Stats::showStats ()
  {
    Info ( "Document Stats : \n" );
    Info ( "\tnumberOfXPathParsed=%llu\n", numberOfXPathParsed );
    Info ( "\tnumberOfXPathInstanciated=%llu\n", numberOfXPathInstanciated );
  }
};
