/*
 * filewriter.cpp
 *
 *  Created on: 6 janv. 2010
 *      Author: francois
 */

#include <Xemeiah/io/filewriter.h>
#include <Xemeiah/trace.h>
#include <Xemeiah/kern/exception.h>

#include <Xemeiah/auto-inline.hpp>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define Log_FileWriter Debug

namespace Xem
{
  FileWriter::FileWriter ( int fd_ )
  {
    fd = fd_;
    ownsFD = false;
  }

  FileWriter::~FileWriter ()
  {
    flushBuffer();
    if ( ownsFD )
      {
        ::close ( fd );
        fd = -1;
      }
  }

  void FileWriter::flushBuffer ()
  {
    if ( fd != -1 && cacheIdx )
      {
        ssize_t res = ::write ( fd, cache, cacheIdx );
        Assert ( cacheIdx ==  res, "Could not write all !\n" );
        sz += cacheIdx;
      }
    cacheIdx = 0;
  }

  void FileWriter::setFD ( int fd_ )
  {
    if ( fd != -1 && ownsFD )
      {
        NotImplemented ( "Shall free fd before reuse !\n" );
      }
    fd = fd_;
    ownsFD = false;
  }

  void FileWriter::setFile ( const String& filepath, bool allowMkdir )
  {
    Log_FileWriter ( "Writing file '%s'\n", filepath.c_str() );

    char* _fname = strdup(filepath.c_str());
    char* fname = _fname;
    char fulldname[4096];
    fulldname[0] = 0;
    while ( strchr ( fname, '/' ) )
      {
        char* dname = fname;
        char* next = strchr ( fname, '/' );
        *next = 0;
        next++;
        strcat ( fulldname, dname );
        strcat ( fulldname, "/" );

        struct stat st;
        if ( ::stat(fulldname,&st) == -1 && ! allowMkdir )
          {
            throwException(Exception, "Directory '%s' does not exist, "
                "and mkdir is disabled : could not create %s\n",
                fulldname, filepath.c_str() );
          }

        mkdir ( fulldname, 0755 );
        fname = next;
      }
    free ( _fname );

    fd = creat ( filepath.c_str(), 00644 );
    if ( fd == -1 )
      {
        throwException ( Exception, "Could not create output file '%s'\n", filepath.c_str() );
      }
    ownsFD = true;
  }

};
