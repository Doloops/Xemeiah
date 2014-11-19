/*
 * xem-valhalla.cpp
 *
 *  Created on: 20 déc. 2009
 *      Author: francois
 */

#include <Xemeiah/xem-valhalla/xem-valhalla.h>
#include <Xemeiah/xprocessor/xprocessorlib.h>
#include <Xemeiah/xemprocessor/xemprocessor.h>
#include <Xemeiah/kern/documentref.h>
#include <Xemeiah/io/stringreader.h>
#include <Xemeiah/io/bufferedwriter.h>

#include <Xemeiah/auto-inline.hpp>

#include <valhalla.h>

#define Log_VH Log
#define Log_VH0 Debug

#define __XEM_VALHALLA_CREATE_ONE_DOCUMENT //< Option : create only one document

namespace Xem
{
  __XProcessorLib_DECLARE_LIB ( XemValhalla, "xem-valhalla" );
  __XProcessorLib_REGISTER_MODULE ( XemValhalla, XemValhallaModuleForge );

#include <Xemeiah/kern/builtin_keys_prolog_inst.h>
#include <Xemeiah/xem-valhalla/builtin-keys/xem-vh>
#include <Xemeiah/xem-valhalla/builtin-keys/valhalla>
#include <Xemeiah/kern/builtin_keys_postlog.h>

  XemValhallaModuleForge::XemValhallaModuleForge ( Store& store )
  : XProcessorModuleForge(store), xem_vh(store.getKeyCache()), valhalla(store.getKeyCache())
  {

  }

  XemValhallaModuleForge::~XemValhallaModuleForge ()
  {

  }

  void XemValhallaModuleForge::install ()
  {
    registerHandler ( xem_vh.scan(), &XemValhallaModule::instructionScan );
    registerHandler ( xem_vh.media_player(), &XemValhallaModule::instructionMediaPlayer );
    registerHandler ( xem_vh.send_command(), &XemValhallaModule::instructionSendCommand );
    registerFunction ( xem_vh.get_status(), &XemValhallaModule::functionGetStatus );
  }

  void XemValhallaModuleForge::instanciateModule ( XProcessor& xprocessor )
  {
    XProcessorModule* module = new XemValhallaModule(xprocessor, *this);
    xprocessor.registerModule(module);
  }

  XemValhallaModule::XemValhallaModule ( XProcessor& xproc, XemValhallaModuleForge& moduleForge )
  : XProcessorModule(xproc,moduleForge),xem_vh(moduleForge.xem_vh),valhalla(moduleForge.valhalla)
  {

  }

  class XemValhallaArg
  {
  protected:
    XemValhallaModule& xemValhallaModule;
#ifdef __XEM_VALHALLA_CREATE_ONE_DOCUMENT
    ElementRef rootElement;
#endif
    ElementRef thisElement;
    valhalla_t* handle;
    String path;
  public:
    XemValhallaArg ( XemValhallaModule& _xemValhallaModule,
#ifdef __XEM_VALHALLA_CREATE_ONE_DOCUMENT
        const ElementRef& _rootElement,
#endif
        const ElementRef& _thisElement,
        const String& path_ )
    : xemValhallaModule(_xemValhallaModule),
#ifdef __XEM_VALHALLA_CREATE_ONE_DOCUMENT
      rootElement(_rootElement),
#endif
      thisElement(_thisElement)
    {
      handle = NULL;
      path = path_;
    }

    ~XemValhallaArg () {}

    XemValhallaModule& getModule() const { return xemValhallaModule; }

#ifdef __XEM_VALHALLA_CREATE_ONE_DOCUMENT
    const ElementRef& getRootElement() const { return rootElement; }
#endif

    const ElementRef& getThisElement() const { return thisElement; }

    valhalla_t* getValhalla() const { return handle; }

    void setValhalla ( valhalla_t* handle_ )
    { handle = handle_; }

    const String& getPath() const { return path; }
  };

#if 0
  static
  void xemValhallaEventMeta_callback ( valhalla_event_md_t e, const char* id, const valhalla_file_t* file, const valhalla_metadata_t* md, void* data )
  {
    Log_VH ( "META event %d : id=%s, path=%s, mtime=%lld, type=%d, name=%s, value=%s, group=%d,"
        " data=%p\n",
        e, id, file->path, file->mtime, file->type,
        md->name, md->value, md->group,
        data );
    /*
     * Get arguments, and acquire references to XProcessor and XemProcessor
     */
    XemValhallaArg* _arg = (XemValhallaArg*) ( data );
    XemValhallaArg& arg = dynamic_cast<XemValhallaArg&> ( *_arg );
    __BUILTIN_NAMESPACE_CLASS(valhalla)& valhalla = arg.getModule().valhalla;
    XProcessor& xproc = arg.getModule().getXProcessor();
  }
#endif


  void XemValhallaModule::updateXemValhallaMeta ( ElementRef& thisElement, ElementRef& fileElt, const String& pathPrefix )
  {
    XProcessor& xproc = getXProcessor();
    XemProcessor& xemProc = XemProcessor::getMe(xproc);

    /*
     * thisElement is the element to call to handle output
     * It must be unlocked write at end of operation
     */
    thisElement.getDocument().incrementRefCount();
    DocumentRef thisDocument(getXProcessor(), thisElement.getDocument());

    try
    {
      xproc.pushEnv();
      NodeSet* nodeSet = xproc.setVariable(KeyCache::getLocalKeyId(valhalla.file()), false );
      nodeSet->pushBack(fileElt,false);
      xproc.setString(KeyCache::getLocalKeyId(xem_vh.prefix()), pathPrefix );
      Log_VH ( "** Calling handleValhallaFile **\n");
      xemProc.callMethod(thisElement,"handleValhallaFile");
      xproc.popEnv();
    }
    catch ( Exception * e )
    {
      Error ( "Thrown exception : %s\n", e->getMessage().c_str() );
      delete ( e );
      xproc.popEnv();
    }
  }

#if 0
  void XemValhallaModule::updateXemValhallaMeta ( ElementRef& thisElement, const String& path, const String& group, const String& name, const String& value )
  {
    XProcessor& xproc = getXProcessor();

    xproc.pushEnv();


#ifdef __XEM_VALHALLA_CREATE_ONE_DOCUMENT
    ElementRef rootElement ( thisElement.getDocument() );

    ElementRef files = rootElement.getChild();
    ElementRef fileElt = files.getDocument().createElement(files, valhalla.file());
    fileElt.addAttr(KeyCache::getLocalKeyId(valhalla.path()), path);

    files.appendChild(fileElt);
#else
    ElementRef rootElement = xproc.createVolatileDocument(false);
    ElementRef fileElt = rootElement.getDocument().createElement(rootElement, valhalla.file());
    fileElt.addAttr(KeyCache::getLocalKeyId(valhalla.path()), path);
    fileElt.addNamespaceAlias(valhalla.defaultPrefix(), valhalla.ns());
#endif

    Log_VH ( "\t%s = %s (%s)\n", name.c_str(), value.c_str(), group.c_str() );
    ElementRef meta = fileElt.getDocument().createElement(fileElt, valhalla.meta());
    meta.addAttr(KeyCache::getLocalKeyId(valhalla.name()), name );
    meta.addAttr(KeyCache::getLocalKeyId(valhalla.group()), group );
    ElementRef valueElt = meta.getDocument().createTextNode(meta, value );
    meta.appendChild(valueElt);
    fileElt.appendChild(meta);

    updateXemValhallaMeta(thisElement, fileElt);

    xproc.popEnv();
    Log_VH ( "META : finished handling of file->path=%s\n", path.c_str() );
  }
#endif

  String doNormalize ( const char* val )
  {
#if 0 // Debug : dumping some text
    if ( strstr ( val, "rurier Noir") )
      {
        Log_VH ( "RURIER : '%s'\n", val );
        for ( unsigned char* d = (unsigned char*) val ; *d; d++ )
          {
            Log_VH ( "Char %d : '%c'\n", *d, *d );
          }
      }
#endif

    String sVal ( val );
    StringReader reader ( sVal );
    reader.setEncoding(Encoding_Unknown);
    BufferedWriter writer;

    bool isStart = true;
    bool hasSpace = false;

    while ( ! reader.isFinished() )
      {
        int utf = reader.getNextChar();

        switch ( utf )
        {
        case ' ':
        case '\r':
        case '\t':
        case '\n':
        case '_':
          if ( ! isStart )
            hasSpace = true;
          break;
        case '(':
        case ')':
        case '[':
        case ']':
        case '.':
        case '/':
        case '-':
          if ( true ) // ! isStart && ( *v == '.' || v[1] ) )
            {
              if ( hasSpace )
                {
                  writer.addChar(' ');
                  hasSpace = false;
                }
              isStart = true;
              writer.addUtf8(utf);
            }
          break;
        default:
          if ( hasSpace )
            {
              writer.addChar(' ');
            }
          if ( hasSpace || isStart )
            {
              if ( 'a' <= utf && utf <= 'z' )
                {
                  writer.addChar(utf + 'A' - 'a');
                }
              else
                {
                  writer.addUtf8(utf);
                }
            }
          else
            {
              if ( 'A' <= utf && utf <= 'Z' )
                {
                  writer.addChar(utf + 'a' - 'A');
                }
              else
                {
                  writer.addUtf8(utf);
                }

            }
          hasSpace = false;
          isStart = false;
        }
      }
    Info ( "Normalize '%s' => '%s'\n", val, writer.toString().c_str() );
    return writer.toString().copy();
  }

  int result_cb (void *data, valhalla_db_fileres_t *res)
  {
    Log_VH0 ( "RES : id=%llx, path=%s, type=%d\n", res->id, res->path, res->type );
    XemValhallaArg* xemValhallaArg_ = (XemValhallaArg*) data;
    XemValhallaArg& xemValhallaArg = dynamic_cast<XemValhallaArg&> (*xemValhallaArg_);

    __BUILTIN_NAMESPACE_CLASS(valhalla) &valhalla = xemValhallaArg.getModule().valhalla;

    valhalla_db_filemeta_t* filemeta;
    valhalla_db_file_get(xemValhallaArg.getValhalla(),res->id, res->path, NULL, &filemeta);

    XProcessor& xproc = xemValhallaArg.getModule().getXProcessor();
    xproc.pushEnv ();

#ifdef __XEM_VALHALLA_CREATE_ONE_DOCUMENT
    ElementRef rootElement = xemValhallaArg.getRootElement();

    ElementRef files = rootElement.getChild();
    ElementRef fileElt = files.getDocument().createElement(files, valhalla.file());
    fileElt.addAttr(KeyCache::getLocalKeyId(valhalla.path()), res->path);

    files.appendChild(fileElt);
#else
    ElementRef rootElement = xproc.createVolatileDocument(false);
    ElementRef fileElt = rootElement.getDocument().createElement(rootElement, valhalla.file());
    fileElt.addAttr(KeyCache::getLocalKeyId(valhalla.path()), res->path);
    fileElt.addNamespaceAlias(valhalla.defaultPrefix(), valhalla.ns());
#endif

    for ( valhalla_db_filemeta_t* m = filemeta ; m ; m = m->next )
      {
        Log_VH0 ( "\tMETA : id=%llx, group=%s (%d), name=%s, value=%s\n", m->meta_id,
            valhalla_metadata_group_str(m->group), m->group,
            m->meta_name, m->data_value );
        ElementRef meta = fileElt.getDocument().createElement(fileElt, valhalla.meta());
        meta.addAttr(KeyCache::getLocalKeyId(valhalla.name()), m->meta_name );
#if 0
        const char* group = valhalla_metadata_group_str(m->group);
        meta.addAttr(KeyCache::getLocalKeyId(valhalla.group()), group );
#endif
        String sValue /* = m->data_value */;
        if ( m->group == VALHALLA_META_GRP_TITLES
          || m->group == VALHALLA_META_GRP_CLASSIFICATION
          || m->group == VALHALLA_META_GRP_ENTITIES )
          {
            sValue = doNormalize ( m->data_value );
          }
        else
          {
            sValue = m->data_value;
          }
        ElementRef valueElt = meta.getDocument().createTextNode(meta, sValue );
        meta.appendChild(valueElt);
        fileElt.appendChild(meta);
      }
    ElementRef thisElement = xemValhallaArg.getThisElement();
    xemValhallaArg.getModule().updateXemValhallaMeta(thisElement, fileElt, xemValhallaArg.getPath() );
    xproc.popEnv ();
    VALHALLA_DB_FILEMETA_FREE(filemeta);

#if 0
    static int ri = 0;
    ri++;
    if ( ri >= 1000 ) return 1;
#endif
    return 0;
  }

  void
  XemValhallaModule::instructionScan(__XProcHandlerArgs__)
  {
    ElementRef thisElement = getXProcessor().getVariable(getKeyCache().getBuiltinKeys().nons.this_())->toElement();
    String path = item.getEvaledAttr(getXProcessor(),xem_vh.path());

#ifdef __XEM_VALHALLA_CREATE_ONE_DOCUMENT
    ElementRef rootElement = getXProcessor().createVolatileDocument(false);
    ElementRef files = rootElement.getDocument().createElement(rootElement, valhalla.files());
    files.addNamespaceAlias(valhalla.defaultPrefix(), valhalla.ns());
    rootElement.appendChild(files);
#endif // __XEM_VALHALLA_CREATE_ONE_DOCUMENT

    XemValhallaArg xemValhallaArg ( *this,
#ifdef __XEM_VALHALLA_CREATE_ONE_DOCUMENT
        rootElement,
#endif // __XEM_VALHALLA_CREATE_ONE_DOCUMENT
        thisElement, path );

    valhalla_t *handle;
    valhalla_init_param_t param;
    memset (&param, 0, sizeof (param));
    param.parser_nb   = 2;
    param.commit_int  = 10;
    param.decrapifier = 1;

#if 0
    param.md_cb     = &xemValhallaEventMeta_callback;
    param.md_data = &xemValhallaArg;
#endif

    // valhalla_verbosity(VALHALLA_MSG_VERBOSE);
    valhalla_verbosity(VALHALLA_MSG_WARNING);

    // const char* db = "noop";
    String dbPath = item.getEvaledAttr(getXProcessor(),xem_vh.db_path());
    Log_VH ( "Configuring for dbPath=%s\n", dbPath.c_str() );

    handle = valhalla_init (dbPath.c_str(), &param);
    if (!handle)
      {
        throwException ( Exception, "Could not initialize Valhalla !\n" );
      }

    xemValhallaArg.setValhalla(handle);

    const char *const suffixes[] = {
      "flac", "m4a", "mp3",  "ogg", "wav", "wma",                 /* audio */
      "avi",  "mkv", "mov",  "mpg", "wmv", "iso",                 /* video */
      "bmp",  "gif", "jpeg", "jpg", "png", "tga", "tif", "tiff",  /* image */
      NULL
    };
    for (const char* const* suffix = suffixes; *suffix; suffix++)
    {
      valhalla_config_set (handle, SCANNER_SUFFIX, *suffix);
    }

    /*
     * Add Path entry
     */
    valhalla_config_set (handle, SCANNER_PATH, path.c_str(), 1);

    /*
     * Add download destination
     */
    String coverPath = item.getEvaledAttr(getXProcessor(),xem_vh.cover_path());
    valhalla_config_set (handle,
                         DOWNLOADER_DEST, coverPath.c_str(), VALHALLA_DL_DEFAULT);

    const char* grabber = NULL;
    while ((grabber = valhalla_grabber_next (handle, grabber)))
      {
        Log_VH ("Available grabber : %s\n", grabber);
        valhalla_config_set (handle, GRABBER_STATE, grabber, 0);
      }
    valhalla_config_set (handle, GRABBER_STATE, "ffmpeg", 1);
    valhalla_config_set (handle, GRABBER_STATE, "local", 1);
    valhalla_config_set (handle, GRABBER_STATE, "exif", 1);
    valhalla_config_set (handle, GRABBER_STATE, "amazon", 1);

    Log_VH ( "Calling valhalla_run()...\n");

#if 1
    /*
     * Run
     */
    int loop_nb = 1;
    int loop_wait = 0;
    int priority = 0;

    int rc = valhalla_run (handle, loop_nb, loop_wait, priority);
    if (rc)
      {
        Error ( "Error code: %i at valhalla_run()\n", rc);
        valhalla_uninit (handle);
        throwException ( Exception, "Could not scan '%s' : err %i\n", path.c_str(), rc );
      }

    Log_VH ( "Finished running scan, waiting...\n" );
    valhalla_wait (handle);
    Log_VH ( "Finished running scan, waiting... OK\n" );
#endif

    Log_VH ( "Browsing valhalla results...\n" );
    valhalla_db_filelist_get ( handle, VALHALLA_FILE_TYPE_AUDIO, NULL, result_cb, &xemValhallaArg );
    valhalla_db_filelist_get ( handle, VALHALLA_FILE_TYPE_VIDEO, NULL, result_cb, &xemValhallaArg );
    valhalla_db_filelist_get ( handle, VALHALLA_FILE_TYPE_IMAGE, NULL, result_cb, &xemValhallaArg );

    Log_VH ( "Finished waiting...\n" );
    valhalla_uninit (handle);
    Log_VH ( "Finished uninit...\n" );

  }

};
