#include <Xemeiah/xprocessor/xprocessorcmd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <time.h>
/**
 * \file Xemeiah bootstrap caller from command-line.
 * This bootstrap uses XProcessor library support to load the corresponding library and call the command-line handler for it.
 * As the main function only delegates processing to one of these handlers, no include is defined here.
 */

#define Usage(...) fprintf ( stderr, __VA_ARGS__ )

void printUsage ( const char* progName )
{
  Usage ( "%s [-h | -v | xsl | misc | pers]\n", progName );
  Usage ( "Valid group-commands are :\n" );
  Usage ( "\txsl : processes XSLT stylesheets. Call '%s xsl' for more information.\n", progName );

  Usage ( "\n" );
  Usage ( "\tmisc : Miscellaneous commands on XML files. Call '%s misc' for more information.\n", progName );

  Usage ( "\n" );
  Usage ( "\tpers : commands using the Persistence Layer of Xemeiah. Call '%s pers' for more information.\n", progName );

  Usage ( "\n" );
}

struct XemCommandAlias
{
  const char* alias;
  const char* name;
};

static const XemCommandAlias xemCommandAliases[] =
    {
        { "-h", "--help" },
        { "-v", "--version" },
        { "pers", "persistence" },
        { NULL, NULL }
    };

int main ( int argc, char** argv)
{
  int ret = 0;
  const char* progName = argv[0];
  const char* version = Xem::XProcessorCmd::getXemVersion();

  const char* cmd = NULL;
  if ( argc < 2 )
    {
      Usage ( "Not enough parameters provided : must at least provide the group-command.\n" );
      goto print_usage;
    }

  cmd = argv[1];

  for ( int i = 0 ; xemCommandAliases[i].alias ; i++ )
    {
      if ( strcmp ( cmd, xemCommandAliases[i].alias ) == 0 )
        {
          cmd = xemCommandAliases[i].name;
        }
    }

  if ( strcmp(cmd,"--help") == 0 )
    {
      goto print_usage;
    }

  if ( strcmp(cmd,"--version") == 0 )
    {
      Usage ( "Xemeiah version %s\n", version );
      return 0;
    }

  return Xem::XProcessorCmd::executeCmdLineHandler(cmd, argc, argv);

print_usage:
  Usage ( "Xemeiah version %s\n", version );

  printUsage ( progName );
  return ret;
}
