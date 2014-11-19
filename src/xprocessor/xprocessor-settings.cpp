#include <Xemeiah/xprocessor/xprocessor.h>

#include <Xemeiah/auto-inline.hpp>

namespace Xem
{
  Env::EnvSettings::EnvSettings ()
  {
    documentAllocatorMaximumAge = 32;
  }

  Env::EnvSettings::~EnvSettings ()
  {
  
  }

  XProcessor::Settings::Settings ()
  {
    xpathChildLookupEnabled = 0;
    xpathChildLookupThreshold = 100;
    autoInstallModules = true;
  }
  
  XProcessor::Settings::~Settings ()
  {
  
  
  }





};

