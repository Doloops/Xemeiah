#ifndef __XEM_XEMPROCESSOR_XEMUSERMODULE_H
#define __XEM_XEMPROCESSOR_XEMUSERMODULE_H

#include <Xemeiah/kern/keycache.h>
#include <Xemeiah/xprocessor/xprocessormodule.h>
#include <Xemeiah/xprocessor/xprocessormoduleforge.h>

namespace Xem
{
#include <Xemeiah/kern/builtin_keys_prolog.h>
#include <Xemeiah/xemprocessor/builtin-keys/xem_user>
#include <Xemeiah/kern/builtin_keys_postlog.h>

  class XemUserModuleForge;

  /**
   * XemUserModule : implements security stuff (http://www.xemeiah.org/ns/xem/user namespace)
   */
  class XemUserModule : public XProcessorModule
  {
    friend class XemUserModuleForge;
  protected:
    void instructionSetUser ( __XProcHandlerArgs__ );
    void functionHome ( __XProcFunctionArgs__ );
    void functionGetUserName ( __XProcFunctionArgs__ );

    UserId userNameToUserId ( const String& userName );
    String userIdToUserName ( UserId userId );
  public:
    XemUserModule(XProcessor& xproc, XemUserModuleForge& moduleForge );
    ~XemUserModule();

    __BUILTIN_NAMESPACE_CLASS(xem_user) &xem_user;
        
    virtual void install ( ) {}

    void setUser ( const String& userName );
    String getUserName ();

    static XemUserModule& getMe(XProcessor& xproc);
  };
  
  /**
   * XemUserModuleForge : instanciates XemUserModule
   */
  class XemUserModuleForge : public XProcessorModuleForge
  {
  protected:
  
  public:
    __BUILTIN_NAMESPACE_CLASS(xem_user) xem_user;
    
    XemUserModuleForge ( Store& store )
    : XProcessorModuleForge ( store ), xem_user(store.getKeyCache())
    {}
    
    ~XemUserModuleForge () {}
    
    NamespaceId getModuleNamespaceId ( )
    {
      return xem_user.ns();
    }
    
    void install ();

    void instanciateModule ( XProcessor& xprocessor )
    {
      XProcessorModule* module = new XemUserModule ( xprocessor, *this );
      xprocessor.registerModule ( module );
    }

    virtual void registerEvents(Document& doc);

  };  
};

#endif // __XEM_XEMPROCESSOR_XEMUSERMODULE_H

