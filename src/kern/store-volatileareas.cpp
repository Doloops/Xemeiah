/*
 * store-volatileareas.cpp
 *
 *  Created on: 27 janv. 2010
 *      Author: francois
 */

#include <Xemeiah/kern/store.h>
#include <Xemeiah/kern/branchmanager.h>
#include <Xemeiah/kern/keycache.h>
#include <Xemeiah/kern/document.h>
#include <Xemeiah/trace.h>

#include <Xemeiah/auto-inline.hpp>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#define Log_VolatileArea Debug

namespace Xem
{
  void*
  Store::getVolatileArea()
  {
    Lock lock ( volatileAreasMutex  );
    if (!volatileAreas.empty())
      {
        void* area = volatileAreas.front();
        volatileAreas.pop_front();
        Log_VolatileArea ( "Providing area=%p from volatileAreas cache, volatileAreas size=%lu.\n",
            area, (unsigned long) volatileAreas.size() );
        stats.numberOfVolatileAreasProvided++;
        return area;
      }
#ifdef __XEM_STORE_VOLATILEAREAS_USE_MMAP
    if (volatileAreasFD == -1)
      {
        static const char* tempFile = "/dev/zero";
        volatileAreasFD = open(tempFile, O_RDWR);
        if (volatileAreasFD == -1)
          {
            Bug ( "Could not open volatileAreasFD ! error=%d:%s\n", errno, strerror(errno) );
          }
      }
    void* area = mmap(NULL, DocumentAllocator::getAreaSize(),
#ifdef XEM_MEM_PROTECT_SYS
        PROT_READ
#else
        PROT_READ | PROT_WRITE
#endif
        , MAP_PRIVATE, volatileAreasFD, 0);
    if (area == MAP_FAILED)
      {
        Bug ( "Could not map VolatileArea. error = %d:%s\n", errno, strerror(errno) );
      }
#else
    void* area = malloc ( DocumentAllocator::getAreaSize() );
#endif

#if 0 // INJECTING RANDOM STUFF HERE
    int randomFd = open ( "/dev/urandom", O_RDONLY );
    read ( randomFd, area, DocumentAllocator::getAreaSize() );
    ::close ( randomFd );
#endif
    stats.numberOfVolatileAreasCreated++;
    Log_VolatileArea ( "Providing area=%p from new map, stats.numberOfVolatileAreasCreated=%llu.\n",
        area, stats.numberOfVolatileAreasCreated );
    return area;
  }

  void
  Store::releaseVolatileArea(void* area)
  {
    size_t volatileAreasBufferSize = 20;
    Lock lock ( volatileAreasMutex );
    if (volatileAreas.size() < volatileAreasBufferSize)
      {
        volatileAreas.push_back(area);
        Log_VolatileArea ( "Releasing area=%p to volatileAreas cache.\n", area );
      }
    else
      {
        Log_VolatileArea ( "Unmapping area=%p.\n", area );
#ifdef __XEM_STORE_VOLATILEAREAS_USE_MMAP
        munmap(area, DocumentAllocator::getAreaSize());
#else
        free ( area );
#endif
        stats.numberOfVolatileAreasDeleted++;
      }
  }
};
