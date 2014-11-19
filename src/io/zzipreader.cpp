/**
 * \file Read a file from a Zip archive using ZZip library
 * TODO Reimplement zip file import
 */
#if 0 // DEPRECATED #ifdef __XEM_HAS_ZZIP

#include <Xemeiah/parser/zzip-feeder.h>
#include <Xemeiah/trace.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_ZZipFeeder Debug

namespace Xem
{
  static const __ui64 buffSize = 4096 * 4096;
  ZZipFeeder::ZZipFeeder ( ZZIP_FILE* _fp, __ui64 __totalSize )
  {
    AssertBug ( _fp, "Null fp !\n" );
    fp = _fp;
    buffer = (char*) malloc ( buffSize );
    totalFed = 0;
    totalSize = __totalSize;
  }

  ZZipFeeder::~ZZipFeeder ()
  {
    if ( buffer )
      free ( buffer );
    if ( fp )
      zzip_file_close ( fp );
  }

  bool ZZipFeeder::feed ( char* &b, __ui64 &length )
  {
    zzip_size_t len = zzip_file_read ( fp, buffer, buffSize );
    totalFed += len;
    Log_ZZipFeeder ( "Feeded %lu / %llu. Total %llu / %llu\n", len, buffSize, totalFed, totalSize );
    length = len;
  
    if ( ! len )
      {
	zzip_file_close ( fp );
	fp = NULL;
	b = NULL;
	return false;
      }
    b = buffer;
    return true;
  }
  
  bool ZZipFeeder::prefetch ()
  {
    return true;
  }


};

#endif // __XEM_HAS_ZZIP
