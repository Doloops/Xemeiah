/*
 * xemservice.cpp
 *
 *  Created on: 5 nov. 2009
 *      Author: francois
 */

#include <Xemeiah/xemprocessor/xemservice.h>
#include <Xemeiah/xemprocessor/xemprocessor.h>
#include <Xemeiah/xemprocessor/xemservicemodule.h>
#include <Xemeiah/kern/branchmanager.h>
#include <Xemeiah/kern/documentref.h>
#include <Xemeiah/dom/childiterator.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_XemService Debug

namespace Xem
{
#if 0
  XemService::XemService(const ElementRef& _configurationElement)
  : configurationElement(_configurationElement)
  {
    configurationElement.getDocument().incrementRefCount ();
    if ( configurationElement.getDocument().getRole() == "none" )
      {
        throwException ( Exception, "configurationElement document has role none !\n" );
      }
    XemProcessor& xemProcessor = XemProcessor::getMe(xproc);
    if ( !configurationElement.hasAttr(xemProcessor.xem.codescope_branch() ) )
      {
        throwException ( Exception, "No code scope set !\n" );
      }
  }
#endif

  XemService::XemService ( XProcessor& xproc, const ElementRef& _configurationElement )
  : configurationElement(_configurationElement)
  {
    configurationElement.getDocument().incrementRefCount ();
    if ( configurationElement.getDocument().getRole() == "none" )
      {
        throwException ( Exception, "configurationElement document has role none !\n" );
      }
    XemProcessor& xemProcessor = XemProcessor::getMe(xproc);
    if ( !configurationElement.hasAttr(xemProcessor.xem.codescope_branch() ) )
      {
        BranchRevId codeScopeBrId = xemProcessor.getCurrentCodeScope().getDocument().getBranchRevId();
        String branchName = configurationElement.getStore().getBranchManager().getBranchName(codeScopeBrId.branchId);
        Info ( "XemService : setting codeScope to '%s'\n", branchName.c_str() );
        defaultCodeScopeBranchName = branchName.copy();
      }
  }

  XemService::~XemService()
  {
    configurationElement.getDocument().decrementRefCount ();
  }

  Store& XemService::getStore() const
  {
    return configurationElement.getDocument().getStore();
  }

  ElementRef& XemService::getConfigurationElement()
  {
    return configurationElement;
  }

  ElementRef XemService::getXProcessorDefaultCurrentNode ()
  {
    return configurationElement;
  }

  String XemService::getXProcessorDefaultCodeScopeBranchName ()
  {
    if ( defaultCodeScopeBranchName.c_str() && *defaultCodeScopeBranchName.c_str() )
      {
        return defaultCodeScopeBranchName;
      }
    XemProcessor& xemProc = XemProcessor::getMe ( getPerThreadXProcessor() );
    return configurationElement.getAttr(xemProc.xem.codescope_branch());
  }

  void XemService::initXProcessor ( XProcessor& xproc )
  {
    String codeScopeBranchName = getXProcessorDefaultCodeScopeBranchName();
    bindCodeScopeDocument(xproc, codeScopeBranchName );
  }

  void XemService::bindCodeScopeDocument ( XProcessor& xproc, const String& codeScopeBranchName )
  {
    BranchId branchId = xproc.getStore().getBranchManager().getBranchId ( codeScopeBranchName );
    if ( ! branchId )
      {
        throwException ( Exception, "Could not get codeScope branch '%s'\n", codeScopeBranchName.c_str() );
      }

    KeyId roleId = xproc.getKeyCache().getKeyId(0,"code",true);
    Document* codeScopeDocument = xproc.getStore().getBranchManager().openDocument ( branchId, 0, DocumentOpeningFlags_ExplicitRead, roleId );
    if ( codeScopeDocument == NULL )
      {
        throwException ( Exception, "Could not get codeScope document, branch name '%s'\n", codeScopeBranchName.c_str() );
      }
    xproc.bindDocument ( codeScopeDocument, false );

    ElementRef codeScopeRoot = codeScopeDocument->getRootElement ();

    XemProcessor& xemProc = XemProcessor::getMe ( xproc );
    xproc.setElement ( xemProc.xem_role.code(), codeScopeRoot );
    xemProc.setCurrentCodeScope ( codeScopeRoot );
  }

  void XemService::registerMyself ( XProcessor& xproc )
  {
    XemServiceModule& xemServiceModule = XemServiceModule::getMe ( xproc );
    String serviceName = configurationElement.getEvaledAttr(xproc, xemServiceModule.xem_service.name());
    if ( ! serviceName.size() )
      {
        throwException ( Exception, "Invalid service name '%s'\n", serviceName.c_str() );
      }
    Log_XemService ( "Registering service '%s'\n", serviceName.c_str() );
    getStore().getServiceManager().registerService(serviceName, this);
  }

  void XemService::callStartThread ( )
  {
    AssertBug ( isStarting(), "Service not starting ! \n" );

    Log_XemService ( "Starting XemService %p ... run StarterThread (state=%s)\n", this, getState().c_str() );
    try
    {
      XProcessor& xproc = getPerThreadXProcessor();
      XemProcessor& xemProc = XemProcessor::getMe ( xproc );

      xemProc.callMethod ( configurationElement, "start" );
    }
    catch ( Exception * e )
    {
      Error ( "Could not start XemService service : exception %s\n", e->getMessage().c_str() );
      delete ( e );
      state = State_Stopping;
      setStarted();
      return;
    }
    setStarted ();
    Log_XemService ( "Starting XemService ... OK (state=%s)\n", getState().c_str() );
  }

  void XemService::start ()
  {
    Log_XemService ( "Starting Service ! %s\n", configurationElement.generateVersatileXPath().c_str() );
    startThread(boost::bind(&XemService::callStartThread,this));
  }

  void XemService::stop ()
  {
    Log_XemService ( "Stopping Service : '%s' ...\n", configurationElement.generateVersatileXPath().c_str() );
    try
      {
        XProcessor& xproc = getPerThreadXProcessor();
        XemProcessor& xemProc = XemProcessor::getMe ( xproc );

        xemProc.callMethod ( configurationElement, "stop" );
      }
    catch ( Exception * e )
      {
        Error ( "Could not stop XemService service : exception %s\n", e->getMessage().c_str() );
        delete ( e );
      }
    Log_XemService ( "Stopping Service : '%s' OK\n", configurationElement.generateVersatileXPath().c_str() );
  }

#if 0
  /**
   * Class thread for creating an asynchronous method call
   * This is tricky, because Document is not designed to go across threads
   */
  class StartMethodThreadArgs
  {
  public:
    BranchRevId brId;
    ElementId thisId;
    LocalKeyId methodId;
    XemService::StartMethodThreadArgumentsMap* argsMap;

    StartMethodThreadArgs () {}
    ~StartMethodThreadArgs () {}
  };
#endif

  void XemService::startMethodThread ( const ElementRef& item_, LocalKeyId methodId, StartMethodThreadArgumentsMap* argsMap )
  {
    ElementRef item = item_;
    startThread(boost::bind(&XemService::callStartMethodThread,this,item.getDocument().getBranchRevId(),
          item.getElementId(), methodId, argsMap) );
  }

  void XemService::callStartMethodThread ( BranchRevId brId, ElementId thisId, LocalKeyId methodId, StartMethodThreadArgumentsMap* argsMap )
  {
    XProcessor& xproc = getPerThreadXProcessor();
    XemProcessor& xemProc = XemProcessor::getMe ( xproc );

    Log_XemService ( "[METHODTHREAD] argsMap size %lu\n", (unsigned long) argsMap->size() );

    for ( StartMethodThreadArgumentsMap::iterator iter = argsMap->begin() ; iter != argsMap->end() ; iter++ )
      {
        String value = stringFromAllocedStr(strdup(iter->second.c_str()));
        xproc.setString(iter->first,value);
        Log_XemService ( "[METHODTHREAD] push arg %s (%x) = %s\n",
            getStore().getKeyCache().dumpKey(iter->first).c_str(), iter->first, value.c_str() );
      }
    delete ( argsMap );

    Document* doc = getStore().getBranchManager().openDocument(brId.branchId,brId.revisionId,DocumentOpeningFlags_AsRevision);
    doc->incrementRefCount();
    DocumentRef docRef ( xproc, *doc );
    docRef.setAutoCommit(false);

    ElementRef item = doc->getElementById(thisId);

    Log_XemService ( "Running thread with this=%s, methodId=%s (%x), xproc=%p\n",
        item.generateVersatileXPath().c_str(), xproc.getKeyCache().dumpKey(methodId).c_str(),
        methodId, &xproc );

    xemProc.callMethod ( item, methodId );
  }

};
