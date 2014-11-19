#ifndef __XEM_XEMFSMODULE_H
#define __XEM_XEMFSMODULE_H

#include <Xemeiah/xprocessor/xprocessormoduleforge.h>
#include <Xemeiah/xprocessor/xprocessormodule.h>
#include <Xemeiah/xprocessor/xprocessor.h>
#include <Xemeiah/dom/string.h>
#include <Xemeiah/kern/mutex.h>

/**
 * Xem FS Module, for file-system related operations
 */
#include <map>

#if 0
#include <magic.h>
#endif

namespace Xem
{
#include <Xemeiah/kern/builtin_keys_prolog.h>
#include <Xemeiah/xemfs/builtin-keys/xem_fs>
#include <Xemeiah/xemfs/builtin-keys/xem_id3>
#include <Xemeiah/xemfs/builtin-keys/xem_media>
#include <Xemeiah/kern/builtin_keys_postlog.h>

  class BlobRef;
  class XemFSModule;
  class XemFSRunCommandService;

  /**
   * The forge for XemFSModule FileSystem extensions
   */
  class XemFSModuleForge : public XProcessorModuleForge
  {
  protected:
#if 0
	  magic_t magic;
#endif
    Mutex magicMutex;

  public:
    __BUILTIN_NAMESPACE_CLASS(xem_fs) xem_fs;
    __BUILTIN_NAMESPACE_CLASS(xem_id3) xem_id3;

    XemFSModuleForge ( Store& store );
    ~XemFSModuleForge ();
    
    NamespaceId getModuleNamespaceId ( ) { return xem_fs.ns(); }
    
    void install ();

    /**
     * Register default DomEvents for this document
     */
    void registerEvents(Document& doc);

    void instanciateModule ( XProcessor& xprocessor );

    /**
     * Get MIME type from a given file path
     */
    String getMimeType ( const String& filePath );
    
    /**
     * Get MIME type from a BlobRef
     */
    String getMimeType ( BlobRef& blobRef );

  };

  /**
   * XemFSModule FileSystem extensions
   */
  class XemFSModule : public XProcessorModule
  {
    friend class XemFSModuleForge;
  protected:

    /**
     * Configuration settings for doBrowse()
     */
    class BrowseSettings
    {
    public:
      bool mayGetMimeType;
      bool mayGetBlob;

      std::list<String> skipNames;
      std::list<String> skipSuffixes;

      BrowseSettings()
      {
        mayGetMimeType = false;
        mayGetBlob = false;
      }

      ~BrowseSettings() {}

      bool mustSkip ( const String& name );
    };

    XemFSRunCommandService& getXemFSRunCommandService ( const String& serviceName );

    void instructionRunCommandService ( __XProcHandlerArgs__ );
    void instructionSendCommand(__XProcHandlerArgs__);
    void functionRecvCommand(__XProcFunctionArgs__);
    void functionGetRecvDocument(__XProcFunctionArgs__);

    void instructionBlobReadFromFile ( __XProcHandlerArgs__ );
    void instructionBlobSaveToFile ( __XProcHandlerArgs__ );

    void instructionCreateBlob ( __XProcHandlerArgs__ );
    void instructionCopyBlob ( __XProcHandlerArgs__ );
  

    void instructionBrowse ( __XProcHandlerArgs__ );
    void instructionMakeLink ( __XProcHandlerArgs__ );

    void instructionGetId3 ( __XProcHandlerArgs__ );
    void getId3 ( ElementRef& documentElement );
    void setId3Property ( ElementRef& documentElement, KeyId keyId, const char* pty, int length );

    void functionBrowse ( __XProcFunctionArgs__ );
    void functionHasBlob ( __XProcFunctionArgs__ );
    void functionGetBlobSize ( __XProcFunctionArgs__ ); // Get Blob
    void functionGetMimeType ( __XProcFunctionArgs__ ); // Get MimeType from a file
    void functionGetBlobMimeType ( __XProcFunctionArgs__ ); // Get Blob MimeType    
    void functionGetCurrentTime ( __XProcFunctionArgs__ );

    void functionResolveLink ( __XProcFunctionArgs__ );

    void domEventMimeType ( DomEventType domEventType, ElementRef& domEventElement, NodeRef& nodeRef );

    void doBrowse ( ElementRef& fromElement, const String& rootUrl, const String& url, int dirfd,
        BrowseSettings& settings );

  public:
    __BUILTIN_NAMESPACE_CLASS(xem_fs) &xem_fs;
    __BUILTIN_NAMESPACE_CLASS(xem_id3) &xem_id3;
      
    XemFSModule ( XProcessor& xproc, XemFSModuleForge& moduleForge );
    ~XemFSModule () {}
    
    void install ();

    /**
     * Get my XemFSModuleForge reference
     */
    XemFSModuleForge& getXemFSModuleForge();

    /**
     * Convert to time
     */
    static String timeToRFC1123Time ( time_t anytime );

    /**
     * Convert from time
     */
    static time_t rfc1123TimeToTime ( const String& time );

    /**
     * Get MIME type from a given file path
     */
    String getMimeType ( const String& filePath );
  };
};

#endif //  __XEM_XEMFSMODULE_H

