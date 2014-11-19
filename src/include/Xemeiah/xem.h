/**
 * \mainpage Xemeiah : an XML Database, XPath/XSLT/XUpdate processor, and more.
 *
 * \section usage Short Introduction on How to Use
 *
 * \section intro Introduction to Xemeiah
 *
 * Xemeiah is yet another XML framework, written from scratch in C++.
 * Xemeiah is built on top of :
 * - a memory penny-wise DOM (Document Object Model) layer,
 * - a fast XPath engine (XPath 1.0 for now, will be including XPath 2.0 soon). 
 * - an efficient XSLT engine (XSLT 1.0 as a base with 2.0 functionnalities).
 *
 * Why is Xemeieah faster than other XSLT implementations ?
 * -# Memory thriftiness : Xemeiah has been designed with great care to limit memory footprint to its minimum.
 *    - XML-parsed DOM (Document Object Model) is made up of loads of little elements (named nodes). 
 *    - Each byte spared for one element is a a million bytes spared for large XML files.
 * -# Strings are nice when they are parsed :
 *    - Xemeiah internally converts each node name to a unique integer, allowing fast key comparison.
 *    - Keys as strings are uniquely stored in a dedicated structure, avoiding to allocate them for each node.
 * -# Large file parsing at speed of light :
 *    - Xemeiah's pseudo-SAX-based XML parser has been designed with great care on performance.
 *    - With the help of a fast hashing mechanism, key names are quickly converted to their corresponding integer representation.
 * -# Avoid deep XML tree-walking to its maximum, and use information as it is :
 *    - Xemeiah's core XSLT engine uses the parsed DOM nodes information as they are, no post-parsing conversion is operated.
 *    - XPath expression parsing is performed on-demand (i.e. when we need to), and the XPath parsed representation is directly stored in the DOM.
 * -# Avoid string duplication when it's possible :
 *    - Xemeiah uses a specially crafted String class that greatly reduces the number of string copying while processing.
 *    - For example, when processing <code><xsl:value-of "@attribute"/></code>, not a single string duplication will occur until serialization of the final result.
 *
 * Xemeiah extensions to standard XSL and XUpdate commands :
 * -# Fast external importing
 *    - Import a file using xem:import_file (equivalent to XPath document())
 *    - Import a whole folder using xem:import_folder
 *    - Import a whole zip file using xem:import_zip
 * -# Memory management
 *    - Release a parsed document using xem:release-document, future calls to document() will re-parse the document
 * -# Multithreading commands
 *    - Spawn multiple processing threads using xem:fork, synchronize using xem:wait-forks
 * -# Exception handling (ala C++/Java)
 *    - Throw an exception using xem:exception
 *    - Catch exceptions with xem:catch-exceptions, containing a xem:try and a xem:catch subsection.
 * -# Call an external stylesheet (independant XSLT processing)
 *    - Use xem:process to process a new stylesheet with a different XML content
 *
 * The following (prototype) modules extend Xemeiah's functionalities :
 * - a persitence layer, with an efficient binary storage of XML contents, able to fetch XML fragments from disk on-demand,
 * - an XUpdate prototype implementation
 * - an integrated WebServer, using XSLT, XUpdate, XForms (?) to offer rich Web-based user experience.
 *
 * \section features Current Version Features
 *
 * Xemeiah is almost able to (what it should do right now) :
 * - Process XSLT stylesheets
 *
 * Xemeiah is quite able to (see it as a POC) :
 * - Handle multiple Gigabytes of XML contents directly, using its persistence layer.
 * - Provide a CVS-type COW (Copy On Write) branch mechanism, with revisions.
 * - Process XUpdate fragments.
 * - Be used as a Web server that generates HTML pages from XML contents, using XSLT, XUpdate, ...
 * 
 * \section roadmap Roadmap
 * 
 * Development Roadmap (as of Xemeiah 0.3.81)
 * - 0.3.x is somewhat a technology preview, showing what Xemeiah will look like in the future.
 * - 0.4.x has a complete and robust XSLT/XUpdate core engine, validating all Oasis specifications.
 * - 0.5.x streightens Persistence Layer (robust checking, ...), with checking, recovery, ...
 * - 0.6.x implements the Security Framework  (including User authentication, ...).
 * - 0.8.x enhances WebServer functionalities and scalability.
 * - 0.9.x introduces external data processing (SQL interconnection, WebServices, ...).
 * - 1.x.x is ready for production, both as a XML Database and as a WebServer.
 *
 * Functions Roadmap
 * - Complete Oasis XSLT conformance tests (see http://www.oasis-open.org/committees/tc_home.php?wg_abbrev=xslt).
 * - Finalize XUpdate implementation
 * - Finalize, optimize and stress-test persistence layer.
 * - Integrate XML Schema definition, and build an object model on top of XML elements
 * - Provide high-level OOP functionalities on to of this object model
 * - Implement XForms, ...
 * - Intergrate an ACL-based security framework, directly on the DOM.
 * - Finalize, optimize and stress-test WebServer features.
 * 
 * \section source_arch Source Architecture
 *
 * Source code is splitted by functional modules.
 * All the header files are sorted in the 'include/Xemeiah' directory.
 *
 * - core - Low-level XML database layer.
     This includes :
     -# Main Xem::Store class, which handles all on-disk (Xem::StoreContext, for persistence) and in-memory (Xem::VolatileContext) document contexts.
     -# On-disk file formats for Key and Namespace persistence (see core-keys.h).
     -# Page handling, memory segmentation and protection, mmap mechanisms...
     -# Global Markup Key mapping (key as string <-> key as unique id) with the Xem::KeyCache
     -# Contextual namespace aliasing functionality : Xem::NamespaceAlias.
     -# Xem::Env (Variables environment) : stackable copy-on-write variables environnement with fast variable lookup

 * - dom - XML DOM (Document Object Model).
     -# XML Elements and Attributes
       Global approach is that XML elements are directly stored in mmaps as structs,
       and the C++ part only contains references to these objects, mostly instanciated in the stack.
       That is why all objects manipulated here as considered References (-Ref suffix).
       This includes standard DOM nodes :
       -# Xem::ElementRef : pivot element type, including normal elements, text(), processing-instruction(), comment()...
       -# Xem::AttributeRef : standard (string) and non-standard attributes for elements.
       This also includes Extended attribute types, i.e. structures which are stored as attributes in the DOM :
       -# Xem::SKMapRef : SkipList-based single map : a single key maps to a single value.
       -# Xem::SKMultiMapRef : SkipList-based multiple map : a single key maps to a list of values.
       -# Xem::XPath : attributes stored as binary pre-compiled xpaths, implemented in /xpath directory.
     -# Xem::NodeSet
       A Xem::NodeSet is a list of Xem::NodeSet::Item, which can be one of the two following :
        -# A singleton of a base type value (String, Number, Interger, bool).
        -# A list of Xem::NodeRef, optionnaly sorted in document order.
     
 * - parser - Generic SAX-type XML parsing functions.
     This includes :
     -# Main parsing class (Xem::Parser), which may lack strong entity & DOCTYPE support yet.
     -# Parser feeders (Xem::Feeder), in charge of feeding the parser with a stream of bytes.
        Implemented feeders are :
        -# Feeder from a file : Xem::FileFeeder
        -# Feeder from a zipped file : Xem::ZZipFeeder (using zzip library).
     -# Event Handler, called from the Parser (To be merged with NodeFlow class).

 * - xpath - Fast pre-compiled XPath language implementation (Xem::XPath)
     This includes :
     -# Parsing of a string to obtain a pre-compiled storable XPath (size optimized). Parsing is achieved :
        -# Tokenize the XPath expression by producing a XPath::Token chain.
        -# Transform the Xem::XPath::Token chain into a XPath::Token tree, reflecting binary operators, ...
        -# Transform the Xem::XPath::Token tree into a serializable, highly space-efficient list of Xem::XPathStep.
     -# Evaluation of an XPath expression in a given context, to a NodeSet, to an ElementRef, to a String or to a bool.
        Evaluation is achieved by processing each Xem::XPathStep of the expression, a step being :
        -# An Axis step, generating a list of nodes from the current node evaluated (see xpath-eval-axis-pos.cpp).
           -# The NodeTest is evaluated, to eliminate non-matching nodes (see xpath-eval-nodetest.hpp)
           -# The predicates are evaluated, to filter the preliminary NodeSet in function of criteras provided in predicates
           -# Each node of the resulting NodeSet is then processed using the next Xem::XPathStep.
        -# A Function step, calling an XPath function (see xpath-eval-func.cpp).
           - Note : some functions may have predicates and next steps.
     -# Matching functionalities, i.e. check that a given node matches an XPath pattern (see xpath-matches.cpp).
 
 * - xprocessor - Generic XML-based processing engine.
     - Xem::XProcessor takes two XML trees for argument : 
       -# The operating tree, which triggers actions depending on element names.
       -# The evaluated tree, which is used to evaluate XPath expression.
     - For example, for processing a XSLT stylesheet, the operating tree is the XSLT stylesheet (in DOM format)
       and the evaluted tree is the XML source document (in DOM format).
     - The operating tree is processed with the help of handlers, which are chosen using the KeyId of the operating element to process.
     - Xem::XProcessor finds the handler by trying :
       -# Key handlers, matching a fully-qualified name.
       -# Namespace handlers, matching all keys in a given namespace (when no Key handler exist for this element).
       -# Default handler, called when no other handler matches.
     - The result of a Xem::XProcessor operation is (generally) a Xem::NodeFlow flow of XML nodes.
     - Xem::XProcessor uses Xem::XArgs to group contextual references (Xem::Env, ...) usefull for processing.
     - xprocessor also contains xem: special extensions (see xem-instructions.cpp).
 *
 * - xsl - XSL language implementation, based on Xem::XProcessor.
     XSL implementation is splitted in two different files :
     -# xsl-instructions.cpp, implementing handlers for each XSLT keyword
     -# xslprocessor.cpp, implementing general functionalities.
     
 * - nodeflow - Xem::NodeFlow handles a stream of node creation events (SAX-based, similar to Xem::EventHandler).
     XSLProcessor puts its output in a Xem::NodeFlow class. 
     Implemented Xem::NodeFlow are :
     -# Xem::NodeFlowDom : append nodes to an existing DOM tree.
     -# Xem::NodeFlowStream : serialize (i.e. generate a byte stream) from the Node events 
        (note : Xem::NodeFlowStream lacks some XML sanity checks, for example attribute unicity in a given element).
 
 * - tools - Generic tools
     This includes :
     -# Importing from a XML file, 
     -# Importing from a directory
     -# Importing from a zip file

 * - xupdate - XUpdate language implementation, based on XProcessor (very early prototype phase).
     
 * - webserver - Web-serving functionnalities
     This includes :
     -# Xem::WebServer : in charge of openning the port, and accepting new clients (i.e. creating a WebConn per connection)
     -# Xem::WebConn : in charge of handling a HTTP query, and generating the corresponding HTML answer.

 * - xem-standard - Default XML code framework for WebServer usages.
     This includes :
     -# Loads of stuff, but still an architecture to find...
 *
 * \section misc Miscellaneous
 *
 * <p>This project is (still) a spare-time hacker's proof of concept : XML will re-structurate computer designs in the years to come. <br/>
 * Copyleft (or GPL, or ...) 2005-2999 by Francois Barre. <br/>
 * Many thanks to my beloved, for her infinite patience, and her ability to refill me with kindness (and food !) at moments of despair...</p>
 */

#include <Xemeiah/version.h>

/**
 * Xemeiah's namespace.
 */
namespace Xem
{

};

