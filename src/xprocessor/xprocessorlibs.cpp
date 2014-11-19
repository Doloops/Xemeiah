#include <Xemeiah/kern/store.h>
#include <Xemeiah/xprocessor/xprocessorlibs.h>
#include <Xemeiah/xprocessor/xprocessorlib.h>
#include <Xemeiah/xprocessor/xprocessormoduleforge.h>
#include <Xemeiah/version.h>

#include <Xemeiah/auto-inline.hpp>

#ifdef __XEM_HAS_DL
#include <dlfcn.h>
#endif
#include <errno.h>
#include <string.h>

#define Log_XProcessorLibs Debug

namespace Xem
{
  XProcessorLibs::XProcessorLibInstance::XProcessorLibInstance(XProcessorLibs& xprocessorLibs_, XProcessorLib* xprocessorLib_)
  : xprocessorLibs(xprocessorLibs_)
  {
    xprocessorLib = xprocessorLib_;
    const XProcessorLib::ModuleForgeConstructorList& constructors = xprocessorLib->getModuleForgeConstructorList();
    for ( XProcessorLib::ModuleForgeConstructorList::const_iterator iter = constructors.begin() ;
        iter != constructors.end() ; iter++ )
      {
        XProcessorModuleForge_REGISTER_FUNCTION registerFunction = *iter;
        XProcessorModuleForge* forge = (*registerFunction) ( xprocessorLibs.getStore() );
        forgeList.push_back(forge);
        xprocessorLibs.registerModuleForge(forge->getModuleNamespaceId(), forge);
        forge->install();
      }
  }

  XProcessorLibs::XProcessorLibInstance::~XProcessorLibInstance()
  {
    for ( ForgeList::iterator iter = forgeList.begin() ; iter != forgeList.end() ; iter++ )
      {
        xprocessorLibs.unregisterModuleForge((*iter)->getModuleNamespaceId(), *iter);
        delete ( *iter );
      }
    forgeList.clear();
    if ( xprocessorLib->getHandle() && ! xprocessorLib->hasCmdLineHandler() )
      {
        void* dlLibHandle = xprocessorLib->getHandle();
        // delete ( xprocessorLib );
        Log_XProcessorLibs ( "Delete instance of xprocessorLib %s\n", xprocessorLib->getName().c_str() );
        Log_XProcessorLibs ( "Delete instance : dlclose(%p)\n", dlLibHandle );
        xprocessorLib = NULL;
#ifdef __XEM_HAS_DL
        if ( dlclose(dlLibHandle) )
          {
            Error ( "Could not dlclose ! err=%s\n", dlerror() );
          }
#endif
      }
  }

  void XProcessorLibs::XProcessorLibInstance::instanciateModules ( XProcessor& xproc )
  {
    for ( ForgeList::iterator iter = forgeList.begin() ; iter != forgeList.end() ; iter++ )
      {
        (*iter)->instanciateModule(xproc);
      }
  }

  void XProcessorLibs::XProcessorLibInstance::dumpExtensions()
  {
    Info ( "Library '%s' (%s) :\n", xprocessorLib->getName().c_str(),
        xprocessorLib->getHandle() ? "external" : "builtin" );

    for ( ForgeList::iterator iter = forgeList.begin() ; iter != forgeList.end() ; iter++ )
      {
        XProcessorModuleForge* forge = *iter;
        forge->dumpExtensions();
      }
  }

  typedef std::list<XProcessorLib*> XProcessorLibList;
  static XProcessorLibList* staticXProcessorLibList = NULL;

  void XProcessorLibs::staticRegisterLib ( XProcessorLib* lib )
  {
    Log_XProcessorLibs ( "Registering static library %p : '%s'\n", lib, lib->getName().c_str() );
    if ( !staticXProcessorLibList )
      {
        staticXProcessorLibList = new XProcessorLibList();
      }
    for ( XProcessorLibList::iterator iter = staticXProcessorLibList->begin() ;
            iter != staticXProcessorLibList->end() ; iter++ )
      {
        AssertBug ( *iter != lib, "Register static library %s : XProcessorLib at %p already registered !\n",
              lib->getName().c_str(), lib );
      }
    staticXProcessorLibList->push_back ( lib );
  }

  void XProcessorLibs::staticUnregisterLib ( XProcessorLib* lib )
  {
    AssertBug ( staticXProcessorLibList,
        "Unregister static library %s : no staticXProcessorLibList defined !\n",
        lib->getName().c_str() );
    for ( XProcessorLibList::iterator iter = staticXProcessorLibList->begin() ;
            iter != staticXProcessorLibList->end() ; iter++ )
      {
        if ( *iter == lib )
          {
            Log_XProcessorLibs ( "Unregister : %s\n", lib->getName().c_str() );
            staticXProcessorLibList->erase(iter);
            return;
          }
      }
   Bug ( "Unregister static library %s : not registered in staticXProcessorLibList !\n",
        lib->getName().c_str() );
  }

  XProcessorLibs::XProcessorLibs ( Store& _store )
  : store(_store)
  {
    Log_XProcessorLibs ( "Instanciate XProcessorLibs at %p for store %p\n", this, &store );
  }

  void XProcessorLibs::registerStaticLibs ()
  {
    if ( staticXProcessorLibList )
      {
        for ( XProcessorLibList::iterator iter = staticXProcessorLibList->begin() ;
            iter != staticXProcessorLibList->end() ; iter++ )
          {
            XProcessorLib* lib = *iter;
            registerLib(lib);
          }
      }
  }
  
  XProcessorLibs::~XProcessorLibs ()
  {
#if 0
    for ( ForgeMap::iterator iter = forgeMap.begin() ; iter != forgeMap.end() ; iter++ )
      {
        Log_XProcessorLibs ( "Unregister moduleForge %s (%x)\n",
            iter->second->getStore().getKeyCache().getNamespaceURL(forge->getModuleNamespaceId()),
            iter->second->getModuleNamespaceId() );
        // delete ( forge );
      }
#endif
    for ( XProcessorLibInstanceMap::iterator iter = libInstanceMap.begin() ; iter != libInstanceMap.end() ; iter++ )
      {
        Log_XProcessorLibs ( "Unregister XProcessorLib instance '%s' at %p\n",
            iter->first.c_str(), iter->second );
        delete ( iter->second );
      }
    libInstanceMap.clear();
  }

  XProcessorLibs::XProcessorLibInstance* XProcessorLibs::registerLib ( XProcessorLib* lib )
  {
    AssertBug ( lib, "Null library provided !\n" );
    AssertBug ( libInstanceMap[lib->getName()] == NULL, "Already created libContents !\n" );
    XProcessorLibInstance* libInstance = new XProcessorLibInstance(*this, lib);
    libInstanceMap[lib->getName()] = libInstance;
    // lib->registerToXProcessorLibs(*this);
    return libInstance;
  }
  

  XProcessorModuleForge* XProcessorLibs::getModuleForge ( NamespaceId nsId )
  {
    ForgeMap::iterator iter = forgeMap.find(nsId);
    if ( iter == forgeMap.end() )
      {
        return NULL;
      }
    return iter->second;
  }
  
  void XProcessorLibs::registerModuleForge ( NamespaceId nsId, XProcessorModuleForge* forge )
  {
    if ( forgeMap.find(nsId) != forgeMap.end() )
      {
        throwException ( Exception, "Duplicate forge registering !\n" );
      }
    forgeMap[nsId] = forge;
  }

  void XProcessorLibs::unregisterModuleForge ( NamespaceId nsId, XProcessorModuleForge* forge )
  {
    ForgeMap::iterator iter = forgeMap.find(nsId);
    if ( iter == forgeMap.end() )
      {
        throwException ( Exception, "Unregistered Forge !\n" );
      }
    forgeMap.erase(iter);
  }

  void XProcessorLibs::installAllModules ( XProcessor& xprocessor )
  {
    for ( ForgeMap::iterator iter = forgeMap.begin () ; iter != forgeMap.end () ; iter++ )
      {
        Log_XProcessorLibs ( "Registering for '%s'\n", xprocessor.getKeyCache().getNamespaceURL(iter->first) );
        xprocessor.installModule ( iter->first ); 
      }
  }

  void XProcessorLibs::registerModuleAsEventTriggerHandler ( NamespaceId nsId )
  {
    eventTriggerHandlerModules.push_back(nsId);
    eventTriggerHandlerModules.unique();
  }

  const XProcessorLibs::EventTriggerHandlerModules& XProcessorLibs::getEventTriggerHandlerModules ()
  {
    return eventTriggerHandlerModules;
  }

  void XProcessorLibs::registerEvents ( Document& doc )
  {
    for ( ForgeMap::iterator iter = forgeMap.begin () ; iter != forgeMap.end () ; iter++ )
      {
        Log_XProcessorLibs ( "Registering Document Events for '%s' on document %p (%s)\n",
            store.getKeyCache().getNamespaceURL(iter->first),
            &doc, doc.getDocumentURI().c_str() );
        AssertBug ( iter->second, "Null Forge for '%s' !\n", store.getKeyCache().getNamespaceURL(iter->first) );
        XProcessorModuleForge* forge = iter->second;
        forge->registerEvents(doc);
      }
  }

  XProcessorLib* XProcessorLibs::staticLoadLibrary ( const String& libName )
  {
    /*
     * We must first check that it was not loaded in the static stuff
     */
    if ( staticXProcessorLibList )
      {
        for ( XProcessorLibList::iterator iter = staticXProcessorLibList->begin() ;
            iter != staticXProcessorLibList->end() ; iter++ )
          {
            XProcessorLib* lib = *iter;
            if ( lib->getName() == libName )
              {
                Log_XProcessorLibs ( "Found library '%s' in the staticXProcessorLibList\n", lib->getName().c_str() );
                return lib;
              }
          }
      }
#ifdef __XEM_HAS_DL
    const char* libLocations[] = { "xemeiah/libxem-", "/usr/lib/xemeiah/libxem-", "libxem-", NULL };

    for ( int i = 0 ; libLocations[i] ; i++ )
      {
        void* dlLibHandle = NULL;

        String path = libLocations[i];
        path += libName;
        path += ".so";

        Log_XProcessorLibs ( "Trying to open '%s' for lib '%s'\n", path.c_str(), libName.c_str() );

        int mode = RTLD_GLOBAL;
        mode |= RTLD_LAZY;
        // mode |= RTLD_NOW;
        dlLibHandle = dlopen ( path.c_str(), mode );

        if ( dlLibHandle == NULL )
          {
            // throwException ( Exception, "Could not open lib '%s'\n", libName.c_str() );
            Log_XProcessorLibs /* Warn */
              ( "Could not load lib '%s' (for libName='%s') : error=%s\n", path.c_str(), libName.c_str(), dlerror() );
            continue;
          }
        Log_XProcessorLibs ( "Openned lib : dlopen(%s,%d)=%p\n", path.c_str(), mode, dlLibHandle );

        void* dlLibGetSymbol = dlsym ( dlLibHandle, "__XProcessorLib_GET_LIB" );

        if ( dlLibGetSymbol == NULL )
          {
            if ( dlclose ( dlLibHandle ) )
              {
                Error ( "While on exception, could not close library '%s'\n", libName.c_str() );
              }
            // throwException ( Exception, "Could not get library getter symbol for lib '%s'\n", libName.c_str() );
            Warn ( "Could not get library getter symbol for lib '%s'\n", libName.c_str() );
            continue;
          }

        Log_XProcessorLibs ( "Library getter symbol at %p\n", dlLibGetSymbol );

        void* (*getter) ();
        getter = (void* (*) ()) dlLibGetSymbol;
        XProcessorLib* lib = (XProcessorLib*) (*getter) ();

        Log_XProcessorLibs ( "Library at %p\n", lib );

        if ( lib->getName() != libName )
          {
            Warn ( "Different library names : wanted '%s', loaded '%s'\n",
                libName.c_str(), lib->getName().c_str() );
          }
        lib->setHandle(dlLibHandle);
        // staticRegisterLib(lib);
        return lib;
      }
#else
    Warn ( "Dynamic library loading disabled at compile time.\n" );
#endif
    Warn ( "Could not load Xemeiah XProccesor library '%s'\n", libName.c_str() );
    return NULL;
  }

  XProcessorLibs::XProcessorLibInstance* XProcessorLibs::doLoadLibrary ( const String& libName )
  {
    XProcessorLibInstanceMap::iterator iter = libInstanceMap.find(libName);
    if ( iter != libInstanceMap.end() )
      {
        AssertBug ( iter->second, "NULL Intance !\n" );
        return iter->second;
      }
    XProcessorLib* lib = staticLoadLibrary(libName);
    if ( ! lib )
      {
        return NULL;
      }
    return registerLib(lib);
  }

  void XProcessorLibs::loadLibrary ( const String& libName, XProcessor* xproc )
  {
    if ( ! libName.size() ) throwException ( Exception, "Invalid null module name !\n" );

    XProcessorLibInstance* libInstance = doLoadLibrary ( libName );

    if ( ! libInstance )
      {
        throwException ( Exception, "Could not load library '%s'\n", libName.c_str() );
      }

    const XProcessorLib::Dependencies dependencies = libInstance->getXProcessorLib().getDependencies();

    for ( XProcessorLib::Dependencies::const_iterator iter = dependencies.begin() ; iter != dependencies.end() ; iter++ )
      {
        const String& dependency = *iter;
        Log_XProcessorLibs ( "Running dependencies : %s\n", dependency.c_str() );
        loadLibrary(dependency, xproc);
      }

    if ( xproc )
      {
        try
        {
          libInstance->instanciateModules(*xproc);
        }
        catch ( Exception * e )
        {
          detailException(e, "Could not load library '%s'\n", libName.c_str() );
          throw;
        }
      }
  }

  void XProcessorLibs::dumpExtensions ()
  {
    for ( XProcessorLibInstanceMap::iterator iter = libInstanceMap.begin() ; iter != libInstanceMap.end() ; iter++ )
      {
        iter->second->dumpExtensions ();
      }
  }

  const char* XProcessorLibs::getXemVersion ()
  {
    return __XEM_VERSION;
  }

  int XProcessorLibs::executeCmdLineHandler ( const String& libName, int argc, char** argv )
  {
    XProcessorLib* lib = XProcessorLibs::staticLoadLibrary(libName);
    if ( ! lib )
      {
        Error ( "Could not load library '%s'\n", libName.c_str() );
        return 1;
      }
    if ( ! lib->hasCmdLineHandler() )
      {
        Error ( "Library '%s' has no command-line handler !\n", libName.c_str() );
        return 1;
      }
    return lib->executeCmdLineHandler(argc, argv);
  }

  int XProcessorLibs::executeCmdLineHandler ( const char* libName, int argc, char** argv )
  {
    String sLibName(libName);
    return executeCmdLineHandler(sLibName, argc, argv);
  }
};
