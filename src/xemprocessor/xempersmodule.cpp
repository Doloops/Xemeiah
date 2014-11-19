#include <Xemeiah/xemprocessor/xempersmodule.h>
#include <Xemeiah/xemprocessor/xemprocessor.h>
#include <Xemeiah/xprocessor/xprocessorlib.h>
#include <Xemeiah/kern/branchmanager.h>
#include <Xemeiah/xpath/xpath.h>
#include <Xemeiah/dom/documentmeta.h>

#include <Xemeiah/trace.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_Pers Debug

namespace Xem
{
  __XProcessorLib_REGISTER_MODULE ( XemProcessor, PersistenceModuleForge );

#include <Xemeiah/kern/builtin_keys_prolog_inst.h>
#include <Xemeiah/xemprocessor/builtin-keys/xem_pers>
#include <Xemeiah/kern/builtin_keys_postlog.h>

  PersistenceModule::PersistenceModule(XProcessor& xproc, PersistenceModuleForge& moduleForge ) 
  : XProcessorModule ( xproc, moduleForge ), xem_pers(moduleForge.xem_pers)
  {
  
  }

  PersistenceModule::~PersistenceModule () {}

  void PersistenceModule::domEventDocumentTrigger ( DomEventType domEventType, ElementRef& domEventElement, NodeRef& nodeRef )
  {
    XemProcessor& xemProc = XemProcessor::getMe(getXProcessor());
    __BUILTIN_NAMESPACE_CLASS(xem) &xem = xemProc.xem;
    Document& document = nodeRef.getDocument();

    getXProcessor().pushEnv();
    Exception* exception = NULL;
    try
    {
      KeyIdList arguments;

      String eventName = DomEventMask(domEventType).toString();
      getXProcessor().setString ( xem.document_event(), eventName );
      arguments.push_back(xem.document_event());

      String targetBranchRevId; stringPrintf ( targetBranchRevId, "%llu:%llu", _brid(document.getBranchRevId()) );
      getXProcessor().setString ( xem.target_branchRevId(), targetBranchRevId );
      arguments.push_back(xem.target_branchRevId());

      String targetBranchName = document.getBranchRevId().branchId ? document.getStore().getBranchManager().getBranchName(document.getBranchRevId().branchId) : "no-branch";
      getXProcessor().setString ( xem.target_branchName(), targetBranchName );
      arguments.push_back(xem.target_branchName());

      String targetRole = document.getRole();
      getXProcessor().setString ( xem.target_role(), targetRole );
      arguments.push_back(xem.target_role());

      Log_Pers ( "Trigger event '%s' for targetBranchRevId %s\n", eventName.c_str(), targetBranchRevId.c_str() );

      getXProcessor().triggerEvent ( xem.document_event(), arguments );
    }
    catch ( Exception* e )
    {
      exception = e;
    }
    getXProcessor().popEnv();
    if ( exception )
      throw ( exception );
  }

  void PersistenceModule::instructionFork ( __XProcHandlerArgs__ )
  {
    XPath selectXPath ( getXProcessor(), item, xem_pers.select() );
    Document& doc = selectXPath.evalElement().getDocument();
    if ( doc.isWritable() )
      {
        throwException ( Exception, "Current Revision is writable, you should commit it before forking.\n" );
      }
    
    String branchName = item.getEvaledAttr ( getXProcessor(), xem_pers.name() );
    
    doc.fork ( getXProcessor(), branchName, BranchFlags_MayIndexNameIfDuplicate );
  }

  void PersistenceModule::instructionReopen ( __XProcHandlerArgs__ )
  {
    XPath selectXPath ( getXProcessor(), item, xem_pers.select() );
    Document& doc = selectXPath.evalElement().getDocument();
    if ( doc.isWritable() )
      {
        throwException ( Exception, "Current Revision is writable, you should commit it before reopen.\n" );
      }
    doc.reopen ( getXProcessor() );
  }

  void PersistenceModule::instructionCommit ( __XProcHandlerArgs__ )
  {
    XPath selectXPath ( getXProcessor(), item, xem_pers.select() );
    Document& doc = selectXPath.evalElement().getDocument();
    if ( !doc.isWritable() )
      {
        AssertBug ( !doc.isLockedWrite(), "Document not writable, but locked write ?\n" );
        Warn ( "Document '%s' (%llx:%llx) not writable !\n", doc.getRole().c_str(), _brid(doc.getBranchRevId()) );
        return;
        throwException ( Exception, "Document not writable !\n" );
      }
    if ( !doc.isLockedWrite() )
      {
        doc.lockWrite();
      }
    doc.commit ( getXProcessor() );
  }
  
  void PersistenceModule::instructionMerge ( __XProcHandlerArgs__ )
  {
    XPath selectXPath ( getXProcessor(), item, xem_pers.select() );
    Document& doc = selectXPath.evalElement().getDocument();
    bool keepBranch = false;

    try
      {
        doc.merge ( getXProcessor(), keepBranch );
      }
    catch ( Exception* e )
      {
        detailException ( e, "xem:merge : could not merge branch.\n" );
        throw ( e);
      }
    doc.scheduleBranchForRemoval ();
  }  

// #define __attr(__u) xproc.getKeyCache().getKeyId(item.getNamespaceId(),STRINGIFY(__u),true)

  void PersistenceModule::instructionDrop ( __XProcHandlerArgs__ )
  {
    XPath selectXPath ( getXProcessor(), item, xem_pers.select() );
    Document& doc = selectXPath.evalElement().getDocument();

    if ( item.hasAttr ( xem_pers.branchRevId() ) )
      {
        String branchRevStr = item.getEvaledAttr ( getXProcessor(), xem_pers.branchRevId() );
        BranchRevId branchRevId = BranchManager::parseBranchRevId ( branchRevStr.c_str() );
        
        if ( ! branchRevId.branchId || ! branchRevId.revisionId )
          {
            throwException ( Exception, "Invalid branch and revision id '%s'\n", branchRevStr.c_str() );
          }
          
        Log_Pers ( "[XEM:BRANCH] Dropping alter revision [%llu:%llu]\n", _brid(doc.getBranchRevId()) );

        NotImplemented ( "We shall schedule revision removal here !\n" );
#if 0        
        // store.dropRevisions ( branchRevId.branchId, branchRevId.revisionId, branchRevId.revisionId );
        persistentDocument = store.createPersistentDocument ( branchRevId.branchId, branchRevId.revisionId, Document_Read );
        persistentDocument->scheduleRevisionForRemoval ();      
        persistentDocument->release ();
#endif
        return;
      }

    Log_Pers ( "[XEM:BRANCH] Dropping current revision [%llu:%llu]\n", _brid(doc.getBranchRevId()) );
    doc.scheduleRevisionForRemoval ();
  }

  void PersistenceModule::instructionRenameBranch ( __XProcHandlerArgs__ )
  {
    XPath selectXPath ( getXProcessor(), item, xem_pers.select() );
    Document& doc = selectXPath.evalElement().getDocument();

    BranchId branchId = doc.getBranchRevId().branchId;
    
    String branchName = item.getEvaledAttr ( getXProcessor(), xem_pers.name() );
    
    Log_Pers ( "[XEM-BRANCH] Rename branch %llu to '%s'\n", branchId, branchName.c_str() );
    
    getXProcessor().getStore().getBranchManager().renameBranch ( branchId, branchName.c_str() );
  }

  void PersistenceModule::instructionCreateBranch ( __XProcHandlerArgs__ )
  {
    String branchName = item.getEvaledAttr ( getXProcessor(), xem_pers.name() );
    String branchFlags = item.getEvaledAttr ( getXProcessor(), xem_pers.branch_flags() );
    Log_Pers ( "[XEM-BRANCH] Creating branch '%s' with flags '%s'\n", branchName.c_str(), branchFlags.c_str() );
    getXProcessor().getStore().getBranchManager().createBranch ( branchName, branchFlags );
      
  }

  /*
   * ********************* FUNCTIONS ***********************
   */

  void PersistenceModule::functionHasBranch ( __XProcFunctionArgs__ )
  {
    if ( args.size() != 1 )
      {
        throwException ( Exception, "Invalid number of arguments for xem-pers:has-branch('branch name')");
      }
    String branchName = args[0]->toString();
    BranchId branchId = getXProcessor().getStore().getBranchManager().getBranchId(branchName);
    bool hasBranch = (branchId != 0);
    result.setSingleton(hasBranch);
  }

  void PersistenceModule::functionGetCurrentBranchName ( __XProcFunctionArgs__ )
  {
    Document& doc = node.getDocument();
    BranchRevId branchRevId = doc.getBranchRevId();
    String branchName = doc.getStore().getBranchManager().getBranchName ( branchRevId.branchId );
    result.setSingleton ( branchName );
  }

  void PersistenceModule::functionGetCurrentBranchId ( __XProcFunctionArgs__ )
  {
    BranchRevId branchRevId = node.getDocument().getBranchRevId();
    String branchId;
    stringPrintf ( branchId, "%llu", branchRevId.branchId );
    result.setSingleton ( branchId );
  }

  void PersistenceModule::functionGetCurrentRevisionId ( __XProcFunctionArgs__ )
  {
    BranchRevId branchRevId = node.getDocument().getBranchRevId();
    String revisionId;
    stringPrintf ( revisionId, "%llu", branchRevId.revisionId );
    result.setSingleton ( revisionId );
  }

  void PersistenceModule::functionGetCurrentBranchRevisionId ( __XProcFunctionArgs__ )
  {
    BranchRevId branchRevId = { 0, 0 };
    if ( args.size() )
      branchRevId = args[0]->front().toNode().getDocument().getBranchRevId();
    else
      branchRevId = node.getDocument().getBranchRevId();
    String brId;
    stringPrintf ( brId, "%llu:%llu", branchRevId.branchId, branchRevId.revisionId );
    result.setSingleton ( brId );
  }

  void PersistenceModule::functionGetCurrentForkedFrom ( __XProcFunctionArgs__ )
  {
    BranchId branchId = node.getDocument().getBranchRevId().branchId;
    BranchRevId branchRevId = node.getDocument().getStore().getBranchManager().getForkedFrom ( branchId );
    if ( branchRevId.branchId )
      {
        String brId;
        stringPrintf ( brId, "%llu:%llu", branchRevId.branchId, branchRevId.revisionId );
        result.setSingleton ( brId );
      }    
  }

  void PersistenceModule::functionGetCurrentRevisionWritable ( __XProcFunctionArgs__ )
  {
    bool isWritable = node.getDocument().isWritable();
    result.setSingleton ( isWritable );
  }

  void PersistenceModule::functionGetBranchesTree ( __XProcFunctionArgs__ )
  {
    Document* branchesDocument = node.getDocument().getStore().getBranchManager().generateBranchesTree ( getXProcessor() );
    getXProcessor().bindDocument ( branchesDocument, true );
    
    ElementRef rootElement = branchesDocument->getRootElement();
    result.pushBack ( rootElement );
  }

  void PersistenceModule::functionLookupRevision ( __XProcFunctionArgs__ )
  {
    BranchId branchId = strtoull ( args[0]->toString().c_str(), NULL, 10 );
    if ( ! branchId )
      {
        throwException ( Exception, "Invalid branchId '%s'\n", args[0]->toString().c_str() );
      }
    RevisionId revisionId = node.getDocument().getStore().getBranchManager().lookupRevision ( branchId ); // , DocumentCreationFlags_Read );
    String res;
    stringPrintf ( res, "%llu", revisionId );
    result.setSingleton ( res );
  }

  void PersistenceModule::functionGetBranchName ( __XProcFunctionArgs__ )
  {
    BranchId branchId = strtoull ( args[0]->toString().c_str(), NULL, 10 );
    if ( ! branchId )
      {
        throwException ( Exception, "Invalid branchId '%s'\n", args[0]->toString().c_str() );
      }
    String branchName = node.getDocument().getStore().getBranchManager().getBranchName ( branchId );
    result.setSingleton ( branchName );
  }

  void PersistenceModule::functionGetBranchId ( __XProcFunctionArgs__ )
  {
    String branchName = args[0]->toString();
    BranchId branchId = node.getStore().getBranchManager().getBranchId ( branchName );
    if ( ! branchId )
      {
        throwException ( Exception, "Could not lookup branch name '%s'\n", branchName.c_str() );
      }
    String branchIdStr;
    stringPrintf ( branchIdStr, "%llu", branchId );    
    result.setSingleton ( branchIdStr );  
  }

  void PersistenceModule::functionGetBranchForkedFrom ( __XProcFunctionArgs__ )
  {
    BranchId branchId = strtoull ( args[0]->toString().c_str(), NULL, 10 );
    if ( ! branchId )
      {
        throwException ( Exception, "Invalid branchId '%s'\n", args[0]->toString().c_str() );
      }

    String forkedFromStr;
    BranchRevId forkedFrom = node.getStore().getBranchManager().getForkedFrom ( branchId );
    stringPrintf ( forkedFromStr, "%llu:%llu", _brid(forkedFrom) );
    result.setSingleton ( forkedFromStr );
  }

  void PersistenceModule::install ()
  {
  }

  void PersistenceModuleForge::install ()
  {
    registerHandler ( xem_pers.fork(), &PersistenceModule::instructionFork );
    registerHandler ( xem_pers.commit(), &PersistenceModule::instructionCommit );
    registerHandler ( xem_pers.merge(), &PersistenceModule::instructionMerge );
    registerHandler ( xem_pers.drop(), &PersistenceModule::instructionDrop );
    registerHandler ( xem_pers.reopen(), &PersistenceModule::instructionReopen );
    registerHandler ( xem_pers.rename_branch(), &PersistenceModule::instructionRenameBranch );
    registerHandler ( xem_pers.create_branch(), &PersistenceModule::instructionCreateBranch );

    registerFunction ( xem_pers.has_branch(), &PersistenceModule::functionHasBranch );

    registerFunction ( xem_pers.get_current_branch_name(), &PersistenceModule::functionGetCurrentBranchName );
    registerFunction ( xem_pers.get_current_branch_id(), &PersistenceModule::functionGetCurrentBranchId );
    registerFunction ( xem_pers.get_current_revision_id(), &PersistenceModule::functionGetCurrentRevisionId );
    registerFunction ( xem_pers.get_current_branch_revision_id(), &PersistenceModule::functionGetCurrentBranchRevisionId );
    registerFunction ( xem_pers.get_current_revision_writable(), &PersistenceModule::functionGetCurrentRevisionWritable );
    registerFunction ( xem_pers.get_current_forked_from(), &PersistenceModule::functionGetCurrentForkedFrom );
    registerFunction ( xem_pers.get_branch_name(), &PersistenceModule::functionGetBranchName );
    registerFunction ( xem_pers.get_branch_id(), &PersistenceModule::functionGetBranchId );
    registerFunction ( xem_pers.get_branch_forked_from(), &PersistenceModule::functionGetBranchForkedFrom );
    registerFunction ( xem_pers.get_branches_tree(), &PersistenceModule::functionGetBranchesTree );
    registerFunction ( xem_pers.lookup_revision(), &PersistenceModule::functionLookupRevision );

    registerDomEventHandler ( xem_pers.domevent_document_trigger(), &PersistenceModule::domEventDocumentTrigger );
  }

  void PersistenceModuleForge::registerEvents(Document& doc)
  {
    registerXPathAttribute(doc,xem_pers.select());

    KeyId rootKeyId = doc.getKeyCache().getBuiltinKeys().xemint.root();
    doc.getDocumentMeta().getDomEvents().registerEvent(DomEventMask_Document, rootKeyId, xem_pers.domevent_document_trigger() );
  }


};

