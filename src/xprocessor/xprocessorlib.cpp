/*
 * xprocessorlib.cpp
 *
 *  Created on: 5 nov. 2009
 *      Author: francois
 */

#include <Xemeiah/xprocessor/xprocessorlib.h>
#include <Xemeiah/xprocessor/xprocessorlibs.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_XProcessorLib Debug

namespace Xem
{
  void __XProcessorLibs_registerLib(XProcessorLib* lib)
  {
    XProcessorLibs::staticRegisterLib(lib);
  }

  void __XProcessorLibs_unregisterLib(XProcessorLib* lib)
  {
    XProcessorLibs::staticUnregisterLib(lib);
  }

  XProcessorLib::XProcessorLib(const String& __libname, bool registerLib)
  {
    libname = __libname;
    cmdLineHandler = NULL;
    Log_XProcessorLib ( "Register lib %p '%s'\n", this, getName().c_str() );
    AssertBug ( registerLib, "registerLib=true is mandatory now !\n" );
    if ( registerLib )
      {
        __XProcessorLibs_registerLib ( this );
      }
  }

  XProcessorLib::~XProcessorLib()
  {
    Log_XProcessorLib ( "Delete XProcessorLib '%s'\n", getName().c_str() );
    __XProcessorLibs_unregisterLib ( this );
  }

  void XProcessorLib::registerModule ( XProcessorModuleForge_REGISTER_FUNCTION __register_function )
  {
    Log_XProcessorLib ( "Register libModule for lib %p '%s' at %p\n", this, getName().c_str(), __register_function );
    moduleForgeConstructorList.push_back(__register_function);
  }

  void XProcessorLib::registerCmdLineHandler ( CmdLineHandler __cmdLineHandler )
  {
    Log_XProcessorLib ( "Register cmldLineHandler for lib %p '%s' at %p\n", this, getName().c_str(), __cmdLineHandler );
    AssertBug ( cmdLineHandler == NULL, "Command-line handler already registered !\n" );
    cmdLineHandler = __cmdLineHandler;
  }

  void XProcessorLib::registerDependency ( const char* dependency )
  {
    Log_XProcessorLib ( "Register dependency : '%s'\n", dependency );
    dependencies.push_back(stringFromAllocedStr(strdup(dependency)));
  }

  bool XProcessorLib::hasCmdLineHandler ( )
  {
    return (cmdLineHandler != NULL);
  }

  int XProcessorLib::executeCmdLineHandler ( int argc, char** argv )
  {
    AssertBug ( hasCmdLineHandler(), "Library '%s' has no command-line handler !\n", getName().c_str() );
    return (*cmdLineHandler) ( argc, argv );
  }
};

