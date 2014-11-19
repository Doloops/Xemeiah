#ifndef __XEM_XEMFSMODULE_H
#define __XEM_XEMFSMODULE_H

#include <Xemeiah/xprocessor/xprocessormoduleforge.h>
#include <Xemeiah/xprocessor/xprocessormodule.h>
#include <Xemeiah/xprocessor/xprocessor.h>
#include <Xemeiah/dom/string.h>
#include <Xemeiah/kern/mutex.h>

/**
 * Xem ImageMagick Module, a frontend of ImageMagick library
 */
#include <map>

namespace Xem
{
#include <Xemeiah/kern/builtin_keys_prolog.h>
#include <Xemeiah/xem-imagemagick/builtin-keys/xem_imagemagick>
#include <Xemeiah/kern/builtin_keys_postlog.h>

  class BlobRef;

  /**
   * The forge for XemFSModule FileSystem extensions
   */
  class XemImageMagickModuleForge : public XProcessorModuleForge
  {
  protected:

  public:
    __BUILTIN_NAMESPACE_CLASS(xem_im) xem_im;

    XemImageMagickModuleForge ( Store& store );
    ~XemImageMagickModuleForge ();
    
    NamespaceId getModuleNamespaceId ( ) { return xem_im.ns(); }
    
    void install ();

    /**
     * Register default DomEvents for this document
     */
    void registerEvents(Document& doc);

    void instanciateModule ( XProcessor& xprocessor );
  };

  /**
   * XemImageMagickModule : ImageMagick lib
   */
  class XemImageMagickModule : public XProcessorModule
  {
    friend class XemImageMagickModuleForge;
  protected:
    void instructionCrop ( __XProcHandlerArgs__ );
    void instructionScale ( __XProcHandlerArgs__ );
    void instructionReflection ( __XProcHandlerArgs__ );
    void instructionPictureMess ( __XProcHandlerArgs__ );

  public:
    __BUILTIN_NAMESPACE_CLASS(xem_im) &xem_im;
      
    XemImageMagickModule ( XProcessor& xproc, XemImageMagickModuleForge& moduleForge );
    ~XemImageMagickModule () {}
    
    XemImageMagickModuleForge& getXemImageMagickModuleForge();

    void install ();
  };
};

#endif //  __XEM_XEMFSMODULE_H

