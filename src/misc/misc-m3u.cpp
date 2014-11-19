/*
 * misc-m3u.cpp
 *
 *  Created on: 24 janv. 2010
 *      Author: francois
 */

#include <Xemeiah/log.h>
#include <Xemeiah/io/filewriter.h>

#include <Xemeiah/auto-inline.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace Xem
{
  int
  XemM3U2XSPF(const char* file)
  {
    FILE* fp = fopen ( file, "r" );
    if ( ! fp )
      {
        Error ( "Could not open file '%s'\n", file );
        return 1;
      }
    FileWriter writer;
    writer.setFD(fileno(stdout));

    writer.addStr ( "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" );
    writer.addStr ( "<playlist version=\"1\" xmlns=\"http://xspf.org/ns/0/\">\n" );
    writer.addStr ( "<trackList>\n" );

    static const int bufferSz = 4096;
    char buffer[bufferSz];

    long duration = 0;
    char* title = NULL;

    while ( ! feof ( fp ) )
      {
        fgets ( buffer, bufferSz, fp );
        char* eol = strchr ( buffer, '\n' );
        if ( eol ) *eol = '\0';

        Log ( "BUFFER : '%s'\n", buffer );
        if ( !strcmp(buffer, "#EXTM3U") )
          {
            Log ( "Found EXTM3U Header !\n" );
            continue;
          }
        if ( !strncmp(buffer,"#EXTINF:", 7) )
          {
            Log ( "EXTINF !\n" );
            char* end;
            duration = strtol(&(buffer[8]), &end, 10);
            Log ( "Length = %ld, end=%s\n", duration, end );
            if ( *end == ',' )
              {
                end++;
                if ( title ) free ( title );
                title = strdup(end);
              }
            continue;
          }
        if ( *buffer == '#' )
          {
            continue;
          }

        writer.addStr ( "<track>\n" );
        writer.addStr ( "\t<location>" );
        writer.serializeText(buffer,true,false,true,false,false);
        writer.addStr ( "</location>\n" );
        if ( title )
          {
            writer.addStr ( "\t<title>" );
            writer.serializeText(title,true,false,true,false,false);
            writer.addStr ( "</title>\n" );
            free ( title );
          }
        if ( duration )
          {
            duration *= 1000;
            writer.addStr ( "\t<duration>" );
            writer.doPrintf("%ld", duration);
            writer.addStr ( "</duration>\n" );
          }
        writer.addStr ( "</track>\n" );

        title = NULL;
        duration = 0;
      }

    writer.addStr ( "</trackList>\n" );
    writer.addStr ( "</playlist>\n" );

    writer.flushBuffer();

    fclose ( fp );
    return 0;
  }

};
