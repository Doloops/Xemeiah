#include <Xemeiah/version.h>

#include <Xemeiah/kern/store.h>
#include <Xemeiah/nodeflow/nodeflow-stream.h>
#include <Xemeiah/nodeflow/nodeflow-dom.h>
#include <Xemeiah/parser/parser.h>
#include <Xemeiah/xpath/xpath.h>

#include <Xemeiah/kern/volatiledocument.h>
#include <Xemeiah/kern/servicemanager.h>

#include <Xemeiah/dom/skmapref.h>
#include <Xemeiah/persistence/persistentstore.h>
#include <Xemeiah/persistence/persistentbranchmanager.h>
#include <Xemeiah/persistence/persistentdocument.h>

#include <Xemeiah/xprocessor/xprocessor.h>
#include <Xemeiah/xprocessor/xprocessorlib.h>

#include <Xemeiah/dom/item-base.h>

#include <Xemeiah/auto-inline.hpp>
#include <Xemeiah/log-time.h>

#include <Xemeiah/persistence/auto-inline.hpp>

#include <errno.h>
#include <string.h>

namespace Xem
{
    static const char* storeFile = "xem-main.xem";
    static const char* branchName = "Main";

    int
    XemRunPersistenceAction (int argc, char** argv);

    __XProcessorLib_DECLARE_LIB(Persistence, "persistence");
    __XProcessorLib_REGISTER_CMDLINEHANDLER(Persistence, XemRunPersistenceAction);

#define ShowTiming(__text,__begin,__start) if ( true ) { __LogTime(Info,__text,__begin,__start); }

    void
    XemShowStructSizes ()
    {
#define __showSize(_x) Info ( "\tsizeof(%s) : %lu bytes (0x%lx)\n", \
    #_x, (unsigned long) sizeof(_x), (unsigned long) sizeof(_x) );
        Info("Various sizes :\n");
        Info("Platform : \n");
        __showSize(int);
        __showSize(long int);
        __showSize(long long int);
        __showSize(__ui8);
        __showSize(__ui16);
        __showSize(__ui32);
        __showSize(__ui64);
        __showSize(double);

        Info("Core : \n");
        __showSize(Integer);
        __showSize(Number);
        __showSize(String);
        __showSize(Store);
        __showSize(KeyCache);
        __showSize(KeyCache::BuiltinKeys);
        __showSize(VolatileDocument);
        __showSize(PersistentDocument);

        Info("Persistence : \n");
        __showSize(AbsolutePagePtr);
        __showSize(RelativePagePtr);
        __showSize(SegmentPtr);
        __showSize(BranchId);
        __showSize(RevisionId);
        __showSize(BranchRevId);
        __showSize(SuperBlock);
        __showSize(FreePageHeader);
        __showSize(PageList);
        __showSize(BranchPage);
        __showSize(FreeSegment);
        __showSize(FreeSegmentsLevelHeader);
        __showSize(DocumentAllocationHeader);
        __showSize(IndirectionPage);
        __showSize(IndirectionHeader);
        __showSize(RevisionPage);
        __showSize(PageInfo);
        __showSize(PageInfoPage);
        Info("\tPageInfo_pointerNumber : %llu (0x%llx)\n", PageInfo_pointerNumber, PageInfo_pointerNumber);
        __showSize(SegmentPage*);
        __showSize(JournalItem);

        Info("Persistence - DOM : \n");
        __showSize(ElementSegment);
        __showSize(ElementTextualContents);
        __showSize(ElementAttributesAndChildren);
        __showSize(AttributeSegment);
        __showSize(SKMapHeader);
        __showSize(SKMapConfig);
        __showSize(SKMapItem);
        __showSize(SKMapList);
        __showSize(XPathSegment);
        __showSize(XPathStep);

        Info("DOM : \n");
        __showSize(String);
        __showSize(ElementRef);
        __showSize(AttributeRef);
        __showSize(SKMapRef);
        __showSize(SKMapRef::iterator);
        __showSize(SKMultiMapRef);
        __showSize(SKMultiMapRef::multi_iterator);
        __showSize(NodeSet);
        __showSize(ItemImpl<bool>);
        __showSize(ItemImpl<Integer>);
        __showSize(ItemImpl<Number>);
#if 0
        __showSize ( ItemImpl<ElementRef> );
        __showSize ( ItemImpl<AttributeRef> );
        __showSize ( ItemImpl<String> );
#endif

        Info("XPath : \n");
        __showSize(XPathStep);
        __showSize(XPath);
        __showSize(XSLNumberingIntegerConvertion);
        __showSize(XSLNumberingFormat);

    }

    int
    XemRunFileImport (PersistentStore& store, const char* filePath)
    {
        bool check = false;
        BranchId branchId = store.getBranchManager().getBranchId(branchName);
        if (!branchId)
        {
            branchId = store.getBranchManager().createBranch(branchName, BranchFlags_None);
        }

        NTime beginImport = getntime();

        XProcessor xproc(store);

        Document* targetDocument = store.getBranchManager().openDocument(branchId, 0, DocumentOpeningFlags_Write);
        targetDocument->incrementRefCount();
        {
            ElementRef targetRoot = targetDocument->getRootElement();
            NTime begin = getntime();
            Info("Importing : '%s'\n", filePath);
            Parser::parseFile(xproc, targetRoot, filePath, "xsl");
            NTime end = getntime();
            WarnTime("Import processing took : ", begin, end);
        }

        NTime endImport = getntime();

        ShowTiming("Import document : ", beginImport, endImport);

        targetDocument->commit(xproc);

        NTime endCommit = getntime();

        ShowTiming("Document commit : ", endImport, endCommit);

        store.releaseDocument(targetDocument);

        if (check)
        {
            store.check(PersistentStore::Check_AllContents);
        }
        // store.stats.showStats ();
        return 0;
    }

    void
    XemRunProcedure (Store& store, const char* filePath)
    {
        NTime beginProcessing = getntime();

        XProcessor xproc(store);
        xproc.loadLibrary("xemprocessor");

        try
        {
            xproc.runProcedure(filePath);
        }
        catch (Exception *e)
        {
            detailException(e, "While running procedure '%s'\n", filePath);
            throw(e);
        }

        NTime endProcessing = getntime();
        ShowTiming("Time spent processing : ", beginProcessing, endProcessing);

        store.getServiceManager().waitTermination();
    }

    int
    XemRunDump (PersistentStore& store)
    {
        bool showElementId = true;
        BranchId branchId = store.getBranchManager().getBranchId(branchName);
        Document* document = store.getBranchManager().openDocument(branchId, 0, DocumentOpeningFlags_Read);
        document->incrementRefCount();
        if (!document)
        {
            Fatal("Could not open branch !\n");
        }
        {
            ElementRef root = document->getRootElement();
            root.toXML(stdout, showElementId ? ElementRef::Flag_ShowElementId : 0);
        }
        store.releaseDocument(document);
        return 0;
    }

    void
    postOpenInit (Store& store)
    {
        /*
         * Set here actions we shall perform after a successful initialization of the store
         */
    }

    int
    XemRunPersistenceAction (int argc, char** argv)
    {
        Info("Xemeiah version '%s'\n", __XEM_VERSION);

        bool isOpenned = false;
        int currentargc = 2;
        PersistentStore store;

        if (argc < 3)
        {
            fprintf(stderr, "Not enought parameters given : must provide at least the action to perform.\n");
            goto print_usage;
        }

#define __checkOpenned() \
        if ( ! isOpenned )  \
          { \
            if ( ! store.open ( storeFile ) ) \
            { Error ( "Could not open file '%s'\n", storeFile ); return 1; } \
            isOpenned = true; \
            postOpenInit ( store ); \
          }

        try
        {
            while (currentargc != argc)
            {
                char* arg = argv[currentargc];
                if (strncmp(arg, "--", 2) == 0)
                {
                    char* param = &(arg[2]);
                    if (strcmp(param, "store") == 0)
                    {
                        if (currentargc + 1 < argc)
                        {
                            currentargc++;
                            storeFile = argv[currentargc];
                        }
                        else
                        {
                            Fatal("No value provided for --store...\n");
                        }
                        // storeFile = &(param[6]);
                    }
                    else if (strcmp(param, "branch") == 0)
                    {
                        // branchName = &(param[7]);
                        if (currentargc + 1 < argc)
                        {
                            currentargc++;
                            branchName = argv[currentargc];
                        }
                        else
                        {
                            Fatal("No value provided for --branch...\n");
                        }

                        Info("Setting branch to '%s'\n", branchName);
                    }
                    else if (strcmp(param, "daemon") == 0)
                    {
                        setXemSysLog("xemeiah", LOG_NDELAY, LOG_DAEMON);
                        int res = daemon(1, 0);
                        if (res == -1)
                        {
                            Fatal("Could not daemonize : err=%d : %s\n", errno, strerror(errno));
                        }
                        Info("Start Xemeiah Daemon.\n");
                    }
                    else
                    {
                        fprintf(stderr, "Invalid parameter '%s'\n", param);
                        goto print_usage;
                    }
                    currentargc++;
                }
                else if (strcmp(arg, "format") == 0 || strcmp(arg, "format-default") == 0)
                {
                    store.format(storeFile);
                    isOpenned = true;
                    postOpenInit(store);
                    currentargc++;

                    if (strcmp(arg, "format-default") == 0)
                    {
                        XemRunProcedure(store, "format-default");
                    }
                }
                else if (strcmp(arg, "import") == 0)
                {
                    currentargc++;
                    if (!isOpenned)
                    {
                        store.open(storeFile);
                        isOpenned = true;
                        postOpenInit(store);
                    }
                    __checkOpenned ();
                    if (currentargc < argc)
                    {
                        const char* filePath = argv[currentargc];
                        currentargc++;

                        XemRunFileImport(store, filePath);
                    }
                    else
                    {
                        Fatal("No file provided for import !\n");
                    }

                }
                else if (strcmp(arg, "run") == 0 || strcmp(arg, "exec") == 0)
                {
                    currentargc++;
                    const char* filePath = argv[currentargc];
                    currentargc++;

                    __checkOpenned ();

                    XemRunProcedure(store, filePath);
                }
                else if (strcmp(arg, "check-all") == 0)
                {
                    currentargc++;
                    if (!isOpenned)
                    {
                        // store.setReadOnly ( true );
                    }
                    __checkOpenned ();
                    store.check(PersistentStore::Check_AllContents);
                }
                else if (strcmp(arg, "dump") == 0)
                {
                    currentargc++;
                    __checkOpenned ();
                    XemRunDump(store);
                }
                else if (strcmp(arg, "dump-branches") == 0)
                {
                    currentargc++;
                    __checkOpenned ();
                    XProcessor xproc(store);
                    Document* branchesDocument = store.getBranchManager().generateBranchesTree(xproc);
                    branchesDocument->getRootElement().toXML(
                            1, ElementRef::Flag_ChildrenOnly | ElementRef::Flag_XMLHeader);
                    store.releaseDocument(branchesDocument);
                }
                else if (strcmp(arg, "show-struct-sizes") == 0)
                {
                    XemShowStructSizes();
                    currentargc++;
                }
                else
                {
                    currentargc++;
                    __checkOpenned ();
                    XemRunProcedure(store, "webserver");
                }
            }
        }
        catch (Exception* e)
        {
            fprintf(stderr, "Catched Exception : %s\n", e->getMessage().c_str());
            delete (e);
        }

        if (isOpenned)
        {
            NTime beginStoreClose = getntime();
            store.close();
            NTime endStoreClose = getntime();

            ShowTiming("Store close : ", beginStoreClose, endStoreClose);
        }

        Info("Finished running persistence action.\n");
        return 0;
        Error("Unknown action : '%s'\n", argv[1]);
        print_usage: fprintf(
                stderr,
                "Usage for Persistence Actions : %s persistence [--store <store file>] [--branch <branch name>] [--daemon]\n"
                "\t[(format|check-all|dump|import|run|exec|show-struct-sizes|'procedure alias') parameters]+\n",
                argv[0]);
        return -1;

    }
}
;
