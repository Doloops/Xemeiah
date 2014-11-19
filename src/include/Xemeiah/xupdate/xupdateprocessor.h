#ifndef __XEM_XUPDATE_XUPDATE_H
#define __XEM_XUPDATE_XUPDATE_H

#include <Xemeiah/xprocessor/xprocessormodule.h>
#include <Xemeiah/xprocessor/xprocessormoduleforge.h>
#include <Xemeiah/kern/exception.h>

namespace Xem
{  
  XemStdException ( XUpdateException );
  XemStdException ( XemException );

#include <Xemeiah/kern/builtin_keys_prolog.h>
#include <Xemeiah/xupdate/builtin-keys/xupdate>
#include <Xemeiah/kern/builtin_keys_postlog.h>

  class XUpdateModuleForge;
  
  /**
   * XUpdateProcessor implements XUpdate language.
   */
  class XUpdateProcessor : public XProcessorModule
  {
    friend class XUpdateModuleForge;
  protected:
    void xupdateSetDocumentWritable ( ElementRef& item, Document& document );
    void xupdateSetDocumentWritable ( ElementRef& item, NodeRef& nodeRef );    
    void xupdateInstructionModifications ( __XProcHandlerArgs__ );
    void xupdateInstructionAppend ( __XProcHandlerArgs__ );
    
    ElementRef xupdateProcessElement ( ElementRef& elementInstruction, ElementRef& baseElement, JournalOperation journalOperation );

    void xupdateInstructionInsertBeforeAfter ( __XProcHandlerArgs__ );
    void xupdateInstructionRename ( __XProcHandlerArgs__ );    
    void xupdateInstructionRemove ( __XProcHandlerArgs__ );
    void xupdateInstructionUpdate ( __XProcHandlerArgs__ );
    void xupdateInstructionElement ( __XProcHandlerArgs__ );
    void xupdateInstructionAttribute ( __XProcHandlerArgs__ );
    void xupdateInstructionNotHandled ( __XProcHandlerArgs__ );
    
  public:
    __BUILTIN_NAMESPACE_CLASS(xupdate) &xupdate;

    XUpdateProcessor ( XProcessor& xproc, XUpdateModuleForge& moduleForge );
    ~XUpdateProcessor () {}
    
    void install ();
  };
  
  /**
   * XUpdateProcessor Module Forge : instanciates XUpdateProcessor
   */
  class XUpdateModuleForge : public XProcessorModuleForge
  {
  protected:
  
  public:
    __BUILTIN_NAMESPACE_CLASS(xupdate) xupdate;

    XUpdateModuleForge ( Store& store );
    ~XUpdateModuleForge () {}
    
    NamespaceId getModuleNamespaceId ( )
    {
      return xupdate.ns();
    }
    
    void install ();

    void instanciateModule ( XProcessor& xprocessor )
    {
      XProcessorModule* module = new XUpdateProcessor ( xprocessor, *this ); 
      xprocessor.registerModule ( module );
    }

    void registerEvents(Document& doc);
  };
  
};

#endif // __XEM_XUPDATE_XUPDATE_H

