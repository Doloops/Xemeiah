#ifndef __XEM_XSLMODULEFORGE_H
#define __XEM_XSLMODULEFORGE_H

#include <Xemeiah/xprocessor/xprocessormoduleforge.h>
namespace Xem
{
  class XPathParser;

#include <Xemeiah/kern/builtin_keys_prolog.h>
#include <Xemeiah/xsl/builtin-keys/xsl>
#include <Xemeiah/xsl/builtin-keys/xslimpl>
#include <Xemeiah/kern/builtin_keys_postlog.h>

  /**
   * XSL Module Forge : instanciator of XSLProcessor
   */
  class XSLModuleForge : public XProcessorModuleForge
  {
  protected:
#if 0
    XPathParser* xpathIdMatch;
    XPathParser* xpathIdUse;
#endif
    XPathParser* xpathDefaultSort;

  public:
    __BUILTIN_NAMESPACE_CLASS(xsl) xsl;
    __BUILTIN_NAMESPACE_CLASS(xslimpl) xslimpl;

    XSLModuleForge ( Store& store );
    ~XSLModuleForge ();
    
    NamespaceId getModuleNamespaceId ( );
    
    void install ();

    void instanciateModule ( XProcessor& xprocessor );

#if 0
    XPathParser& getXPathIdMatch() { return *xpathIdMatch; }
    XPathParser& getXPathIdUse() { return *xpathIdUse; }
#endif

    XPathParser& getXPathDefaultSort() { return *xpathDefaultSort; }

    /**
     * Register XSL events
     */
    virtual void registerEvents ( Document& doc );
  };
};

#endif //  __XEM_XSLMODULEFORGE_H
