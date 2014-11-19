#include <Xemeiah/xsl/xslprocessor.h>
#include <Xemeiah/xsl/xsl-numbering.h>
#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/dom/childiterator.h>
#include <Xemeiah/xpath/xpath.h>
#include <Xemeiah/nodeflow/nodeflow-file.h>

#include <Xemeiah/auto-inline.hpp>

#include <math.h> // We need -INFINITY for imported templates fetching.
#include <time.h>
#include <sys/timeb.h>

#define Log_XSL Debug

#define __XSL_PROCESS_MESSAGES // Option : Process xsl:message instructions, write result to stderr (enabled by default)

namespace Xem
{
  void XSLProcessor::xslInstructionStylesheet ( __XProcHandlerArgs__ )
  {
    Log_XSL ( "At stylesheet id=%llx\n", item.getElementId() );

    installStylesheetExtensions ( item );
    
    try
      {
        setCurrentStylesheet ( item );

        item.serializeNamespaceAliases ( getNodeFlow() );

        ElementRef rootTemplate = chooseTemplate ( item );
        if ( rootTemplate )
          {
            Log_XSL ( "Processing root template arguments\n" );
            /*
             * We must set the variables here !
             */
            processTemplateArguments ( rootTemplate );
            getXProcessor().process ( rootTemplate );
          }
        else
          {
            defaultTemplate ( item );
          }
      }
    catch ( Exception* e )
      {
        detailException ( e, "At stylesheet : '%s'\n", item.generateVersatileXPath().c_str() );
        uninstallStylesheetExtensions ( item );
        throw ( e );
      }
    uninstallStylesheetExtensions ( item );
  }

  void XSLProcessor::xslInstructionTemplate ( __XProcHandlerArgs__ )
  {
    /*
     * Normally, all the Parameter assignation (xsl:with-param, xsl:param) has been done in the caller.
     */
   
    /*
     * Option : change the operating stylesheet.
     */  
    ElementRef stylesheet = item.getFather();
    if ( stylesheet.getKeyId() != xsl.stylesheet() )
      throwXSLException ( "Invalid father of template : '%s', at %s\n", 
          item.getFather().getKey().c_str(), item.generateVersatileXPath().c_str() );
    getXProcessor().setElement ( xslimpl.current_stylesheet(), stylesheet );

    getXProcessor().processChildren ( item );
  }

  void XSLProcessor::xslInstructionForEach ( __XProcHandlerArgs__ )
  {
    XPath xpath ( getXProcessor(), item, xsl.select() );
    NodeSet nodes;
    xpath.eval ( nodes );
    Log_XSL ( "For-each : processing %lu results from '%s', xpath='%s'.\n", 
        (unsigned long) nodes.size(), item.toElement().getKey().c_str(), xpath.getExpression() );

    processSortInstructions ( item, nodes );
    for ( NodeSet::iterator iter(nodes, getXProcessor()) ; iter ; iter ++ )
      {
        try
          {
            if ( iter->isNode() )
              {
                Log_XSL ( "ITER : (item=%s) current='%s'\n",
                    item.generateVersatileXPath().c_str(),
                    iter->toNode().generateVersatileXPath().c_str() );                
                getXProcessor().processChildren ( item );
              }
            else
              {
                throwException ( Exception, "NotImplemented : Item is not an element nor an attribute : itemType=%d, value='%s'.\n",
                  iter->getItemType(), iter->toString().c_str() );
              }
          }
        catch ( Exception * xpe )
          {
            String transmittable = iter->isNode() ? iter->toNode().generateVersatileXPath() : iter->toString();
            detailException(xpe, "At xsl:for-each item=%s\n", transmittable.c_str());
            throw ( xpe );
          }
      }
  }

  void XSLProcessor::xslInstructionApplyTemplates ( __XProcHandlerArgs__ )
  {
    Log_XSL ( "xsl:apply-templates item=%llx:%s\n",
      item.getElementId(), item.getKey().c_str() );

    NodeSet nodes; 
    if ( item.hasAttr ( xsl.select() ) )
      {
        XPath xpath( getXProcessor(), item, xsl.select() );
        Log_XSL ( "Evaluating xsl:apply-templates resulting nodeset, xpath='%s'\n", xpath.getExpression() );
        xpath.eval ( nodes );
      }
    else
      {
        /*
         * Default apply : fill the NodeSet with all the children of this currentNode !
         */
        if ( getCurrentNode().isElement() )
          {
            for ( ChildIterator child(getCurrentNode().toElement()) ; child ; child++ )
              {
                if ( child.mustSkipWhitespace ( getXProcessor() ) ) continue;
                nodes.pushBack ( child );
              }	  
          }
        else
          {
            NotImplemented ( "xsl:apply-templates on attribute with no select.\n" );
          }
      }

    Log_XSL ( "Apply-templates : processing %lu results.\n", (unsigned long) nodes.size() );

    processSortInstructions ( item, nodes );

    for ( NodeSet::iterator iter(nodes) ; iter ; iter ++ )
    {
        if ( ! iter->isElement() && ! iter->isAttribute() )
        {
            NotImplemented ( "Iter is neither element or attribute !\n" );
        }

        NodeRef& myNode = getCurrentNode ();

        /*
         * Note : we must push current Env to protect variable instanciation in xslProcessTemplateArguments().
         */
        getXProcessor().pushEnv ();
        
        try
        {

            getXProcessor().pushIterator ( iter );

            ElementRef templ = chooseTemplate ( item );
            if ( ! templ )
              {
                Log_XSL ( "Could not find any template for '%s' (id=%s)\n",
                    getCurrentNode().generateVersatileXPath().c_str(), getCurrentNode().generateId().c_str() );
                defaultTemplate ( item );
                getXProcessor().popEnv ();
                getXProcessor().popIterator ( iter );
                continue;
              }

            Log_XSL ( "Chosen template for '%s' (id=%s) => '%s'\n",
                getCurrentNode().generateVersatileXPath().c_str(),
                getCurrentNode().generateId().c_str(),
                templ.generateVersatileXPath().c_str() );

            getXProcessor().popIterator ( iter );

            std::map<KeyId,bool> providedArguments;

            try
              {
                processCallerTemplateArguments ( templ, item, providedArguments, false );
              }
            catch ( Exception* e )
              {
                getXProcessor().pushIterator ( iter );
                throw ( e );
              }

            getXProcessor().pushIterator ( iter );

            Log_XSL ( "Processing with template=%llx, match='%s', child=%llx:%s\n", 
                    templ.getElementId(), templ.getAttr(xsl.match()).c_str(),
                    iter->isElement() ? iter->toElement().getElementId() : 0, myNode.getKey().c_str() );
                    
            processRemainingTemplateArguments ( templ, providedArguments );
            
#if 0
            Log_XSL ( "----------- ENV -----------------\n" );
            getXProcessor().dumpEnv ();
            Log_XSL ( "----------- ENV -----------------\n" );
#endif
            getXProcessor().process ( templ );

            getXProcessor().popIterator ( iter );
        }
        catch ( Exception * xpe )
        {
            getXProcessor().popIterator ( iter );
            detailException ( xpe, "At xsl:apply-templates:%s\n",
                item.generateVersatileXPath().c_str() );
            detailException ( xpe, "\tcurrentNode=%s\n",     
                myNode.generateVersatileXPath().c_str() );
            getXProcessor().handleException ( xpe, item, false );
        }
      getXProcessor().popEnv ();
    }
  }

  void XSLProcessor::xslInstructionApplyImports ( __XProcHandlerArgs__ )
  {
    ElementRef currentStylesheet = getCurrentStylesheet ( );
    if ( 1 )
      {
        ElementRef root = currentStylesheet.getFather();
        if ( root.getFather() )
          {
            ElementRef impincl = root.getFather();
            Log_XSL ( "******************* Stylesheet grandfather is '%s'\n", impincl.getKey().c_str() );
            if ( impincl.getKeyId() == xsl.import() )
              {
                getXProcessor().setElement ( xslimpl.template_stylesheet(), currentStylesheet );
              }          
          }
      }   
  
    Number initialPriority = -INFINITY;
    
    ElementRef stylesheet = getTemplateStylesheet ( );
    
    ElementMultiMapRef importedModeTemplates = stylesheet.findAttr ( xslimpl.imported_match_templates(), AttributeType_SKMap );
    
    KeyId modeId = 0;
    
    for ( ElementRef ancestor = item.getFather() ; ancestor ; ancestor = ancestor.getFather() )
      {
        if ( ancestor.hasAttr ( xsl.mode() ) )
          {
            modeId = ancestor.getAttrAsKeyId ( getXProcessor(), xsl.mode() );
            AssertBug ( modeId, "Invalid null modeId !\n" );
            break;
          }
        
        if ( ancestor.getKeyId() == xsl.stylesheet() )
          break;
      }
    
    Log_XSL ( "*** Import : choosing template for '%s'\n",
        getCurrentNode().generateVersatileXPath().c_str() );
    
    ElementRef templ = chooseTemplate ( item, importedModeTemplates, modeId, initialPriority ); 
    
    if ( ! templ ) 
      {
        Log_XSL ( "apply-imports : no template found.\n" );
        defaultTemplate ( item );
        return;
      }

    stylesheet = templ.getFather();
    
    AssertBug ( stylesheet.getKeyId() == xsl.stylesheet(),
        "Invalid father of template : '%s'\n", item.getFather().getKey().c_str() );
    getXProcessor().setElement ( xslimpl.template_stylesheet(), stylesheet );

    std::map<KeyId,bool> providedArguments;
    processCallerTemplateArguments ( templ, item, providedArguments, true );

    processRemainingTemplateArguments ( templ, providedArguments );

    getXProcessor().process ( templ );
  }

  void XSLProcessor::xslInstructionCallTemplate ( __XProcHandlerArgs__ )
  {
    if ( ! item.hasAttr ( xsl.name() ) )
      {
        throwXSLException ( "Invalid xsl:call-template with no xsl:name provided !\n" );
      }
      
    KeyId nameId = item.getAttrAsKeyId ( getXProcessor(), xsl.name() );

    KeyId modeId = 0;
    if ( item.hasAttr ( xsl.mode() ) )
      {
        modeId = item.getAttrAsKeyId ( getXProcessor(), xsl.mode() );
        AssertBug ( modeId, "Invalid null modeId !\n" );
        /* 
         * Oasis considers that mode must be skipped for a named template
         */
        modeId = 0;
      }

    ElementRef stylesheet = getMainStylesheet ();
    ElementMapRef nameTemplates = stylesheet.findAttr ( xslimpl.named_templates(), AttributeType_SKMap );
    if ( ! nameTemplates )
      {
        throwXSLException ( "No xsl:template defined by name in this template or imported ones ! Could not call at %s\n",
            item.generateVersatileXPath().c_str() );
      }
    
    SKMapHash hash = getNamedTemplateHash ( modeId, nameId );
    ElementRef templ = nameTemplates.get ( hash );
    
    if ( ! templ )
      {
        throwXSLException ( "Invalid template name : '%s'\n", item.getAttr ( xsl.name() ).c_str() );
      }
    Log_XSL ( "Call-templates : selected template = 0x%llx %s : name=%s, select=%s, mode=%s\n",
        templ.getElementId(), templ.getKey().c_str(), 
        templ.hasAttr ( xsl.name() )  ? templ.getAttr ( xsl.name() ).c_str() : "(none)",
        templ.hasAttr ( xsl.select() ) ? templ.getAttr ( xsl.select() ).c_str() : "(none)",
        templ.hasAttr ( xsl.name() ) ? templ.getAttr ( xsl.name() ).c_str() : "(none)" );

    std::map<KeyId,bool> providedArguments;
    processCallerTemplateArguments ( templ, item, providedArguments, true );

    processRemainingTemplateArguments ( templ, providedArguments );

    getXProcessor().process ( templ );
  }

  void XSLProcessor::xslInstructionElement ( __XProcHandlerArgs__ )
  {
    String name = item.getEvaledAttr ( getXProcessor(), xsl.name() );
  
    LocalKeyId prefixId = 0, localKeyId = 0;
  
    try
      {
        getXProcessor().getKeyCache().parseKey ( name, prefixId, localKeyId );
      }
    catch ( Exception* e )
      {
        // throwException ( Exception, "Invalid key format '%s'\n", name.c_str() );
        Warn ( "Invalid xsl:element name attribute : '%s' at '%s', skipping element creation. Exception was '%s'\n", 
            name.c_str(), item.generateVersatileXPath().c_str(), e->getMessage().c_str() );
        delete ( e );
        getXProcessor().processChildren ( item );
        return;
      }
   
    Log_XSL ( "Parsed : name=%s, prefixId=%x, localKeyId=%x\n", name.c_str(), prefixId, localKeyId );

    AssertBug ( localKeyId, "Invalid zero localKeyId !\n" );  
#if 0
    KeyId keyId = 0;

    try 
      {
        keyId = item.getAttrAsKeyId ( getXProcessor(), xsl.name() );
      }
    catch ( Exception* e )
      {
        keyId = 0;
        Log_XSL ( "Caught exception while processing AVT name for xsl:element.\n" );
        delete ( e );
      }
    
    if ( ! keyId )
      {
        Warn ( "Empty / Invalid name ! Skipping element creation.\n" );
        getXProcessor().processChildren ( item );
        return;
      }
#endif
 
    NamespaceId nsId = 0;

    if ( item.hasAttr ( xsl.namespace_() ) )
      {
        String nsStr = item.getEvaledAttr ( getXProcessor(), xsl.namespace_() );
        nsId = getXProcessor().getKeyCache().getNamespaceId ( nsStr.c_str() );

        if ( prefixId )
          {
            Log_XSL ( "[AFFINITY] : nsId=%s (%x), prefixId=%s (%x)\n", 
                getXProcessor().getKeyCache().getNamespaceURL(nsId), nsId, 
                getXProcessor().getKeyCache().getLocalKey(prefixId), prefixId );
            getXProcessor().getNodeFlow().setNamespacePrefix ( getXProcessor().getKeyCache().buildNamespaceDeclaration ( prefixId ), nsId, true );
          }
        else if ( getXProcessor().getNodeFlow().getDefaultNamespaceId() != nsId )
          {
            getXProcessor().getNodeFlow().setNamespacePrefix ( __builtin.nons.xmlns(), nsId, true );
          }
      }
    else if ( prefixId )
      {
        nsId = item.getNamespaceIdFromPrefix ( prefixId, true );
                
        if ( ! nsId )
          {
            Warn ( "Could not resolve namespace prefix for name='%s' at '%s', skipping element creation.\n",
                name.c_str(), item.generateVersatileXPath().c_str() );
            getXProcessor().processChildren ( item );
            return;
          }
      }
    else
      {
        nsId = item.getDefaultNamespaceId ( true );
      }
   
    Log_XSL ( "[XSL:ELEMENT] local=%x, prefix=%x, ns=%x\n", localKeyId, prefixId, nsId );

    KeyId keyId = KeyCache::getKeyId ( nsId, localKeyId );

    getXProcessor().getNodeFlow().newElement ( keyId, false );

    item.ensureNamespaceDeclaration ( getXProcessor().getNodeFlow(), nsId, false, prefixId );

    item.serializeNamespaceAliases ( getXProcessor().getNodeFlow() );

    if ( item.hasAttr ( xsl.use_attribute_sets() ) )
      processAttributeSets ( item );

    try
      {
        getXProcessor().processChildren ( item );
      }
    catch ( Exception* e )
      {
        getXProcessor().getNodeFlow().elementEnd ( keyId );
        throw ( e );
      }
    getXProcessor().getNodeFlow().elementEnd ( keyId );
  }
  
  void XSLProcessor::xslInstructionAttribute ( __XProcHandlerArgs__ )
  {
    getXProcessor().evalAttributeContent ( item, xsl.name(), xsl.namespace_(), xslimpl.attribute_value() );
  }

  void XSLProcessor::xslInstructionAttributeSet ( __XProcHandlerArgs__ )
  {
    if ( item.hasAttr ( xsl.use_attribute_sets() ) )
      processAttributeSets ( item );
    getXProcessor().processChildren ( item );
  }

  void XSLProcessor::xslInstructionVariable  ( __XProcHandlerArgs__ )
  {
#if 1
    getXProcessor().processInstructionVariable ( item, xsl.name(),
        xsl.select(), item.getKeyId() == xsl.variable() );
#else
    bool setBehind = false;
    if ( item.getKeyId() == xsl.variable() )
      setBehind = true;
    else if ( item.getKeyId() == xsl.param() )
      setBehind = false;
    else if ( item.getKeyId() == xsl.with_param() )
      setBehind = false;
    else { Bug ( "Invalid key '%s'\n", item.getKey().c_str() ); }
    getXProcessor().processInstructionVariable ( item, xsl.name(), xsl.select(), setBehind );
#endif
  }

  void XSLProcessor::xslInstructionIf ( __XProcHandlerArgs__ )
  {
    XPath testXPath ( getXProcessor(), item, xsl.test() );
    NodeSet result; 
    testXPath.eval ( result );
    if ( result.toBool () )
      getXProcessor().processChildren ( item );
  }
  
  void XSLProcessor::xslInstructionChoose ( __XProcHandlerArgs__ )
  {
    for ( ChildIterator child(item) ; child ; child++ )
      {
        if ( child.getKeyId() == xsl.otherwise() )
          {
            getXProcessor().processChildren ( child );
            return;
          }

        if ( child.getKeyId() != xsl.when() )
          continue;
        XPath testXPath ( getXProcessor(), child, xsl.test() );
        NodeSet result; testXPath.eval ( result );
        if ( result.toBool() )
          {
            getXProcessor().processChildren ( child );
            return;
          }
      }
  }

  void XSLProcessor::xslInstructionValueOf ( __XProcHandlerArgs__ )
  {
#if PARANOID  
    if ( ! item.hasAttr ( xsl.select() ) )
      {
        throwXSLException ( "Instruction xsl:value-of has no attribute xsl:select\n" );      
      }
#endif
    Log_XSL ( "[XSLVALUEOF] At xsl:value-of : (item=%s) (node=%s)\n", 
      item.generateVersatileXPath().c_str(), getCurrentNode().generateVersatileXPath().c_str() );
    XPath textXPath ( getXProcessor(), item, xsl.select() );
    String result = textXPath.evalString ();
    Log_XSL ( "[XSLVALUEOF] text=%s\n", result.c_str() );

    bool disableOutputEscaping = false;
    if ( item.findAttr ( xsl.disable_output_escaping() ) )
      disableOutputEscaping = ( item.getAttr ( xsl.disable_output_escaping() ) == "yes");
    
    if ( result.size() )
      {
        getXProcessor().getNodeFlow().appendText ( result, disableOutputEscaping );
      }
  }

  void XSLProcessor::xslInstructionMessage ( __XProcHandlerArgs__ )
  {
#ifdef __XSL_PROCESS_MESSAGES
    if ( getXProcessor().hasVariable ( xslimpl.disable_messages() ) )
      return;
    String message = getXProcessor().evalChildrenAsString ( item );
#if 1
    Message ( " %s\n", message.c_str() );
#elif 0
    struct timeb tb; ftime (&tb);
    fprintf(stderr, "%4d:%3d|MESSAGE : %s\n", (int)(tb.time%10000), tb.millitm, message.c_str() );
#else
    fprintf ( stderr, "MESSAGE : %s\n", message.c_str() );
#endif
#endif // __XSL_PROCESS_MESSAGES
  }

  void XSLProcessor::xslInstructionComment ( __XProcHandlerArgs__ )
  {
    String comment = getXProcessor().evalChildrenAsString ( item );
    getXProcessor().getNodeFlow().newComment ( comment );
  }

  void XSLProcessor::xslInstructionResultDocument ( __XProcHandlerArgs__ )
  {
    if ( getXProcessor().hasVariable ( xslimpl.disable_result_document() ) )
      {
        Warn ( "xsl:result-document : writing to a document has been forbidden by xslimpl:disable-result-document variable (--nowrite argument).\n");
        return;
      }

    String filename = item.getEvaledAttrNS ( getXProcessor(), xsl.href() );
    Log_XSL ( "xsl:result-document : save to filename : '%s'\n", filename.c_str() );

    NodeFlowFile nodeFlow ( getXProcessor() );

    if ( getXProcessor().hasVariable ( xslimpl.disable_result_document_mkdir() ) )
      {
        nodeFlow.disableMkdir ();
      }

    nodeFlow.setFile ( filename );
    String encoding = "UTF-8";

    if ( item.hasAttrNS ( xsl.encoding() ) )
      encoding = item.getEvaledAttrNS ( getXProcessor(), xsl.encoding() );

    String method = "xml";
    if ( item.hasAttrNS ( xsl.method() ) )
      method = item.getEvaledAttrNS ( getXProcessor(), xsl.method() );

    bool indent = false;
    if ( item.hasAttrNS ( xsl.indent() ) )
      indent = item.getEvaledAttrNS ( getXProcessor(), xsl.indent() ) == "yes";

    bool standalone = false;
    if ( item.hasAttrNS ( xsl.standalone() ) )
      standalone = item.getEvaledAttrNS ( getXProcessor(), xsl.standalone() ) == "yes";

    bool omitXMLDeclaration = false;
    if ( item.hasAttrNS ( xsl.omit_xml_declaration() ) )
      omitXMLDeclaration = item.getEvaledAttrNS ( getXProcessor(), xsl.omit_xml_declaration() ) == "yes";
    
    nodeFlow.setOutputFormat ( method, encoding, indent, standalone, omitXMLDeclaration );
    getXProcessor().setNodeFlow ( nodeFlow );
    getXProcessor().processChildren ( item );
  }

  void XSLProcessor::xslInstructionSequence ( __XProcHandlerArgs__ )
  {
    if ( item.hasAttr ( xsl.select() ) )
      {
        XPath selectXPath ( getXProcessor(), item, xsl.select() );
        getNodeFlow().processSequence ( selectXPath );
      }
    else
      {
        NotImplemented ( "xsl:sequence without a xsl:select (guess we should not care..)!\n" );
      }  
  }
   
  void XSLProcessor::xslInstructionCopyOf ( __XProcHandlerArgs__ )
  {
    XPath copyXPath ( getXProcessor(), item, xsl.select() );
    NodeSet result;
    copyXPath.eval ( result );
    
    if ( result.isSingleton() )
      {
        getXProcessor().getNodeFlow().appendText ( result.toString(), false );
        return;
      }
    for ( NodeSet::iterator iter(result) ; iter ; iter++ )
      {
        if ( iter->isAttribute() )
          {
            AttributeRef attrRef = iter->toAttribute();
            if ( attrRef.getType() == AttributeType_NamespaceAlias )
              { 
                NotImplemented ( "At xsl:copy-of : copying a namespace attribute : %s",
                  attrRef.generateVersatileXPath().c_str() ); 
              }
            getXProcessor().getNodeFlow().newAttribute ( attrRef.getKeyId(), attrRef.toString().c_str() );
          }
        else
          {
            ElementRef eltRef = iter->toElement();
            if ( !eltRef.isText() && getXProcessor().getNodeFlow().isRestrictToText() )
              {
                Log_XSL ( "Current nodeFlow defines isRestrictToText() as true, skipping element : %s\n",
                  eltRef.generateVersatileXPath().c_str() );
                continue;
              }
            eltRef.serialize ( getXProcessor().getNodeFlow() );
          }
      }
  }

  void XSLProcessor::xslInstructionCopy ( __XProcHandlerArgs__ )
  {
    NodeRef& currentNode = getCurrentNode();
    if ( currentNode.isAttribute() )
      {
        AttributeRef& attr = currentNode.toAttribute();
        if ( attr.getType() == AttributeType_NamespaceAlias )
          {
            if ( attr.getNamespaceAliasId() == __builtin.xml.ns() )
              {
                /*
                 * We shall skip the http://www.w3.org/XML/1998/namespace
                 */
                return;
              }
            getXProcessor().getNodeFlow().setNamespacePrefix ( attr.getKeyId(), attr.getNamespaceAliasId(), false );
          }
        else
          getXProcessor().getNodeFlow().newAttribute ( attr.getKeyId(), attr.toString() );
        return;
      }

    ElementRef sourceRef = currentNode.toElement();
    if ( sourceRef.isText() )
      {
        bool disableOutputEscaping = false;
        if ( AttributeRef disableOutputEscapingAttr = item.findAttr ( xsl.disable_output_escaping() ) )
          disableOutputEscaping = (disableOutputEscapingAttr.toString() == "yes");
        getXProcessor().getNodeFlow().appendText ( sourceRef.getText(), disableOutputEscaping );
        return;
      }
    if ( sourceRef.isComment() )
      {
        getXProcessor().getNodeFlow().newComment ( sourceRef.getText() );
        return;
      }
    if ( sourceRef.isPI() )
      {
        getXProcessor().getNodeFlow().newPI ( sourceRef.getPIName(), sourceRef.getText() );
        return;
      }

    if ( sourceRef.getKeyId() == __builtin.xemint.root() )
      {
        processAttributeSets ( item );
        getXProcessor().processChildren ( item );
        return;
      }
    
    if ( sourceRef.getNamespaceId() )  
      sourceRef.ensureNamespaceDeclaration ( getXProcessor().getNodeFlow(), sourceRef.getNamespaceId(), true );

    sourceRef.serializeNamespaceAliases ( getXProcessor().getNodeFlow() );

    getXProcessor().getNodeFlow().newElement ( sourceRef.getKeyId(), false );

    processAttributeSets ( item );
      
    Exception *excep = NULL;
    if ( item.getChild() )
      {
        try
          {
            getXProcessor().processChildren ( item );
          }
        catch ( Exception * e )
          {
            excep = e;
          }      
      }
    getXProcessor().getNodeFlow().elementEnd ( sourceRef.getKeyId() );
    if ( excep ) throw ( excep );
  }

  void XSLProcessor::xslInstructionText ( __XProcHandlerArgs__ )
  {
    if ( ! item.getChild() ) return;
    if ( ! item.getChild().isText() || item.getChild().getYounger() )
      {
        throwXProcessorException ( "Invalid xsl:text contents !\n" );
      }
    String text = item.getChild().getText();

    bool disableOutputEscaping = false;
    if ( AttributeRef disableOutputEscapingAttr = item.findAttr ( xsl.disable_output_escaping() ) )
      disableOutputEscaping = (disableOutputEscapingAttr.toString() == "yes");
    
    Log_XSL ( "(item=%s) TEXT=%s\n", item.generateVersatileXPath().c_str(), text.c_str() );
    
    getXProcessor().getNodeFlow().appendText ( text, disableOutputEscaping );
  }

  void XSLProcessor::xslInstructionProcessingInstruction ( __XProcHandlerArgs__ )
  {
    String name = item.getEvaledAttr ( getXProcessor(), xsl.name() );
    String content = getXProcessor().evalChildrenAsString ( item );
    getXProcessor().getNodeFlow().newPI ( name, content );
  }
  
  void XSLProcessor::xslInstructionDefault ( __XProcHandlerArgs__ )
  {
    if ( item.isComment() ) return;

#if PARANOID
    if ( item.getNamespaceId() == xsl.ns() )
      {
        throwXSLException ( "Not implemented or wrong xsl name : '%s'\n", item.getKey().c_str() );
        return;
      }
#endif

#if PARANOID
#define __checkHasNodeFlow() \
    do { try { getXProcessor().getNodeFlow(); } \
     catch ( Exception * e ) \
      { \
        detailException ( e, "No NodeFlow defined, at item='%s' !\n", item.generateVersatileXPath().c_str() ); \
        throw ( e ); \
      } } while(0)
#else
#define __checkHasNodeFlow() do {} while(0)
#endif
    
    if ( item.isPI() )
      {
        throwXSLException ( "Unhandled PI in XSL : '%s' '%s'\n",
             item.getPIName().c_str(), item.getText().c_str() );      
      }
    if ( item.isText() )
      {
        throwXSLException ( "Unhandled text node in XSL : '%s' '%s'\n",
             item.generateVersatileXPath().c_str(), item.getText().c_str() );
#if 0
        if ( item.isWhitespace() )
          {
            if ( item.mustSkipWhitespace(getXProcessor()) )
              {
                Log_XSL ( "Skipping : '%s'\n", item.generateVersatileXPath().c_str() );
                return;              
              }
            if ( ! getXProcessor().hasNodeFlow() )
              {
                Log_XSL ( "Skipping space item, because we don't have any workflow set yet.\n" );
                return;
              }
            Log_XSL ( "Writing source whitespace !\n" );
          }
        Log_XSL ( "(item=%s) TEXT %s\n", item.generateVersatileXPath().c_str(), item.getText().c_str() );
        __checkHasNodeFlow();
        getXProcessor().getNodeFlow().appendText ( item.getText(), false );
        return;
#endif
      }

    __checkHasNodeFlow();
    Log_XSL ( "XSL Instruction Default on node 0x%llx:'%s'\n",
        item.getElementId(), getKeyCache().dumpKey ( item.getKeyId() ).c_str() );

    KeyId itemKeyId = item.getKeyId();

    /**
     * \todo : Re-implement cleanly the xsl namespace alias me
     */
    KeyId resultKeyId = processXSLNamespaceAlias ( getXProcessor(), itemKeyId );

    item.serializeNamespaceAliases ( getXProcessor().getNodeFlow() );

    getXProcessor().getNodeFlow().newElement ( resultKeyId );

    item.ensureNamespaceDeclaration ( getXProcessor().getNodeFlow(), KeyCache::getNamespaceId(resultKeyId), false );

    if ( item.hasAttr ( xsl.use_attribute_sets() ) )
      processAttributeSets ( item );

    for ( AttributeRef attr ( item ) ; attr ; attr = attr.getNext() )
      {
        if ( ! attr.isBaseType() ) continue;
        KeyId attrKeyId = attr.getKeyId();
        if ( attrKeyId == xsl.exclude_result_prefixes() )
          {
            // We may have thought to drop it before...
            continue;
          }
        if ( KeyCache::getNamespaceId(attrKeyId) == xsl.ns() )
          continue;
        /*
         * Section 7.1.1. We have to also exclude extension and excluded namespaces 
         */
        KeyId resultAttrKeyId = processXSLNamespaceAlias ( getXProcessor(), attrKeyId );

        if ( attr.getType() == AttributeType_String )
          {
            String result = item.getEvaledAttr ( getXProcessor(), attr.getKeyId() );
            item.ensureNamespaceDeclaration ( getXProcessor().getNodeFlow(), KeyCache::getNamespaceId(resultAttrKeyId), false );
            getXProcessor().getNodeFlow().newAttribute ( resultAttrKeyId, result );
          }
        else if ( attr.getType() == AttributeType_NamespaceAlias )
          {
#if 0
            NamespaceId nsId = attr.getNamespaceAliasId ();
            getXProcessor().setNamespacePrefix ( attr.getKeyId(), nsId, false );
#endif
          }
        else
          {
            Bug ( "Invalid attribute type %x\n", attr.getType() );
          }
      }
    
    try
      {    
        getXProcessor().processChildren ( item );
      }
    catch ( Exception* e )
      {
        getXProcessor().getNodeFlow().elementEnd ( resultKeyId );
        throw ( e );
      }
    getXProcessor().getNodeFlow().elementEnd ( resultKeyId );
  }

  void XSLProcessor::xslInstructionFallback ( __XProcHandlerArgs__ )
  {
    getXProcessor().processChildren ( item );
  }


  void XSLProcessor::xslInstructionNoAction ( __XProcHandlerArgs__ )
  {

  }

  void XSLProcessor::xslInstructionExtension ( __XProcHandlerArgs__ )
  {
    for ( ChildIterator child(item) ; child ; child++ )
      {
        if ( child.getKeyId() == xsl.fallback() )
          {
            getXProcessor().process ( child );
            return;
          }
      }
    Warn ( "Unsupported extension '%s', no xsl:fallback provided.\n", item.getKey().c_str() );  
  }

  void XSLProcessor::xslInstructionNotHandled ( __XProcHandlerArgs__ )
  {
    throwXSLException ( "XSL Markup Not Handled : '%s'\n", item.getKey().c_str() );
    Bug ( "." );
  }

};
