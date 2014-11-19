/*
 * Not to be included directly : 
 * List of all known (builtin) functions
 */


// 1 - String related Functions
__XPath_Func ( "string", String, 1, true );
__XPath_Func ( "concat", Concat, 0, false );
__XPath_Func ( "starts-with", StartsWith, 2, false );
__XPath_Func ( "ends-with", EndsWith, 2, false );
__XPath_Func ( "contains", Contains, 2, false );
__XPath_Func ( "substring-before", SubstringBefore, 2, false );
__XPath_Func ( "substring-after", SubstringAfter, 2, false );
__XPath_Func ( "substring", Substring, 3, false );
__XPath_Func ( "string-length", StringLength, 1, true );
__XPath_Func ( "normalize-space", NormalizeSpace, 1, true );
__XPath_Func ( "translate", Translate, 3, false );
__XPath_Func ( "upper-case", UpperCase, 1, false );
__XPath_Func ( "lower-case", LowerCase, 1, false );

// 2 - List and node-set related Functions
__XPath_Func ( "last", Last, 0, false );
__XPath_Func ( "position", Position, 0, false );
__XPath_Func ( "count", Count, 1, false );
__XPath_Func ( "id", Id, 1, false );
__XPath_Func ( "local-name", LocalName, 1, true );
__XPath_Func ( "namespace", NameSpace, 1, true );
__XPath_Func ( "namespace-uri", NameSpaceURI, 1, true );
__XPath_Func ( "name", Name, 1, true );
__XPath_Func ( "key", Key, 2, false );
__XPath_Func ( "current", Current, 0, false  );
__XPath_Func ( "generate-id", GenerateId, 1, true );
__XPath_Func ( "document", Document, 2, false  ); // But document also accepts a single argument.
__XPath_Func ( "Union", Union, 0, false );
// __XPath_Func ( "Intersection", Intersection, 0, false );

// 3 : Boolean Functions
__XPath_Func ( "boolean", Boolean, 1, false );
__XPath_Func ( "not", Not, 1, false );
__XPath_Func ( "true", True, 0, false );
__XPath_Func ( "false", False, 0, false );
__XPath_Func ( "lang", Lang, 1, false );
__XPath_Func ( "BooleanAnd", BooleanAnd, 0, false );
__XPath_Func ( "BooleanOr", BooleanOr, 0, false );

// 4 : Number Functions
__XPath_Func ( "number", Number, 1, true );
__XPath_Func ( "sum", Sum, 1, false );
__XPath_Func ( "floor", Floor, 1, false );
__XPath_Func ( "ceiling", Ceiling, 1, false );
__XPath_Func ( "round", Round, 1, false );
__XPath_Func ( "Plus", Plus, 0, false );
__XPath_Func ( "Minus", Minus, 0, false );
__XPath_Func ( "Multiply", Multiply, 0, false );
__XPath_Func ( "Div", Div, 0, false );
__XPath_Func ( "Modulo", Modulo, 0, false );
__XPath_Func ( "format-number", FormatNumber, 2, false );

// 5 : XSL-Numbering functions
__XPath_Func ( "#xslnumbering", XSLNumbering, 0, false );
__XPath_Func ( "#xslnumbering-singlechar", XSLNumbering_SingleCharacterConverter, 0, false );
__XPath_Func ( "#xslnumbering-integer",  XSLNumbering_IntegerConverter, 0, false );

// 6 : Implementation Specific
__XPath_Func ( "element-available", ElementAvailable, 1, false );
__XPath_Func ( "function-available", FunctionAvailable, 1, false );
__XPath_Func ( "unparsed-entity-uri", UnparsedEntityURI, 1, false );
__XPath_Func ( "system-property", SystemProperty, 1, false );


// 7, 8, 9 : Unused

// a : Xemeiah Extensions
__XPath_Func ( "avt", AVT, 1, false );
__XPath_Func ( "#XPathIndirection", XPathIndirection, 1, false  );
__XPath_Func ( "if", If, 3, false );
__XPath_Func ( "uniq", Uniq, 2, false );
__XPath_Func ( "matches", Matches, 2, false  );
__XPath_Func ( "has-variable", HasVariable, 1, false );
__XPath_Func ( "#Function", FunctionCall, 1, false  );
__XPath_Func ( "#ElementFunction", ElementFunctionCall, 1, false  );
__XPath_Func ( "NodeSetTest", NodeSetTest, 2, false );
__XPath_Func ( "child-lookup", ChildLookup, 3, false );
__XPath_Func ( "revert-nodeset", NodeSetRevert, 1, false );

// d : Comparators (not defined this way)
// e : Standard Axes : defined in <Xemeiah/xpath/xpath-axis.h>
// f : Non-Standard Axes : defined in <Xemeiah/xpath/xpath-axis.h>

