#ifndef __XEM_XPROCESSOR_XPROCESSORLIBS_H
#define __XEM_XPROCESSOR_XPROCESSORLIBS_H

#include <Xemeiah/kern/format/core_types.h>

#include <map>
#include <list>
/**
 * Utility class for the Store to keep trace of the XProcessorModule
 */
namespace Xem
{
  class Store;
  class XProcessor;
  class XProcessorModuleForge;
  class XProcessorLib;
  class Document;
  class String;
  
  /**
   * The stored modules : the list of forges we could have in a store, that we have at our disposal for inclusion in XProcessor
   */
  class XProcessorLibs
  {
    /**
     * Only the static register function is allowed to register a XProcessorLib in the static list registery
     * This way, we do not make friends with class XProcessorLib
     */
    friend void __XProcessorLibs_registerLib(XProcessorLib* lib);

    /**
     * Only the static unregister function is allowed to unregister a XProcessorLib in the static list registery
     * This way, we do not make friends with class XProcessorLib
     */
    friend void __XProcessorLibs_unregisterLib(XProcessorLib* lib);

  protected:
    /**
     * Our reference to the store
     */
    Store& store;
    
    /**
     * The forge map : the map of all the namespaces we have at our disposal
     */
    typedef std::map<NamespaceId,XProcessorModuleForge*> ForgeMap;

    /**
     * The forge map instance : the map of all the namespaces we have at our disposal
     */    
    ForgeMap forgeMap;

  public:
    /**
     * The list of modules capable of handling an Event Trigger
     */
    typedef std::list<NamespaceId> EventTriggerHandlerModules;

  protected:
    /**
     * Instance : The list of modules capable of handling an Event Trigger
     */
    EventTriggerHandlerModules eventTriggerHandlerModules;

    /**
     * The XProcessorLibInstance class, as a per-XProcessorLibs instance of XProcessorLib
     */
    class XProcessorLibInstance
    {
    protected:
      /**
       * Reference to our XProcessorLib
       */
      XProcessorLib* xprocessorLib;

      /**
       * Reference to our XProcessorLibs container
       */
      XProcessorLibs& xprocessorLibs;

      /**
       *
       */
      typedef std::list<XProcessorModuleForge*> ForgeList;

      /**
       *
       */
      ForgeList forgeList;
    public:
      /**
       * Constructor
       */
      XProcessorLibInstance(XProcessorLibs& xprocessorLibs, XProcessorLib* xprocessorLib_);

      /**
       * Destructor
       */
      ~XProcessorLibInstance();

      /**
       *
       */
      XProcessorLib& getXProcessorLib() const { return *xprocessorLib; }

      /**
       * Instanciate all modules defined in this Library to the provided xprocessor
       */
      void instanciateModules ( XProcessor& xproc );

      /**
       * Dump Extensions for this library
       */
      void dumpExtensions();
    };

    /**
     * The libs contents map : set which set of NamespaceIds each lib implements
     */
    typedef std::map<String, XProcessorLibInstance*> XProcessorLibInstanceMap;

    /**
     * The libs contents map instance
     */
    XProcessorLibInstanceMap libInstanceMap;

    /**
     * Statically register a library in XProcessorLibs
     */
    static void staticRegisterLib ( XProcessorLib* lib );

    /**
     * Statically unregister a library in XProcessorLibs
     */
    static void staticUnregisterLib ( XProcessorLib* lib );

    /**
     * Statically load a library in XProcessorLibs
     */
    static XProcessorLib* staticLoadLibrary ( const String& libName );

    /**
     * Per-instance register a library
     */
    XProcessorLibInstance* registerLib ( XProcessorLib* lib );

    /**
     * Try to load library based on its name
     */
    XProcessorLibInstance* doLoadLibrary ( const String& libName );
  public:
    /**
     * XProcessorLibs constructor, bound to a Store
     */
    XProcessorLibs(Store& store);
    
    /**
     * Default destructor
     */
    ~XProcessorLibs();
    
    /** 
     * Reference to our store
     */
    Store& getStore() const { return store; }

    /**
     * Register static libs
     */
    void registerStaticLibs ();

    /**
     * Register a single forge on the list
     */
    void registerModuleForge ( NamespaceId nsId, XProcessorModuleForge* forge );

    /**
     * Unregister a single forge on the list
     */
    void unregisterModuleForge ( NamespaceId nsId, XProcessorModuleForge* forge );

    /**
     * Install all modules
     */
    void installAllModules ( XProcessor& xprocessor );

    /**
     * Load external module library
     * @param libName the library to load
     * @param xproc optional XProcessor to install modules to
     */
    void loadLibrary ( const String& libName, XProcessor* xproc = NULL );

    /**
     * Get Module Forge from its NamespaceId
     */
    XProcessorModuleForge* getModuleForge ( NamespaceId nsId );

    /**
     * Register a Namespace XProcessor Module as capable of handling an event trigger
     */
    void registerModuleAsEventTriggerHandler ( NamespaceId nsId );

    /**
     * Get a list of modules capable of handling an event trigger
     */
    const EventTriggerHandlerModules& getEventTriggerHandlerModules ();

    /**
     * Register events for all the modules we have instanciated yet
     */
    void registerEvents ( Document& doc );

    /**
     * Dump registered extensions (libraries, instructions and functions)
     */
    void dumpExtensions ();

    /**
     * Get Xem Version running
     */
    static const char* getXemVersion ();

    /**
     * Delegate running main() using a command-line handler
     */
    static int executeCmdLineHandler ( const String& libName, int argc, char** argv );

    /**
     * Delegate running main() using a command-line handler
     */
    static int executeCmdLineHandler ( const char* libName, int argc, char** argv );
  };
};
 
#endif // __XEM_XPROCESSOR_XPROCESSORSTOREDMODULES_H
