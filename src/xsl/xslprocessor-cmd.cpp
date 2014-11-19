#include <Xemeiah/xprocessor/xprocessorlib.h>
#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/dom/skmapref.h>
#include <Xemeiah/dom/elementmapref.h>
#include <Xemeiah/xpath/xpath.h>
#include <Xemeiah/nodeflow/nodeflow-file.h>
#include <Xemeiah/dom/elementmapref.h>
#include <Xemeiah/kern/volatilestore.h>
#include <Xemeiah/xsl/xslprocessor.h>

#include <Xemeiah/version.h>
#include <Xemeiah/log-time.h>

#include <Xemeiah/auto-inline.hpp>

#define __XSLPROCESSOR_TIME // Option : show parsing and processing times

#define Log_XSLCmd Debug
#define Usage(...) fprintf ( stderr, __VA_ARGS__ )

namespace Xem
{
  int XemRunXSLProcessor ( int argc, char** argv );

  __XProcessorLib_REGISTER_CMDLINEHANDLER(XSL, XemRunXSLProcessor);


  int printUsage ( const char* progName, const char* groupCmd )
  {
    Usage ( "Xemeiah " __XEM_VERSION " XSLT Processing :\n" );
    Usage ( "Usage : %s %s [options] stylesheet xml-file\n", progName, groupCmd );
    Usage ( "Where options are :\n" );
    Usage ( "\t-V --version .\n" );
    Usage ( "\t   --dump-xsl : dump the XSL tree as parsed, to output.\n" );
    Usage ( "\t   --dump-xml : dump the XSL tree as parsed, to output.\n" );
    Usage ( "\t   --noout : do not output result.\n" );
    Usage ( "\t   --nowrite : disable xsl:result-document instruction.\n" );
    Usage ( "\t   --nomkdir : disable directory creation in xsl:result-document instruction.\n" );
    Usage ( "\t-D --maxdepth val : increase (or decrease) the maximum depth of XSL processing.\n" );
    Usage ( "\t-T --timing : dump information on parse and process time.\n" );
    Usage ( "\t-P --param 'name' 'value' : sets a stylesheet parameter.\n" ); 
    Usage ( "\t-C --conflict : give a warning when templates are in conflict.\n" ); 
    Usage ( "\t-I --iterations n : repeat XSL processing n times.\n" );
    Usage ( "\t-o --output FILE : save to a given file.\n" );
    Usage ( "\t   --disable-messages : disable MESSAGE: messages.\n" );
    Usage ( "\t   --module 'module name' : load external XProcessor extension library (deprecated).\n" );
    Usage ( "\t   --library 'module name' : load external XProcessor extension library.\n" );
    Usage ( "\t   --setting 'name' 'value' : set a XProcessor Settings parameter.\n" );
    Usage ( "\t   --dumpextensions : dump the registered extension elements and functions to stderr.\n" );
    return 1;
  }

  int XemRunXSLProcessor ( int argc, char** argv )
  {
    if ( argc == 2 )
      {
        return printUsage ( argv[0], argv[1] );        
      }
    
    /*
     * Beginning of the Store allocation section, used to unwind stack
     */
    {
    
    VolatileStore store;

    /*
     * Beginning of the XProc allocation section, used to unwind stack
     */
    {
    XProcessor xproc(store);
    
    /**
     * Install default PI handler (unused)
     */
    xproc.installModule ( "http://www.xemeiah.org/ns/xemint-pi" );

    /**
     * Install XSL namespace handler
     */
    xproc.installModule ( "http://www.w3.org/1999/XSL/Transform" );
    xproc.installModule ( "http://www.xemeiah.org/ns/xem-xsl-implementation" );

#if 0
    /**
     * Install EXSL namespace handler
     */
    xproc.installModule ( "http://exslt.org/common" );
    xproc.installModule ( "http://exslt.org/dynamic" );    
    xproc.installModule ( "http://exslt.org/sets" );    
#endif

    xproc.loadLibrary("exslt", false);

    XSLProcessor& xslProcessor = XSLProcessor::getMe(xproc);

    NodeFlowFile nodeFlow ( xproc );
    xproc.setNodeFlow ( nodeFlow );

    /*
     * Default NodeFlow configuration : set to stdout
     */
    nodeFlow.setFD ( stdout );

    const char* xslFile = NULL;
    const char* xmlFile = NULL;

    bool dumpXSL = false;
    bool dumpXML = false;
    bool showTiming = false;

    int iterations = 1; // Number of times we have to iterate
    
    for ( int a = 2 ; a < argc ; a++ )
      {
        Log_XSLCmd ( "argv[%d]='%s'\n", a, argv[a] );
        if ( argv[a][0] == '-' && argv[a][1] != '\0' )
          {
#define __isOption(__short,__long) ( ( argv[a][1] == __short && argv[a][2] == '\0' ) || strcmp ( argv[a], __long ) == 0 )
            if ( __isOption( 'V', "--version" ) )
              {
                Usage ( "Xemeiah " __XEM_VERSION " XSLT Processing :\n" );
                return 0;
              }
            else if ( __isOption ( 0, "--dump-xsl" ) )
              {
                dumpXSL = true;
              }
            else if ( __isOption ( 0, "--dump-xml" ) )
              {
                dumpXML = true;
              }
            else if ( __isOption ( 0, "--module" ) || __isOption ( 0, "--library" ) )
              {
                if ( a + 1 == argc )
                  {
                    Usage ( "Invalid parameter syntax for '--library'\n" );
                    return printUsage ( argv[0], argv[1] );
                  }
                const char* libName = argv[a+1];
                a++;
                xproc.loadLibrary ( libName );
              }
            else if ( __isOption ( 0, "--noout" ) )
              {
                nodeFlow.setFD(-1);
              }
            else if ( __isOption ( 0, "--nowrite" ) )
              {
                xproc.setString ( xslProcessor.xslimpl.disable_result_document(), "true" );
              }
            else if ( __isOption ( 0, "--nomkdir" ) )
              {
                xproc.setString ( xslProcessor.xslimpl.disable_result_document_mkdir(), "true" );
                nodeFlow.disableMkdir ();
              }
            else if ( __isOption ( 'o', "--output" ) )
              {
                if ( a + 1 == argc )
                  {
                    Usage ( "Invalid parameter syntax for '--output'\n" );
                    return printUsage ( argv[0], argv[1] );
                  }
                const char* outName = argv[a+1];
                a++;
                nodeFlow.setFile(outName);
              }
            else if ( __isOption ( 0, "--disable-messages" ) )
              {
                xproc.setString ( xslProcessor.xslimpl.disable_messages(), "true" );
              }
            else if ( __isOption ( 'D', "--maxdepth" ) )
              {
                int depth = 0;
                if ( a + 1 < argc && ( depth = atoi(argv[a+1]) ) )
                  {
                    Log_XSLCmd ( "Setting maxdepth to %d\n", depth );
                    xproc.setMaxLevel ( depth );
                    a++;
                  }
                else
                  {
                    Usage ( "Invalid maxdepth value.\n" );
                    return printUsage ( argv[0], argv[1] );
                  }
              }
            else if ( __isOption ( 'I', "--iterations" ) )
              {
                int iters = 0;
                if ( a + 1 < argc && ( iters = atoi(argv[a+1]) ) )
                  {
                    Log_XSLCmd ( "Setting iterations to %d\n", iters );
                    iterations = iters;
                    a++;
                  }
                else
                  {
                    Usage ( "Invalid maxdepth value.\n" );
                    return printUsage ( argv[0], argv[1] );
                  }
              }
            else if ( __isOption ( 'T', "--timing" ) )
              {
                showTiming = true;
              }
            else if ( __isOption ( 'P', "--param" ) )
              {
                if ( a + 2 >= argc )
                  {
                    Usage ( "Incomplete param option : shall provide --param 'name' 'value'\n" );
                    return printUsage ( argv[0], argv[1] );
                  }
                const char* paramName = argv[a+1];
                const char* paramValue = argv[a+2];
                
                NamespaceAlias defaultNamespaceAlias ( store.getKeyCache() );
                KeyId keyId = store.getKeyCache().getKeyId ( defaultNamespaceAlias, paramName, false );
                if ( keyId == 0 )
                  {
                    Usage ( "Invalid parameter name '%s'\n", paramName );
                    return printUsage ( argv[0], argv[1] );
                  }
                xproc.setString ( keyId, strdup(paramValue) );
                a+=2;
              }
            else if ( __isOption ( 0, "--setting" ) )
              {
                if ( a + 2 >= argc )
                  {
                    Usage ( "Incomplete param option : shall provide --param 'name' 'value'\n" );
                    return printUsage ( argv[0], argv[1] );
                  }
                const char* paramName = argv[a+1];
                const char* paramValue = argv[a+2];
                if ( strcmp(paramName,"documentAllocatorMaximumAge") == 0 )
                  {
                    xproc.setSettings().documentAllocatorMaximumAge = atoi(paramValue);
                  }
                else if ( strcmp(paramName,"xpathChildLookupEnabled") == 0 )
                  {
                    xproc.setSettings().xpathChildLookupEnabled = atoi(paramValue);
                  }
                else if ( strcmp(paramName,"xpathChildLookupThreshold") == 0 )
                  {
                    xproc.setSettings().xpathChildLookupThreshold = atoi(paramValue);
                  }
                else
                  {
                    Usage ( "Unknown XProcessor setting : '%s'.", paramName );
                    return printUsage ( argv[0], argv[1] );
                  }
                a+=2;
              }
            else if ( __isOption ( 'C', "--conflict" ) )
              {
                xproc.setString ( xslProcessor.xslimpl.warn_on_conflicts(), "true" );
              }
            else if ( __isOption ( 0, "--dumpextensions") )
              {
                xproc.dumpExtensions ();
                return 0;
              }
            else
              {
                Usage ( "Unrecognized option '%s'\n", argv[a] );
                return printUsage ( argv[0], argv[1] );
              }
            continue;
          }  
        if ( ! xslFile )
          {
            xslFile = argv[a];
            Log_XSLCmd ( "xsl=%s\n", xslFile );
            continue;
          }
        if ( ! xmlFile )
          {
            xmlFile = argv[a];
            Log_XSLCmd ( "xml=%s\n", xmlFile );
            continue;
          }
        Usage ( "Invalid argument : %s\n", argv[a] );
        return printUsage ( argv[0], argv[1] );
      }

#define ShowTiming(__text,__begin,__start) if ( showTiming ) { __LogTime(Info,__text,__begin,__start); }
   
    try
      {
        NTime beginImportXSL = getntime ();

        ElementRef xslRoot = xproc.getDocumentRoot ( xslFile );
        // xproc.registerEvents(xslRoot.getDocument());

        NTime endImportXSL = getntime ();
        ShowTiming ( "Import XSL : ", beginImportXSL, endImportXSL );
        
        ElementRef xslStylesheet (xslRoot.getDocument());

        for ( xslStylesheet = xslRoot.getChild(); xslStylesheet ; xslStylesheet = xslStylesheet.getYounger() )
          {
            Log_XSLCmd ( "At toplevel : '%s'\n", xslStylesheet.getKey().c_str() );
            if ( xslStylesheet.getKeyId() == xslProcessor.xsl.stylesheet() )
              {
                Log_XSLCmd ( "Found stylesheet : '%s'\n", xslStylesheet.getKey().c_str() );
                break;
              }
          } 
        if ( dumpXSL )
          {
            fprintf ( stdout, "----------------- XSL Contents : ---------------------|\n" );
            xslRoot.toXML ( stdout, ElementRef::Flag_ShowElementId );
          }

        if ( ! xslStylesheet )
          {
            Error ( "No valid xsl:stylesheet in file !\n" );
            return 1;
          }
          
#if 1
        /*
         * Prepare stylesheet is performed at parse-time (at end for xsl:stylesheet)
         */     
        xproc.setElement ( xslProcessor.xslimpl.main_stylesheet(), xslStylesheet );
#endif
              
        NTime beginImportXML = getntime ();

        ElementRef xmlTree = xproc.getDocumentRoot ( xmlFile );

        NTime endImportXML = getntime ();
        ShowTiming ( "Import XML : ", beginImportXML, endImportXML );

        if ( dumpXML )
          {
            fprintf ( stdout, "----------------- XML Contents : ---------------------|\n" );
            xmlTree.toXML ( stdout, ElementRef::Flag_ShowElementId );
          }

        Log_XSLCmd ("xmlTree=%s\n", xmlTree.getKey().c_str() );

#ifdef __xslimpl_DUMP_TREES
        fprintf ( stdout, "----------------- Result Tree : ---------------------|\n" );
#endif

        NTime beginProcess = getntime ();

        NodeSet xmlTreeNodeSet;
        xmlTreeNodeSet.pushBack ( xmlTree );
        NodeSet::iterator xmlTreeIterator ( xmlTreeNodeSet, xproc );

//        for ( int iter = 0 ; iter < iterations ; iter++ )
        xproc.process ( xslStylesheet );


        NTime endProcess = getntime ();

        ShowTiming ( "Process XML : ", beginProcess, endProcess );
        ShowTiming ( "Total time : ", beginImportXSL, endProcess );

      }
    catch ( Exception * e )
      {
        Error ( "While processing : catched exception : \n%s",  e->getMessage().c_str() );
        delete ( e );
      }

    /*
     * End of the XProcessor allocation section, the store and xproc are un-allocated
     */
    }
      
    // store.stats.showStats ();

    /*
     * End of the Store allocation section, the store and xproc are un-allocated
     */
    }
    
    
    return 0;
  }
  
};

