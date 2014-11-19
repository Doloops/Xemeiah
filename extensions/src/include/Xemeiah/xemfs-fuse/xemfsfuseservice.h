/*
 * xemfs-fuseservice.h
 *
 *  Created on: 28 oct. 2009
 *      Author: francois
 */

#ifndef __XEM_XEMFSFUSESERVICE_H_
#define __XEM_XEMFSFUSESERVICE_H_

#include <Xemeiah/xprocessor/xprocessormoduleforge.h>
#include <Xemeiah/xprocessor/xprocessormodule.h>
#include <Xemeiah/xprocessor/xprocessor.h>
#include <Xemeiah/xemprocessor/xemservice.h>
#include <Xemeiah/xemprocessor/xemusermodule.h>
#include <Xemeiah/xemfs/xemfsmodule.h>
#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/dom/blobref.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define FUSE_USE_VERSION 29
#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif // _BSD_SOURCE
#ifdef linux
/* For pread()/pwrite() */
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 600
#endif
#ifndef _ATFILE_SOURCE
#define _ATFILE_SOURCE
#endif // _ATFILE_SROUCE
#ifndef __USE_GNU
#define __USE_GNU
#endif // __USE_GNU
#endif

#include <fuse.h>
#include <stdio.h>
#include <errno.h>
#include <semaphore.h>

namespace Xem
{
#include <Xemeiah/kern/builtin_keys_prolog.h>
#include <Xemeiah/xemfs-fuse/builtin-keys/xem_fs_fuse>
#include <Xemeiah/kern/builtin_keys_postlog.h>

  /**
   * The forge for XemFSFuseModule Fuse extensions
   */
  class XemFSFuseModuleForge : public XProcessorModuleForge
  {
  public:
    __BUILTIN_NAMESPACE_CLASS(xem_fs_fuse) xem_fs_fuse;
    __BUILTIN_NAMESPACE_CLASS(xem_fs) xem_fs;
    __BUILTIN_NAMESPACE_CLASS(xem_user) xem_user;

    XemFSFuseModuleForge ( Store& store );
    ~XemFSFuseModuleForge ();

    NamespaceId getModuleNamespaceId ( ) { return xem_fs_fuse.ns(); }

    void instanciateModule ( XProcessor& xprocessor );
    void install ();

    /**
     * Register default DomEvents for this document
     */
    virtual void registerEvents(Document& doc) {}
  };

  /**
   * The XemFSFuseModule Fuse extension module
   */
  class XemFSFuseModule : public XProcessorModule
  {
    friend class XemFSFuseModuleForge;
  protected:
    void instructionFuseService ( __XProcHandlerArgs__ );

  public:
    __BUILTIN_NAMESPACE_CLASS(xem_fs_fuse)& xem_fs_fuse;
    __BUILTIN_NAMESPACE_CLASS(xem_fs)& xem_fs;
    __BUILTIN_NAMESPACE_CLASS(xem_user)& xem_user;

    XemFSFuseModule ( XProcessor& xproc, XemFSFuseModuleForge& moduleForge );
    ~XemFSFuseModule () {}

    virtual void install () {}
  };

  /**
   * Xem Fuse service : export xem-fs structures to fuse
   */
  class XemFSFuseService : public XemService
  {
    friend void __xem_fuse_unlockBlob ( BlobRef& blob, void* arg );
  protected:
    /**
     * Reference to our XemFSModule instance
     */
    XemFSFuseModule& xemFSFuseModule;

    /**
     * Reference to the xem-fs builtin namespace
     */
    __BUILTIN_NAMESPACE_CLASS(xem_fs_fuse)& xem_fs_fuse;
    __BUILTIN_NAMESPACE_CLASS(xem_fs) &xem_fs;
    __BUILTIN_NAMESPACE_CLASS(xem_user) &xem_user;

    /**
     * Start that service
     */
    virtual void start ();

    /**
     * Stop that service
     */
    virtual void stop ();

    /**
     * Post-start service
     */
    virtual void postStart();

    /**
     * Post-stop that service
     */
    virtual void postStop ();

    /**
     * The current mointpoint used (only valid if service is started)
     */
    String mountpoint;

    /**
     * The fuse structure used (only valid if service is started)
     */
    fuse* current_fuse;

    /**
     * Main Fuse thread
     */
    void startFuseThread ( );

    /**
     * Check that the mountpoint is correct and usable, create it if it does not exist
     */
    void checkMountpoint ( const String& mountpoint );

    /**
     * Background commit fuse thread
     */
    void backgroundCommitThread ( void * arg );

    /**
     * Initialize XProcessor
     */
    virtual void initXProcessor ( XProcessor& xproc );

    /**
     * Background fetch semaphore
     */
    sem_t backgroundFetchSemaphore;

    /**
     * Initialize Background fetch structures
     */
    void initBackgroundFetchThreads ( __ui64 maxConcurrentThreads );

    /**
     * Background fetch thread
     */
    void backgroundFetchThread ( void* arg );

    /**
     * Background fetch periodic event
     */
    void backgroundFetchWindowEvent ( BlobRef& blob );

    /**
     * Create a background fetch thread to fetch file contents
     */
    void doBackgroundFetch ( ElementRef& file, const String& path );

    /**
     * Get the currently operating Document
     */
    Document& getDocument ( bool write = false );

    /**
     * Get the Root Element from the running document
     */
    ElementRef getRoot ( Document& doc );

    /**
     * Lookup file from the root document
     */
    ElementRef getFile ( Document& doc, const String& path );

    /**
     * Get the file size of a given file
     */
    __ui64 getFileSize ( ElementRef& file, blkcnt_t* blocks = NULL );

    /**
     * Build the stat struct from a given ElementRef
     */
    void statFile ( ElementRef& file, struct ::stat * stbuf );

    /**
     * Get the BlobRef from a given file
     */
    BlobRef getBlobRef ( ElementRef& file );

    /**
     * Wait for a BlobRef to be completed in background fetch
     */
    BlobRef waitBlobRef ( ElementRef& file, __ui64 maxOffset );

    /**
     * Fuse low-level operations
     */
    struct fuse_operations xemfs_fuse_oper;

    /**
     * Initialize fuse low-level operation mapping
     */
    void
    init_fuse_operations(struct fuse_operations& xemfs_fuse_oper);

    /**
     * Serialize XML contents, then read
     */
    int fuseSerializeRead(ElementRef file, char* buf, size_t size, off_t offset);
    void fuseUnserializeWrite(ElementRef file);
  public:
    /**
     * XemFSFuseService constructor
     */
    XemFSFuseService ( XemFSFuseModule& xemFSFuseModule, ElementRef& configurationElement );

    /**
     * XemFSFuseSerice destructor
     */
    ~XemFSFuseService ();

    /**
     * Get a reference to our per-thread XProcessor
     */
    XProcessor& getXProcessor();

    /*
     * Fuse Operations Initializer
     */
    void fuseInit ( fuse* _fuse );

    /**
     * Fuse Operations Backends
     */
    int fuseGetAttr(const String& path, struct ::stat *stbuf);
    int fuseReadLink(const String& path, char *, size_t);
    int fuseReadDir(const String& path, void *buf, fuse_fill_dir_t filler,
          off_t offset, struct fuse_file_info *info);

    int fuseOpen(const String& path, bool write, uint64_t& fh);
    int fuseRelease(const String& path, bool write, uint64_t& fh);

    int fuseRead(const String& path, uint64_t fh, char *buf, size_t size, off_t offset);
    int fuseWrite(const String& path, uint64_t fh, const char *buf, size_t size, off_t offset);

    int fuseMkNod(const String& path, mode_t mode);
    int fuseChmod(const String& path, mode_t mode);
    int fuseChown(const String& path, uid_t uid, gid_t gid);
    int fuseTruncate(const String& path, off_t size);
    int fuseUtime(const String& path, time_t atime, time_t mtime);
  };

  void xem_enforce_fuse_user_data ( void* user_data );
};

#endif /* __XEM_XEMFSFUSESERVICE_H_ */
