/*
 * xprocessorcmd.h
 *
 *  Created on: 16 janv. 2010
 *      Author: francois
 */

#ifndef __XEM_XPROCESSORCMD_H
#define __XEM_XPROCESSORCMD_H

namespace Xem
{
  /**
   * This bootstrap class is a simple stub to XProcessorLibs, aimed at reducing dependencies
   * required by an external caller
   */
  class XProcessorCmd
  {
  public:

    /**
     * Delegate running main() using a command-line handler
     */
    static int executeCmdLineHandler ( const char* libName, int argc, char** argv );

    /**
     * Get Xem Version running
     */
    static const char* getXemVersion ();
  };
};

#endif /* __XEM_XPROCESSORCMD_H */
