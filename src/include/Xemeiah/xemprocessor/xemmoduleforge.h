#ifndef __XEM_XEMMODULEFORGE_H
#define __XEM_XEMMODULEFORGE_H

#include <Xemeiah/xprocessor/xprocessormoduleforge.h>
#include <Xemeiah/xemprocessor/xemmoduleforge.h>

namespace Xem
{
#include <Xemeiah/kern/builtin_keys_prolog.h>
#include <Xemeiah/xemprocessor/builtin-keys.h>
#include <Xemeiah/kern/builtin_keys_postlog.h>
  
  class XemModuleForge : public XProcessorModuleForge
  {
  protected:
  
  public:
    __BUILTIN_NAMESPACE_CLASS(xem) xem;
    __BUILTIN_NAMESPACE_CLASS(xem_role) xem_role;
    __BUILTIN_NAMESPACE_CLASS(xem_event) xem_event;
      
    XemModuleForge ( Store& store ) 
    : XProcessorModuleForge ( store ), xem(store.getKeyCache()), xem_role(store.getKeyCache()), xem_event(store.getKeyCache())
    {
    }
    ~XemModuleForge () {}
    
    NamespaceId getModuleNamespaceId ( );
    
    void install ();

    void instanciateModule ( XProcessor& xprocessor );

    /**
     * Register default DomEvents for this document
     */
    void registerEvents ( Document& doc );
  };


};

#endif //  __XEM_XSLMODULEFORGE_H

