/**
 * \file Import file
 * TODO Reimplement zip file import
 */

#if 0 // DEPRECATED

#include <Xemeiah/tools/import.h>
#include <Xemeiah/parser/parser.h>
#include <Xemeiah/parser/zzip-feeder.h>
#include <Xemeiah/parser/eventhandler-dom.h>
#include <Xemeiah/parser/eventhandler-null.h>
#include <Xemeiah/dom/attributeref.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <dirent.h>
#include <sys/resource.h>
#include <pthread.h>

#include <list>

#ifdef __XEM_HAS_ZZIP
#include <zzip/zzip.h>
#endif // __XEM_HAS_ZZIP

#include <Xemeiah/auto-inline.hpp>

namespace Xem
{
#ifdef __XEM_HAS_ZZIP
  bool importZip ( ElementRef& elt, const String& _zipPath, std::list<String*> excludedFiles )
  {
   const char* zipPath = _zipPath.c_str();
   Info ( "Importing Zip File '%s'\n", zipPath );
   ZZIP_DIR* dir = zzip_dir_open ( zipPath, 0 );
   if ( ! dir )
     {
       Error ( "Could not open zip file '%s'\n", zipPath );
       return false;
     }

   ZZIP_DIRENT dirent;
   __ui64 nbImported = 0;
   while ( zzip_dir_read ( dir, &dirent ) )
     {
       const char* filePath = dirent.d_name;
       if ( *filePath == '/' )
         filePath++;
       bool excludeThisFile = false;
       for ( std::list<String*>::iterator excluded = excludedFiles.begin() ; excluded != excludedFiles.end () ; excluded++ )
         {
            if ( strcmp ( (*excluded)->c_str(), filePath ) == 0 )
              {
                excludeThisFile = true;
              }
         }
       if ( excludeThisFile )
         {
            Info ( "Skipping file '%s' as it is excluded.\n", filePath );
            continue;
         }
       Info ( "Zip File '%s' : %i bytes compressed / %i bytes total\n", filePath, dirent.d_csize, dirent.st_size );
       ZZIP_FILE* fp = zzip_file_open ( dir, dirent.d_name, 0 );
       if ( ! fp )
         {
           Error ( "Could not read file '%s'\n", filePath );
           continue;
         }
       ElementRef child = elt.getDocument().createElement ( elt, elt.getKeyCache().builtinKeys.xemdocs_file() );
       elt.appendLastChild ( child );
       child.addAttr ( elt.getKeyCache().builtinKeys.xemdocs_name(), filePath );

       ZZipFeeder zzipFeeder ( fp, dirent.st_size );
#if 1
       EventHandlerDom eventHandler(child);
#else
       EventHandlerNull eventHandler;
#endif
       Parser parser ( zzipFeeder, eventHandler );
       try
         {
           parser.parse ();
         }
       catch ( Exception *e )
         {
           Warn ( "Parsing error, could not import '%s'\n", dirent.d_name );
           child.addAttr ( elt.getKeyCache().builtinKeys.xem_exception(), "could not import" );
           delete ( e );
         }
       nbImported++;
       //       if ( nbImported == 100 ) break;
     }
   zzip_dir_close ( dir );
   return true;

  }
#else // __XEM_HAS_ZZIP
  bool importZip ( ElementRef& elt, const String& _zipPath, std::list<String*> excludedFiles )
  {
    Fatal ( "Xemeiah has no ZZip library compiled.\n" );
    return false;
  }

#endif // __XEM_HAS_ZZIP

};

#endif
