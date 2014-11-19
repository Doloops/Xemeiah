#include <Xemeiah/xprocessor/xprocessor.h>
#include <Xemeiah/xprocessor/xprocessormodule.h>
#include <Xemeiah/xprocessor/xprocessormoduleforge.h>
#include <Xemeiah/xprocessor/xprocessorlibs.h>
#include <Xemeiah/kern/volatiledocument.h>
#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/dom/childiterator.h>
#include <Xemeiah/dom/attributeref.h>
#include <Xemeiah/dom/nodeset.h>
#include <Xemeiah/xpath/xpath.h>
#include <Xemeiah/xpath/xpathdecimalformat.h>
#include <Xemeiah/nodeflow/nodeflow-dom.h>
#include <Xemeiah/nodeflow/nodeflow-sequence.h>
#include <Xemeiah/nodeflow/nodeflow-stream.h>
#include <Xemeiah/nodeflow/nodeflow-textcontents.h>
#include <Xemeiah/dom/documentmeta.h>

#include <Xemeiah/auto-inline.hpp>

#include <unistd.h>

#undef __builtin
#define __builtin getKeyCache().getBuiltinKeys()

#define Log_XProcessor Debug
#define Log_XProcedure Debug

namespace Xem
{
  XProcessor::XProcessor ( Store& _store ) : Env ( _store, settings )
  {
    maxLevel = 8192;
    currentNodeFlow = NULL;
    userId = 0;
  }

  XProcessor::~XProcessor ()
  {
    Log_XProcessor ( "Deleting XProcessor at %p\n", this );
    for ( ModuleMap::iterator iter = moduleMap.begin() ; iter != moduleMap.end() ; iter++ )
      {
        delete ( iter->second );
      }
  }

  void XProcessor::setMaxLevel ( EnvId maxEnvId )
  {
    maxLevel = maxEnvId;    
  }

  void XProcessor::getXProcessorModuleProperty ( NodeSet& nodeSet, KeyId ptyId )
  {
    if ( ! KeyCache::getNamespaceId(ptyId) )
      {
        throwXProcessorException("Invalid non-namespaced key '%s'\n", getKeyCache().dumpKey(ptyId).c_str());
      }
    XProcessorModule* module = getModule(KeyCache::getNamespaceId(ptyId));
    if ( ! module )
      {
        throwXProcessorException("No module installed for '%s'\n", getKeyCache().dumpKey(ptyId).c_str());
      }
    module->getXProcessorModuleProperty(nodeSet, ptyId);
  }

  void XProcessor::setNodeFlow ( NodeFlow& nodeFlow )
  {
    nodeFlow.setPreviousNodeFlow ( currentNodeFlow );
    currentNodeFlow = &nodeFlow;
  }

  void XProcessor::installModule ( NamespaceId moduleNSId, bool throwException )
  {
    if ( moduleMap.find(moduleNSId) != moduleMap.end() )
      {
        Warn ( "Module already instanciated : '%s' (%x)\n", getKeyCache().getNamespaceURL(moduleNSId), moduleNSId );
        return;
      }
    XProcessorModuleForge* moduleForge = store.getXProcessorLibs().getModuleForge ( moduleNSId );
    if ( ! moduleForge )
      {
        if ( ! throwException ) return;
        throwException ( Exception, "Could not install module '%s' (%x) : not configured in store.\n",
          store.getKeyCache().getNamespaceURL(moduleNSId), moduleNSId );
      }
    moduleForge->instanciateModule ( *this );
  }

  void XProcessor::installModule ( const char* moduleName )
  {
    NamespaceId moduleNSId = getKeyCache().getNamespaceId ( moduleName );
    installModule ( moduleNSId );  
  }
  
  void XProcessor::uninstallModule ( NamespaceId moduleNSId )
  {
    ModuleMap::iterator iter = moduleMap.find ( moduleNSId );

    if ( iter == moduleMap.end() )
      {
        throwException ( Exception, "No module installed for '%s' (%x)\n", getKeyCache().getNamespaceURL(moduleNSId), moduleNSId );
      }
    XProcessorModule* module = iter->second;
    moduleMap.erase(iter);
    Log_XProcessor ( "Unregister module %s (%x) (at %p)\n", getKeyCache().getNamespaceURL(moduleNSId), moduleNSId,
        module );
    delete ( module );
  }

  void XProcessor::installAllModules ()
  {
    store.getXProcessorLibs().installAllModules ( *this );
  }
  
  void XProcessor::loadLibrary ( const String& libName, bool installModules )
  {
    store.getXProcessorLibs().loadLibrary(libName, installModules ? this : NULL );
  }

  void XProcessor::registerModule ( NamespaceId moduleNSId, XProcessorModule* xprocessorModule )
  {
    xprocessorModule->install ();
    moduleMap [ moduleNSId ] = xprocessorModule;
    Log_XProcessor ( "Registered module : '%s' at %p\n", getStore().getKeyCache().getNamespaceURL ( moduleNSId ), xprocessorModule );
  }
  
  void XProcessor::registerModule ( XProcessorModule* xprocessorModule )
  {
    NamespaceId moduleNSId = xprocessorModule->getXProcessorModuleForge().getModuleNamespaceId();
    registerModule ( moduleNSId, xprocessorModule );
  }

  XProcessorModule* XProcessor::getModule ( const char* moduleName, bool mayAutoInstall )
  {
    NamespaceId moduleNSId = getKeyCache().getNamespaceId ( moduleName );
    return getModule ( moduleNSId, mayAutoInstall );
  }

  void XProcessor::registerEvents ( Document& doc )
  {
#if 0
    for ( ModuleMap::iterator iter = moduleMap.begin() ; iter != moduleMap.end() ; iter++ )
      {
        XProcessorModule* module = iter->second;
        if ( ! module )
          {
            Warn ( "NULL module for namespace=%s (%x) !\n", getKeyCache().getNamespaceURL(iter->first), iter->first );
            continue;
          }
        module->getXProcessorModuleForge().registerEvents(doc);
      }
#endif
    store.getXProcessorLibs().registerEvents(doc);
    doc.getDocumentMeta().getDomEvents().buildEventMap ();
  }

  String XProcessor::getBaseURI ()
  {
    if ( ! baseURIHook.module ) return "";
    return ( baseURIHook.module->*baseURIHook.hook ) ();
  }

  ElementRef XProcessor::getDefaultDocument ()
  {
    if ( ! getDefaultDocumentHook.module ) throwException ( Exception, "No getDefaultDocument() hook registered !\n" );
    return ( getDefaultDocumentHook.module->*getDefaultDocumentHook.hook ) ();
  }

  XPathDecimalFormat XProcessor::getXPathDecimalFormat ( KeyId decimalFormatId )
  {
    if ( ! getXPathDecimalFormatHook ) throwException ( Exception, "No getXPathDecimalFormat() hook registered !\n" );
    return ( getXPathDecimalFormatHook.module->*getXPathDecimalFormatHook.hook ) ( decimalFormatId );
  }

  bool XProcessor::mustSkipWhitespace ( KeyId keyId )
  {
    if ( ! mustSkipWhitespaceHook.module ) return false;
    return ( mustSkipWhitespaceHook.module->*mustSkipWhitespaceHook.hook ) ( keyId );
  }

  void XProcessor::getXPathKeyExpressions ( KeyId keyId, XPath& matchXPath, XPath& useXPath )
  {
    if ( ! getXPathKeyExpressionsHook ) throwException ( Exception, "No getXPathKeyExpressions() hook registered !\n" );
    ( getXPathKeyExpressionsHook.module->*getXPathKeyExpressionsHook.hook ) ( keyId, matchXPath, useXPath );
  }
    
  void XProcessor::triggerEvent ( KeyId eventId, KeyIdList& attributes )
  {
    const XProcessorLibs::EventTriggerHandlerModules& moduleList = store.getXProcessorLibs().getEventTriggerHandlerModules();
    for ( XProcessorLibs::EventTriggerHandlerModules::const_iterator iter = moduleList.begin() ;
        iter != moduleList.end() ; iter++ )
      {
        XProcessorModule* module = getModule(*iter, true);
        module->triggerEvent(eventId, attributes);
      }
  }


  void XProcessor::handleException ( Exception* xpe, ElementRef& actionItem, bool processOrProcessChildren )
  {
    String currentNodeKey;
    const char* currentNodeType;
    NodeRef& currentNode = getCurrentNode();

    if ( currentNode.isElement() )
      {
        currentNodeKey = currentNode.toElement().getKey();
        currentNodeType = "Element";
      }
    else
      {
        currentNodeKey = currentNode.toAttribute().getKey();
        currentNodeType = "Attribute";
      }

    if ( processOrProcessChildren )
    {
        detailException ( xpe, 
            "While processing %s : current %s : %s\n",
            actionItem.getKey().c_str(),
            currentNodeType,
            currentNodeKey.c_str() );    
        detailException ( xpe, "\tProcessing : %s\n", actionItem.generateVersatileXPath().c_str() );
        detailException ( xpe, "\tCurrentNode : %s\n", currentNode.generateVersatileXPath().c_str() );
    }
    dumpEnv(xpe);
    popEnv ();
    throw ( xpe );
  }
  

  void XProcessor::processInstructionVariable  ( ElementRef& item, KeyId nameKeyId, KeyId selectKeyId, bool setBehind )
  {
    KeyId varKeyId = item.getAttrAsKeyId ( *this, nameKeyId );
    
    if ( item.hasAttr ( selectKeyId ) )
      {
        XPath varXPath ( *this, item, selectKeyId );
        if ( varXPath.isSingleVariableReference ( varKeyId ) )
          {
            Log_XProcessor ( "[XPROC_VAR] Single variable reference at %s\n",
               item.generateVersatileXPath().c_str() );
            return;
          }
        Log_XProcessor ( "[XPROC_VAR] varXPath : %s\n", varXPath.getExpression() );
        NodeSet* result = allocateVariable ( varKeyId, setBehind );
        varXPath.eval ( *result );
        AssertBug ( result, "NULL result !\n" );
        assignVariable ( varKeyId, result, setBehind );
        Log_XProcessor ( "[XPROC_VAR] size : %lx, expr=%s, currentNode='%s'\n",
            (unsigned long) result->size(), varXPath.getExpression(), getCurrentNode().generateVersatileXPath().c_str() );
      }
    else if ( ! item.getChild() )
      {
        NodeSet* result = setVariable ( varKeyId, setBehind );
        static const char* emptyStringStr = "";
        String emptyString = emptyStringStr;
        result->setSingleton ( emptyString );
      }
    else if ( item.getChild().isText() && ! item.getChild().getYounger() )
      {
        NodeSet* result = setVariable ( varKeyId, setBehind );
        result->setSingleton ( item.getChild().getText() );
      }
    else
      {
        ElementRef root = createVolatileDocument(setBehind);

#if 0 // Use NodeFlowSequence
        NodeSet* result = allocateVariable ( varKeyId, setBehind );

        NodeFlowSequence nodeFlowSequence ( *this, root, *result );
        setNodeFlow ( nodeFlowSequence );
        processChildren ( item );
#else // Fallback : NodeFlowDom
        NodeFlowDom nodeFlowDom(*this, root);
        setNodeFlow ( nodeFlowDom );
        processChildren ( item );
        
        NodeSet* result = allocateVariable ( varKeyId, setBehind );

        for ( ChildIterator child(root) ; child ; child++ )
          {
            result->pushBack ( child, false );
          }
#endif
        assignVariable ( varKeyId, result, setBehind );
      }
  }

  void XProcessor::processTextNode ( ElementRef& item )
  {
#if PARANOID
    AssertBug ( item.isText(), "Item is not a textual node !\n" );
#endif
    if ( item.isWhitespace() )
      {
        if ( item.mustSkipWhitespace(*this) )
          {
            Log_XProcessor ( "Skipping : '%s'\n", item.generateVersatileXPath().c_str() );
            return;
          }
        if ( ! hasNodeFlow() )
          {
            Log_XProcessor ( "Skipping space item, because we don't have any workflow set yet.\n" );
            return;
          }
        Log_XProcessor ( "Writing source whitespace !\n" );
      }
    Log_XProcessor ( "(item=%s) TEXT %s\n", item.generateVersatileXPath().c_str(), item.getText().c_str() );
    getNodeFlow().appendText ( item.getText(), false );
  }

  void XProcessor::evalAttributeContent ( ElementRef& item, KeyId attrNameKeyId, KeyId namespaceAttrKeyId, KeyId attributeValueKeyId )
  {
    String name = item.getEvaledAttr ( *this, attrNameKeyId );

    LocalKeyId prefixId = 0, localKeyId = 0;
    getKeyCache().parseKey ( name, prefixId, localKeyId );

    NamespaceId nsId = 0;

    if ( prefixId == __builtin.nons.xmlns() &&
        namespaceAttrKeyId && item.hasAttr (namespaceAttrKeyId) )
      {
        Warn ( "At attribute '%s' : skipping prefix '%s'\n",
            item.generateVersatileXPath().c_str(),
            getKeyCache().getLocalKey(prefixId) );
        prefixId = 0;
      }

    if ( namespaceAttrKeyId && item.hasAttr (namespaceAttrKeyId) ) // item.hasAttr ( __builtin.xsl.namespace_() ) )
      {
        nsId = getKeyCache().getNamespaceId ( item.getEvaledAttr ( *this, namespaceAttrKeyId ).c_str() );
        if ( prefixId )
          {
            Log_XProcessor  ( "ns=%x, prefix=%x\n", nsId, prefixId );
#if 1
            if ( ! getNodeFlow().isSafeAttributePrefix ( prefixId, nsId ) )
              {
                Warn ( "Prefix not safe : prefix='%s', already mapped to '%s' (whereas namespace shall be '%s')\n",
                    getKeyCache().getLocalKey(prefixId),
                    getKeyCache().getNamespaceURL(getNodeFlow().getNamespaceIdFromPrefix(prefixId)),
                    getKeyCache().getNamespaceURL(nsId) );
                prefixId = 0;
              }
#else
            if ( getNodeFlow().getNamespaceIdFromPrefix ( prefixId ) == 0 )
              {
                KeyId declarationId = getKeyCache().buildNamespaceDeclaration ( prefixId );
                getNodeFlow().setNamespacePrefix ( declarationId, nsId, false );
              }
            else if ( getNodeFlow().getNamespaceIdFromPrefix ( prefixId ) != nsId )
              {
                Warn ( "Prefix already set : prefix='%s', already mapped to '%s' (whereas namespace shall be '%s')\n",
                    getKeyCache().getLocalKey(prefixId),
                    getKeyCache().getNamespaceURL(getNodeFlow().getNamespaceIdFromPrefix(prefixId)),
                    getKeyCache().getNamespaceURL(nsId) );
                prefixId = 0;
              }
#endif
          }
        else if ( ( prefixId = getNodeFlow().getNamespacePrefix ( nsId ) ) != 0 && prefixId != __builtin.nons.xmlns() )
          {
            Log_XProcessor  ( "[xsl:attribute] prefixed=%x namespace %s (%x)\n", prefixId, getKeyCache().getNamespaceURL(nsId), nsId );
          }
        else
          {
            Log_XProcessor  ( "[xsl:attribute] non-prefixed namespace %s (%x)\n", getKeyCache().getNamespaceURL(nsId), nsId );
          }
      }
    else if ( prefixId )
      {
        nsId = item.getNamespaceIdFromPrefix ( prefixId, true );
                
        if ( ! nsId )
          {
            if ( prefixId == __builtin.nons.xmlns() )
              {
                nsId = __builtin.xmlns.ns();
              }
            else
              {
                throwException ( Exception, "No namespace defined for QName='%s' (prefixId=%x), at %s\n", 
                    name.c_str(), prefixId, item.generateVersatileXPath().c_str() );
              }
          }      
      }
    else
      {
        nsId = item.getDefaultNamespaceId ( true );
      }

    KeyId attrKeyId = KeyCache::getKeyId ( nsId, localKeyId );
          
    Log_XProcessor  ( "[xsl:attribute] nsId=%x, prefix=%x, local=%x\n", nsId, prefixId, localKeyId );

    item.serializeNamespaceAliases ( getNodeFlow(), false );

#if PARANOID
    AssertBug ( &(getNodeFlow()) != NULL, "Null NodeFlow !\n" );
#endif
    if ( nsId ) getNodeFlow().verifyNamespacePrefix ( nsId );
    
#if PARANOID
    if ( nsId && getNodeFlow().getDefaultNamespaceId() != nsId 
        && nsId != __builtin.xmlns.ns()
        && getNodeFlow().getNamespacePrefix(nsId) == 0 )
      {
        Warn ( "Shall have generated a namespace prefix here for nsId=%x, ns='%s'.\n",
            nsId, getKeyCache().getNamespaceURL(nsId) );
#if 1
        throwException ( Exception, "Shall have generated a namespace prefix here for nsId=%x, ns='%s'. Current NodeFlow Element is '%s'.\n",
            nsId, getKeyCache().getNamespaceURL(nsId),
            getNodeFlow().getCurrentElement().generateVersatileXPath().c_str() );
#else
        Bug ( "This is fatal !\n" );
#endif
      }
#endif

    String contents;
    if ( item.hasAttr(attributeValueKeyId) ) // __builtin.xslimpl.attribute_value() ) )
      {
        contents = item.getAttr(attributeValueKeyId); // __builtin.xslimpl.attribute_value());
      }
    else
      {
        contents = evalChildrenAsString ( item );
      }
    Log_XProcessor  ( "[XSLATTR] contents=%s\n", contents.c_str() );
    getNodeFlow().newAttribute ( attrKeyId, contents );
          
#if 0
    KeyId attrKeyId = item.getAttrAsKeyId ( xproc, currentNode, attrNameKeyId );
    if ( ! attrKeyId )
      {
        throwXProcessorException ( "xsl:attribute : invalid attribute name '%x' !\n", 
           attrKeyId );
      }
    if ( item.hasAttr ( __builtin.xsl.namespace_() ) )
      {
        if ( getKeyCache().getNamespaceId(attrKeyId) )
          {
            Log_XProcessor  ( "xsl:attribute has a xsl:namespace attribute, "
              "but attribute is already defined with a namespace : '%x' !\n", attrKeyId );
          }
        NamespaceId nsId = getKeyCache().getNamespaceId ( item.getAttr ( __builtin.xsl.namespace_() ).c_str() );

        if ( nsId )
          {

            getNodeFlow().verifyNamespacePrefix ( nsId );
          }
        attrKeyId = getKeyCache().getKeyId ( nsId, KeyCache::getLocalKeyId(attrKeyId) );
      }
#endif

  }

  void XProcessor::processElement ( ElementRef& processingItem, KeyId handlerId )
  {
    if ( ! processingItem )
      {
        throwXProcessorException("Provided an empty processingItem !\n");
      }
    if ( ! KeyCache::getNamespaceId(handlerId) )
      {
        throwXProcessorException("Could not get handler for '%s' (%x), processingItem=%s : QName has no namespace !\n",
            getKeyCache().dumpKey(handlerId).c_str(), handlerId,
            processingItem.generateVersatileXPath().c_str() );
      }
    XProcessorHandler handler = getXProcessorHandler ( handlerId );
    if ( !handler.module || !handler.hook )
      {
        handler = defaultHandler;
      }

    if ( ! handler.module || ! handler.hook )
      {
        throwXProcessorException("Could not get handler for '%s' (%x), processingItem=%s\n",
            getKeyCache().dumpKey(handlerId).c_str(), handlerId,
            processingItem.generateVersatileXPath().c_str() );
      }
    (handler.module->*handler.hook) ( processingItem );
  }

  /**
   * Eval all children of an item as a String (xsl:value-of, ...)
   */
  String XProcessor::evalChildrenAsString ( ElementRef& item )
  {
    if ( ! item.getChild() )
      return String();
    if ( item.getChild() && item.getChild().isText() && ! item.getChild().getYounger() )
      return item.getChild().getText();

    NodeFlowTextContents nodeFlow ( *this );

    setNodeFlow ( nodeFlow );
    processChildren ( item );
    const char* contents = nodeFlow.getContents();
    Log_XProcessor  ( "Contents = [%s]\n", contents );
    return stringFromAllocedStr ( strdup ( contents ) );
  }

  String XProcessor::getProcedureAlias ( const String& aliasFile, const String& procedureName )
  {
    if ( ::access(aliasFile.c_str(),R_OK) )
      {
        Log_XProcedure ( "aliasFile '%s' is not accessible !\n", aliasFile.c_str() );
        return "";
      }
    try
    {
      ElementRef aliasesRoot = getDocumentRoot(aliasFile);
      NamespaceId configNSId = getKeyCache().getNamespaceId("http://www.xemeiah.org/ns/xem/config");
      KeyId procedureAliasesId = getKeyCache().getKeyId(configNSId,"procedure-aliases",true);
      KeyId aliasId = getKeyCache().getKeyId(configNSId,"alias",true);
      KeyId nameId = getKeyCache().getKeyId(configNSId,"name",true);
      KeyId urlId = getKeyCache().getKeyId(configNSId,"url",true);

      ChildIterator aliases(aliasesRoot);
      for (  ; aliases ; aliases++ )
        {
          if ( aliases.getKeyId() == procedureAliasesId )
            break;
          if ( aliases.isRegularElement() )
            {
              Warn ( "Erroneous element : '%s'\n", aliases.generateVersatileXPath().c_str() );
            }
        }
      if ( ! aliases )
        {
          throwXProcessorException( "Wrong Element name at file '%s', expecting '%s'\n",
              aliasFile.c_str(), getKeyCache().dumpKey(procedureAliasesId).c_str() );
        }
      ElementRef aliasesElement(aliases);
      for ( ChildIterator alias(aliasesElement) ; alias ; alias++ )
        {
          if ( alias.getKeyId() != aliasId ) continue;
          Log_XProcedure ( "Alias : '%s'='%s' (searching '%s')\n",
              alias.getAttr(nameId).c_str(), alias.getAttr(urlId).c_str(), procedureName.c_str());
          if ( alias.getAttr(nameId) == procedureName )
            return alias.getAttr(urlId);
        }
    }
    catch ( Exception*e )
    {
      delete ( e );
    }
    return "";
  }

  void XProcessor::runProcedure ( const String& procedureName )
  {
    String filePath;
    /**
     * Static list of files to search for procedure-aliases.xml file
     * Array must be filled in precedence order
     */
    static const char* procedureAliasPaths[] =
        {
          ".", "~/.xemeiah", "/etc/xemeiah",
          NULL
        };
    for ( const char** procedureAliasPath = procedureAliasPaths ; *procedureAliasPath ; procedureAliasPath++ )
      {
        String aliasFile = String(*procedureAliasPath) + "/procedure-aliases.xml";
        Log_XProcedure ( "Search in '%s'\n", aliasFile.c_str() );
        filePath = getProcedureAlias(aliasFile, procedureName );
        if ( filePath.size() ) break;
      }
    if ( !filePath.size() )
      filePath = procedureName;
    Log_XProcedure ( "Using '%s'\n", filePath.c_str() );
    ElementRef rootElement = getDocumentRoot(filePath);

    KeyId baseURIId = getKeyCache().getKeyId(0, "xem-procedure-base-uri", true);
    setString(baseURIId,
        rootElement.getDocument().getDocumentBaseURI().c_str());

    Info ( "Successfully openned and parsed file '%s', now executing :\n",
        filePath.c_str() );

    rootElement.getDocument().setRole("procedure");

    NodeSet initialNodeSet;
    initialNodeSet.pushBack(rootElement);
    NodeSet::iterator initialNodeSetIterator(initialNodeSet, *this);

    processChildren(rootElement);
  }

  void XProcessor::dumpExtensions ()
  {
    getStore().getXProcessorLibs().dumpExtensions ();
  }
};
