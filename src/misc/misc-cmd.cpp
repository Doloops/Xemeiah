#include <Xemeiah/kern/volatilestore.h>
#include <Xemeiah/kern/servicemanager.h>
#include <Xemeiah/xprocessor/xprocessor.h>
#include <Xemeiah/kern/volatiledocument.h>
#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/parser/parser.h>
#include <Xemeiah/io/filereader.h>
#include <Xemeiah/parser/saxhandler-null.h>

#include <Xemeiah/xprocessor/xprocessorlib.h>

#include <Xemeiah/auto-inline.hpp>

namespace Xem
{
  int
  XemMiscCmd(int argc, char** argv);

  int
  XemM3U2XSPF(const char* file);

  __XProcessorLib_DECLARE_LIB_INTERNAL(Miscellaneous,"misc");
  __XProcessorLib_REGISTER_CMDLINEHANDLER(Miscellaneous,XemMiscCmd);

  int
  XemNormalize(const char* file)
  {
    VolatileStore store;

    XProcessor xproc(store);

    ElementRef root = xproc.getDocumentRoot(file);

    root.toXML(1, ElementRef::Flag_ChildrenOnly | ElementRef::Flag_NoIndent
        | ElementRef::Flag_XMLHeader
        | ElementRef::Flag_SortAttributesAndNamespaces);

    return 0;
  }

  int
  XemParseNull(const char* file)
  {
    FileReader fileReader(file);
    SAXHandlerNull saxHandler;

    Parser parser(fileReader, saxHandler);

    parser.parse();

    return 0;
  }

  int
  XemParseDom(const char* file)
  {
    VolatileStore store;

    XProcessor xproc(store);

    xproc.installAllModules();

    ElementRef root = xproc.getDocumentRoot(file);

    Document& doc = root.getDocument();
    Info ( "After parsing : document has %llu areas alloced, %llu areas mapped (area size=%llu kb).\n",
        doc.getNumberOfAreasAlloced(), doc.getNumberOfAreasMapped (), doc.getAreaSize() >> 10 );

    store.stats.showStats();

    return 0;
  }

  int
  XemRunProcedure(const char* file)
  {
    VolatileStore store;

    XProcessor xproc(store);

    xproc.loadLibrary("xemprocessor");

    xproc.runProcedure(String(file));

    store.getServiceManager().waitTermination();
    return 0;
  }

  int
  XemMiscCmd(int argc, char** argv)
  {
    if (argc == 2)
      {
        Error ( "xem misc : shall provide action.\n" );
        Error ( "\t Syntax : xem misc [normalize|parse|parse-null|run|m3u2xspf] file\n" );
        return -1;
      }
    const char* cmd = argv[2];

    try
      {
        if (strcmp(cmd, "normalize") == 0)
          {
            const char* file = argv[3];
            return XemNormalize(file);
          }
        else if (strcmp(cmd, "parse-null") == 0)
          {
            const char* file = argv[3];
            return XemParseNull(file);
          }
        else if (strcmp(cmd, "parse") == 0)
          {
            const char* file = argv[3];
            return XemParseDom(file);
          }
        else if (strcmp(cmd, "run") == 0)
          {
            const char* file = argv[3];
            return XemRunProcedure(file);
          }
        else if (strcmp(cmd, "m3u2xspf") == 0)
          {
            const char* file = argv[3];
            return XemM3U2XSPF(file);
          }
        Fatal ( "Invalid command '%s'\n", cmd );
      }
    catch (Exception * e)
      {
        Error ( "Encountered exception : %s\n", e->getMessage().c_str() );
        delete (e);
        return 1;
      }
    return 1;
  }

};
