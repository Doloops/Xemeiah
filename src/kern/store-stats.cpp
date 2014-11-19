#include <Xemeiah/kern/store.h>

#include <Xemeiah/auto-inline.hpp>

namespace Xem
{
  Store::Stats::Stats()
  {
    numberOfXPathParsed = 0;
    numberOfXPathInstanciated = 0;
    numberOfStaticXPathParsed = 0;
    numberOfVolatileAreasProvided = 0;
    numberOfVolatileAreasCreated = 0;
    numberOfVolatileAreasDeleted = 0;
  }
  
  Store::Stats::~Stats()
  {
  
  }

  void Store::Stats::showStats ()
  {
    Info ( "Store stats :\n" );
    Info ( "\tnumberOfXPathParsed=%llu\n", numberOfXPathParsed );
    Info ( "\tnumberOfXPathInstanciated=%llu\n", numberOfXPathInstanciated );
    Info ( "\tnumberOfStaticXPathParsed=%llu\n", numberOfStaticXPathParsed );
    Info ( "\tnumberOfVolatileAreasProvided=%llu\n", numberOfVolatileAreasProvided );
    Info ( "\tnumberOfVolatileAreasCreated=%llu\n", numberOfVolatileAreasCreated );    
    Info ( "\tnumberOfVolatileAreasDeleted=%llu\n", numberOfVolatileAreasDeleted );
  }
};
