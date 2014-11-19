#ifndef __XEM_EXSLTMODULE_H
#define __XEM_EXSLTMODULE_H

#include <Xemeiah/xprocessor/xprocessormoduleforge.h>
#include <Xemeiah/xprocessor/xprocessormodule.h>
#include <Xemeiah/xprocessor/xprocessor.h>
#include <Xemeiah/dom/string.h>

#include <map>

namespace Xem
{
#include <Xemeiah/kern/builtin_keys_prolog.h>
#include <Xemeiah/exslt/builtin-keys/exslt>
#include <Xemeiah/kern/builtin_keys_postlog.h>

  class Document;

  /**
   * XProcessorModuleForge for EXSLT Common Namespace
   */
  class EXSLTCommonModuleForge : public XProcessorModuleForge
  {
  public:
    __BUILTIN_NAMESPACE_CLASS(exslt_common) exslt_common;

    EXSLTCommonModuleForge ( Store& store );
    ~EXSLTCommonModuleForge ();
    
    NamespaceId getModuleNamespaceId ( ) { return exslt_common.ns(); }
    
    void instanciateModule ( XProcessor& xprocessor );
    
    void install ();

    void registerEvents ( Document& doc ) {}
  };

  /**
   * XProcessorModule for EXSLT Common Namespace
   */
  class EXSLTCommonModule : public XProcessorModule
  {
    friend class EXSLTCommonModuleForge;
  protected:
    void functionNodeSet ( __XProcFunctionArgs__ );
    void instructionDocument ( __XProcHandlerArgs__ );
  public:
    __BUILTIN_NAMESPACE_CLASS(exslt_common) &exslt_common;
    
    EXSLTCommonModule ( XProcessor& xproc, EXSLTCommonModuleForge& moduleForge );
    ~EXSLTCommonModule ();
    
    
    void install ();

  };


  /**
   * XProcessorModuleForge for EXSLT Dynamic Namespace
   */
  class EXSLTDynamicModuleForge : public XProcessorModuleForge
  {
  public:
    __BUILTIN_NAMESPACE_CLASS(exslt_dynamic) exslt_dynamic;

    EXSLTDynamicModuleForge ( Store& store );
    ~EXSLTDynamicModuleForge ();
    
    NamespaceId getModuleNamespaceId ( ) { return exslt_dynamic.ns(); }
    
    void instanciateModule ( XProcessor& xprocessor );
    
    void install ();

    void registerEvents ( Document& doc ) {}
  };

  /**
   * XProcessorModule for EXSLT Dynamic Namespace
   */
  class EXSLTDynamicModule : public XProcessorModule
  {
    friend class EXSLTDynamicModuleForge;
  protected:
    typedef std::map<String, XPath*> DynamicXPathExpressionMap;
    DynamicXPathExpressionMap dynamicXPathExpressionMap;

    void functionEvaluate ( __XProcFunctionArgs__ );
  public:
    __BUILTIN_NAMESPACE_CLASS(exslt_dynamic) &exslt_dynamic;
    
    EXSLTDynamicModule ( XProcessor& xproc, EXSLTDynamicModuleForge& moduleForge );
    ~EXSLTDynamicModule ();
    
    
    void install ();

  };


  /**
   * XProcessorModuleForge for EXSLT Sets Namespace
   */
  class EXSLTSetsModuleForge : public XProcessorModuleForge
  {
  public:
    __BUILTIN_NAMESPACE_CLASS(exslt_sets) exslt_sets;

    EXSLTSetsModuleForge ( Store& store );
    ~EXSLTSetsModuleForge ();
    
    NamespaceId getModuleNamespaceId ( ) { return exslt_sets.ns(); }
    
    void instanciateModule ( XProcessor& xprocessor );
    
    void install ();

    void registerEvents ( Document& doc ) {}
  };

  /**
   * XProcessorModule for EXSLT Sets Namespace
   */
  class EXSLTSetsModule : public XProcessorModule
  {
  protected:
    void functionLeading ( __XProcFunctionArgs__ );
    void functionTrailing ( __XProcFunctionArgs__ );
  public:
    __BUILTIN_NAMESPACE_CLASS(exslt_sets) &exslt_sets;
    
    EXSLTSetsModule ( XProcessor& xproc, EXSLTSetsModuleForge& moduleForge );
    ~EXSLTSetsModule ();
    
    
    void install ();

  };

};

#endif // __XEM_EXSLTMODULE_H

