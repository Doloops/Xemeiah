/*
 * xprocessorlib.h
 *
 *  Created on: 5 nov. 2009
 *      Author: francois
 */

#ifndef __XEM_XPROCESSOR_XPROCESSORLIB_H_
#define __XEM_XPROCESSOR_XPROCESSORLIB_H_

#include <Xemeiah/dom/string.h>
#include <Xemeiah/trace.h>
#include <Xemeiah/kern/exception.h>

#include <list>

namespace Xem
{
  class Store;
  class String;
  class XProcessorModuleForge;
  class XProcessorLibs;

  /**
   * The hook to instanciate a ModuleForge
   */
  typedef XProcessorModuleForge* (*XProcessorModuleForge_REGISTER_FUNCTION) ( Store& store );

  /**
   * XProcessorLib contains compile-time information (built before main()) about modules it contains
   */
  class XProcessorLib
  {
    template<typename T> friend class XProcessorLibRegisterModule;
    friend class XProcessorLibRegisterCmdLine;
    friend class XProcessorLibRegisterDependency;
  public:
    /**
     * Type for modules list
     */
    typedef std::list<XProcessorModuleForge_REGISTER_FUNCTION> ModuleForgeConstructorList;

    /**
     * Type of function for command-line handling
     */
    typedef int (*CmdLineHandler) ( int argc, char** argv );

    /**
     * Type of function to construct a lib
     */
    typedef void (*LibConstructor) ( void );

    /**
     * Type for list of dependencies
     */
    typedef std::list<String> Dependencies;
  protected:
    /**
     * The library name
     */
    String libname;

    /**
     * Set handle
     */
    void* handle;

    /**
     * The list of registered module constructors
     */
    ModuleForgeConstructorList moduleForgeConstructorList;

    /**
     * The registered command-line handler
     */
    CmdLineHandler cmdLineHandler;

    /**
     * The list of dependencies to resolve
     */
    Dependencies dependencies;

    /**
     * Register a module
     */
    void registerModule ( XProcessorModuleForge_REGISTER_FUNCTION __register_function );

    /**
     * Register a command-line handler
     */
    void registerCmdLineHandler ( CmdLineHandler cmdLineHandler );

    /**
     * Register a dependency
     */
    void registerDependency ( const char* dependency );
  public:
    /**
     * Simple constructor
     * @param __libname the library unique name
     * @param registerLib set to true to directly and statically register this library in XProcessorLibs
     */
    XProcessorLib(const String& libname, bool registerLib);

    /**
     * Simple destructor
     */
    ~XProcessorLib();

    /**
     * Name accessor
     */
    String& getName() { return libname; }

    /**
     * Get dependencies
     */
    const Dependencies& getDependencies() { return dependencies; }

    /**
     * Get the list of XProcessorModuleForge constructors
     */
    const ModuleForgeConstructorList& getModuleForgeConstructorList() { return moduleForgeConstructorList; }

    /**
     * Checks if this library has a command-line handler
     */
    bool hasCmdLineHandler ( );

    /**
     * Executes command-line handler
     */
    int executeCmdLineHandler ( int argc, char** argv );

    /**
     * Set handle
     */
    void setHandle ( void* handle_ ) { handle = handle_; }

    /**
     * Get handle
     */
    void* getHandle () { return handle; }
  };

  /**
   * The hook template to instanciate a ModuleForge derivation
   */
  template<typename T>
  XProcessorModuleForge* XProcessorModuleForge_REGISTER_FUNCTION_TEMPLATE ( Store& store )
  { return new T(store); }

  /**
   * Hook class to register a module to a given lib
   */
  template<typename T>
  class XProcessorLibRegisterModule
  {
  protected:

  public:
    XProcessorLibRegisterModule(XProcessorLib* &xprocLib, XProcessorLib::LibConstructor constructor )
    {
      if ( !xprocLib ) (*constructor) ();
      xprocLib->registerModule(XProcessorModuleForge_REGISTER_FUNCTION_TEMPLATE<T>);
    }
    ~XProcessorLibRegisterModule() {}
  };

  /**
   * Hook class to register a cmdline handler to a given lib
   */
  class XProcessorLibRegisterCmdLine
  {
  public:
    XProcessorLibRegisterCmdLine(XProcessorLib* &xprocLib, XProcessorLib::LibConstructor constructor, XProcessorLib::CmdLineHandler cmdLineHandler )
    {
      if ( !xprocLib ) (*constructor) ();
      xprocLib->registerCmdLineHandler(cmdLineHandler);
    }
    ~XProcessorLibRegisterCmdLine() {}
  };

  /**
   * Hook class to register a dependency between XProcessorLib
   */
  class XProcessorLibRegisterDependency
  {
  public:
    XProcessorLibRegisterDependency ( XProcessorLib* &xprocLib, XProcessorLib::LibConstructor constructor, const char* dependency )
    {
      if ( !xprocLib ) (*constructor) ();
      xprocLib->registerDependency(dependency);
    }
    ~XProcessorLibRegisterDependency() {}
  };

  /**
   * Hook called when the XProcessorLib is to be destructed (library unload)
   */
  class XProcessorLibDestructorHook
  {
  protected:
    XProcessorLib* &instanciatedLib;
  public:
    XProcessorLibDestructorHook ( XProcessorLib* &instanciatedLib_ )
    : instanciatedLib ( instanciatedLib_ ) {}

    ~XProcessorLibDestructorHook()
    {
      AssertBug ( instanciatedLib, "Library not instanciatied !\n" );
      Debug ( "Destructing library '%s'\n", instanciatedLib->getName().c_str() );
      XProcessorLib* lib = instanciatedLib;
      instanciatedLib = NULL;
      delete ( lib );
    }
  };

#define __XProcessorLib_DECLARE_LIB_GENERIC__(__LIB_NAME,__lib_file_name,__register_lib) \
                XProcessorLib* __XProcessorLib_REGISTERED_LIB__##__LIB_NAME = NULL; \
                XProcessorLibDestructorHook __XProcessorLib_REGISTERED_LIB__##__LIB_NAME##__DESTRUCTOR \
                  ( __XProcessorLib_REGISTERED_LIB__##__LIB_NAME ); \
                void __XProcessorLib_REGISTERED_LIB__##__LIB_NAME##__CONSTRUCTOR() \
                { __XProcessorLib_REGISTERED_LIB__##__LIB_NAME = new XProcessorLib(__lib_file_name,__register_lib); }

#define __XProcessorLib_DECLARE_LIB_INTERNAL(__LIB_NAME,__lib_file_name) \
                __XProcessorLib_DECLARE_LIB_GENERIC__(__LIB_NAME,__lib_file_name, true)

#ifdef __XEM_INTERNALIZE_LIBS
#define __XProcessorLib_DECLARE_LIB __XProcessorLib_DECLARE_LIB_INTERNAL
#else
#define __XProcessorLib_DECLARE_LIB(__LIB_NAME,__lib_file_name) \
  __XProcessorLib_DECLARE_LIB_GENERIC__(__LIB_NAME,__lib_file_name, true) \
  extern "C" { void* __XProcessorLib_GET_LIB()  \
    { return (void*) __XProcessorLib_REGISTERED_LIB__##__LIB_NAME; } }
#endif

#define __XProcessorLib_DEFINE_EXTERN_SYMBOLS(__LIB_NAME) \
  extern XProcessorLib* __XProcessorLib_REGISTERED_LIB__##__LIB_NAME; \
  extern void __XProcessorLib_REGISTERED_LIB__##__LIB_NAME##__CONSTRUCTOR()

#define __XProcessorLib_REGISTER_MODULE(__LIB_NAME,__class) \
  __XProcessorLib_DEFINE_EXTERN_SYMBOLS(__LIB_NAME); \
  XProcessorLibRegisterModule<__class> __##__LIB_NAME##__class##__REGISTER \
  (__XProcessorLib_REGISTERED_LIB__##__LIB_NAME, &__XProcessorLib_REGISTERED_LIB__##__LIB_NAME##__CONSTRUCTOR );

#define __XProcessorLib_REGISTER_CMDLINEHANDLER(__LIB_NAME,__handler) \
  __XProcessorLib_DEFINE_EXTERN_SYMBOLS(__LIB_NAME); \
  XProcessorLibRegisterCmdLine __##__LIB_NAME##__handler##__REGISTER \
  (__XProcessorLib_REGISTERED_LIB__##__LIB_NAME, &__XProcessorLib_REGISTERED_LIB__##__LIB_NAME##__CONSTRUCTOR, &__handler );

#define __XProcessorLib_REGISTER_DEPENDENCY(__LIB_NAME,__dependency) \
  __XProcessorLib_DEFINE_EXTERN_SYMBOLS(__LIB_NAME); \
  XProcessorLibRegisterDependency __##__LIB_NAME##__LINE__##__DEP__##__REGISTER \
  (__XProcessorLib_REGISTERED_LIB__##__LIB_NAME, &__XProcessorLib_REGISTERED_LIB__##__LIB_NAME##__CONSTRUCTOR, __dependency );

};

#endif /* __XEM_XPROCESSOR_XPROCESSORLIB_H_ */
