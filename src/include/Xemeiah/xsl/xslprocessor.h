#ifndef __XEMEIAH_XSL_H
#define __XEMEIAH_XSL_H

#include <Xemeiah/xprocessor/xprocessormodule.h>
#include <Xemeiah/xsl/xslmoduleforge.h>
#include <Xemeiah/dom/elementmapref.h>
#include <Xemeiah/xpath/xpath.h>

#include <list>

namespace Xem
{
  XemStdException ( XSLException );
  XemStdException ( XSLFormatNumberException );
#define throwXSLException(...) throwException ( XSLException, __VA_ARGS__ )
#define throwXSLFormatNumberException(...) throwException ( XSLFormatNumberException, __VA_ARGS__ )

  class ElementRef;
  class XPathParser;

  /**
   * XSLProcessor : regroups XSL-specific functions and xsl:* handlers
   */
  class XSLProcessor : public XProcessorModule
  {
    friend class XSLModuleForge;
  protected:
    /**
     * DOM Events
     */
    void xslDomEventStylesheet ( DomEventType domEventType, ElementRef& domEventElement, NodeRef& nodeRef );

    /**
     * Handlers
     */
    void xslInstructionStylesheet ( __XProcHandlerArgs__ );
    void xslInstructionTemplate ( __XProcHandlerArgs__ );
    void xslInstructionForEach ( __XProcHandlerArgs__ );
    void xslInstructionApplyTemplates ( __XProcHandlerArgs__ );
    void xslInstructionApplyImports ( __XProcHandlerArgs__ );
    void xslInstructionCallTemplate ( __XProcHandlerArgs__ );
    void xslInstructionElement ( __XProcHandlerArgs__ );
    void xslInstructionAttribute ( __XProcHandlerArgs__ );
    void xslInstructionVariable  ( __XProcHandlerArgs__ );
    void xslInstructionIf ( __XProcHandlerArgs__ );
    void xslInstructionChoose ( __XProcHandlerArgs__ );
    void xslInstructionValueOf ( __XProcHandlerArgs__ );
    void xslInstructionMessage ( __XProcHandlerArgs__ );
    void xslInstructionComment ( __XProcHandlerArgs__ );
    void xslInstructionCopyOf ( __XProcHandlerArgs__ );
    void xslInstructionCopy ( __XProcHandlerArgs__ );
    void xslInstructionNumber ( __XProcHandlerArgs__ );
    void xslInstructionText ( __XProcHandlerArgs__ );
    void xslInstructionProcessingInstruction ( __XProcHandlerArgs__ );
    void xslInstructionFallback ( __XProcHandlerArgs__ );
    void xslInstructionNoAction ( __XProcHandlerArgs__ );
    void xslInstructionExtension ( __XProcHandlerArgs__ );
    void xslInstructionNotHandled ( __XProcHandlerArgs__ );
    void xslInstructionOutput ( __XProcHandlerArgs__ );
    void xslInstructionAttributeSet ( __XProcHandlerArgs__ );

    /*
     * XSL 2.0 Instructions
     */
  public:
    void xslInstructionResultDocument ( __XProcHandlerArgs__ ); //< Provide the ResultDocument to EXSLT
  protected:
    void xslInstructionSequence ( __XProcHandlerArgs__ );

  public:
    void xslInstructionDefault ( __XProcHandlerArgs__ );  //< Some modules may wish to use that function
  protected:

    void xslFormatNumber ( ElementRef& item, std::list<Integer>& positions );
    void xslFormatNumber ( ElementRef& item, Integer value );

    ElementRef getDefaultDocument ();
    
    bool mustSkipWhitespace ( KeyId keyId );
    
    void getXPathKeyExpressions ( KeyId keyId, XPath& matchXPath, XPath& useXPath );

    void processAttributeSets ( ElementRef& item );
    void processAttributeSet ( KeyId attributeSetId );


    class TemplateInfo
    {
    public:
      ElementRef templ;
      Number priority;

      TemplateInfo ( const ElementRef& _templ, Number _priority )
      : templ(_templ), priority(_priority)
      {}
    };

    typedef std::list<TemplateInfo> TemplateInfoList;
    typedef std::map<KeyId, TemplateInfoList > TemplateInfoMap;

    void copyList ( TemplateInfoList& list2, TemplateInfoList& list1 );
    void insertMatchTemplate ( TemplateInfoList& list, TemplateInfo& info );
    
    void mergeTemplateInfoMap ( ElementMultiMapRef& optimizedModeTemplates,
      TemplateInfoMap& templateInfoMap, KeyId modeId );
    void mergeFinalSteps ( TemplateInfoMap& templateInfoMap,
        XPath::XPathFinalSteps& finalSteps, ElementRef& templ,
        bool hasTemplatePriority, Number templatePriority );
    void buildTemplateInfoMap ( TemplateInfoMap& templateInfoMap, 
      ElementMultiMapRef& modeTemplates, KeyId modeId );

    void prepareOptimizedMatchTemplates ( ElementMultiMapRef& modeTemplates, 
      ElementMultiMapRef& importedModeTemplates, ElementMultiMapRef& optimizedModeTemplates );

  public:
    /**
     * The toplevel parameters mapping type
     */
    typedef ElementMapRef TopLevelParameterMap;

    __BUILTIN_NAMESPACE_CLASS(xsl) &xsl;
    __BUILTIN_NAMESPACE_CLASS(xslimpl) &xslimpl;

    /**
     * Constructor and destructor
     */
    XSLProcessor ( XProcessor& xprocessor, XSLModuleForge& moduleForge );
    ~XSLProcessor();

    /**
     * getMe()
     */
    static XSLProcessor& getMe ( XProcessor& xproc );

    /**
     * Get the module forge casted to the XSLModuleForge
     */
    XSLModuleForge& getXSLModuleForge() const { return dynamic_cast<XSLModuleForge&> ( moduleForge ); }

    /**
     * Set the current processing stylesheet.
     */
    void setCurrentStylesheet ( __XProcHandlerArgs__ );

    /**
     * Get the main processing stylesheet.
     */
    ElementRef getMainStylesheet ( );

    /**
     * Get the current processing stylesheet.
     */
    ElementRef getCurrentStylesheet ( );

    /**
     * Defines wether there is a current processing stylesheet or not.
     */
    bool hasCurrentStylesheet ( );


    /**
     * Get the current processing stylesheet which holds the template list scope.
     */
    ElementRef getTemplateStylesheet ( );

    /**
     * Get the base URI
     */
    String getBaseURI ( );

    /**
     * Get the current DecimalFormat
     */
    XPathDecimalFormat getXPathDecimalFormat ( KeyId decimalFormatId );

    /**
     *
     */
    void setXPathDecimalFormatFrom ( XPathDecimalFormat& format, ElementRef& xslFormat );
    
    /**
     * Prepare stylesheet operations : parse templates, namespace aliases, top-level parameters, ...
     * Part of the Stylesheet preparation process :
     * - prepareStylesheet
     * - importStylesheet
     * - getNamedTemplateHash
     * - convertStripSpaceElementsToList
     */
    void prepareStylesheet ( ElementRef& stylesheet );
    
    /**
     * Prepare stylesheet for the main and imported stylesheets.
     */
    void prepareStylesheet ( ElementRef& currentStylesheet, ElementRef& mainStylesheet,
          NodeSet& stripSpacesDeclarations,
					bool isImported );

    /**
     * Build the list of top-level elements, filtering out import stuff and included stylesheets
     */
    void buildTopLevelElements ( ElementRef& stylesheet, ElementRef& mainStylesheet, 
        NodeSet& stripSpacesDeclarations, NodeSet& myXSLDeclarations );
   
    /**
     * Add a top-level element
     */
    void addTopLevelElement ( ElementRef& mainStylesheet, ElementRef& topLevelElement );
   
    /**
     * Resolve stylesheet relative URL
     * @param importItem the xsl:include or xsl:import instruction
     * @param mainStylesheet the importing stylesheet
     */
    String resolveStylesheetURI ( ElementRef& importItem, ElementRef& mainStylesheet );
   
    /**
     * Check if the xsl:import or xsl:include is not recursive
     */
    void checkStylesheetImportCycle ( ElementRef& importItem, const String& completeURI );
   
    /**
     * Import or include a stylesheet in an Element node.
     */
    ElementRef importStylesheet ( ElementRef& importItem, ElementRef& mainStylesheet );

    /**
     * Process xsl:strip-space and xsl:preserve-space top-level elements
     */
    void processStylesheetStripSpaces ( ElementRef& stylesheet, NodeSet& stripSpaces );

    /**
     * Parse the extensions attribute
     */
    void parseExtensionNamespaces ( AttributeRef& prefixes, std::list<NamespaceId>& namespaceList );

    /**
     * Process xsl:extension-element-prefixes attributes, and bind each namespace declared to the xslInstructionExtension().
     */
    void installStylesheetExtensions ( ElementRef& stylesheet );

    /**
     * Uninstall Stylesheet extensions
     */
    void uninstallStylesheetExtensions ( ElementRef& stylesheet );

    /**
     * Set output format
     */
    void setOutputFormat ( ElementRef& stylesheet );

    /**
     * Set output format
     */
    void processAttributeSetValues ( ElementRef& stylesheet );

    /**
     * Process top-level xsl:param and xsl:variable elements.
     */
    void processTopLevelParameters ( __XProcHandlerArgs__, TopLevelParameterMap& topLevelParameters );

    /**
     * Hashing function to store named templates in a Map.
     */
    SKMapHash getNamedTemplateHash ( KeyId modeId, KeyId nameId )
    { return ( (((SKMapHash)modeId) << 32 ) + ((SKMapHash)nameId )); }
 
    /**
     * Evaluate if an element is text() and is part of text elements we have to strip.
     * \deprecated isTextStripSpaced() in favor of ElementRef::mustStripWhitespace ();
     */
    bool isTextStripSpaced ( XProcessor& xproc, ElementRef& textElement );

    /**
     * Get the targetted namespace alias.
     * @param xproc the XProcessor
     * @param fromKeyId the final key before conversion
     * @return the KeyId being converted.
     */
    KeyId processXSLNamespaceAlias ( XProcessor& xproc, KeyId fromKeyId );

    /**
     * Get a xsl:key declaration element for a given KeyId, using the currently defined stylesheet
     */
    ElementRef getKeyInstructionElement ( KeyId keyNameId );

    /**
     * choose template according to currentNode properties.
     * @param applyImports set to true if to restrict to imported templates only.
     */
    ElementRef chooseTemplate ( ElementRef& item, ElementMultiMapRef& modeTemplates, 
        KeyId modeId, Number& initialPriority );

    /**
     * Choose an optimized template
     */
    ElementRef chooseTemplateOptimized ( ElementRef& item, ElementMultiMapRef& optimizedModeTemplates, 
        KeyId modeId, Number& initialPriority );


    ElementRef chooseTemplate ( ElementRef& item );

    /**
     * Process templates arguments with no explicit caller (called from : defaultTemplate and setCurrentStylesheet)
     * That is, we do not have any xsl:with-param provided.
     */
    void processTemplateArguments ( ElementRef& templ );

    /**
     * Process xsl:with-param, caller template arguments 
     *
     * @param skipUndefinedInCallee skip xsl:with-param argument if no xsl:param has been declared in callee.
     */
    void processCallerTemplateArguments ( ElementRef& templ, ElementRef& caller, 
          std::map<KeyId,bool>& processedArguments, bool skipUndefinedInCallee );

    /**
     * Process remaining xsl:param template arguments, when no xsl:with-param template argument is provided
     */
    void processRemainingTemplateArguments ( ElementRef& templ, std::map<KeyId,bool>& argumentsToProcess );

    /**
     * Process using default template.
     */
    void defaultTemplate ( __XProcHandlerArgs__ ); 

    /**
     * Process a <xsl:sort/> instruction.
     */
    void processSortInstruction ( ElementRef& item, NodeSet& nodeSet );

    /**
     * Process all sort instructions of a given element.
     */
    void processSortInstructions ( ElementRef& item, NodeSet& nodeSet );


    bool process ( ElementRef& xslStylesheet, ElementRef& xmlTree );

#if 0
    XPathParser& getXPathIdMatch() { return getXSLModuleForge().getXPathIdMatch(); }
    XPathParser& getXPathIdUse() { return getXSLModuleForge().getXPathIdUse(); }
#endif
    XPathParser& getXPathDefaultSort() { return getXSLModuleForge().getXPathDefaultSort(); }

    /**
     * Get XProcessorModule property
     */
    virtual void getXProcessorModuleProperty ( NodeSet& nodeSet, KeyId ptyId );

    /** 
     * Install Handlers for the xsl:* elements
     */
    virtual void install ( );

  };
};

#endif // __XEMEIAH_XSL_H
