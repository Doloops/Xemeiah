/*
 * filereader.cpp
 *
 *  Created on: 6 janv. 2010
 *      Author: francois
 */

#include <Xemeiah/io/filereader.h>

#include <Xemeiah/trace.h>
#include <Xemeiah/kern/exception.h>

#include <Xemeiah/auto-inline.hpp>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define Log_FileReader Debug

namespace Xem
{
  void FileReader::init ()
  {
    windowSize = (1ULL << 20) * 256;
    fd = -1;
    buffer = NULL;
    offset = 0;
    ownsMyFd = false;
  }

  FileReader::FileReader ( int fd_ )
  {
    init ();
    fd = fd_;
    ownsMyFd = false;
  }

  FileReader::FileReader ( const String& path )
  {
    init ();
    ownsMyFd = true;
    fd = ::open(path.c_str(), O_RDONLY);
    if ( fd == -1 )
      {
        throwException ( Exception, "Could not read file '%s' : err=%d:%s\n", path.c_str(), errno, strerror(errno) );
      }

    setURI ( path );

    try
    {
      setFileLength();
    }
    catch ( Exception* e )
    {
      detailException ( e, "Could not set file length on file '%s'\n", path.c_str() );
      ::close ( fd );
      fd = -1;
      throw ( e );
    }
  }


  FileReader::~FileReader ()
  {
    if ( buffer )
      {
        /*
         * Compute how much we mmap() the last time
         * If we did not reach the end of the file, then we have mmaped a whole window
         * Otherwise, we have just mmaped the tail of the file.
         * This is a bit useless, because we could always mmap a whole windowSize including at file tail.
         */
        __ui64 toUnmap = ( offset < fileLength ) ? windowSize : fileLength % windowSize;

        Log_FileReader ( "Destructor : unmapping at offset=%llu/%llu, toUnmap=%llu\n",
            offset, fileLength, toUnmap );
        munmap ( buffer, toUnmap );
        buffer = NULL;
      }
    fileLength = 0;

    if ( ownsMyFd && fd != -1 )
      {
        ::close ( fd );
      }
  }

  void FileReader::setURI ( const String& path )
  {
    char* file_dup = strdup(path.c_str());
    char* file_sep = strrchr ( file_dup, '/' );
    if ( file_sep )
      {
        file_sep ++; *file_sep = '\0';
        baseURI = stringFromAllocedStr ( file_dup );
      }
    else
      {
        free ( file_dup );
      }
    uri = stringFromAllocedStr ( strdup(path.c_str()) );
  }

  void FileReader::setFileLength ()
  {
    struct stat64 fileStat;
    if ( fstat64 ( fd, &fileStat ) == - 1 )
      {
        throwException ( Exception, "Could not stat(2) file '%s', Error %d:%s\n", getCurrentURI().c_str(), errno, strerror(errno) );
      }


    if ( S_ISREG(fileStat.st_mode) || S_ISLNK(fileStat.st_mode) || S_ISBLK(fileStat.st_mode) )
      {
        fileLength = (__ui64) fileStat.st_size;
      }
    else
      {
        throwException ( Exception, "Invalid mode %x for file '%s'\n", fileStat.st_mode, getCurrentURI().c_str() );
      }

    Log_FileReader ( "=> fileLength = 0x%llx\n", fileLength );

    if ( fileLength == 0)
      {
        Warn ( "File '%s' is empty (size null).\n", getCurrentURI().c_str() );
        buffer = NULL;
        close ( fd );
        fd = -1;
      }
  }

  bool FileReader::fill ()
  {
    Log_FileReader ( "Feed (this=%p) : offset=%llu, fileLength=%llu\n", this, offset, fileLength );
    if ( fd == -1 || fileLength == 0 ) return false;

    __ui64 remaining = fileLength - offset;
    if ( ! remaining )
      {
        Log_FileReader ( "Feed : fileLength=%llu, offset=%llu, remaining=%llu, closing.\n",
          fileLength, offset, remaining );
        return false;
      }

    if ( offset )
      {
        AssertBug ( offset % windowSize == 0, "Offset not aligned to window size !\n" );

        int res = madvise ( buffer, windowSize, MADV_DONTNEED );
        if ( res == -1 )
          {
            Warn ( "madvise() failed : error=%d:%s\n", errno, strerror(errno) );
          }

        Assert ( bufferSz == windowSize, "Buffer size smaller than window size ?\n" );
        if ( munmap ( buffer, windowSize ) != 0 )
          {
            Warn ( "Could not unmap() !\n" );
          }
      }
    bufferSz = ( remaining < windowSize ) ? remaining : windowSize;

    buffer = (char*) mmap ( NULL, bufferSz, PROT_READ, MAP_SHARED, fd, offset );
    if ( buffer == MAP_FAILED )
      {
        buffer = NULL;
        throwException ( Exception, "Could not mmap() : offset=%llu, remaining=%llu, fileLength=%llu, error=%d:%s.\n",
            offset, remaining, fileLength,
            errno, strerror(errno) );
      }

    /*
     * \todo Is madvise() very usefull here ?
     */
    int res = madvise ( buffer, bufferSz, MADV_SEQUENTIAL | MADV_WILLNEED );
    if ( res == -1 )
      {
        Warn ( "madvise() failed : error=%d:%s\n", errno, strerror(errno) );
      }

    if ( offset )
      {
        Log_FileReader ( "Parsed %llu MBytes / %llu total Mbytes.\n", offset >> 20ULL, fileLength >> 20ULL );
      }

    offset += bufferSz;

    Log_FileReader ( "fill ok (this=%p) : given length=%llu, offset=%llu, fileLength=%llu, buffer=%p\n",
        this,
        bufferSz, offset, fileLength, buffer );
    bufferIdx = 0;
    return true;
  }
};
