#ifndef __XEM_STORE_XPATH_FORMAT_H
#define __XEM_STORE_XPATH_FORMAT_H

#ifndef __XEM_KERN_FORMAT_CORE_TYPES_H
#error Shall include <Xemeiah/core/format/core_types.h> first !
#endif
/**
 * \file On-disk/In-memory parsed (pre-compiled) XPath Format
 */

namespace Xem
{
  /**
   * Implemented actions, an action being a function or an axis step.
   */
  enum __XPathStepAction
    {
      XPathAction_NULL = 0x00,

      // 0 : Unused yet
      
      // 1 : String-related functions
      XPathFunc_String          = 0x10,
      XPathFunc_Concat          = 0x11,
      XPathFunc_StartsWith      = 0x12,
      XPathFunc_EndsWith        = 0x13,
      XPathFunc_Contains        = 0x14,
      XPathFunc_SubstringBefore = 0x15,
      XPathFunc_SubstringAfter  = 0x16,
      XPathFunc_Substring       = 0x17,
      XPathFunc_StringLength    = 0x18,
      XPathFunc_NormalizeSpace  = 0x19,
      XPathFunc_Translate       = 0x1a,
      XPathFunc_UpperCase       = 0x1b,
      XPathFunc_LowerCase       = 0x1c,
      
      // 2 : List and node-set related Functions
      XPathFunc_Last         = 0x20,
      XPathFunc_Position     = 0x21,
      XPathFunc_Count        = 0x22,
      XPathFunc_Id           = 0x23,
      XPathFunc_LocalName    = 0x24,
      XPathFunc_NameSpaceURI = 0x25,
      XPathFunc_Name         = 0x26,
      XPathFunc_NameSpace    = 0x27,
      XPathFunc_Key          = 0x28,
      XPathFunc_Current      = 0x29,
      XPathFunc_GenerateId   = 0x2a,
      XPathFunc_Document     = 0x2b, //< Retrieve a document from filesystem or URI
      XPathFunc_Union        = 0x2c, //< Union as indicated by '|' characters
      XPathFunc_Intersection = 0x2d,

      // 3 : Boolean Functions
      XPathFunc_Boolean    = 0x30,
      XPathFunc_Not        = 0x31,
      XPathFunc_True       = 0x32,
      XPathFunc_False      = 0x33,
      XPathFunc_Lang       = 0x34,
      XPathFunc_BooleanAnd = 0x35,
      XPathFunc_BooleanOr  = 0x36,
      
      // 4 : Number Functions
      XPathFunc_Number       = 0x40,
      XPathFunc_Sum          = 0x41,
      XPathFunc_Floor        = 0x42,
      XPathFunc_Ceiling      = 0x43,
      XPathFunc_Round        = 0x44,
      XPathFunc_Plus         = 0x45,
      XPathFunc_Minus        = 0x46,
      XPathFunc_Multiply     = 0x47,
      XPathFunc_Div          = 0x48,
      XPathFunc_Modulo       = 0x49,
      XPathFunc_FormatNumber = 0x4a,

      // 5 : XSLNumbering functions (used in xsl:number)
      XPathFunc_XSLNumbering                            = 0x50,
      XPathFunc_XSLNumbering_SingleCharacterConverter   = 0x51,
      XPathFunc_XSLNumbering_IntegerConverter           = 0x52,      

      // 6 : Implementation-specific functions
      XPathFunc_ElementAvailable  = 0x65, // Defined by XSLT 1.0 15
      XPathFunc_FunctionAvailable = 0x66, // Idem
      XPathFunc_SystemProperty    = 0x67, // Defined in XSLT 1.0 12.4 Misc Additional Functions
      XPathFunc_UnparsedEntityURI = 0x68, // Idem
         
      // 7, 8, 9 : Unused

      // a : Xemeiah Extensions
      XPathFunc_AVT                 = 0xa0, //< Explicitly evaluate an attribute as an AVT
      XPathFunc_XPathIndirection    = 0xa1, //< Indirect the xpath retrieving mechanism, using {} inside a XPath

      XPathFunc_If                  = 0xa2, //< The Logical C ?: trigraph ; Use with if ( condition, action-if-true, action-if-false )
      XPathFunc_Uniq                = 0xa3, //< Sort a NodeSet using a XPath to retrieve string-based key comparator

      XPathFunc_Matches             = 0xa4, //< Check that a given node matches a given XPath expression
      XPathFunc_HasVariable         = 0xa5, //< Check that a variable has been assigned in the current context

      XPathFunc_FunctionCall        = 0xa6, //< External Function Call
      XPathFunc_ElementFunctionCall = 0xa7, //< External Element Function Call (the '->' operator)

      XPathFunc_ChildLookup         = 0xaa, //< Child Lookup : childlookup('element QName', 'attribute QName', 'value')

      XPathFunc_NodeSetTest         = 0xaf, //< Used for parenthesis expression protection

      // Deprecated Xemeiah Extensions
      XPathFunc_NodeSetRevert     = 0xaf,
      XPathFunc_LastFunc          = 0xcf,

      // d : Comparison Functions
      XPathComparator_Equals              = 0xd0,
      XPathComparator_NotEquals           = 0xd1,
      XPathComparator_LessThan            = 0xd2,
      XPathComparator_LessThanOrEquals    = 0xd3,
      XPathComparator_GreaterThan         = 0xd4,
      XPathComparator_GreaterThanOrEquals = 0xd5,

      // e : Standard Axes action
      XPathAxis_Ancestor           = 0xe1,
      XPathAxis_Ancestor_Or_Self   = 0xe2,
      XPathAxis_Attribute          = 0xe3,
      XPathAxis_Child              = 0xe4,
      XPathAxis_Descendant         = 0xe5,
      XPathAxis_Descendant_Or_Self = 0xe6,
      XPathAxis_Following          = 0xe7,
      XPathAxis_Following_Sibling  = 0xe8,
      XPathAxis_Namespace          = 0xe9,
      XPathAxis_Parent             = 0xea,
      XPathAxis_Preceding          = 0xeb,
      XPathAxis_Preceding_Sibling  = 0xec,
      XPathAxis_Self               = 0xed,

      // f : Non-standard Axes
      XPathAxis_Root            = 0xf0,
      XPathAxis_Home            = 0xf1,
      XPathAxis_Variable        = 0xf2,
      XPathAxis_Resource        = 0xf3,
      XPathAxis_ConstInteger    = 0xf4,
      XPathAxis_ConstNumber     = 0xf5,
      XPathAxis_Blob            = 0xfa,

    };
  typedef __ui8 XPathAction;
  typedef __ui8 XPathStepFlags;

#define __XPathAction_isFunction(__action) ( (__action) <= XPathFunc_LastFunc )
#define __XPathAction_isComparator(__action) ( (__action & 0xf0) == 0xd0 )
#define __XPathAction_isNumberFunction(__action) ( (__action & 0xf0 ) == 0x40 )

  enum __XPathStepFlags
    {
      XPathStepFlags_PredicateHasPosition      = 0x1, //< Indicates that the Predicate uses position()
      XPathStepFlags_PredicateHasLast          = 0x2, //< Indicates that the Predicate uses last()
      XPathStepFlags_NodeTest_Namespace_Only   = 0x4, //< Indicates that the NodeTest uses only namespace() part of the key
      XPathStepFlags_NodeTest_LocalName_Only   = 0x8, //< Indicates that the NodeTest uses only local-name() part of the key
    };
  
  typedef __ui16 XPathStepId;
  typedef __ui32 XPathResourceOffset;
  
  static const XPathStepId XPathStepId_NULL = 0xffff;
  static const int XPathStep_MaxPredicates = 4;
  static const int XPathStep_MaxFuncCallArgs = 8;
  
  /**
   * XSL numbering integer conversion format (to rename)
   */
  struct XSLNumberingIntegerConvertion
  {
    __ui8 precision;
    __ui8 groupingSize;
    char groupingSeparator;
  }; // size : 3
  
  /**
   * XSL numbering format (to rename)
   */
  struct XSLNumberingFormat
  {
    XPathStepId preToken;
    union
      {
        __ui8 singleCharacter;
        XSLNumberingIntegerConvertion integerConversion;
      };
  }; // size : 2 + max(1,3) => 6
  
  /**
   * XPathStep represents an action of the XPath expression, with its settings
   */
  struct XPathStep
  {
    XPathAction action;
    XPathStepFlags flags;
    XPathStepId lastStep;
    XPathStepId nextStep;
    XPathStepId predicates[XPathStep_MaxPredicates];
    KeyId keyId;
    union 
    {
      XPathStepId functionArguments[XPathStep_MaxFuncCallArgs];
      XPathResourceOffset resource;
      Integer constInteger;
      Number constNumber;
      XSLNumberingFormat xslNumberingFormat;
    };
  }; 
  /* 
   * Head Size : 1 + 1 + 2 + 4*2 = 12
   * Union size : max(4, 8*2 = 16, 4, 8, 8, 6) = 16
   * Total size : 12 + 16 = 28 => 32
   */


  /**
   * XPathStepBlock represents the array of XPathStep
   */
  struct XPathStepBlock
  {
    XPathStep steps[1];
  };

  /**
   * XPathResourceBlock represents the array of string resources.
   */
  typedef char XPathResourceBlock;
  
  /**
   * XPathSegment is the header for the parsed format of an XPath expression.
   */
  struct XPathSegment
  {
    /**
     * Total number of steps
     */
    XPathStepId nbStepId;
    
    /**
     * Index of the head step
     */
    XPathStepId firstStep;
  };
};


#endif

