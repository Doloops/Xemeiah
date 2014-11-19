#ifndef __XEM_PERSISTENCE_XPROCESSORMODULE_H
#define __XEM_PERSISTENCE_XPROCESSORMODULE_H

#include <Xemeiah/kern/keycache.h>
#include <Xemeiah/xprocessor/xprocessormodule.h>
#include <Xemeiah/xprocessor/xprocessormoduleforge.h>

namespace Xem
{
#include <Xemeiah/kern/builtin_keys_prolog.h>
#include <Xemeiah/xemprocessor/builtin-keys/xem_pers>
#include <Xemeiah/kern/builtin_keys_postlog.h>

  class PersistenceModuleForge;

  /**
   * PersistenceModule : implements http://www.xemeiah.org/ns/xem/persistence namespace
   */
  class PersistenceModule : public XProcessorModule
  {
    friend class PersistenceModuleForge;
  protected:
    void instructionFork ( __XProcHandlerArgs__ );
    void instructionReopen ( __XProcHandlerArgs__ );
    void instructionCommit ( __XProcHandlerArgs__ );
    void instructionMerge ( __XProcHandlerArgs__ );
    void instructionDrop ( __XProcHandlerArgs__ );
    void instructionRenameBranch ( __XProcHandlerArgs__ );
    void instructionCreateBranch ( __XProcHandlerArgs__ );

    void functionHasBranch ( __XProcFunctionArgs__ );
    void functionGetCurrentBranchName ( __XProcFunctionArgs__ );
    void functionGetCurrentBranchId ( __XProcFunctionArgs__ );
    void functionGetCurrentRevisionId ( __XProcFunctionArgs__ );
    void functionGetCurrentBranchRevisionId ( __XProcFunctionArgs__ );
    void functionGetCurrentForkedFrom ( __XProcFunctionArgs__ );
    void functionGetCurrentRevisionWritable ( __XProcFunctionArgs__ );
    void functionGetBranchesTree ( __XProcFunctionArgs__ );
    void functionLookupRevision ( __XProcFunctionArgs__ );
    void functionGetBranchName ( __XProcFunctionArgs__ );
    void functionGetBranchId ( __XProcFunctionArgs__ );
    void functionGetBranchForkedFrom ( __XProcFunctionArgs__ );
    
    void domEventDocumentTrigger ( DomEventType domEventType, ElementRef& domEventElement, NodeRef& nodeRef );
  public:
    PersistenceModule(XProcessor& xproc, PersistenceModuleForge& moduleForge );
    ~PersistenceModule();

    __BUILTIN_NAMESPACE_CLASS(xem_pers) &xem_pers;
        
    virtual void install ( );
  };
  
  /**
   * PersistenceModuleForge : instanciates PersistenceModule
   */
  class PersistenceModuleForge : public XProcessorModuleForge
  {
  protected:
  
  public:
    __BUILTIN_NAMESPACE_CLASS(xem_pers) xem_pers;
    
    PersistenceModuleForge ( Store& store ) 
    : XProcessorModuleForge ( store ), xem_pers(store.getKeyCache())
    {}
    
    ~PersistenceModuleForge () {}
    
    NamespaceId getModuleNamespaceId ( )
    {
      return xem_pers.ns();
    }
    
    void install ();

    void instanciateModule ( XProcessor& xprocessor )
    {
      XProcessorModule* module = new PersistenceModule ( xprocessor, *this ); 
      xprocessor.registerModule ( module );
    }

    virtual void registerEvents(Document& doc);

  };  
};

#endif //  __XEM_PERSISTENCE_XPROCESSORMODULE_H

