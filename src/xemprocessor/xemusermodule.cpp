#include <Xemeiah/xemprocessor/xemusermodule.h>
#include <Xemeiah/xemprocessor/xemprocessor.h>
#include <Xemeiah/xprocessor/xprocessorlib.h>
#include <Xemeiah/xpath/xpath.h>
#include <Xemeiah/dom/childiterator.h>

#include <Xemeiah/trace.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_XemUser Debug

namespace Xem
{
  __XProcessorLib_REGISTER_MODULE ( XemProcessor, XemUserModuleForge );

#include <Xemeiah/kern/builtin_keys_prolog_inst.h>
#include <Xemeiah/xemprocessor/builtin-keys/xem_user>
#include <Xemeiah/kern/builtin_keys_postlog.h>

  XemUserModule::XemUserModule(XProcessor& xproc, XemUserModuleForge& moduleForge )
  : XProcessorModule ( xproc, moduleForge ), xem_user(moduleForge.xem_user)
  {
  
  }

  XemUserModule::~XemUserModule () {}

  XemUserModule& XemUserModule::getMe(XProcessor& xproc)
  {
    XProcessorModule* module = xproc.getModule("http://www.xemeiah.org/ns/xem/user",true);
    AssertBug ( module, "No module set !\n" );
    XemUserModule* userModule = dynamic_cast<XemUserModule*> ( module );
    AssertBug ( userModule, "Wrong cast : not a user module !\n" );
    return *userModule;
  }

  void XemUserModuleForge::install ()
  {
    // registerHandler ( xem_user.fork(), &XemUserModule::instructionFork );
    // registerFunction ( xem_user.has_branch(), &XemUserModule::functionHasBranch );

    registerHandler ( xem_user.set_user(), &XemUserModule::instructionSetUser );
    registerFunction ( xem_user.home(), &XemUserModule::functionHome );
    registerFunction ( xem_user.get_user_name(), &XemUserModule::functionGetUserName );
  }

  void XemUserModuleForge::registerEvents(Document& doc)
  {

  }

  UserId XemUserModule::userNameToUserId ( const String& userName )
  {
    KeyId userId = 0;
    try
    {
      userId = getKeyCache().getKeyId(0,userName.c_str(),true);
    }
    catch ( Exception *e )
    {
      detailException(e, "Could not lookup user name '%s'\n", userName.c_str() );
      throw;
    }
    return (UserId) userId;
  }

  String XemUserModule::userIdToUserName ( UserId userId )
  {
    return getKeyCache().dumpKey(userId);
  }

  void XemUserModule::setUser ( const String& userName )
  {
    UserId userId = userNameToUserId ( userName );
    getXProcessor().setUserId(userId);
  }

  String XemUserModule::getUserName ()
  {
    UserId userId = getXProcessor().getUserId();
    if ( ! userId )
      {
        throwException ( Exception, "No user set in XProcessor !\n" );
      }
    return userIdToUserName(userId);
  }

  void XemUserModule::instructionSetUser ( __XProcHandlerArgs__ )
  {
    String userName = item.getEvaledAttr(getXProcessor(),xem_user.user_name());
    setUser ( userName );
  }

  void XemUserModule::functionGetUserName ( __XProcFunctionArgs__ )
  {
    result.setSingleton(getUserName());
  }

  void XemUserModule::functionHome ( __XProcFunctionArgs__ )
  {
    UserId userId = getXProcessor().getUserId();
    if ( ! userId )
      {
        throwException ( Exception, "No user set in XProcessor !\n" );
      }
    String userName = userIdToUserName(userId);

    // ElementRef mainRoot = node.toElement().getRootElement();
    XemProcessor& xemProcessor = XemProcessor::getMe(getXProcessor());
    ElementRef mainRoot = getXProcessor().getVariable(xemProcessor.xem_role.main())->toElement();
    ElementRef homes = mainRoot.getChild();

    if ( ! homes || homes.getKeyId() != xem_user.homes() )
      {
        Warn ( "Main root : not a xem-user:homes() !\n" );
        return;
        throwException ( Exception, "Main root : not a xem-user:homes() !\n" );
      }

    for ( ChildIterator home(homes) ; home ; home++ )
      {
        if ( home.getKeyId() != xem_user.user() || !home.hasAttr(xem_user.user_name()))
          continue;
        if ( home.getAttr(xem_user.user_name()) == userName )
          {
            result.pushBack(home);
            return;
          }
      }
    Warn ( "Could not fetch user home for '%s'\n", userName.c_str() );
    return;
    throwException ( Exception, "Could not fetch user home for '%s'\n", userName.c_str() );
  }

};

