/*
 * xem-valhalla.h
 *
 *  Created on: 20 déc. 2009
 *      Author: francois
 */

#ifndef __XEM_XEMVALHALLA_H_
#define __XEM_XEMVALHALLA_H_

#include <Xemeiah/xprocessor/xprocessormoduleforge.h>
#include <Xemeiah/xprocessor/xprocessormodule.h>
#include <Xemeiah/xprocessor/xprocessor.h>


namespace Xem
{
#include <Xemeiah/kern/builtin_keys_prolog.h>
#include <Xemeiah/xem-valhalla/builtin-keys/xem-vh>
#include <Xemeiah/xem-valhalla/builtin-keys/valhalla>
#include <Xemeiah/kern/builtin_keys_postlog.h>

  /**
   * The forge for XemValhallaModule Fuse extensions
   */
  class XemValhallaModuleForge : public XProcessorModuleForge
  {
  public:
    __BUILTIN_NAMESPACE_CLASS(xem_vh) xem_vh;
    __BUILTIN_NAMESPACE_CLASS(valhalla) valhalla;

    XemValhallaModuleForge ( Store& store );
    ~XemValhallaModuleForge ();

    NamespaceId getModuleNamespaceId ( ) { return xem_vh.ns(); }

    void instanciateModule ( XProcessor& xprocessor );
    void install ();

    /**
     * Register default DomEvents for this document
     */
    virtual void registerEvents(Document& doc) {}
  };

  /**
   * The XemValhallaModule Fuse extension module
   */
  class XemValhallaModule : public XProcessorModule
  {
    friend class XemValhallaModuleForge;
  protected:
    void instructionScan ( __XProcHandlerArgs__ );
    void instructionMediaPlayer ( __XProcHandlerArgs__ );
    void instructionSendCommand ( __XProcHandlerArgs__ );
    void functionGetStatus(__XProcFunctionArgs__);

  public:
    void updateXemValhallaMeta ( ElementRef& thisElement, ElementRef& fileElt, const String& pathPrefix );
    void updateXemValhallaMeta ( ElementRef& thisElement, const String& path, const String& group, const String& name, const String& value );
  public:
    __BUILTIN_NAMESPACE_CLASS(xem_vh)& xem_vh;
    __BUILTIN_NAMESPACE_CLASS(valhalla)& valhalla;

    XemValhallaModule ( XProcessor& xproc, XemValhallaModuleForge& moduleForge );
    ~XemValhallaModule () {}

    virtual void install () {}
  };

};
#endif /* __XEM_XEMVALHALLA_H_ */
