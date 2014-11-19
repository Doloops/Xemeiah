/*
 * parser-parsefile.cpp
 *
 *  Created on: 19 déc. 2009
 *      Author: francois
 */
#include <Xemeiah/parser/parser.h>
#include <Xemeiah/parser/saxhandler-dom.h>
#include <Xemeiah/trace.h>
#include <Xemeiah/io/filereader.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_Parser Debug

namespace Xem
{
  void Parser::parseFile ( XProcessor& xproc, ElementRef& elt, const String& filePath, const String& keepTextMode )
  {
    Log_Parser ( "Importing file '%s'\n", filePath.c_str() );
    // FileFeeder fileFeeder( filePath.c_str() );
    FileReader fileReader ( filePath );

    SAXHandlerDom saxHandler(xproc, elt);

    Log_Parser ( "Setting keepTextMode : '%s'\n", keepTextMode.c_str() );

    saxHandler.setKeepTextMode ( keepTextMode );

    Parser parser(fileReader, saxHandler );
    parser.setKeepAllText ( true );
    bool keepComments = (keepTextMode != "xsl");

    Log_Parser ( "keepComments=%s\n", keepComments ? "true" : "false" );

    parser.setKeepComments ( keepComments );

    try
      {
        parser.parse ();
      }
    catch ( Exception *e )
      {
        detailException ( e, "Parsing error, could not import '%s'\n", filePath.c_str() );
        throw ( e );
     }
  }

};
