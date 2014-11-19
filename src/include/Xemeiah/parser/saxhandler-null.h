#ifndef __XEM_PARSER_SAXHANDLER_NULL_H
#define __XEM_PARSER_SAXHANDLER_NULL_H


namespace Xem
{
  /**
   * Test only : empty event handler for parsing (@see Xem::Parser).
   */
  class SAXHandlerNull : public SAXHandler
    {
    public:
      SAXHandlerNull() {}
      ~SAXHandlerNull() {}
      inline void eventElement ( const char* ns, const char *name )                    {}
      inline void eventAttr ( const char* ns, const char *name, const char *value ) {} 
      inline void eventAttrEnd ()                                   {}
      inline void eventElementEnd ( const char* ns, const char* name )                              {}
      inline void eventText ( const char *text )                    {}
      inline void eventComment ( const char *comment )              {}
      inline void eventProcessingInstruction ( const char * name, const char* content ) {}
      inline void parsingFinished ()                                {}
      inline void eventEntity ( const char* entityName, const char* entityValue ) {}
    };

};
#endif // __XEM_PARSER_SAXHANDLER_NULL_H
