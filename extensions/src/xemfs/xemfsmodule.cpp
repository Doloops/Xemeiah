#include <Xemeiah/xemfs/xemfsmodule.h>
#include <Xemeiah/kern/volatiledocument.h>
#include <Xemeiah/xsl/xslprocessor.h>
#include <Xemeiah/dom/blobref.h>
#include <Xemeiah/dom/childiterator.h>
#include <Xemeiah/xpath/xpath.h>
#include <Xemeiah/kern/servicemanager.h>

#include <Xemeiah/nodeflow/nodeflow-dom.h>
#include <Xemeiah/xemprocessor/xemprocessor.h>

#include <Xemeiah/xprocessor/xprocessorlib.h>
#include <Xemeiah/xemfs/xemfsruncommandservice.h>

#include <Xemeiah/auto-inline.hpp>

#include <sys/types.h>
#include <dirent.h>

#ifndef _ATFILE_SOURCE
#define _ATFILE_SOURCE
#endif // _ATFILE_SOURCE
#include <fcntl.h> /* Definition of AT_* constants */
#include <sys/stat.h>
#include <errno.h>
#include <string.h>

#define Log_XemFS Debug
#define Log_RelativizeLink Debug

namespace Xem
{
  __XProcessorLib_DECLARE_LIB ( XemFS, "xemfs" );
  __XProcessorLib_REGISTER_MODULE ( XemFS, XemFSModuleForge );

#include <Xemeiah/kern/builtin_keys_prolog_inst.h>
#include <Xemeiah/xemfs/builtin-keys/xem_fs>
#include <Xemeiah/xemfs/builtin-keys/xem_media>
#include <Xemeiah/kern/builtin_keys_postlog.h>

  XemFSModuleForge::XemFSModuleForge(Store& store) :
    XProcessorModuleForge(store), xem_fs(store.getKeyCache()), xem_id3(store.getKeyCache())
  {
    static const char * magicFile = "/usr/share/file/magic";

    magic = magic_open(MAGIC_MIME);
    if (magic_errno(magic) > 0)
      {
        throwException ( Exception, "Could not open magic lib : %s\n", magic_error(magic));
      }

    magic_load(magic, magicFile);
    if (magic_errno(magic) > 0)
      {
        throwException ( Exception, "Could not load magic lib from file %s : %s\n", magicFile, magic_error(magic));
      }
  }

  XemFSModuleForge::~XemFSModuleForge()
  {
    magic_close(magic);
    if (magic_errno(magic) > 0)
      {
        throwException ( Exception, "Could not close magic lib : %s\n", magic_error(magic));
      }
  }

  String
  XemFSModuleForge::getMimeType(const String& filePath)
  {
    Lock lock (magicMutex);
    /*
     * Enforce the text/css mime type for .css files
     */
    if (stringEndsWith(filePath, ".css"))
      return "text/css";
    if (stringEndsWith(filePath, ".html"))
      return "text/html";

    const char* resultTmp = magic_file(magic, filePath.c_str());
    if (magic_errno(magic) > 0)
      {
        Error ( "Could not get magic : error %s\n", magic_error(magic));
        return String("unknown");
      }
    if ( resultTmp == NULL || *resultTmp == '\0' )
      return "";
    if ( strchr(resultTmp,';') )
      {
        char* result = strdup(resultTmp);
        char* sep = strchr(result,';');
        *sep = '\0';
        return stringFromAllocedStr(result);
      }
    return String(resultTmp);
  }

  String
  XemFSModuleForge::getMimeType(BlobRef& blobRef)
  {
    Lock lock (magicMutex);

    void* window;
    __ui64 windowSize = blobRef.getPiece(&window, ~((__ui64 ) 0), 0);
    const char* resultTmp = magic_buffer(magic, window, windowSize);
    if (magic_errno(magic) > 0)
      {
        Error ( "Could not get magic : error %s\n", magic_error(magic));
        return String("unknown");
      }
    if ( resultTmp && strchr(resultTmp,';') )
      {
        char* result = strdup(resultTmp);
        char* sep = strchr(result,';');
        *sep = '\0';
        return stringFromAllocedStr(result);
      }
    return String(resultTmp);
  }

  void
  XemFSModuleForge::instanciateModule(XProcessor& xprocessor)
  {
    XProcessorModule* module = new XemFSModule(xprocessor, *this);
    xprocessor.registerModule(module);
  }

  XemFSModule::XemFSModule(XProcessor& xproc, XemFSModuleForge& moduleForge) :
    XProcessorModule(xproc, moduleForge), xem_fs(moduleForge.xem_fs), xem_id3(moduleForge.xem_id3)
  {

  }

  XemFSModuleForge& XemFSModule::getXemFSModuleForge()
  {
    return dynamic_cast<XemFSModuleForge&> ( moduleForge );
  }

  void
  XemFSModule::functionGetCurrentTime(__XProcFunctionArgs__)
  {
    result.setSingleton(timeToRFC1123Time(time(NULL)));
  }

  String
  XemFSModule::getMimeType(const String& filePath)
  {
    return getTypedModuleForge<XemFSModuleForge> ().getMimeType(filePath);
  }

  void
  relativizeLink(const char* src, char* link, const char* base)
  {
    char realbase[PATH_MAX];
    int res = readlink(base, realbase, PATH_MAX - 1);

    const char* baseIter = base;
    const char* relativeSrc = src;
    while (*baseIter)
      {
        AssertBug ( *baseIter == *relativeSrc, "Diverging base='%s', src='%s'\n", base, src );
        baseIter++;
        relativeSrc++;
      }
    if (*relativeSrc == '/')
      relativeSrc++;
    Log_RelativizeLink ( "relativize : base='%s', src='%s', relative src='%s'\n", base, src, relativeSrc );

    if (res == -1)
      {
        res = strlen(base);
        strcpy(realbase, base);
      }
    else
      {
        AssertBug ( res, "Null res !\n" );
        if (realbase[res - 1] != '/')
          realbase[res++] = '/';
        realbase[res] = '\0';
        Log_RelativizeLink ( "base='%s', realbase='%s'\n", base, realbase );
      }
    if (strncmp(link, realbase, res) == 0)
      {
        Log_RelativizeLink ( "Relative link '%s' : target '%s' is relative to '%s'\n", src, link, realbase );
        char* relativeLink = strdup(&(link[res]));

        Log_RelativizeLink ( "Relative link '%s' => '%s'\n", link, relativeLink );
        link[0] = '\0';
        const char* relativeSrcIter = relativeSrc;
        while ((relativeSrcIter = strchr(relativeSrcIter, '/')) != NULL)
          {
            strcat(link, "../");
            relativeSrcIter++;
          }
        strcat(link, relativeLink);

        free(relativeLink);
        Log_RelativizeLink ( "Post-op : link='%s'\n", link );
      }
  }

  void
  XemFSModule::functionGetMimeType(__XProcFunctionArgs__)
  {
    if (args.size() != 1)
      throwException ( Exception, "Invalid number of arguments !\n" );
    String href = args[0]->toString();
    result.setSingleton(getMimeType(href));
  }

  bool XemFSModule::BrowseSettings::mustSkip ( const String& name )
  {
    for ( std::list<String>::iterator iter = skipNames.begin() ; iter != skipNames.end() ; iter ++ )
      {
        if ( *iter == name ) return true;
      }
    for ( std::list<String>::iterator iter = skipSuffixes.begin() ; iter != skipSuffixes.end() ; iter ++ )
      {
        if ( stringEndsWith(name,*iter) )
          return true;
      }
    return false;
  }

  void
  XemFSModule::doBrowse(ElementRef& fromElement, const String& rootUrl,
      const String& url, int dirfd, BrowseSettings& settings )
  {
    if (dirfd == -1)
      dirfd = open(url.c_str(), O_RDONLY);
    if (dirfd == -1)
      {
        Error ( "Could not open('%s') : error %d:%s\n", url.c_str(), errno, strerror(errno) );
        return;
        throwException ( Exception, "Could not open('%s') : error %d:%s\n", url.c_str(), errno, strerror(errno) );
      }

    DIR* dir = fdopendir(dirfd);
    if (!dir)
      {
        close(dirfd);
        Error ( "Could not fdopendir('%s') : error %d:%s\n", url.c_str(), errno, strerror(errno) );
        return;
        throwException ( Exception, "Could not fdopendir('%s') : error %d:%s\n", url.c_str(), errno, strerror(errno) );
      }

    struct dirent* de;
    struct stat st;

    while ((de = readdir(dir)) != NULL)
      {
        Log_XemFS ( "de=%p : name=%s, type=%d\n", de, de->d_name, de->d_type );
        if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
          continue;

        String childUrl = url.c_str();
        if ( ! stringEndsWith(url,"/") )
          childUrl += "/";
        childUrl += de->d_name;

        String childName = de->d_name;

        if ( settings.mustSkip ( childName ) )
          continue;

        if (fstatat(dirfd, de->d_name, &st, AT_SYMLINK_NOFOLLOW))
          {
            Error ( "Could not fstatat(%d, %s, %p) : err=%d:%s\n", dirfd, de->d_name, &st, errno, strerror(errno) );
            continue;
          }
        if (S_ISREG(st.st_mode))
          {
            ElementRef documentElt = fromElement.getDocument().createElement(
                fromElement, xem_fs.document());
            fromElement.appendChild(documentElt);

            documentElt.addAttr(getXProcessor(),xem_fs.name(), childName);
            documentElt.addAttr(getXProcessor(),xem_fs.href(), childUrl);
            documentElt.addAttr(getXProcessor(),xem_fs.creation_time(), timeToRFC1123Time(
                st.st_ctime));
            documentElt.addAttr(getXProcessor(),xem_fs.last_modified_time(), timeToRFC1123Time(
                st.st_mtime));
            documentElt.addAttr(getXProcessor(),xem_fs.last_access_time(),
                timeToRFC1123Time(st.st_atime));

            String fileSize;
            stringPrintf ( fileSize, "%llu", (__ui64) st.st_size );
            documentElt.addAttr(getXProcessor(),xem_fs.size(), fileSize);

            XemProcessor::getMe(getXProcessor()).callElementConstructor(documentElt);
            documentElt.eventElement(getXProcessor(),DomEventType_CreateElement);

            Log_XemFS ( "At %s, getBlob=%d\n", childUrl.c_str(), settings.mayGetBlob );

            String mimeType;
            if (settings.mayGetMimeType)
              {
                mimeType = getMimeType(childUrl);
              }

            if ( settings.mayGetBlob )
              {
                BlobRef blobRef = documentElt.addBlob(xem_fs.contents());
                blobRef.readFromFile(childUrl);
                Log_XemFS ( "Read blob from '%s'\n", childUrl.c_str() );

                if (settings.mayGetMimeType)
                  {
                    Log_XemFS ( "Setting mimeType '%s'\n", mimeType.c_str() );
                    blobRef.setMimeType(mimeType);
                  }
              }

            if (settings.mayGetMimeType)
              {
                Log_XemFS ( "[XEM-FS] : Getting mimeType for '%s'\n", childUrl.c_str() );
                AttributeRef mimeTypeAttr = documentElt.addAttr(xem_fs.mime_type(), mimeType);
                Log_XemFS ( "Mime of '%s' is '%s'\n", childUrl.c_str(), documentElt.getAttr(xem_fs.mime_type() ).c_str() );
                documentElt.eventAttribute(getXProcessor(),DomEventType_CreateAttribute,mimeTypeAttr);
              }
          }
        else if (S_ISDIR(st.st_mode))
          {
            ElementRef folderElt = fromElement.getDocument().createElement(
                fromElement, xem_fs.folder());
            fromElement.appendChild(folderElt);

            folderElt.addAttr(getXProcessor(),xem_fs.name(), childName);
            folderElt.addAttr(getXProcessor(),xem_fs.href(), childUrl);
            folderElt.addAttr(getXProcessor(),xem_fs.creation_time(), timeToRFC1123Time(
                st.st_ctime));
            folderElt.addAttr(getXProcessor(),xem_fs.last_modified_time(),
                timeToRFC1123Time(st.st_mtime));
            folderElt.addAttr(getXProcessor(),xem_fs.last_access_time(), timeToRFC1123Time(
                st.st_atime));
            folderElt.eventElement(getXProcessor(),DomEventType_CreateElement);

            XemProcessor::getMe(getXProcessor()).callElementConstructor(folderElt);

            doBrowse(folderElt, rootUrl, childUrl, -1, settings );
          }
        else if (S_ISLNK(st.st_mode))
          {
            char targetLink[PATH_MAX + 1];
            ssize_t res = readlink(childUrl.c_str(), targetLink, PATH_MAX);
            if (res == -1)
              {
                Warn ( "Could not readlink(%s) : err=%d:%s\n", childUrl.c_str(), errno, strerror(errno));
              }
            else
              {
                targetLink[res] = '\0';

                if (targetLink[0] == '/')
                  {
                    relativizeLink(childUrl.c_str(), targetLink,
                        rootUrl.c_str());
                  }

                ElementRef linkElt = fromElement.getDocument().createElement(
                    fromElement, xem_fs.link());
                fromElement.appendChild(linkElt);

                linkElt.addAttr(getXProcessor(),xem_fs.name(), childName);
                linkElt.addAttr(getXProcessor(),xem_fs.href(), childUrl);
                linkElt.addAttr(getXProcessor(),xem_fs.target(), targetLink);
                linkElt.eventElement(getXProcessor(),DomEventType_CreateElement);

                XemProcessor::getMe(getXProcessor()).callElementConstructor(linkElt);
              }
          }
        else
          {
            Warn ( "Node %s has unhandled mode %x\n", childUrl.c_str(), st.st_mode );
          }
      }
    ::closedir(dir);
    ::close(dirfd);
  }

  void
  XemFSModule::functionBrowse(__XProcFunctionArgs__)
  {
    if (args.size() == 0)
      throwException ( Exception, "Not enough arguments to xem-fs:browse() !\n" );

    String url = args[0]->toString();
    Log_XemFS ( "Browsing from '%s'\n", url.c_str() );

    ElementRef rootElement = getXProcessor().createVolatileDocument(true);

    ElementRef baseElement =
        rootElement.getDocument().createElement(rootElement,xem_fs.folder());
    rootElement.appendChild(baseElement);
    baseElement.addAttr(xem_fs.name(), "(root)");
    baseElement.addNamespaceAlias(xem_fs.defaultPrefix(), xem_fs.ns());

    BrowseSettings settings;
    doBrowse(baseElement, url, url, -1, settings);

    result.pushBack(baseElement);
  }

  void XemFSModule::instructionMakeLink ( __XProcHandlerArgs__ )
  {
    XPath selectXPath ( getXProcessor(), item, xem_fs.select() );
    ElementRef target = selectXPath.evalElement();

    String linkName;
    if ( item.hasAttr(xem_fs.name() ) )
      linkName = item.getEvaledAttr(getXProcessor(),xem_fs.name());
    else
      linkName = target.getAttr(xem_fs.name());

    getNodeFlow().newElement(xem_fs.link());
    ElementRef linkElt = getNodeFlow().getCurrentElement();

    if ( &(target.getDocument()) == &(linkElt.getDocument()) )
      {
        linkElt.addAttrAsElementId(xem_fs.target_id(), target.getElementId());
      }
    else
      {
        linkElt.addAttr(xem_fs.target_id(), target.generateId());
      }

    linkElt.addAttr(getXProcessor(), xem_fs.name(), linkName);

    XemProcessor::getMe(getXProcessor()).callElementConstructor(linkElt);
    /*
     * Now process Item children
     */
    if ( item.getChild() )
      {
        getXProcessor().setElement(getKeyCache().getBuiltinKeys().nons.this_(), linkElt, false );
        NodeFlowDom nodeFlow(getXProcessor(),linkElt);
        getXProcessor().setNodeFlow(nodeFlow);

        getXProcessor().processChildren(item);
      }
    getNodeFlow().elementEnd(xem_fs.link());
  }

  void XemFSModule::functionResolveLink ( __XProcFunctionArgs__ )
  {
    if ( args.size() != 1 )
      {
        throwException ( Exception, "Wrong number of arguments while calling xem-fs:resolve-link()\n" );
      }
    for ( NodeSet::iterator iter(*args[0]) ; iter ; iter++ )
      {
        ElementRef linkElt = iter->toElement();
        if ( linkElt.getKeyId() != xem_fs.link() )
          {
            throwException ( Exception, "Wrong type for xem-fs:resolve-link() : %s\n",
                linkElt.generateVersatileXPath().c_str() );
          }
        if ( linkElt.hasAttr(xem_fs.target_id(),AttributeType_ElementId) )
          {
            ElementId eId = linkElt.getAttrAsElementId(xem_fs.target_id());
            ElementRef target = linkElt.getDocument().getElementById(eId);
            if ( ! target )
              {
                throwException ( Exception, "Could not resolve link %s : unknown ElementId '%llx'\n",
                    linkElt.generateVersatileXPath().c_str(), eId );
              }
            result.pushBack(target,true);
          }
        else if ( linkElt.hasAttr(xem_fs.target_id(),AttributeType_String))
          {
            String eId = linkElt.getAttr(xem_fs.target_id());
            XemProcessor& xemProc = XemProcessor::getMe(getXProcessor());
            xemProc.resolveNodeAndPushToNodeSet(result,eId);
          }
        else if ( linkElt.hasAttr(xem_fs.target(),AttributeType_String))
          {
            String targetPath = linkElt.getAttr(xem_fs.target());
            std::list<String> path;
            ElementRef current = linkElt.getFather();
            targetPath.tokenize(path,'/');
            for ( std::list<String>::iterator iter = path.begin() ; iter != path.end() ; iter++ )
              {
                if ( *iter == ".." )
                  current = current.getFather();
                else
                  current = current.lookup(getXProcessor(),xem_fs.folder_contents(), *iter);
                if ( ! current )
                  {
                    throwException ( Exception, "Could not resolve link %s\n", targetPath.c_str() );
                  }
              }
            result.pushBack(current,true);
          }
        else
          {
            // Warn ( "NotImplemented : Another way to resolve links..\n" );
            NotImplemented ( "Another way to resolve links..\n" );
          }
      }
  }

  void
  XemFSModule::instructionBrowse(__XProcHandlerArgs__)
  {
    String url = item.getEvaledAttr(getXProcessor(), xem_fs.href());
    BrowseSettings settings;

    if (item.hasAttr(xem_fs.get_mime_type()))
      settings.mayGetMimeType = (item.getEvaledAttr(getXProcessor(),
          xem_fs.get_mime_type()) == "yes");
    if (item.hasAttr(xem_fs.get_blob()))
      settings.mayGetBlob = (item.getEvaledAttr(getXProcessor(),
          xem_fs.get_blob()) == "yes");

    for ( ChildIterator child(item) ; child ; child++ )
      {
        if ( child.getKeyId() == xem_fs.exclude_name() )
          {
            String name = child.getEvaledAttr(getXProcessor(),xem_fs.name());
            settings.skipNames.push_back(name);
          }
        else if ( child.getKeyId() == xem_fs.exclude_suffix() )
          {
            String suffix = child.getEvaledAttr(getXProcessor(),xem_fs.suffix());
            settings.skipSuffixes.push_back(suffix);
          }
      }

    if ( item.hasAttr(xem_fs.select()))
      {
        XPath targetXPath(getXProcessor(), item, xem_fs.select());
        ElementRef target = targetXPath.evalElement();

        Log_XemFS ( "Browse : url=%s, target=%s, getMimeType=%s, getBlob=%s\n",
            url.c_str(), target.generateVersatileXPath().c_str(),
            settings.mayGetMimeType ? "yes" : "no",
            settings.mayGetBlob ? "yes" : "no" );


        doBrowse(target, url, url, -1, settings );
      }
    else
      {
        ElementRef target = getNodeFlow().getCurrentElement();

        Log_XemFS ( "Browse : url=%s, target=%s, getMimeType=%s, getBlob=%s\n",
            url.c_str(), target.generateVersatileXPath().c_str(),
            settings.mayGetMimeType ? "yes" : "no",
            settings.mayGetBlob ? "yes" : "no" );

        doBrowse(target, url, url, -1, settings );
      }
  }

  void
  XemFSModule::functionHasBlob(__XProcFunctionArgs__)
  {
    ElementRef blobElement = args[0]->toElement();
    String blobName = args[1]->toString();
    KeyId blobNameId = getKeyCache().getKeyIdWithElement(callingXPath.getSourceElementRef(),blobName);

    if (!blobNameId)
      throwException ( Exception, "Could not parse blob nameId '%s'\n", blobName.c_str() );

    BlobRef blobRef = blobElement.findAttr(blobNameId, AttributeType_SKMap);
    if (blobRef)
      result.setSingleton(true);
    else
      result.setSingleton(false);
  }

  void
  XemFSModule::functionGetBlobSize(__XProcFunctionArgs__) // Temporary
  {
    if ( args.size() ) { throwException(Exception,"Erroneous number of arguments !\n"); }

    BlobRef blobRef = node.toAttribute();
    AssertBug ( blobRef, "No blob ref !\n" );

    Integer blobSize = blobRef.getSize();

    Log_XemFS ( "BLOB SIZE : 0x%llx\n", (__ui64) blobSize );

    result.setSingleton(blobSize);
  }

  void
  XemFSModule::functionGetBlobMimeType(__XProcFunctionArgs__) // Get Blob MimeType
  {
    if ( args.size() ) { throwException(Exception,"Erroneous number of arguments !\n"); }

    BlobRef blobRef = node.toAttribute();

    AssertBug ( blobRef, "No blob ref !\n" );

    String mimeType = blobRef.getMimeType();
    if ( mimeType.c_str() && *mimeType.c_str() )
      {
        Log_XemFS ( "Got mimeType from blob : '%s'\n", mimeType.c_str() );
      }
    else
      {
        mimeType = getTypedModuleForge<XemFSModuleForge>().getMimeType(blobRef);
        Log_XemFS ( "Lookup mimeType : '%s'\n", mimeType.c_str() );
      }

    result.setSingleton(mimeType);
  }

  void
  XemFSModule::instructionBlobReadFromFile(__XProcHandlerArgs__)
  {
    String href = item.getEvaledAttr(getXProcessor(), xem_fs.href());

    XPath blobXPath(getXProcessor(), item, xem_fs.select());
    ElementRef blobElement = blobXPath.evalElement();

    blobElement.getDocument().grantWrite(getXProcessor());

    KeyId blobNameId = item.getAttrAsKeyId(getXProcessor(), xem_fs.blob_name());

    BlobRef blobRef = blobElement.findAttr(blobNameId, AttributeType_SKMap);
    if (!blobRef)
      {
        blobRef = blobElement.addBlob(blobNameId);
      }
    try
      {
        blobRef.readFromFile(href);
      }
    catch ( Exception* e )
      {
        blobElement.deleteAttr(blobNameId,AttributeType_SKMap);
        throw ( e );
      }

    String mimeType = getTypedModuleForge<XemFSModuleForge>().getMimeType(blobRef);
    Log_XemFS ( "Setting mimeType '%s'\n", mimeType.c_str() );
    blobRef.setMimeType(mimeType);
  }

  void
  XemFSModule::instructionBlobSaveToFile(__XProcHandlerArgs__)
  {
    String href = item.getEvaledAttr(getXProcessor(), xem_fs.href());

    /*
     * TODO Handle situations when blobXPath returns a blob
     */
    XPath blobXPath(getXProcessor(), item, xem_fs.select());
    // ElementRef blobElement = blobXPath.evalElement();
    NodeRef& blobNode = blobXPath.evalNode();

    if ( blobNode.isAttribute() )
      {
        BlobRef blobRef (blobNode.toAttribute());
        Log ( "Save blob (length %llu) to file %s\n", blobRef.getSize(), href.c_str() );
        blobRef.saveToFile(href);
      }
    else
      {
        KeyId blobNameId = item.getAttrAsKeyId(getXProcessor(), xem_fs.blob_name());
        BlobRef blobRef = blobNode.toElement().findAttr(blobNameId, AttributeType_Blob);
        if (!blobRef)
          {
            throwException ( Exception, "Could not find blob !\n" );
          }
        blobRef.saveToFile(href);
      }
  }

  void
  XemFSModule::instructionCreateBlob(__XProcHandlerArgs__)
  {
    XPath blobSourceXPath(getXProcessor(), item, xem_fs.select());
    ElementRef blobSourceElement = blobSourceXPath.evalElement();
    KeyId blobSourceNameId = item.getAttrAsKeyId(getXProcessor(), xem_fs.blob_name());

    BlobRef blobSourceRef = blobSourceElement.findAttr(blobSourceNameId,
        AttributeType_SKMap);

    if (blobSourceRef)
      {
        throwException ( Exception, "Blob already exists !\n" );
      }
    blobSourceRef.getDocument().grantWrite(getXProcessor());
    blobSourceRef = blobSourceElement.addBlob(blobSourceNameId);

    if (!blobSourceRef)
      {
        throwException ( Exception, "Could not create blob !\n" );
      }
  }

  void
  XemFSModule::instructionCopyBlob(__XProcHandlerArgs__)
  {
    XPath blobSourceXPath(getXProcessor(), item, xem_fs.select_source());
    ElementRef blobSourceElement = blobSourceXPath.evalElement();
    KeyId blobSourceNameId = item.getAttrAsKeyId(getXProcessor(),
        xem_fs.source_blob_name());

    XPath blobTargetXPath(getXProcessor(), item, xem_fs.select_target());
    ElementRef blobTargetElement = blobTargetXPath.evalElement();
    KeyId blobTargetNameId = item.getAttrAsKeyId(getXProcessor(),
        xem_fs.target_blob_name());

    BlobRef blobSourceRef = blobSourceElement.findAttr(blobSourceNameId,
        AttributeType_SKMap);

    if (!blobSourceRef)
      {
        throwException ( Exception, "Could not get source Blob !\n" );
      }

    BlobRef blobTargetRef = blobTargetElement.findAttr(blobTargetNameId,
        AttributeType_SKMap);

    blobTargetElement.getDocument().grantWrite(getXProcessor());
    if (!blobTargetRef)
      {
        blobTargetRef = blobTargetElement.addBlob(blobTargetNameId);
      }
    blobTargetRef.copyFrom(blobSourceRef);
  }

  void
  XemFSModule::domEventMimeType ( DomEventType domEventType, ElementRef& domEventElement, NodeRef& nodeRef )
  {
#if 0
    if ( domEventType == DomEventType_CreateAttribute || domEventType == DomEventType_AfterModifyAttribute )
      {
        AttributeRef mimeTypeAttr = nodeRef.toAttribute();
        ElementRef documentElement = mimeTypeAttr.getElement();
        if ( documentElement.getKeyId() != xem_fs.document() )
          {
            Warn ( "Skipping mime-type event for Element %s\n", documentElement.generateVersatileXPath().c_str() );
          }

        String mimeType = mimeTypeAttr.toString();
        if ( mimeType == "audio/mpeg" || mimeType == "application/ogg" )
          {
            getId3(documentElement);
          }
      }
#endif
  }

  void
  XemFSModule::install()
  {

  }

  void
  XemFSModuleForge::install()
  {
    registerFunction ( xem_fs.get_current_time(), &XemFSModule::functionGetCurrentTime );

    registerHandler ( xem_fs.read_blob_from_file(), &XemFSModule::instructionBlobReadFromFile );
    registerHandler ( xem_fs.save_blob_to_file(), &XemFSModule::instructionBlobSaveToFile );

    registerHandler ( xem_fs.create_blob(), &XemFSModule::instructionCreateBlob );
    registerHandler ( xem_fs.copy_blob(), &XemFSModule::instructionCopyBlob );
    registerFunction ( xem_fs.has_blob(), &XemFSModule::functionHasBlob );
    registerFunction ( xem_fs.get_mime_type(), &XemFSModule::functionGetMimeType );
    registerFunction ( xem_fs.get_blob_size(), &XemFSModule::functionGetBlobSize );
    registerFunction ( xem_fs.get_blob_mime_type(), &XemFSModule::functionGetBlobMimeType );

    registerHandler ( xem_fs.browse(), &XemFSModule::instructionBrowse );
    registerFunction ( xem_fs.browse(), &XemFSModule::functionBrowse );

    registerHandler ( xem_fs.make_link(), &XemFSModule::instructionMakeLink );
    registerFunction ( xem_fs.resolve_link(), &XemFSModule::functionResolveLink );

    registerHandler ( xem_fs.get_id3(), &XemFSModule::instructionGetId3 );

    registerHandler ( xem_fs.run_command_service(), &XemFSModule::instructionRunCommandService );
    registerHandler ( xem_fs.send_command(), &XemFSModule::instructionSendCommand );
    registerFunction ( xem_fs.get_recv_document(), &XemFSModule::functionGetRecvDocument );
    registerFunction ( xem_fs.recv_command(), &XemFSModule::functionRecvCommand );

    registerDomEventHandler(xem_fs.mime_type_trigger(),&XemFSModule::domEventMimeType);
  }


  void
  XemFSModuleForge::registerEvents(Document& doc)
  {
    registerXPathAttribute ( doc, xem_fs.select() );
    registerXPathAttribute ( doc, xem_fs.select_source() );
    registerXPathAttribute ( doc, xem_fs.select_target() );

    doc.getDocumentMeta().getDomEvents().registerEvent(DomEventMask_Attribute,
        xem_fs.mime_type(), xem_fs.mime_type_trigger());
  }

};
