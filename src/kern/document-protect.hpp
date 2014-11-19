#include <Xemeiah/kern/document.h>
#include <Xemeiah/dom/noderef.h>
// #include <Xemeiah/kern/env.h>
#include <Xemeiah/trace.h>

#include <sys/mman.h>
#include <errno.h>

/**
 * The VERY verbous log
 */
#define Log_MemProtect Debug
#define Log_PROT Debug

namespace Xem
{
  __INLINE bool Document::isWritable ()
  {
    return getDocumentAllocator().isWritable();
  }

#ifdef XEM_MEM_PROTECT_SYS

  __INLINE void Document::alterDocumentHead ()
  {
    DocumentHead* documentHead = &(getDocumentHead());
    getDocumentAllocator().alter ( documentHead );
  }
  
  __INLINE void Document::protectDocumentHead () 
  {
    DocumentHead* documentHead = &(getDocumentHead());
    getDocumentAllocator().protect ( documentHead );
  }
#endif
 
};
