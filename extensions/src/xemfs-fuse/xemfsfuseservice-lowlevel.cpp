/*
 * xemfsfuseservice-lowlevel.cpp
 *
 *  Created on: 30 oct. 2009
 *      Author: francois
 */
#include <Xemeiah/xemfs-fuse/xemfsfuseservice.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_FuseLL Debug

namespace Xem
{
  /*
   * Low-level bridge
   */
  static XemFSFuseService&
  getFuseService()
  {
    void* ptr = fuse_get_context()->private_data;
    AssertBug ( ptr, "Invalid NULL pointer !\n" );
    return *((XemFSFuseService*)ptr);
  }

#define __IS_WRITE(__flag) ( (__flag) & ( O_RDWR | O_WRONLY ) )

#define __FUSE_TRYCATCH(...) \
   try { int res = __VA_ARGS__; return res; } \
   catch ( Exception *e ) { Error ( "Fuse '%s' : exception '%s'\n", "(mountpoint)", e->getMessage().c_str() ); delete ( e ); } \
   return -EIO;

  static int
  xemfs_fuse_getattr(const char *path, struct stat *stbuf)
  {
    __FUSE_TRYCATCH(getFuseService().fuseGetAttr(path, stbuf));
  }

  static int
  xemfs_fuse_readlink(const char *path, char* buf, size_t size)
  {
    __FUSE_TRYCATCH(getFuseService().fuseReadLink(path, buf, size));
  }

  static int
  xemfs_fuse_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
      off_t offset, struct fuse_file_info *info)
  {
    __FUSE_TRYCATCH(getFuseService().fuseReadDir(path, buf, filler, offset, info));
  }

  static int
  xem_fuse_open(const char *path, struct fuse_file_info* info)
  {
    bool wr = __IS_WRITE(info->flags);
    __FUSE_TRYCATCH(getFuseService().fuseOpen(path, wr, info->fh));
  }

  static int
  xem_fuse_read(const char *path, char *buf, size_t size, off_t offset,
      struct fuse_file_info* info)
  {
    Log_FuseLL ( "[FUSELL] read(%s,%llu,%llu)\n",
        path, (unsigned long long) size, (unsigned long long) offset );
    __FUSE_TRYCATCH(getFuseService().fuseRead(path, info->fh, buf, size, offset));
  }

  static int
  xem_fuse_write(const char *path, const char *buf, size_t size, off_t offset,
      struct fuse_file_info* info)
  {
    __FUSE_TRYCATCH(getFuseService().fuseWrite(path, info->fh, buf, size, offset));
  }

  static int
  xem_fuse_release(const char *path, struct fuse_file_info* info)
  {
    bool wr = __IS_WRITE(info->flags);
    __FUSE_TRYCATCH(getFuseService().fuseRelease(path, wr, info->fh));
  }

  static int
  xemfs_fuse_mknod(const char *path, mode_t mode, dev_t rdev)
  {
    __FUSE_TRYCATCH(getFuseService().fuseMkNod(path, mode));
  }

  static int
  xemfs_fuse_mkdir(const char *path, mode_t mode)
  {
    __FUSE_TRYCATCH(getFuseService().fuseMkNod(path, mode | S_IFDIR));
  }

  static int
  xemfs_fuse_chmod(const char *path, mode_t mode)
  {
    __FUSE_TRYCATCH(getFuseService().fuseChmod(path, mode));
  }

  static int
  xemfs_fuse_chown(const char *path, uid_t uid, gid_t gid)
  {
    __FUSE_TRYCATCH(getFuseService().fuseChown(path, uid, gid));
  }

  static int
  xemfs_fuse_truncate (const char * path, off_t size)
  {
    __FUSE_TRYCATCH(getFuseService().fuseTruncate(path, size));
  }

  static int
  xemfs_fuse_utime(const char *path, struct utimbuf *buf)
  {
    __FUSE_TRYCATCH(getFuseService().fuseUtime(path, buf->actime, buf->modtime));
  }

  static Mutex xem_enforce_fuse_user_data_Mutex;
  static void* xem_enforce_fuse_user_data_value = NULL;

  void xem_enforce_fuse_user_data ( void* user_data )
  {
    xem_enforce_fuse_user_data_Mutex.lock ();
    xem_enforce_fuse_user_data_value = user_data;
  }

  static void*
  xemfs_fuse_init(struct fuse_conn_info *conn)
  {
    Log_FuseLL ( "Fuse init for mountpoint!\n" );
    Log_FuseLL ( "context->fuse=%p, context->private_data=%p\n",
        fuse_get_context()->fuse, fuse_get_context()->private_data );

    if ( fuse_get_context()->private_data == NULL )
      {
        AssertBug ( xem_enforce_fuse_user_data_value, "No xem_enforce_fuse_user_data() provided !\n" );
        Warn ( "Fuse : enforcing user_data=%p to fix a fuse bug...\n", xem_enforce_fuse_user_data_value );
        fuse_get_context()->private_data = xem_enforce_fuse_user_data_value;
        xem_enforce_fuse_user_data_value = NULL;
        xem_enforce_fuse_user_data_Mutex.unlock ();
      }

    getFuseService().fuseInit(fuse_get_context()->fuse);
    return fuse_get_context()->private_data;
  }

  static void
  xemfs_fuse_destroy(void* conn)
  {
    Info ( "Fuse destroy !\n");
  }

  void
  XemFSFuseService::init_fuse_operations(struct fuse_operations& xemfs_fuse_oper)
  {
    memset ( &xemfs_fuse_oper, 0, sizeof(struct fuse_operations) );

    xemfs_fuse_oper.getattr = xemfs_fuse_getattr;
    xemfs_fuse_oper.readlink = xemfs_fuse_readlink;
    xemfs_fuse_oper.getdir = NULL;
    xemfs_fuse_oper.mknod = xemfs_fuse_mknod;
    xemfs_fuse_oper.mkdir = xemfs_fuse_mkdir;
    xemfs_fuse_oper.unlink = NULL;
    xemfs_fuse_oper.rmdir = NULL;
    xemfs_fuse_oper.symlink = NULL;
    xemfs_fuse_oper.rename = NULL;
    xemfs_fuse_oper.link = NULL;
    xemfs_fuse_oper.chmod = xemfs_fuse_chmod;
    xemfs_fuse_oper.chown = xemfs_fuse_chown;
    xemfs_fuse_oper.truncate = xemfs_fuse_truncate;
    xemfs_fuse_oper.utime = xemfs_fuse_utime;
    xemfs_fuse_oper.open = xem_fuse_open;
    xemfs_fuse_oper.read = xem_fuse_read;
    xemfs_fuse_oper.write = xem_fuse_write;
    xemfs_fuse_oper.statfs = NULL;
    xemfs_fuse_oper.flush = NULL;
    xemfs_fuse_oper.release = xem_fuse_release;
    xemfs_fuse_oper.fsync = NULL;
    xemfs_fuse_oper.setxattr = NULL;
    xemfs_fuse_oper.getxattr = NULL;
    xemfs_fuse_oper.listxattr = NULL;
    xemfs_fuse_oper.opendir = NULL;
    xemfs_fuse_oper.readdir = xemfs_fuse_readdir;
    xemfs_fuse_oper.fsyncdir = NULL;
    xemfs_fuse_oper.init = xemfs_fuse_init;
    xemfs_fuse_oper.destroy = xemfs_fuse_destroy;
    xemfs_fuse_oper.access = NULL;
    xemfs_fuse_oper.create = NULL;
    xemfs_fuse_oper.ftruncate = NULL;
    xemfs_fuse_oper.fgetattr = NULL;
    xemfs_fuse_oper.lock = NULL;
    xemfs_fuse_oper.utimens = NULL;
    xemfs_fuse_oper.bmap = NULL;
  }

};
