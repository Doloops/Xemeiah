#include <Xemeiah/xsl/xslprocessor.h>
#include <Xemeiah/xsl/xslfallbackmodule.h>
#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/dom/childiterator.h>
#include <Xemeiah/dom/elementmapref.h>
#include <Xemeiah/dom/integermapref.h>
#include <Xemeiah/xpath/xpath.h>
#include <Xemeiah/xpath/xpathdecimalformat.h>
#include <Xemeiah/version.h>

#include <string.h>
#include <math.h>

#include <map>
#include <list>

#include <Xemeiah/auto-inline.hpp>

#define Log_XSL Debug

#define __XEM_XSL_CHECK_CALLEE_ARGUMENTS
#define __XEM_XSL_CHECK_CALLER_ARGUMENTS_DUPLICATES

namespace Xem
{
  XSLProcessor::XSLProcessor ( XProcessor& xprocessor, XSLModuleForge& moduleForge )
  : XProcessorModule ( xprocessor, moduleForge ), xsl(moduleForge.xsl), xslimpl(moduleForge.xslimpl)
  {
  }

  XSLProcessor::~XSLProcessor ()
  {
  }

  XSLProcessor& XSLProcessor::getMe ( XProcessor& xproc )
  {
    const char* xslns = "http://www.w3.org/1999/XSL/Transform";
    // XProcessorModule* module = xproc.getModule(xproc.getKeyCache().getBuiltinKeys().xsl.ns(),true);
    XProcessorModule* module = xproc.getModule(xslns,true);
    AssertBug ( module, "XSLProcessor not instanciated !\n" );
    return dynamic_cast<XSLProcessor&> ( *module );
  }

  ElementRef XSLProcessor::getMainStylesheet ( )
  {
    if ( ! getXProcessor().hasVariable ( xslimpl.main_stylesheet() ) )
      return ElementRef ( (*(Document*)NULL) );
    NodeSet* stylesheet = getXProcessor().getVariable ( xslimpl.main_stylesheet() );
    if ( ! stylesheet )
      {
        throwXSLException ( "No root stylesheet defined !\n" );
      }
    return stylesheet->toElement ();
  }

  ElementRef XSLProcessor::getCurrentStylesheet ( )
  {
    NodeSet* stylesheet = getXProcessor().getVariable ( xslimpl.current_stylesheet() );
    if ( ! stylesheet )
      {
        throwXSLException ( "No root stylesheet defined !\n" );
      }
    return stylesheet->toElement ();
  }

  bool XSLProcessor::hasCurrentStylesheet ( )
  {
    return getXProcessor().hasVariable ( xslimpl.current_stylesheet() );
  }

  ElementRef XSLProcessor::getTemplateStylesheet ( )
  {
    NodeSet* stylesheet = getXProcessor().getVariable ( xslimpl.template_stylesheet() );
    if ( ! stylesheet )
      {
        throwXSLException ( "No root stylesheet defined !\n" );
      }
    return stylesheet->toElement ();
  }

  String XSLProcessor::getBaseURI ( )
  {
    String baseURI;
    if ( ! hasCurrentStylesheet() ) return baseURI;
    
    ElementRef stylesheet = getCurrentStylesheet ( );
    if ( ! stylesheet.getFather().isRootElement() )
      {
        Log_XSL ( "fetching the stylesheet baseURI when not at root.\n" );
        baseURI = stylesheet.getFather().getAttr ( __builtin.xemint.document_base_uri() );
      }
    else
      {
        baseURI = stylesheet.getDocument().getDocumentBaseURI();
      }
    return baseURI;
  }
  
  ElementRef XSLProcessor::getDefaultDocument ()
  {
    ElementRef rootStylesheet = getCurrentStylesheet ();

    if ( rootStylesheet.getFather() )
      rootStylesheet = rootStylesheet.getFather();
  
    return rootStylesheet;
  }

  bool XSLProcessor::mustSkipWhitespace ( KeyId keyId )
  {
    ElementRef stylesheet = getMainStylesheet ();
    if ( !stylesheet ) return false;
    
    IntegerMapRef stripMap = stylesheet.findAttr ( xslimpl.strip_spaces_map(), AttributeType_SKMap );
    if ( ! stripMap ) 
      {
        Log_XSL ( "No strip map !\n" );
        return false;
      }     
    
    /*
     * First, check if we have the perfect match
     */
    Integer result = stripMap.get ( keyId );
    if ( result ) 
      {
        Log_XSL ( "Found perfect match for %s (%x), result=%llx\n", getKeyCache().dumpKey(keyId).c_str(), keyId, result );
        return (result == 2);
      }
    
    /*
     * Then, try to find if we have a Namespace assigned
     */
    result = stripMap.get ( KeyCache::getKeyId(KeyCache::getNamespaceId(keyId),0) );
    if ( result ) 
      {
        Log_XSL ( "Found NS match for %s (%x), result=%llx\n", getKeyCache().dumpKey(keyId).c_str(), keyId, result );
        return (result == 2);
      }
    
    /*
     * Finally, find if there is a general '*' xemint.element match
     */
    result = stripMap.get ( __builtin.xemint.element() );
    Log_XSL ( "Final '*' match for %s (%x) : result=%llx\n", getKeyCache().dumpKey(keyId).c_str(), keyId, result );
    return (result == 2);
  }

  void XSLProcessor::getXPathKeyExpressions ( KeyId keyId, XPath& matchXPath, XPath& useXPath )
  {
    ElementRef instructKey = getKeyInstructionElement ( keyId );
    if ( ! instructKey )
      {
        throwXSLException ( "Could not get key mapping for key '%s' (%x)\n",
            getKeyCache().dumpKey(keyId).c_str(),
            keyId );
      }
    matchXPath.init(instructKey,xsl.match());
    useXPath.init(instructKey,xsl.use());
  }

  void XSLProcessor::setXPathDecimalFormatFrom ( XPathDecimalFormat& format, ElementRef& xslFormat )
  {
    if ( xslFormat.hasAttr(xsl.name())) format.setName(xslFormat.getAttr(xsl.name()));
    if ( xslFormat.hasAttr(xsl.digit())) format.setDigit(xslFormat.getAttr(xsl.digit()));
    if ( xslFormat.hasAttr(xsl.pattern_separator())) format.setPatternSeparator(xslFormat.getAttr(xsl.pattern_separator()));
    if ( xslFormat.hasAttr(xsl.minus_sign())) format.setMinusSign(xslFormat.getAttr(xsl.minus_sign()));
    if ( xslFormat.hasAttr(xsl.infinity())) format.setInfinity(xslFormat.getAttr(xsl.infinity()));
    if ( xslFormat.hasAttr(xsl.NaN())) format.setNoNumber(xslFormat.getAttr(xsl.NaN()));
    if ( xslFormat.hasAttr(xsl.decimal_separator())) format.setDecimalSeparator(xslFormat.getAttr(xsl.decimal_separator()));
    if ( xslFormat.hasAttr(xsl.grouping_separator())) format.setGroupingSeparator(xslFormat.getAttr(xsl.grouping_separator()));
    if ( xslFormat.hasAttr(xsl.percent())) format.setPercent(xslFormat.getAttr(xsl.percent()));
    if ( xslFormat.hasAttr(xsl.per_mille())) format.setPermille(xslFormat.getAttr(xsl.per_mille()));
    if ( xslFormat.hasAttr(xsl.zero_digit())) format.setZeroDigit(xslFormat.getAttr(xsl.zero_digit()));

  }

  XPathDecimalFormat XSLProcessor::getXPathDecimalFormat ( KeyId decimalFormatId )
  {
    ElementRef mainStylesheet = getMainStylesheet ();
    if ( mainStylesheet )
      {
        ElementMapRef decimalFormatsMap = mainStylesheet.findAttr ( xslimpl.decimal_formats(), AttributeType_SKMap);

        if ( decimalFormatsMap )
          {
            ElementRef decimalFormat = decimalFormatsMap.get ( decimalFormatId );
            if ( decimalFormat )
              {
                // return decimalFormat;
                XPathDecimalFormat xpathDecimalFormat;
                setXPathDecimalFormatFrom(xpathDecimalFormat, decimalFormat);
                return xpathDecimalFormat;
              }
          }
      }
    // Document& doc = getCurrentNode().getDocument();
    XPathDecimalFormat defaultDecimalFormat; // = ElementRef(doc); // ElementRef(mainStylesheet.getDocument() );
    return defaultDecimalFormat;
  }
      
  void XSLProcessor::setCurrentStylesheet ( __XProcHandlerArgs__ )
  {
    AssertBug ( item.getKeyId() == xsl.stylesheet(), "Provided element is not a stylesheet : '%s'\n", item.getKey().c_str() );

    Log_XSL ( "Setting stylesheet = %llu\n", item.getElementId() );

    /*
     * Set the stylesheet in the xem-xsl:main-stylesheet and xem-xsl:template-stylesheet variables.
     */    
    getXProcessor().setElement ( xslimpl.main_stylesheet(), item );
    getXProcessor().setElement ( xslimpl.template_stylesheet(), item );


    if ( ! item.hasAttr ( xslimpl.match_templates(), AttributeType_SKMap )
        && ! item.hasAttr ( xslimpl.imported_match_templates(), AttributeType_SKMap ) )
      {
        Warn ( "Preparing stylesheet !\n" );
        /*
         * TODO : check better that we did not already prepare this stylesheet !
         */
        prepareStylesheet ( item );
      }
    else
      {
        /*
         * Stylesheet has already been prepared, so the maps for templates, namespace-aliases, top-level parameters are here.
         */
        Log_XSL ( "XSL processing without prepare : we must build up the xsl:output list !\n" );
      }

#if 0 // Extensions are already installed in xslInstructionStylesheet
    /*
     * Process extension prefixes (if any)
     */
    AttributeRef extensionPrefixes = item.findAttr ( xsl.extension_element_prefixes() );
    if ( extensionPrefixes )
      {
        processStylesheetExtensionPrefixes ( item, extensionPrefixes );
      }
#endif
    
    /*
     * Process top-level parameters
     */
    TopLevelParameterMap topLevelParameters = item.findAttr ( xslimpl.toplevel_params(), AttributeType_SKMap );
    if ( topLevelParameters )
      {
        processTopLevelParameters ( item, topLevelParameters );
      }
    
    /*
     * Set output format
     */
    setOutputFormat ( item );

    /**
     * Process xsl:attribute-set values
     */
    processAttributeSetValues ( item );

    /*
     * Set the stylesheet in the xem-xsl:current-stylesheet variable.
     */    
    getXProcessor().setElement ( xslimpl.current_stylesheet(), item );
  }

  void XSLProcessor::parseExtensionNamespaces ( AttributeRef& prefixes, std::list<NamespaceId>& namespaceList )
  {
    String extensionTokens = prefixes.toString();
    std::list<String> extensions;
    extensionTokens.tokenize ( extensions );
    while ( extensions.size() )
      {
        String prefixStr = extensions.front ();
        extensions.pop_front ();

        String decl = prefixStr;

        LocalKeyId prefixId = getXProcessor().getKeyCache().getKeyId ( 0, decl.c_str(), true );
        KeyId keyId = getXProcessor().getKeyCache().buildNamespaceDeclaration ( prefixId );

        Log_XSL ( "New extension prefix : %s -> %x\n", decl.c_str(), keyId );            
        NamespaceId nsId = prefixes.getElement().getNamespaceAlias ( keyId, true );
        namespaceList.push_back ( nsId );
        if ( ! nsId )
          {
            throwXSLException ( "Unable to get namespaceId from prefix : '%s'\n", decl.c_str() );
          }
      }  
  }

  void XSLProcessor::installStylesheetExtensions ( ElementRef& stylesheet )
  {
    AttributeRef prefixes = stylesheet.findAttr ( xsl.extension_element_prefixes() );
    if ( ! prefixes ) return;    
    std::list<NamespaceId> namespaceList;
    parseExtensionNamespaces ( prefixes, namespaceList );
    for (  std::list<NamespaceId>::iterator iter = namespaceList.begin() ; iter !=  namespaceList.end() ; iter++ )
      {    
        NamespaceId nsId = *iter;
        if ( getXProcessor().getModule ( nsId ) )
          {
            Warn ( "Module '%s' already instanciated !\n", getKeyCache().getNamespaceURL(nsId) );
            continue;
            throwException ( Exception, "Module '%s' already instanciated !\n", getKeyCache().getNamespaceURL(nsId) );                      
          }
        
        try
          {
            getXProcessor().installModule ( nsId );
          }
        catch ( Exception* e )
          {
            delete ( e );
            Warn ( "Could not register module '%s'\n", getKeyCache().getNamespaceURL(nsId) );                      
            XProcessorModule* module = new XSLFallbackModule ( getXProcessor(), *this );
            getXProcessor().registerModule ( nsId, module );
            Warn ( "Registered XSLFallbackModule at %p for '%s' (from ModuleForge ns=%s)\n", module, getKeyCache().getNamespaceURL(nsId),
                getKeyCache().getNamespaceURL(getXProcessorModuleForge().getModuleNamespaceId()));
          }        
      }
  }

  void XSLProcessor::uninstallStylesheetExtensions ( ElementRef& stylesheet )
  {
    AttributeRef prefixes = stylesheet.findAttr ( xsl.extension_element_prefixes() );
    if ( ! prefixes ) return;    
    std::list<NamespaceId> namespaceList;
    parseExtensionNamespaces ( prefixes, namespaceList );
    for (  std::list<NamespaceId>::iterator iter = namespaceList.begin() ; iter !=  namespaceList.end() ; iter++ )
      {    
        NamespaceId nsId = *iter;
        getXProcessor().uninstallModule ( nsId );
      }
  }

  void XSLProcessor::processAttributeSetValues ( ElementRef& stylesheet )
  {
    ElementMultiMapRef topLevelElements = stylesheet.findAttr ( xslimpl.toplevel_elements(), AttributeType_SKMap );
    if ( topLevelElements )
      {
        Log_XSL ( "Output : topLevelElements = '%s'\n", topLevelElements.generateVersatileXPath().c_str() );
        for ( ElementMultiMapRef::multi_iterator iter ( topLevelElements, xsl.attribute_set() ) ; iter ; iter++ )
          {
            ElementRef attributeSet = topLevelElements.get ( iter );
            for ( ChildIterator attr(attributeSet) ; attr ; attr++ )
              {
                if ( attr.getKeyId() == xsl.attribute() )
                  {
                    if ( attr.hasAttr(xslimpl.attribute_value()) )
                      continue;
                    String value = getXProcessor().evalChildrenAsString(attr);
                    Log_XSL ( "[XSL] xsl:attribute-set : '%s' => '%s'\n",
                        attr.generateVersatileXPath().c_str(), value.c_str() );
                    attr.addAttr(xslimpl.attribute_value(), value);
                  }
              }
          }
      }
  }
  
  void XSLProcessor::setOutputFormat ( ElementRef& stylesheet )
  {
    bool isOutputFormatExplicit = false;
    bool indent = false;
    bool standalone = false;
    bool omitXMLDeclaration = false;
    String encoding = "UTF-8";
    String method = "xml";
    String docTypePublic, docTypeSystem;

    Log_XSL ( "At setOutputFormat(), stylesheet='%s'\n", stylesheet.generateVersatileXPath().c_str() );

    ElementMultiMapRef topLevelElements = stylesheet.findAttr ( xslimpl.toplevel_elements(), AttributeType_SKMap );
    if ( topLevelElements )
      {
        Log_XSL ( "Output : topLevelElements = '%s'\n", topLevelElements.generateVersatileXPath().c_str() );
        for ( ElementMultiMapRef::multi_iterator iter ( topLevelElements, xsl.output() ) ; iter ; iter++ )
          {
            ElementRef output = topLevelElements.get ( iter );
            Log_XSL ( "At output '%s'\n", output.generateVersatileXPath().c_str() );
            isOutputFormatExplicit = true;
            if ( output.hasAttr ( xsl.method() ) )
              {
                method = output.getAttr ( xsl.method() );
                if ( method == "html" )
                  indent = true;
              }
            if ( output.hasAttr ( xsl.indent() ) )
              indent = output.getAttr ( xsl.indent() ) == "yes";
            if ( output.hasAttr ( xsl.standalone() ) )
              standalone = output.getAttr ( xsl.standalone() ) == "yes";
            if ( output.hasAttr ( xsl.encoding() ) )
              encoding = output.getAttr ( xsl.encoding() );
            if ( output.hasAttr ( xsl.omit_xml_declaration() ) )
              omitXMLDeclaration = output.getAttr ( xsl.omit_xml_declaration() ) == "yes";
            if ( output.hasAttr ( xsl.doctype_public() ) )
              docTypePublic = output.getAttr ( xsl.doctype_public() );
            if ( output.hasAttr ( xsl.doctype_system() ) )
              docTypeSystem = output.getAttr ( xsl.doctype_system() );
              
            if ( output.hasAttr ( xsl.cdata_section_elements() ) )
              {
#if 0
                String elementsStr = output.getAttr ( xsl.cdata_section_elements() );
                std::list<String> tokens;
                elementsStr.tokenize ( tokens );
                for ( std::list<String>::iterator iter = tokens.begin() ; iter != tokens.end() ; iter++ )
                  {
                    String cdataQName = *iter;
                    KeyId keyId = getKeyCache().getKeyIdWithElement ( output, cdataQName );
                    getXProcessor().getNodeFlow().setCDataSectionElement ( keyId );
                    Log_XSL ( "CData : '%s' -> '%x'\n", cdataQName.c_str(), keyId );
                  }
#endif
                KeyIdList qnames = output.getAttrAsKeyIdList(xsl.cdata_section_elements());
                for ( KeyIdList::iterator iter = qnames.begin() ; iter != qnames.end() ; iter++ )
                  getXProcessor().getNodeFlow().setCDataSectionElement ( *iter );
              }
          }
      }
    if ( isOutputFormatExplicit )
      getXProcessor().getNodeFlow().setOutputFormat ( method, encoding, indent, standalone, omitXMLDeclaration );
    
    if ( docTypePublic.size() || docTypeSystem.size() )
      {
        Log_XSL ( "DOCTYPE : public='%s', system='%s'\n", docTypePublic.c_str(), docTypeSystem.c_str() );
        getXProcessor().getNodeFlow().setDocType ( docTypePublic, docTypeSystem );
      }
  }


  void XSLProcessor::xslInstructionOutput ( __XProcHandlerArgs__ )
  {
    /**
     * This handler is never called directly
     */
    bool indent = false;
    bool standalone = false;
    bool omitXMLDeclaration = false;
    String encoding = "UTF-8";
    String method = "xml";

    ElementRef output = item;
    
    if ( output.hasAttr ( xsl.method() ) )
      method = output.getEvaledAttr ( getXProcessor(), xsl.method() );
    if ( output.hasAttr ( xsl.indent() ) )
      indent = output.getEvaledAttr ( getXProcessor(), xsl.indent() ) == "yes";
    if ( output.hasAttr ( xsl.standalone() ) )
      standalone = output.getEvaledAttr ( getXProcessor(), xsl.standalone() ) == "yes";
    if ( output.hasAttr ( xsl.encoding() ) )
      encoding = output.getEvaledAttr ( getXProcessor(), xsl.encoding() );
    if ( output.hasAttr ( xsl.omit_xml_declaration() ) )
      omitXMLDeclaration = output.getEvaledAttr ( getXProcessor(), xsl.omit_xml_declaration() ) == "yes";
  
    Log_XSL ( "setOutputFormat to : m=%s, e=%s, i=%s, s=%s, o=%s\n",
        method.c_str(), encoding.c_str(), indent ? "indent" : "no-indent",
        standalone ? "standalone" : "no-standalone",
        omitXMLDeclaration ? "omitXMLDeclaration" : "no-omitXMLDeclaration" );
    getNodeFlow().setOutputFormat ( method, encoding, indent, standalone, omitXMLDeclaration );
  }

  ElementRef XSLProcessor::getKeyInstructionElement ( KeyId keyNameId )
  {
    ElementRef stylesheet = getMainStylesheet ( );
    if ( ! stylesheet ) 
      {
        return stylesheet;
      }
    ElementMapRef keyMap = stylesheet.findAttr ( xslimpl.key_definitions(), AttributeType_SKMap);
    if ( ! keyMap )
      {
        return ElementRef ( stylesheet.getDocument() );
      }
    return keyMap.get ( keyNameId ); 
  }

  void XSLProcessor::processAttributeSets ( ElementRef& item )
  {
    if ( ! item.hasAttr ( xsl.use_attribute_sets() ) )
      return;
    KeyIdList qnames = item.getAttrAsKeyIdList(xsl.use_attribute_sets(),false);
    for ( KeyIdList::iterator iter = qnames.begin() ; iter != qnames.end() ; iter++ )
      processAttributeSet(*iter);
  }
  
  void XSLProcessor::processAttributeSet ( KeyId attributeSetId )
  {
    Log_XSL ( "Processing attribute-set %s (%x)\n", getKeyCache().dumpKey(attributeSetId).c_str(), attributeSetId );
    ElementRef stylesheet = getMainStylesheet ();
    ElementMultiMapRef attributeSetMap = stylesheet.findAttr ( xslimpl.attribute_set_map(), AttributeType_SKMap );
    if ( ! attributeSetMap )
      {
        throwXSLException ( "Could not process attribute-set %s (%x) : no attribute-set map !\n", 
            getKeyCache().dumpKey(attributeSetId).c_str(), attributeSetId );
      }
    bool processed = false;
    for ( ElementMultiMapRef::multi_iterator iter(attributeSetMap, attributeSetId) ; iter ; iter++ )
      {
        ElementRef attributeSet = attributeSetMap.get(iter);
        getXProcessor().process ( attributeSet );
        processed = true;
      }
    if ( ! processed )
      {
        throwXSLException ( "Could not process attribute-set %s (%x) : not declared !\n",
            getKeyCache().dumpKey(attributeSetId).c_str(), attributeSetId );
      }
  }

  void XSLProcessor::processTopLevelParameters ( __XProcHandlerArgs__, TopLevelParameterMap& topLevelParameters )
  {
    Log_XSL ( "Processing topLevelParameters\n" );
    std::map<KeyId,bool> processedVariables;

    Log_XSL ( "Iterating to resolve toLevelParameters !\n" );
#if 0
    Log ( "Processing TopLevel : \n" );
    for ( TopLevelParameterMap::iterator iter(topLevelParameters) ; iter ; iter ++ )
      {
        KeyId nameKeyId = (KeyId) iter.getHash ();
        Log ( "\tnameKeyId=%x -> %s\n", nameKeyId, getXProcessor().getKeyCache().dumpKey ( nameKeyId ).c_str() );
        ElementRef elt = topLevelParameters.get(nameKeyId);
        Log ( "\tElt = %s\n", elt.generateVersatileXPath().c_str() );

      }
#endif
    __ui64 lastSkippedVariables = ~((__ui64)0);
    while ( true )
      {
        __ui64 skippedVariables = 0;
        Log_XSL ( "Iterating to resolve toLevelParameters !\n" );
        for ( TopLevelParameterMap::iterator iter(topLevelParameters) ; iter ; iter ++ )
          {
            KeyId nameKeyId = (KeyId) iter.getHash ();
            if ( processedVariables[nameKeyId] == true )
              continue;
            Log_XSL ( "\tnameKeyId=%x -> %s\n", nameKeyId, getXProcessor().getKeyCache().dumpKey ( nameKeyId ).c_str() );
            ElementRef child = topLevelParameters.get ( iter );
            Log_XSL ( "\telt=0x%llx '%s'\n", child.getElementId(), child.getKey().c_str() );
            
            if ( child.getKeyId() == xsl.param() && getXProcessor().hasVariable ( nameKeyId ) )
              {
                Info ( "Skipping xsl:param '%s', because already in the Env.\n", 
                    getXProcessor().getKeyCache().getLocalKey ( KeyCache::getLocalKeyId ( nameKeyId ) ) );
                continue;
              }
            
            ElementRef stylesheet = child.getFather();
            AssertBug ( stylesheet.getKeyId() == xsl.stylesheet(),
                "Top-level param father is not stylesheet : '%s'\n", stylesheet.getKey().c_str() );
            getXProcessor().setElement ( xslimpl.current_stylesheet(), stylesheet );
#if 1
            if ( child.hasAttr ( xsl.select() ) )
              {
                XPath selectXPath ( getXProcessor(), child, xsl.select() );
                std::list<KeyId> unresolvedVariableReferences;
                selectXPath.getUnresolvedVariableReferences ( unresolvedVariableReferences );
                if ( unresolvedVariableReferences.size() )
                  {
                    for ( std::list<KeyId>::iterator refIterator = unresolvedVariableReferences.begin () ;
                      refIterator != unresolvedVariableReferences.end() ; refIterator++ )
                      {
                        KeyId refKeyId = *refIterator;
                        if ( ! topLevelParameters.get ( refKeyId ) )
                          {
                            throwXProcessorException ( "At TopLevel variable/param definition of '%s' (expression %s): "
                              "variable '%s' is never defined\n", getXProcessor().getKeyCache().dumpKey(nameKeyId).c_str(),
                              selectXPath.getExpression(), getXProcessor().getKeyCache().dumpKey(refKeyId).c_str() );
                          }
                      }
                    Log_XSL ( "==> Setting skippedVariables !\n" );
                    skippedVariables ++;
                    continue;
                  }
              }
#endif
            try 
              {
                Log_XSL ( "[XSLINSTRVAR] %s\n", child.getAttr(xsl.name()).c_str() );
                /*
                 * TODO : Set XPathUndefinedVariableException to silent some way or another !
                 */
                xslInstructionVariable ( child );
              }
            catch ( Exception* xpe )
              {
                XPathUndefinedVariableException* xuve = dynamic_cast<XPathUndefinedVariableException*> ( xpe );
                if ( xuve )
                  {
                    KeyId refKeyId = xuve->undefinedVariable;
                    if ( ! topLevelParameters.has ( refKeyId ) )
                      {
                        detailException ( xuve, "At TopLevel variable/param definition of '%s' : "
                          "variable '%s' is never defined\n", getXProcessor().getKeyCache().dumpKey(nameKeyId).c_str(),
                          getKeyCache().dumpKey(refKeyId).c_str() );
                        detailException ( xuve, "TopLevel param at : %s\n", child.generateVersatileXPath().c_str() );
                        throw ( xuve );
                      }
                    if ( refKeyId == nameKeyId )
                      {
                        detailException ( xuve, "At TopLevel variable/param definition of '%s' : "
                          "definition depends on itself !\n", getXProcessor().getKeyCache().dumpKey(nameKeyId).c_str() );
                        throw ( xuve );
                      }
                    Log_XSL ( "Indirect long dependancy definition from variable '%s' depending on top-level variable '%s'\n",
                        child.getAttr(xsl.name()).c_str(),
                        getKeyCache().dumpKey(refKeyId).c_str() );

                    delete ( xuve );
                    Log_XSL ( "==>Setting skippedVariables !\n" );
                    skippedVariables ++;
                    continue;
                  }
                detailException ( xpe, "While processing top-level variable '%s'\n", child.getAttr(xsl.name()).c_str() );
                throw ( xpe );
              }
            processedVariables[nameKeyId] = true;
          }
        if ( !skippedVariables )
          break;
        if ( skippedVariables == lastSkippedVariables )
          {
            throwXSLException ( "Cycle detected in top-level parameter definition !\n" );
          }
        lastSkippedVariables = skippedVariables;
      }
  }

  KeyId XSLProcessor::processXSLNamespaceAlias ( XProcessor& xproc, KeyId fromKeyId )
  {
    if ( ! hasCurrentStylesheet ( ) ) return fromKeyId;
    NamespaceId nsId = KeyCache::getNamespaceId ( fromKeyId );
    LocalKeyId localKeyId = KeyCache::getLocalKeyId ( fromKeyId );
    if ( ! nsId ) return fromKeyId;
    ElementRef mainStylesheet = getMainStylesheet ( );
    IntegerMapRef nsAliasMap = mainStylesheet.findAttr ( xslimpl.namespace_aliases(), AttributeType_SKMap);
    if ( ! nsAliasMap )
      return fromKeyId;
    
    NamespaceId targetNSId = nsAliasMap.get ( nsId );
    if ( ! targetNSId )
      targetNSId = nsId;
    
    KeyId toKeyId = KeyCache::getKeyId ( targetNSId, localKeyId );
    Log_XSL ( "[NSALIAS] : ns '%x' -> '%x', key '%x' -> '%x'\n",
      nsId, targetNSId, fromKeyId, toKeyId );
    return toKeyId;
  }

  void XSLProcessor::processSortInstructions ( ElementRef& item, NodeSet& nodes )
  {
    for ( ElementRef child = item.getLastChild() ; child ; child = child.getElder() )
      {
        if ( child.getKeyId() == xsl.sort() )
          {
            processSortInstruction ( child, nodes );
          }
      }
#if 0
    getXProcessor().disableIterator (); 
    try
      {
      }
    catch ( Exception* e )
      {
        getXProcessor().restoreIterator ();
        throw ( e );      
      }
    getXProcessor().restoreIterator ();
#endif
  }

  void XSLProcessor::processSortInstruction ( ElementRef& item, NodeSet& nodes )
  {
    bool dataTypeText = true; 
    bool orderAscending = true;
    bool caseOrderUpperFirst = true;
    String lang = "";
    if ( item.hasAttr ( xsl.lang() ) )
      {
        lang = item.getEvaledAttr ( getXProcessor(), xsl.lang() );
      }
    if ( item.hasAttr ( xsl.data_type() ) )
      {
        String dataType = item.getEvaledAttr ( getXProcessor(), xsl.data_type() );
        if ( dataType == "text" )
          dataTypeText = true;
        else if ( dataType == "number" )
          dataTypeText = false;
        else
          {
            throwXSLException ( "Invalid xsl:sort xsl:data-type value : '%s'\n", dataType.c_str() );
          }        
      }
    if ( item.hasAttr ( xsl.order() ) )
      {
        String order = item.getEvaledAttr ( getXProcessor(), xsl.order() );
        if ( order == "ascending" )
          orderAscending = true;
        else if ( order == "descending" )
          orderAscending = false;
        else
          {
            throwXSLException ( "Invalid xsl:sort xsl:order value : '%s'\n", order.c_str() );
          }        
      }
    if ( item.hasAttr ( xsl.case_order() ) )
      {
        String caseOrder = item.getEvaledAttr ( getXProcessor(), xsl.case_order() );
        if ( caseOrder == "upper-first" )
          caseOrderUpperFirst = true;
        else if ( caseOrder == "lower-first" )
          caseOrderUpperFirst = false;
        else
          {
            throwXSLException ( "Invalid xsl:sort xsl:case-order value : '%s'\n", caseOrder.c_str() );
          }        
      }
    else
      {
#if 1
        if ( lang.substr(0,2) == "en" )
          caseOrderUpperFirst = false;
#endif
      }

    getXProcessor().disableIterator (); 
    try
      {
        if ( item.hasAttr ( xsl.select() ) )
          {
            Log_XSL ( "Sorting : '%s' as XPath, dataType=%s, order=%s, upperFirst=%s\n", 
                item.getAttr(xsl.select()).c_str(),
                dataTypeText ? "text" : "number", orderAscending ? "ascending" : "descending", caseOrderUpperFirst ? "yes" : "no" );
            XPath sortXPath ( getXProcessor(), item, xsl.select() );
            nodes.sort ( sortXPath, dataTypeText, orderAscending, caseOrderUpperFirst );
          }
        else
          {
            XPath sortXPath (getXProcessor(), getXPathDefaultSort ());
            nodes.sort ( sortXPath, dataTypeText, orderAscending, caseOrderUpperFirst );
          }
      }
    catch ( Exception* e )
      {
        getXProcessor().restoreIterator ();
        throw ( e );      
      }
    getXProcessor().restoreIterator ();
#ifdef LOG
    nodes.log ();
#endif
  }

  void XSLProcessor::processTemplateArguments ( ElementRef& templ )
  {
    for ( ChildIterator param(templ) ; param ; param++ )
      {
        if ( param.getKeyId() != xsl.param() )
          continue;
        Log_XSL ( "Processing non-caller xsl:param : '%s'\n", param.getAttr(xsl.name()).c_str() );
        xslInstructionVariable ( param );
      }  
  }

  void XSLProcessor::processCallerTemplateArguments ( ElementRef& templ, ElementRef& caller,
      std::map<KeyId,bool>& processedArguments, bool skipUndefinedInCallee )
  {
#ifdef __XEM_XSL_CHECK_CALLEE_ARGUMENTS  
    std::map<KeyId,bool> calleeArguments;
    for ( ChildIterator param(templ) ; param ; param++ )
      {
        if ( param.getKeyId() != xsl.param() )
          continue;
        KeyId keyId = param.getAttrAsKeyId(getXProcessor(),  xsl.name());
        calleeArguments[keyId] = true;
      }
#endif // __XEM_XSL_CHECK_CALLEE_ARGUMENTS
      
    for ( ChildIterator param(caller) ; param ; param++ )
      {
        if ( param.getKeyId() != xsl.with_param() )
          {
            if ( param.getKeyId() == xsl.sort() )
              continue;
            if ( param.getNamespaceId() == xsl.ns() )
              break; 
            continue;
          }
        Log_XSL ( "Processing caller xsl:with-param : '%s'\n", param.getAttr(xsl.name()).c_str() );
        KeyId keyId = param.getAttrAsKeyId(getXProcessor(),  xsl.name());
#ifdef __XEM_XSL_CHECK_CALLEE_ARGUMENTS  
        if ( calleeArguments.find(keyId) == calleeArguments.end() )
          {
#if 0
            Warn ( "[XSL-PARAM] Caller has attribute '%s' which callee does not have, %s.\n",
                param.getAttr(xsl.name()).c_str(),
                skipUndefinedInCallee ? "skipping this attribute" : "setting it anyway" );
#endif                
            if ( skipUndefinedInCallee )
              continue;
          }
#endif // __XEM_XSL_CHECK_CALLEE_ARGUMENTS

#ifdef __XEM_XSL_CHECK_CALLER_ARGUMENTS_DUPLICATES
        if ( processedArguments.find(keyId) != processedArguments.end() )
          {
            throwXSLException ( "Duplicate param while calling template : %s\n",
              param.generateVersatileXPath().c_str() );
          }
#endif // __XEM_XSL_CHECK_CALLER_ARGUMENTS_DUPLICATES

        xslInstructionVariable ( param );
        processedArguments[keyId] = true;
      }
  }



  void XSLProcessor::processRemainingTemplateArguments ( ElementRef& templ, std::map<KeyId,bool>& argumentsToProcess )
  {
    for ( ChildIterator param(templ) ; param ; param++ )
      {
        if ( param.getKeyId() != xsl.param() )
          {
            if ( param.getNamespaceId() != xsl.ns() )
              break;
            continue;
          }
        KeyId keyId = param.getAttrAsKeyId ( getXProcessor(),  xsl.name() );
        if ( argumentsToProcess.find(keyId) != argumentsToProcess.end() )
          {
            Log_XSL ( "Callee xsl:param : '%s' already set, skipping.\n", param.getAttr(xsl.name()).c_str() );
            continue;
          }
        Log_XSL ( "Processing callee xsl:param : '%s'\n", param.getAttr(xsl.name()).c_str() );
        xslInstructionVariable ( param );
      }
  }


  void XSLProcessor::getXProcessorModuleProperty ( NodeSet& result, KeyId ptyId )
  {
    if ( ptyId == xsl.version() )
      result.setSingleton ( (Number) 1.0 );
    else if ( ptyId == xsl.vendor() )
      result.setSingleton ( String("Xemeiah " __XEM_VERSION) );
    else if ( ptyId == xsl.vendor_url() )
      result.setSingleton ( String("http://www.xemeiah.org" ) );
    else
      {
        throwXSLException ( "Invalid or unknown system-property : %s\n", getKeyCache().dumpKey(ptyId).c_str() );
      }

  }

  void XSLProcessor::install ( )
  {
    Log_XSL ( "Installing XSLProcessor !\n" );

    /**
     * Miscelaneous
     */

    defaultNSHandler = (XProcessorHandler) (&XSLProcessor::xslInstructionNotHandled);

    XProcessor::XProcessorHandler defaultHandler ( this, (XProcessorHandler) &XSLProcessor::xslInstructionDefault );
    getXProcessor().registerDefaultHandler ( defaultHandler );

    XProcessor::XProcessorHook<XProcessorModule::BaseURIHook> baseURIHook ( this, (XProcessorModule::BaseURIHook) &XSLProcessor::getBaseURI );
    getXProcessor().registerBaseURIHook ( baseURIHook );

    XProcessor::XProcessorHook<XProcessorModule::GetDefaultDocumentHook> getDefaultDocumentHook
      ( this, (XProcessorModule::GetDefaultDocumentHook) &XSLProcessor::getDefaultDocument );
    getXProcessor().registerGetDefaultDocumentHook ( getDefaultDocumentHook );

    XProcessor::XProcessorHook<XProcessorModule::GetXPathDecimalFormatHook> getXPathDecimalFormatHook
      ( this, (XProcessorModule::GetXPathDecimalFormatHook) &XSLProcessor::getXPathDecimalFormat );
    getXProcessor().registerGetXPathDecimalFormatHook ( getXPathDecimalFormatHook );

    XProcessor::XProcessorHook<XProcessorModule::MustSkipWhitespaceHook> mustSkipWhitespaceHook
      ( this, (XProcessorModule::MustSkipWhitespaceHook) &XSLProcessor::mustSkipWhitespace );
    getXProcessor().registerMustSkipWhitespaceHook ( mustSkipWhitespaceHook );

    XProcessor::XProcessorHook<XProcessorModule::GetXPathKeyExpressionsHook> getXPathKeyExpressionsHook
      ( this, (XProcessorModule::GetXPathKeyExpressionsHook) &XSLProcessor::getXPathKeyExpressions );
    getXProcessor().registerGetXPathKeyExpressionsHook ( getXPathKeyExpressionsHook );
  }
};


