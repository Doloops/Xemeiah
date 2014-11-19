/*
 * xemfsfuseservice.cpp
 *
 *  Created on: 30 oct. 2009
 *      Author: francois
 */

#include <Xemeiah/xemfs-fuse/xemfsfuseservice.h>

#include <Xemeiah/xpath/xpath.h>
#include <Xemeiah/xpath/xpathparser.h>
#include <Xemeiah/xprocessor/xprocessor.h>
#include <Xemeiah/kern/documentref.h>
#include <Xemeiah/kern/branchmanager.h>
#include <Xemeiah/dom/childiterator.h>
#include <Xemeiah/xemprocessor/xemservicemodule.h>
#include <Xemeiah/xemprocessor/xemprocessor.h>
#include <Xemeiah/xemprocessor/xemusermodule.h>
#include <Xemeiah/xprocessor/xprocessorlib.h>
#include <Xemeiah/io/bufferedwriter.h>
#include <Xemeiah/io/blobreader.h>
#include <Xemeiah/parser/parser.h>
#include <Xemeiah/parser/saxhandler-dom.h>
#include <Xemeiah/auto-inline.hpp>

#include <unistd.h>
#include <errno.h>

#define Log_Fuse Log // Debug
#define Info_Fuse Info

namespace Xem
{
  __XProcessorLib_DECLARE_LIB ( XemFSFuse, "xemfs-fuse" );
  __XProcessorLib_REGISTER_MODULE ( XemFSFuse, XemFSFuseModuleForge );

#include <Xemeiah/kern/builtin_keys_prolog_inst.h>
#include <Xemeiah/xemfs-fuse/builtin-keys/xem_fs_fuse>
#include <Xemeiah/kern/builtin_keys_postlog.h>

  /*
   * ************************* Service Module Part *************************
   */
  XemFSFuseModuleForge::XemFSFuseModuleForge ( Store& store )
  : XProcessorModuleForge(store), xem_fs_fuse(store.getKeyCache()), xem_fs(store.getKeyCache()), xem_user(store.getKeyCache())
  {

  }

  XemFSFuseModuleForge::~XemFSFuseModuleForge ()
  {

  }

  void XemFSFuseModuleForge::install ()
  {
    registerHandler ( xem_fs_fuse.fuse_service(), &XemFSFuseModule::instructionFuseService );
  }

  void XemFSFuseModuleForge::instanciateModule ( XProcessor& xprocessor )
  {
    XProcessorModule* module = new XemFSFuseModule(xprocessor, *this);
    xprocessor.registerModule(module);
  }

  XemFSFuseModule::XemFSFuseModule ( XProcessor& xproc, XemFSFuseModuleForge& moduleForge )
  : XProcessorModule(xproc,moduleForge),xem_fs_fuse(moduleForge.xem_fs_fuse),xem_fs(moduleForge.xem_fs),xem_user(moduleForge.xem_user)
  {

  }

  void
  XemFSFuseModule::instructionFuseService(__XProcHandlerArgs__)
  {
    XemService* service = new XemFSFuseService(*this, item);
    service->registerMyself(getXProcessor());
  }

  /*
   * ************************* Service Part *************************
   */

  extern "C"
  {
    void fuse_unmount_compat22(const char *mountpoint);
  }

  XemFSFuseService::XemFSFuseService(XemFSFuseModule& _xemFSFuseModule,ElementRef& _configurationElement)
  : XemService(_xemFSFuseModule.getXProcessor(), _configurationElement),
    xemFSFuseModule(_xemFSFuseModule),
    xem_fs_fuse(_xemFSFuseModule.xem_fs_fuse),
    xem_fs(_xemFSFuseModule.xem_fs),
    xem_user(_xemFSFuseModule.xem_user)
  {
    Log_Fuse ( "Instanciating fuse service !\n" );
    current_fuse = NULL;

    init_fuse_operations(xemfs_fuse_oper);
  }

  XemFSFuseService::~XemFSFuseService()
  {

  }

  void XemFSFuseService::initXProcessor ( XProcessor& xproc )
  {
    XemUserModule& xemUserModule = XemUserModule::getMe(xproc);
    String userName = configurationElement.getAttr(xem_user.user_name());
    Log ( "User Name : '%s'\n", userName.c_str() );
    xemUserModule.setUser(userName);
    XemService::initXProcessor(xproc);
  }

  XProcessor&
  XemFSFuseService::getXProcessor()
  {
    XProcessor& xproc = getPerThreadXProcessor();
    if ( !xproc.getUserId() )
      {
        Bug ( "No userId set !\n" );
      }
    return xproc;
  }

  Document&
  XemFSFuseService::getDocument(bool write)
  {
    String mainBranch = configurationElement.getEvaledAttr(getXProcessor(),xem_fs_fuse.use_branch());
    RoleId roleId = KeyCache::getLocalKeyId(xem_fs_fuse.main());

    Document* document = getStore().getBranchManager().openDocument(mainBranch, "follow-branch", roleId);
    if ( !document )
      {
        Bug ( "Could not open document !\n" );
      }

    if ( write )
      {
        document->grantWrite();
      }
    Info_Fuse ( "[FUSEDOC] Openned doc at %p : [%llx:%llx]\n", document, _brid(document->getBranchRevId()) );
    document->incrementRefCount();
    return *document;
  }

  ElementRef
  XemFSFuseService::getRoot(Document& doc)
  {
    XemProcessor& xemProcessor = XemProcessor::getMe(getXProcessor());
    getXProcessor().setElement(xemProcessor.xem_role.main(), doc.getRootElement() );
    XPath xpath(getXProcessor(), configurationElement, xem_fs_fuse.root());
    ElementRef docRoot = doc.getRootElement();

    ElementRef root = xpath.evalElement(docRoot);
    return root;
  }

  ElementRef
  XemFSFuseService::getFile(Document& doc, const String& path)
  {
    ElementRef result(doc);
    ElementRef root = getRoot(doc);

    if (path == "/")
      {
        return root;
      }
    if (path == "")
      {
        return result;
      }

    result = root;

    std::list<String> tokens;
    path.tokenize(tokens, '/');
    Log_Fuse ( "[TOKEN] Path : '%s', root=%s\n", path.c_str(), root.generateVersatileXPath().c_str() );
    for ( std::list<String>::iterator iter = tokens.begin() ;
        iter != tokens.end() ; iter ++ )
      {
        Log_Fuse ( "[TOKEN] '%s'\n", iter->c_str() );
        try
        {
          result = result.lookup(getXProcessor(),xem_fs.folder_contents(),*iter);
          Log_Fuse ( "[TOKEN] At res=%s\n", result.generateVersatileXPath().c_str() );
        }
        catch ( Exception * e )
        {
          Error ( "[Fuse %s] Could not find file '%s' : %s\n",
              mountpoint.c_str(),path.c_str(),
              e->getMessage().c_str() );
          delete (e);
          return ElementRef(doc);
        }
      }
    return result;
  }

  BlobRef
  XemFSFuseService::getBlobRef(ElementRef& file)
  {
    BlobRef blob = file.findAttr(xem_fs.contents(), AttributeType_Blob);
    return blob;
  }

  __ui64 XemFSFuseService::getFileSize ( ElementRef& file, blkcnt_t* blocks )
  {
    __ui64 size = 0;
    if (file.getKeyId() == xem_fs.document())
      {
        BlobRef blob = getBlobRef(file);
        if (blob)
          size = blob.getSize();
        else if (file.hasAttr(xem_fs.size()))
          {
            size = strtoull(file.getAttr(xem_fs.size()).c_str(), NULL, 10 );
          }
      }
    else
      {
        NotImplemented ( "And will not be : stat for file : %s\n", file.generateVersatileXPath().c_str() );
      }
    if ( blocks )
      {
        *blocks = size / 512;
      }
    return size;
  }

  void
  XemFSFuseService::statFile(ElementRef& file, struct ::stat * stbuf)
  {
    memset(stbuf, 0, sizeof(struct ::stat));
    stbuf->st_blksize = 512;

    if (file.getKeyId() == xem_fs.folder() || file.getKeyId() == xem_user.user() )
      {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_size = 0;
        stbuf->st_blocks = 1;
      }
    else if (file.getKeyId() == xem_fs.document())
      {
        stbuf->st_mode = S_IFREG | 0644;
        stbuf->st_size = getFileSize(file, &(stbuf->st_blocks));
      }
    else if ( file.getKeyId() == xem_fs.link())
      {
        stbuf->st_mode = S_IFLNK | 0644;
        String target = file.getAttr(xem_fs.target());
        stbuf->st_size = target.size();
      }
    else if ( file.hasAttr(xem_fs.name()) )
      {
        stbuf->st_mode = S_IFREG | 0644;
        BufferedWriter writer;
        file.toXML(writer);
        stbuf->st_size = writer.getBufferSize();
      }
    else
      {
        NotImplemented ( "Not implemented yet ! : %s\n", file.generateVersatileXPath().c_str() );
        return;
      }
    if (file.hasAttr(xem_fs.creation_time()))
      {
        stbuf->st_ctime = XemFSModule::rfc1123TimeToTime(file.getAttr(
            xem_fs.creation_time()));
      }
    if (file.hasAttr(xem_fs.last_modified_time()))
      {
        stbuf->st_mtime = XemFSModule::rfc1123TimeToTime(file.getAttr(
            xem_fs.last_modified_time()));
      }
    if (file.hasAttr(xem_fs.last_access_time()))
      {
        stbuf->st_atime = XemFSModule::rfc1123TimeToTime(file.getAttr(
            xem_fs.last_access_time()));
      }
    if (file.hasAttr(xem_fs.uid()))
      {
        stbuf->st_uid = atoi(file.getAttr(xem_fs.uid()).c_str());
      }
    else
      {
        stbuf->st_uid = getuid();
      }
    if (file.hasAttr(xem_fs.gid()))
      {
        stbuf->st_uid = atoi(file.getAttr(xem_fs.gid()).c_str());
      }
    else
      {
        stbuf->st_gid = getgid();
      }
    stbuf->st_nlink = 2;
  }

  int
  XemFSFuseService::fuseGetAttr(const String& path, struct ::stat *stbuf)
  {
    Log_Fuse("getattr(%s)\n", path.c_str() );

    DocumentRef doc (getXProcessor(), getDocument());

    ElementRef file = getFile(doc, path);
    if (!file)
      return -ENOENT;
    Log_Fuse ( "Selected file : '%s'\n", file.generateVersatileXPath().c_str());
    Log_Fuse ( "File namespace is %x (%s)\n", file.getNamespaceId(), file.getKeyCache().getNamespaceURL(file.getNamespaceId()));
    statFile(file, stbuf);
    return 0;
  }

  int
  XemFSFuseService::fuseReadLink(const String& path, char* buf, size_t sz)
  {
    DocumentRef doc (getXProcessor(), getDocument());
    ElementRef file = getFile(doc, path);
    if (!file)
      return -ENOENT;

    String target = file.getAttr(xem_fs.target());
    StringSize s = target.size() + 1;
    size_t rsize = ( s < (sz-1) ) ? s : (sz-1);
    memcpy ( buf, target.c_str(), rsize );
    buf[rsize] = '\0';
    return 0;
  }

  int
  XemFSFuseService::fuseReadDir(const String& path, void *buf,
      fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *info)
  {
    Log_Fuse ( "readdir(%s)\n", path.c_str() );
    DocumentRef doc (getXProcessor(), getDocument());
    ElementRef file = getFile(doc, path);
    if (!file)
      {
        return -ENOENT;
      }
    Log_Fuse ( "Selected file : '%s'\n", file.generateVersatileXPath().c_str());

    if (file.getKeyId() != xem_fs.folder() && file.getKeyId() != xem_user.user() )
      {
        Warn ( "Not a folder !\n");
        return -ENOENT;
      }

    int res = 0;

    for (ChildIterator child(file); child; child++)
      {
        if ( ! child.hasAttr(xem_fs.name() ) )
          continue;
        String name = child.getAttr(xem_fs.name());
        struct ::stat stbuf;
        statFile(file, &stbuf);
        res = filler(buf, name.c_str(), &stbuf, 0);
        if (res)
          break;
      }
    Log_Fuse ( "readdir(%s) OK res=%d.\n", path.c_str(), res );
    return res;
  }

  int
  XemFSFuseService::fuseOpen(const String& path, bool write, uint64_t& fh)
  {
    Log_Fuse("open(%s,write=%s)\n", path.c_str(), write ? "yes" : "no");
    DocumentRef doc (getXProcessor(), getDocument());
    doc.setAutoCommit(false);
    ElementRef file = getFile(doc, path);
    if (!file)
      {
        return -ENOENT;
      }
    if ( file.hasAttr(xem_fs.contents(),AttributeType_Blob) )
      {

      }
    else if ( file.hasAttr(xem_fs.contents_temp(), AttributeType_Blob ) )
      {
        Warn ( "Openning while contents temp already created : %s\n", path.c_str() );
      }
    else if ( file.hasAttr(xem_fs.href() ))
      {
        String href = file.getAttr(xem_fs.href());
        doBackgroundFetch(file, href);
      }
    else if ( write && file.getChild() )
      {
        Log_Fuse ( "Created blob !\n" );
        doc->grantWrite();
        BlobRef blob = file.addBlob(xem_fs.contents());
        Assert ( blob, "Could not create blob !\n" );
        file.addAttrAsKeyId(xem_fs_fuse.auto_blob(), xem_fs_fuse.auto_blob());
      }
    fh = file.getElementId();
    Log_Fuse ( "open(%s, write=%s) : file=%s, fh=%llx\n",
        path.c_str(), write ? "yes" : "no",
        file.generateVersatileXPath().c_str(), (unsigned long long) fh );
    return 0;
  }


  int
  XemFSFuseService::fuseRelease(const String& path, bool write, uint64_t& fh)
  {
    if ( write )
      {
        DocumentRef doc (getXProcessor(), getDocument());
        doc.setAutoCommit(true);
        ElementRef file = getFile(doc, path);
        if ( file.hasAttr(xem_fs_fuse.auto_blob(),AttributeType_KeyId) )
          {
            fuseUnserializeWrite ( file );
          }
      }
    return 0;
  }

  BlobRef XemFSFuseService::waitBlobRef ( ElementRef& file, __ui64 maxOffset )
  {
    AssertBug ( ! file.getDocument().isLockedWrite(), "DOC IS LOCKED WRITE !!!\n" );
    Document& doc = file.getDocument();
    AssertBug ( maxOffset, "Zero endwindow !\n" );
    __ui64 nbIter = 0;
    while ( true )
      {
        nbIter ++;
        Log_Fuse ( "[BACKFETCH] WAITING FOR END !!!\n" );
        if ( file.hasAttr(xem_fs.contents(), AttributeType_Blob ) )
          {
            Info_Fuse ( "[WAITBLOB] %llu\n", nbIter );
            return getBlobRef(file);
          }

        BlobRef blob = file.findAttr(xem_fs.contents_temp(), AttributeType_Blob);
        if ( ! blob )
          {
            usleep ( 5000 );
            continue;
          }

        AssertBug ( blob, "No temp blob !\n" );

        if ( blob.getSize() >= maxOffset )
          {
            Log_Fuse ( "[BACKFETCH] FINISHED PARTIAL END !!! : blobSize=%llu, maxOffset=%llu\n",
                blob.getSize(), maxOffset );
            Info_Fuse ( "[WAITBLOB] %llu\n", nbIter );
            return blob;
          }
        usleep ( 5000 );
      }
    Log_Fuse ( "[BACKFETCH] END WATING !!!\n" );
    Bug ( "SHALL NOT BE HERE.\n" );
    return BlobRef ( doc );
  }

  int
  XemFSFuseService::fuseSerializeRead(ElementRef file, char* buf, size_t size, off_t offset)
  {
    BufferedWriter writer;
    file.toXML(writer);

    Log ( "Serialize : asked[%llu, %llu], serialized %llu\n", (__ui64) size, (__ui64) offset, writer.getBufferSize() );

    if ( (__ui64) offset > writer.getBufferSize() )
      return 0;

    const char* serialized = writer.getBuffer();
    size_t windowSize = size;
    if ( (__ui64) offset + windowSize > writer.getBufferSize() )
      {
        windowSize = writer.getBufferSize() - offset;
      }
    memcpy ( buf, &(serialized[offset]), windowSize );
    return windowSize;
  }

  void
  XemFSFuseService::fuseUnserializeWrite(ElementRef file)
  {
    BlobRef blob = getBlobRef(file);
    BlobReader reader(blob);
    file.getDocument().grantWrite(getXProcessor());

    ElementRef tmpRoot = file.getDocument().createElement(file,file.getKeyCache().getBuiltinKeys().xemint.root());

    Log_Fuse ( "Parsing back %llu bytes\n", blob.getSize() );

    SAXHandlerDom saxHandler(getXProcessor(), tmpRoot);
    saxHandler.setKeepTextMode("all");
    Parser parser(reader, saxHandler);
    parser.setKeepAllText(true);
    try
    {
      parser.parse();
    }
    catch ( Exception * e )
    {
      detailException ( e, "Could not parse back %s\n", file.generateVersatileXPath().c_str() );
      Error ( "Exception : %s\n", e->getMessage().c_str() );
      file.deleteAttr(xem_fs_fuse.auto_blob(),AttributeType_KeyId);
      file.deleteAttr(xem_fs.contents(), AttributeType_Blob);
      return;
    }

    Log_Fuse ( "Parsing OK, delete originating children :\n" );
    file.deleteChildren(getXProcessor());

    ElementRef tmp = tmpRoot.getChild();
    while ( tmp.getChild() )
      {
        ElementRef child = tmp.getChild();
        child.unlinkElementFromFather();
        file.appendLastChild(child);
        Log_Fuse ( "Pushed : %s\n", child.generateVersatileXPath().c_str() );
      }

    file.appendLastChild(tmp);
    tmp.deleteElement(getXProcessor());

    file.deleteAttr(xem_fs_fuse.auto_blob(),AttributeType_KeyId);
    file.deleteAttr(xem_fs.contents(), AttributeType_Blob);
  }

  int
  XemFSFuseService::fuseRead(const String& path, uint64_t fh, char *buf,
      size_t size, off_t offset)
  {
    DocumentRef doc (getXProcessor(), getDocument());
    doc.setAutoCommit(false);
    ElementRef file = doc->getElementById(fh);

    BlobRef blob = getBlobRef(file);
    if (!blob)
      {
        Log_Fuse ( "FUSE-READ : path='%s', elt='%s' : NO BLOB !\n",
            path.c_str(), file.generateVersatileXPath().c_str() );
        if (file.hasAttr(xem_fs.href()))
          {
            Log_Fuse ( "[BACKFETCH] : read(), no blob, but a href...\n" );

            __ui64 endWindow = size + offset;
            blob = waitBlobRef(file, endWindow);
          }
        else
          {
            return fuseSerializeRead(file, buf, size, offset);
          }
      }
    Log_Fuse ( "FUSE-READ : path='%s', elt='%s', size=%llu, offset=%llu\n",
        path.c_str(), file.generateVersatileXPath().c_str(),
        (unsigned long long) size, (unsigned long long) offset );
    int res = blob.readPiece(buf, size, offset);
    Log_Fuse ( "=> res=%d\n", res );
    return res;
  }

  int
  XemFSFuseService::fuseWrite(const String& path, uint64_t fh, const char *buf,
      size_t size, off_t offset)
  {
    DocumentRef doc (getXProcessor(), getDocument(true));
    doc.setAutoCommit(false);
    ElementRef file = doc->getElementById(fh);

    BlobRef blob = getBlobRef(file);

    if (!blob)
      {
        if ( file.hasAttr(xem_fs.contents_temp(), AttributeType_Blob ) )
          {
            NotImplemented ( "File has a temp blob !\n" );
          }
        if ( file.hasAttr(xem_fs.href() ) )
          {
            NotImplemented ( "File has a href !\n" );
            blob = waitBlobRef ( file, size + offset );
            doc->grantWrite();
          }
        else
          {
            // doc->grantWrite();
            // blob = file.addBlob(xem_fs.contents());
            Error ( "No blob exists !\n" );
            return -EIO;
          }
      }
    else
      {
        doc->grantWrite();
      }
    Log_Fuse ( "FUSE-WRITE: path='%s', elt='%s', size=%llu, offset=%llu\n",
        path.c_str(), file.generateVersatileXPath().c_str(),
        (unsigned long long) size, (unsigned long long) offset );

    int res = blob.writePiece(buf, size, offset);
    Log_Fuse ( "=> res=%d\n", res );
    return res;
  }

  int
  XemFSFuseService::fuseMkNod(const String& path, mode_t mode)
  {
    DocumentRef doc (getXProcessor(), getDocument(true));

    char* _dpath = strdup(path.c_str());
    char* dpath = _dpath;
    if (dpath[0] == '/')
      dpath++;
    char* ends = dpath;
    while (*ends)
      ends++;
    while (ends > dpath && *ends != '/')
      ends--;

    Log_Fuse ( "dpath=%s, ends=%s\n", dpath, ends );
    const char* lpath = dpath, *lname;
    if (ends == dpath)
      {
        lpath = "/";
        lname = ends;
      }
    else
      {
        ends[0] = '\0';
        ends++;
        lname = ends;
      }
    Log_Fuse ( "lpath=%s, lname=%s\n", lpath, lname );

    ElementRef file = getFile(doc, lpath);
    if (!file || file.getKeyId() != xem_fs.folder())
      {
        Warn ( "Invalid folder : '%s' -> '%s'\n", lpath, file.generateVersatileXPath().c_str() );
        free(_dpath);
        return -ENOENT;
      }

    KeyId childKeyId = S_ISDIR(mode) ? xem_fs.folder() : xem_fs.document();

    try
      {
        ElementRef newChild = file.getDocument().createElement(file, childKeyId);
        newChild.addAttr(xem_fs.name(), lname);
        file.appendChild(newChild);

        newChild.eventElement(getXProcessor(),DomEventType_CreateElement);
        XemProcessor::getMe(getXProcessor()).callElementConstructor(newChild);
      }
    catch (Exception * e)
      {
        Error("Thrown exception : %s\n", e->getMessage().c_str() );
        delete (e);
        free(_dpath);
        return -EIO;
      }
    free(_dpath);
    return 0;
  }

  int
  XemFSFuseService::fuseChmod(const String& path, mode_t mode)
  {
    DocumentRef doc (getXProcessor(), getDocument(true));
    ElementRef file = getFile(doc, path);

    Log_Fuse ( "Chmod : %s : %x\n", path.c_str(), mode );
    if (!file)
      return -ENOENT;
    return 0;
  }

  int
  XemFSFuseService::fuseChown(const String& path, uid_t uid, gid_t gid)
  {
    DocumentRef doc (getXProcessor(), getDocument(true));
    ElementRef file = getFile(doc, path);

    Log_Fuse ( "Chown : %s : uid=%x, gid=%x\n", path.c_str(), uid, gid );
    if (!file)
      return -ENOENT;
    String sUid, sGid;
    stringPrintf(sUid, "%d", uid);
    stringPrintf(sGid, "%d", gid);
    file.addAttr(xem_fs.uid(), sUid);
    file.addAttr(xem_fs.gid(), sGid);
    return 0;
  }

  int XemFSFuseService::fuseTruncate(const String& path, off_t size)
  {
    DocumentRef doc (getXProcessor(), getDocument(true));
    ElementRef file = getFile(doc, path);

    if (!file) return -ENOENT;
    if ( size )
      {
        Bug ( "Not implemented : truncate(%s,%llu)\n", path.c_str(), (unsigned long long) size );
        return -ENOSYS;
      }
    BlobRef blob = getBlobRef(file);
    if ( ! blob )
      {
        return 0;
        // Bug ( "Not implemented : no blob !\n" );
        // return -ENOENT;
      }
    // blob.deleteAttribute();
    file.deleteAttr(xem_fs.contents(), AttributeType_Blob);
    return 0;
  }

  int
  XemFSFuseService::fuseUtime(const String& path, time_t atime, time_t mtime)
  {
    DocumentRef doc (getXProcessor(), getDocument(true));
    ElementRef file = getFile(doc, path);

    if (!file)
      return -ENOENT;

    String sAtime = XemFSModule::timeToRFC1123Time(atime);
    String sMtime = XemFSModule::timeToRFC1123Time(mtime);

    Log_Fuse ( "utime(%s, %s, %s)\n", path.c_str(), sAtime.c_str(), sMtime.c_str() );

    file.addAttr(xem_fs.last_access_time(), sAtime);
    file.addAttr(xem_fs.last_modified_time(), sMtime);
    return 0;
  }

  void
  XemFSFuseService::fuseInit(fuse* _fuse)
  {
    Info_Fuse ( "Initialize fuse for mountpoint=%s (fuse=%p)\n", mountpoint.c_str(), _fuse );
    current_fuse = _fuse;
    setStarted();

    // startThread((ThreadFunction) (&XemFSFuseService::backgroundCommitThread), NULL);
  }

  void
  XemFSFuseService::checkMountpoint ( const String& mountpoint )
  {
    if ( mountpoint.isSpace() )
      {
        Error ( "Invalid mountpoint value !\n" );
        state = State_Stopping;
        setStarted();
        return;
      }

    if ( mountpoint.c_str()[0] != '/' )
      {
        throwException(Exception,"Mountpoint path shall be absolute : '%s'\n", mountpoint.c_str() );
      }
    std::list<String> path;
    mountpoint.tokenize(path,'/');

    int fdir = open ( "/", O_RDONLY );
    if ( ! fdir )
      {
        throwException(Exception,"Could not open '/' : err=%d:%s\n", errno, strerror(errno) );
      }

    size_t index = 0;
    for ( std::list<String>::iterator iter = path.begin() ; iter != path.end() ; iter++ )
      {
        int nfdir = openat(fdir, iter->c_str(), O_RDONLY);
        if ( nfdir == -1 )
          {
            int res = errno;
            Log_Fuse ( "Could not open '%s' : err=%d:%s\n", iter->c_str(), res, strerror(res) );

            if ( ( res == ENOTCONN ) && (index == path.size()-1) )
              {
                Info_Fuse ( "Final mountpoint is said to be not connected, trying to recover from previous crash...\n" );
                fuse_unmount_compat22(mountpoint.c_str());
                return;
              }
            else if ( res == ENOENT )
              {
                int res = mkdirat(fdir, iter->c_str(), 0755 );
                if ( res == -1 )
                  {
                    res = errno;
                    throwException(Exception,"Could not open / create '%s' : err=%d:%s\n", iter->c_str(), res, strerror(res) );
                  }
                Log_Fuse ( "Created dir '%s'\n", iter->c_str() );
                nfdir = openat(fdir, iter->c_str(), O_RDONLY);
              }
          }
        ::close(fdir);
        fdir = nfdir;
        index++;
      }
    Log_Fuse ( "At end : fdir=%d\n", fdir );
    ::close ( fdir );
  }

  void
  XemFSFuseService::startFuseThread()
  {
    Log_Fuse ( "Starting fuse thread %lx for this=%p !\n", pthread_self(), this );

    AssertBug ( isStarting(), "Service not starting !\n" );

    if (mountpoint.size())
      {
        throwException ( Exception, "Fuse thread already running for mountpoint : %s\n", mountpoint.c_str() );
      }
    if (current_fuse)
      {
        Bug ( "Already has current_fuse = %p\n", current_fuse );
      }

    mountpoint = configurationElement.getEvaledAttr(getXProcessor(), xem_fs_fuse.mountpoint());

    checkMountpoint(mountpoint);

    Log_Fuse ( "Starting fuse thread for mountpoint='%s'\n", mountpoint.c_str() );

    initBackgroundFetchThreads(1);

    int argc = 0;

    char* argv[8];
    memset(argv,0,sizeof(argv));
    argv[argc++] = (char*) "xem";
    argv[argc++] = (char*) mountpoint.c_str();
    argv[argc++] = (char*) "-f";
    argv[argc++] = (char*) "-s";
    argv[argc++] = (char*) "-d";

    void* user_data = (void*) this;

    Info_Fuse ( "Starting fuse_main() for mountpoint='%s'\n", mountpoint.c_str() );

    xem_enforce_fuse_user_data ( user_data );

    fuse_main(argc, argv, &xemfs_fuse_oper, user_data);

    Log_Fuse ( "End of fuse loop. finishing...\n" );
    current_fuse = NULL;
    mountpoint = "";

    if (isRunning())
      {
        Info_Fuse ( "Fuse loop finished while service running, stopping service.\n" );
        stopService();
      }
    else if (isStarting())
      {
        Info_Fuse ( "Failed to start service.\n" );
        state = State_Stopping;
        setStarted();
      }
  }

  void
  XemFSFuseService::start()
  {
    Info_Fuse ( "Starting fuse service !\n")
    startThread(boost::bind(&XemFSFuseService::startFuseThread,this));
  }

  void
  XemFSFuseService::postStart()
  {
    AssertBug ( isRunning(), "Fuse service not running !\n" );
    if (mountpoint.size())
      {
        Info_Fuse ( "Fuse started OK for mountpoint : %s.\n", mountpoint.c_str() );
      }
    else
      {
        throwException ( Exception, "Could not start FUSE service !\n" );
      }
  }

  void
  XemFSFuseService::stop()
  {
    Info_Fuse ( "Stopping fuse service, current_fuse=%p, mountpoint=%s !\n", current_fuse, mountpoint.c_str() );
    if (mountpoint.size())
      {
        Info_Fuse ( "Unmounting fuse mountpoint '%s' ...\n", mountpoint.c_str() );
        fuse_unmount_compat22(mountpoint.c_str());
        Info_Fuse ( "Unmounting fuse mountpoint '%s' ... OK\n", mountpoint.c_str() );
      }
    else
      {
        Info_Fuse ( "Fuse service stopping, mountpoint already unmounted.\n" );
      }
  }

  void
  XemFSFuseService::postStop()
  {
    Info_Fuse ( "Stop fuse service : notified effective stop of service.\n" );
    sem_destroy ( &backgroundFetchSemaphore );
  }
}
;
