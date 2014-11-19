/*
 * xprocessorcmd.cpp
 *
 *  Created on: 16 janv. 2010
 *      Author: francois
 */

#include <Xemeiah/xprocessor/xprocessorcmd.h>
#include <Xemeiah/xprocessor/xprocessorlibs.h>
#include <Xemeiah/auto-inline.hpp>

namespace Xem
{
  int XProcessorCmd::executeCmdLineHandler ( const char* libName, int argc, char** argv )
  {
    return XProcessorLibs::executeCmdLineHandler(libName, argc, argv);
  }

  const char* XProcessorCmd::getXemVersion ()
  {
    return XProcessorLibs::getXemVersion();
  }
};
