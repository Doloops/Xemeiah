#ifndef __XEM_XPATH_H
#define __XEM_XPATH_H

#include <Xemeiah/trace.h>
#include <Xemeiah/kern/format/core_types.h>
#include <Xemeiah/kern/format/xpath.h>
#include <Xemeiah/kern/document.h>
#include <Xemeiah/kern/exception.h>
#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/dom/attributeref.h>
#include <Xemeiah/dom/nodeset.h>

#include <stdlib.h>
#include <string.h>
#include <list>
#include <vector>

#define __XEM_XPATH_RUNTIME_KEEP_NODEREF
#define __XEM_XPATH_RUNTIME_GET_EXPRESSION


namespace Xem
{
  class XPath;
  class XPathParser;
  class XProcessor;
  class ElementMultiMapRef;

  XemStdException ( XPathException );

  /**
   * When XPath triggers a reference to an undefined variable, it throws this exception which contains
   * the KeyId of the undefined reference. This may be used by caller to handle cases 
   * (for example, handling of top-level parameters in a XSL stylesheet).
   */
  class XPathUndefinedVariableException : public XPathException
  {
    public:
    XPathUndefinedVariableException() {}
    ~XPathUndefinedVariableException() {}
    
    KeyId undefinedVariable;
  };

#define throwXPathException(...)		\
  do { char _buff[4096]; sprintf ( _buff, __VA_ARGS__ );	\
    throwException ( XPathException, "XPath Expression '%s' : Exception '%s'\n", \
        getExpression(), _buff ); } while (0)

  /**
   * In-memory parsed representation of an XPath expression.
   * An XPath class can be created :
   * -# from a simple const char* or String textual expression, 
   *    in which case the XPath class temporarily allocates the parsed representation and frees it 
   *    at destructor time.
   * -# from an existing AttributeRef of a given ElementRef, in which case the parsed representation
   *    is stored in the same Document, as a special AttributeType_XPath attribute of the ElementRef.
   *    This allows to store and retrieve parsed XPath representations easily, and parse XPath expressions on-demand.
   */   
  class XPath
  {
    friend class XPathStaticInit;
  public:
    /*
     * Exposed Type definitions
     */
    /**
     * Vector of function arguments
     */
    class FunctionArguments : public std::vector<NodeSet*>
    {
    public:
      /**
       * Simple constructor
       */
      FunctionArguments();
      
      /**
       * Destructor : will free all provided NodeSets
       */
      ~FunctionArguments();
    };
  protected:
    /**
     * The XProcessor we have been instanciated against
     */
    XProcessor& xproc;

    /**
     * Runtime (or parse-time) in-mem segment pointer
     */
    XPathSegment* xpathSegment;
    
    /**
     * Array of XPathStep
     */
    XPathStepBlock* stepBlock;
    
    /**
     * Array of XPathResource.
     */
    XPathResourceBlock* resourceBlock;
    
#ifdef __XEM_XPATH_RUNTIME_GET_EXPRESSION
    /**
     * (Debug only) Current expression of the XPath.
     * The expression is retrieved from the provided expression in constructor, 
     * or from the originating textual AttributeRef (i.e. AttributeType_String) if built from an ElementRef.
     */
    const char* expression;
#endif

#ifdef __XEM_XPATH_RUNTIME_KEEP_NODEREF
    /**
     * Keep trace of the ElementRef or AttributeRef we have been instanciated against
     */
    NodeRef* sourceNodeRef;
#endif

    /**
     * When being parsed from an attribute on a non-writable Document, we have to keep an in-mem version of 
     * what we have parsed. This is completely counter-productive, but we don't have any other way (yet).
     * Except having the ability to store this on a per-document basis...
     */
    XPathParser* temporaryParserForReadOnlyDocuments;

    /**
     * Accession routine : XPathStep
     * @param stepId the XPathStepId of the XPathStep to get
     * @return the corresponding XPathStep 
     */
    INLINE XPathStep* getStep ( XPathStepId stepId );
    
    /**
     * Accession routine : XPathResource
     * @param resId the XPathResourceId of the resource to get
     * @return the corresponding textual contents of the resource.
     */
    INLINE const char* getResource ( XPathResourceOffset offset );
        
    /**
     * Load an already parsed XPath from a buffer
     */
    INLINE void loadFromPacked ( XPathSegment* packed );
     
    /**
     * Load an already parsed XPath, stored as an AttributeRef
     * @param attrRef the AttributeRef (with AttributeType_XPath type) to fetch XPath from.
     * @return true on succes, false if not.
     */
    INLINE bool loadFromStore ( AttributeRef& attrRef );

    /**
     * Runtime Evaluation
     * @param env the Env to use
     * @param result the resulting sequence
     * @param node the current node to be evaluated
     * @param stepId the stepId to eval
     */
    INLINE void evalStep ( NodeSet& result, NodeRef& node, XPathStepId stepId ) __FORCE_INLINE;

    /**
     * Runtime Evaluation : eval a given step as a KeyId, or throw an exception on error.
     * @param env the Env to use
     * @param node the current node to be evaluated
     * @param stepId the stepId to eval
     * @return the KeyId evaled
     */
    INLINE KeyId evalStepAsKeyId ( NodeRef& node, XPathStepId stepId );

    /**
     * Match a NodeRef against a reverse series of XPathStep.
     * @param env the Env to use for evaluation
     * @param node the node to match
     * @param stepIds the reverse chain of XPathStep.
     * @param index the current position in stepIds chain.
     * @return true if node matches, false if not.
     * \note For KeyId filtering, matches() uses evalNodeKey() to check a NodeRef.
     */
    bool matches ( NodeRef& node, XPathStep* step, Number& effectivePriority );

    /**
     * Match a NodeRef against one of its predicates.
     * @param env the Env to use for evaluation
     * @param result the NodeSet corresponding to the previous predicate (or the Axis Step).
     * @param previousNode the node we will match next if the predicates are true for this Step.
     *        That is, the originating NodeRef from which the current node would have been found 
     *        if we were evaluating the XPath expression, and not matching against.
     * @param node the node to match
     * @param stepIds the reverse chain of XPathStep.
     * @param index the current position in stepIds chain.
     * @param predicateId the index of the predicate in the XPathStep predicate array.
     * @return true if node matches, false if not.
     */
    bool matchesPredicate ( NodeSet& result, 
        NodeRef& previousNode, NodeRef& node,
        XPathStep* step, int predicateId, Number& effectivePriority );

    bool matchesPredicate ( NodeSet::iterator& iter, NodeRef& node, XPathStepId predicateStepId );

    /**
     * Compute the effective priority of a step
     */        
    Number computeEffectivePriority ( XPathStep* step ); 
    
    /**
     * Function Evaluation : Standard Arguments
     */
#define __XPath_Functor_Args NodeSet& result, NodeRef& node, XPathStep* step

    /**
     * __evalFunctor : the function type for XPath axis and function evaluation.
     */
    typedef void (XPath::*Functor) ( __XPath_Functor_Args );
   
    /**
     * Action Map. As the ActionId is (still) a __ui8, the higest value is 255.
     * Action Map contains both Axis specifiers and Function specifier
     */
    static Functor actionMap[256];
    
#define __XPath_Func(__name,__id,__cardinality,__defaults)		\
    void evalFunction##__id ( __XPath_Functor_Args )
#include <Xemeiah/xpath/xpath-funcs.h>
#undef __XPath_Func

#define __XPath_Axis(__name,__id)			\
    void evalAxis##__id ( __XPath_Functor_Args )
#include <Xemeiah/xpath/xpath-axis.h>
#undef __XPath_Axis

    /**
     * evalComparatorHelper is in charge of handling comparison operators,
     * @param env the Env to use.
     * @param result of the comparison
     * @param comparator the XPathFunc id of the comparison operator.
     * @param leftItem the left item to compare
     * @param rightItem the right item to compare
     * @return true if we may interrupt the comparison loop (when left and right operands are non-scalar)
     *         false if we may continue the loop comparison.
     */
    bool evalComparatorHelper ( bool& result, XPathAction comparator, 
        Item& leftItem, Item& rightItem );
        
    /**
     * evalComparator handles binary comparison operators.
     * It first evaluates left and right operands, and uses evalComparatorHelper() to perform a bi-dimensionnal
     * matrical comparison. We must check this better, anyway.
     * @param env the Env to use
     * @param comparator the XPathFunc id of the comparison operator.
     * @param node the current node used for evaluation
     * @param leftStepId the left XPathStepId operand
     * @param rightStepId the right XPathStepId operand
     * @return the comparison result.
     */
    bool evalComparator ( XPathAction comparator, NodeRef& node, 
        XPathStepId leftStepId, XPathStepId rightStepId );

    /**
     * Main comparator function
     */
    void evalActionComparator ( __XPath_Functor_Args );

  public:
    /**
     * Builds a SKMultiMapRef representing a 'xsl:key' mapping.
     * The SKMultiMapRef is equivalent to a std::map<String, ElementRef> template.
     * @param env the Env to use for evaluation
     * @param baseElement the root element, from which we check all descendants
     * @param map the SKMultiMapRef where to put the association.
     * @param matchXPath the XPath to use for filtering descendant elements
     * @param useXPath for each matching descendant element, use this XPath to compute the string key.
     * @param scopeXPath if the map is null, it will be resolved each time a new element is found
     * \note baseElement may not be the root element of the Document, in order to handle the xsl:base extension attribute.
     */
    void evalFunctionKeyBuild ( ElementRef& baseElement, ElementMultiMapRef& map, 
        XPath& matchXPath, XPath& useXPath, XPath& scopeXPath, KeyId mapId );
    
    /**
     * Retrieves a key result from a NodeSet
     */
    static void evalFunctionKeyGet ( ElementMultiMapRef& map,
        XPath& useXPath, NodeSet& resultNodeSet, NodeSet& argNodeSet );

  protected:

    /**
     * Retrieves a key result from a String
     */
    static void evalFunctionKeyGet ( ElementMultiMapRef& map,
        XPath& useXPath, NodeSet& resultNodeSet, String& stringArgument );

    /**
     * Helper : Get a NodeSet from the key() function
     */    
    void evalFunctionKeyGet ( __XPath_Functor_Args );

    /**
     * Helper : Get a NodeSet from the id() function
     */
    void evalFunctionIdGet ( __XPath_Functor_Args );

    /**
     * Complex scenario
     */
    INLINE bool evalNodeKeyNonWildCard ( NodeRef& node, XPathStep* step, KeyId nodeKeyId ) __FORCE_INLINE;

    /** 
     * Check that a node matches the KeyId part of a XPathStep
     * The following checks are operated :
     * -# If the XPathStep checks for an explicit element() node :
     *    - Return false root Element, attributes, and all elements defined in xemint: namespace text(), comment(), pi()
     * -# If the XPathStep checks for a specific namespace (i.e. '(namespace):*' expression)
     *    - Return false for different NamespaceIds
     * -# For Attribute axis, extrapolate the NamespaceId to allow unprefixed attributes.
     * -# Remove all xmlns:(namespace) and xmlns typed attributes for attribute:: axis.
     * @param node the NodeRef to check.
     * @param step the XPathStep to use for checking.
     * @return true if matches, false otherwise.
     * \todo The actual mechanism can not handle '(namespace):element()' expression. Change xemint_element() mechanism.
     */
    INLINE bool evalNodeKey ( NodeRef& node, XPathStep* step ) __FORCE_INLINE;

    /**
     * Check that an Element node matches the KeyId part of a XPathStep
     * @see evalNodeKey()
     */
    INLINE bool evalNodeKey ( ElementRef& node, XPathStep* step ) __FORCE_INLINE;

    /**
     * Checks if the 
     * @param pushResult push back the node to the pushResult NodeSet if it matches XPathStep.
     * @param node the node to check
     * @param step the XPathStep to check against.
     */
    INLINE void evalNodeKey ( NodeSet& pushResult, NodeRef& node, XPathStep* step );

    /**
     * @param pushResult push back the node to the pushResult NodeSet if it matches XPathStep.
     * @param node the ElementRef to check
     * @param step the XPathStep to check against.
     */
    INLINE void evalNodeKey ( NodeSet& pushResult, ElementRef& node, XPathStep* step );
    
    /**
     * Evaluate predicates (i.e filter a NodeSet using a series of predicate expressions).
     * At the end of predicate evaluation, the next step will be evaluated directly on the resulting NodeSet.
     * @param env the Env to use
     * @param targetResult the final (target) result to pushBack final items in.
     * @param result the current evaluation NodeSet.
     * @param step the step holding predicates.
     * @param revertResult for reverse axes (ancestor, preceding), we have to revert the provided result
     *    before evaluating next steps.
     * @return bool upon success. And there is always success.
     */
    void evalNodeSetPredicate ( NodeSet& targetResult, NodeSet& result, XPathStep* step, bool revertResult = false );
    
    /**
     * Shared function to evaluate both ancestor:: and ancestor-or-self:: axes.
     */
    void evalAxisAncestorGeneric ( __XPath_Functor_Args, bool includeSelf );
    
    /**
     * Shared funciton to evaluate both descendant:: and descendant-or-self:: axes.
     */
    void evalAxisDescendantGeneric ( __XPath_Functor_Args, bool includeSelf );
    
    /**
     * Shared recursive function to evaluate descendant[-or-self]:: axes on the Element
     */
    void evalAxisDescendantGenericElement ( NodeSet& result, ElementRef& element, XPathStep* step );

    /**
     * Evaluate functions arguments as a vector of nodesets
     */
    void evalFunctionArguments ( FunctionArguments& functionArguments, NodeRef& node, XPathStep * step );

    /**
     * Element does not have a XPath Attribute, so we have to call XPathParser to parse it.
     */ 
    void parseFromAttribute ( ElementRef& ref, KeyId attrKeyId, bool isAVT );
    
    /**
     * Get the base node we have been instanciated against
     */
    NodeRef& getBaseNode ( );
    
    /**
     * Protected Constructor
     */ 
    INLINE void init ();

    /**
     * XPath static initializer. 
     * Initializer builds the static map to Axis and Function handlers.
     */
    static void initStatic ();

  public:
    /**
     * Direct empty constructor
     */
    INLINE XPath ( XProcessor& xproc );

    /**
     * Direct constructor from an attribute
     */
    INLINE XPath ( XProcessor& xproc, AttributeRef& attr ); 

    /**
     * Direct constructor from a parsed XPathSegment
     */
    INLINE XPath ( XProcessor& xproc, XPathSegment* xpathSegment );

    /**
     * Direct constructor from a parsed XPathParser
     */
    INLINE XPath ( XProcessor& xproc, XPathParser& xpathParser );

    /**
     * Direct constructor from a parsed XPathParser which we will have to destroy at XPath destruction
     */
    INLINE XPath ( XProcessor& xproc, XPathParser* xpathParser, bool mayDelete = true );
    
    /**
     * Instanciate an XPath from a DOM AttributeRef.
     * If the ElementRef already has a parsed XPath, we use it.
     * Otherwise, the expression is parsed, using the ElementRef hierarchy to resolve namespace prefixes.
     * @param xproc the XProcessor to use
     * @param elementRef the ElementRef holding the attribute to use.
     * @param attrKeyId the attribute KeyId to use.
     * @param isAVT true if the expr is to be parsed as a AVT, false if not.
     */
    INLINE XPath ( XProcessor& xproc, ElementRef& elementRef, KeyId attrKeyId, bool isAVT = false );

    /**
     * XPath destructor.
     */
    virtual ~XPath();

    /**
     * Assignment Operator
     */
    INLINE XPath& operator= (XPath& xpath);

    /**
     * Existence operator
     */
    inline operator bool() const { return xpathSegment != NULL; }

    /**
     * Simple XProcessor accessor
     */
    inline XProcessor& getXProcessor() const { return xproc; }

    /**
     * Initialization routine : initialization from a given ElementRef and attribute KeyId.
     * @param ref the ElementRef where the originating AttributeRef is located.
     * @param attrKeyId the KeyId of the AttributeRef to parse XPath expression from.
     * @param embedded set to true for embedded (Attribute Value Template), or false for direct XPath expression.
     */
    INLINE void init ( ElementRef& ref, KeyId attrKeyId, bool isAVT = false );
    
    /**
     * Copy parsed XPath to another Element
     */
    AttributeRef copyTo ( ElementRef& ref, KeyId attrKeyId );

    /**
     * Evaluates the XPath from a given node.
     * @param result the NodeSet to put resulting items in.
     */
    INLINE void eval ( NodeSet& result );

    /**
     * Evaluates the XPath from a given node.
     * @param result the NodeSet to put resulting items in.
     * @param node the base node for XPath evaluation
     */
    INLINE void eval ( NodeSet& result, NodeRef& node );

    /**
     * Evaluates the XPath as a String result.
     * @return the resulting String.
     * \note Exception is raised if casting to a String failed
     */
    INLINE String evalString ( );

    /**
     * Single evalString
     */
    INLINE String evalString ( const NodeRef& node );
    
    
    /**
     * Evaluates the XPath as a Number result.
     * @return the resulting Number.
     * \note Exception is raised if casting to a Number failed
     */
    INLINE Number evalNumber ( );    
    INLINE Number evalNumber ( NodeRef& node );

    /**
     * Evaluates the XPath as a Node.
     * @return the resulting ElementRef.
     * \note Exception is raised if casting to an ElementRef failed
     */
    INLINE NodeRef& evalNode ( );
    // INLINE ElementRef evalNode ( NodeRef& node );
    
    /**
     * Evaluates the XPath as an Element.
     * @return the resulting ElementRef.
     * \note Exception is raised if casting to an ElementRef failed
     */
    INLINE ElementRef evalElement ( );
    INLINE ElementRef evalElement ( NodeRef& node );

    /**
     * Evaluates the XPath as an Attribute
     * @return the resulting AttributeRef
     */
    INLINE AttributeRef evalAttribute ();
    
    /**
     * Evaluates the XPath as a boolean.
     * @return the resulting bool.
     * \note Exception is raised if casting to a bool failed.
     */
    INLINE bool evalBool ( );
    // bool evalBool ( NodeRef& node );


    /**
     * XPath evaluation for XSLNumbering
     */
    String evalNumbering ( std::list<Integer>& positions );

    /**
     * XPath external function evaluator
     */
    static void evalXProcessorFunction ( XProcessor& xproc, KeyId functionId, NodeRef& node, NodeSet& result, const String& value );

    /**
     * Matches a node against a given XPath
     * @param effectivePriority the priority while evaluating templates.
     * @return true if the node matches, false otherwise.
     */
    bool matches ( NodeRef& node, Number& effectivePriority );

    /**
     * Matches a node without caring about priority
     * @param node the node to match against
     * @return true if the node matches, false otherwise.
     */
    inline bool matches ( NodeRef& node )
    { 
       Number unused;
       return matches ( node, unused );
    }

    /**
     * Final step record
     */
    class XPathFinalStep
    {
    public:
      XPathFinalStep() {}
      ~XPathFinalStep() {}

      bool elementOrAttribute;
      KeyId keyId;
      Number priority;

      std::list<KeyId> predicateAttributeIds;
    };

    /**
     * List of final steps
     */
    typedef std::list<XPathFinalStep> XPathFinalSteps;
    
  protected:
    /**
     * Compute final steps
     */
    void buildFinalSteps ( XPathFinalSteps& finalSteps, XPathStep* step );

    /**
     * Compute final step for predicate
     */
    void buildFinalStepPredicate ( XPathFinalStep& finalStep, XPathStep* step );
  public:
    /**
     * Compute final steps for XSL templates optimizations
     */
    void buildFinalSteps ( XPathFinalSteps& finalSteps );
    
    /**
     * Try to get the source ElementRef we have been instanciated against
     */
    ElementRef getSourceElementRef ();
    
    /**
     * Provide a list of unresolved variable references in the XPath expression
     */
    void getUnresolvedVariableReferences ( std::list<KeyId>& variableReferences );

    /**
     * Optimization : check if the XPath expression is a single variable reference
     */
    INLINE bool isSingleVariableReference ( KeyId varKeyId );

    /**
     * Show all the XPath parsed XPathSteps and resources
     * \note this is only for debugging.
     */
    void logXPath ();

    /**
     * Show the contents of a XPathStep 
     * @param stepId the XPathStepId of the XPathStep to show.
     * \note this is only for debugging.
     */
    void logStep ( XPathStepId stepId );
    
    /**
     * Show the contents of a resource
     * @param resId the id of the resource to show.
     * \note this is only for debugging.
     */
    void logResource ( XPathResourceOffset offset );
    
    /**
     * Returns the string expression of the XPath.
     * The string expression comes from the XPath expression provided at constructor
     * or from the AttributeType_String attribute from which the XPath originates.
     * @return the string expression of the parsed XPath.
     */
    const char* getExpression() const 
#ifdef __XEM_XPATH_RUNTIME_GET_EXPRESSION
    { return expression; }
#else
    { return "(unkown)"; }
#endif
  };

#ifdef XEM_XPATH_ITEM_COUNT
  void __countOrphanXPathResults();
#endif

};

#endif // __XEM_XPATH_H

