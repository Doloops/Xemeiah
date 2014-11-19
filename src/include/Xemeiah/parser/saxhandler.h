#ifndef __XEM_PARSER_SAXHANDLER_H
#define __XEM_PARSER_SAXHANDLER_H

#include <Xemeiah/kern/exception.h>

namespace Xem
{
  XemStdException ( EventHandlerException );
  XemStdException ( UndefinedNamespaceException );
  
  /**
   * SAX Event callbacks for Xem::Parser : all events are called as soon as possible during parsing.
   */
  class SAXHandler
    {
    public:
      /**
       * Virtual destructor
       */
      virtual ~SAXHandler() {}

      /**
       * Called after the parsing of the node name (key)
       * @param ns the namespace prefix (may be null if no namespace declared)
       * @param name the local part of the name
       */
      virtual void eventElement ( const char* ns, const char *name ) = 0;
     
      /**
       * Called after the parsing of the value
       * @param ns the namespace prefix (may be null if no namespace declared)
       * @param name the local part of the name
       * @param value the value associated with the param (entities resolved).      
       */
      virtual void eventAttr ( const char* ns, const char *name, const char *value ) = 0;
       
      /**
       * Called after the last attribute ( closing of the element start markup)
       */
      virtual void eventAttrEnd () = 0;

      /**
       * Called after a close markup ( <../> or <..> </..> )
       * @param ns the namespace prefix (may be null for a EmptyElementTag)
       * @param name the local part of the name (may be null for a EmptyElementTag)
       */
      virtual void eventElementEnd ( const char* ns, const char* name ) = 0;

      /**
       * Called after the parsing of a text between nodes 
       * @param the text contents (entities resolved).
       */
      virtual void eventText ( const char *text ) = 0;

      /**
       * Called at the end of a comment ( <!-- --!> ), if not disabled in the Parser.
       * @param comment the parsed comment 
       */
      virtual void eventComment ( const char *comment ) = 0;
      
      /**
       * Called at the end of a processing instruction ( <?name contents?> )
       * @param name the name of the PI
       * @param content the text contents of the PI
       */
      virtual void eventProcessingInstruction ( const char * name, const char* content ) = 0;

      /**
       * Called at discovery of a new entity
       */
      virtual void eventEntity ( const char* entityName, const char* entityValue ) = 0;

      /**
       * Called at discovery of an unparsed NDATA entity
       */
      virtual void eventNDataEntity ( const char* entityName, const char* ndata ) {}

      /**
       * Called at discovery of a DOCTYPE ELEMENT or ATTRIBUTE (to be refined)
       * @param markupName ELEMENT or ATTLIST
       * @param value raw value of the markup
       */
      virtual void eventDoctypeMarkupDecl ( const char* markupName, const char* value ) {}
      
      /**
       * Called at the (normal) end of the parsing.
       */
      virtual void parsingFinished () = 0;
    };

};
#endif // __XEM_PARSER_SAXHANDLER_H

