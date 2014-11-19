/*
 * \file This file defines a simple compile-time frontend 'NodeFlowFile'
 * which allows to choose between different implementations of
 * XML serialization to a file.
 */
#ifndef __XEM_NODEFLOW_FILE_H
#define __XEM_NODEFLOW_FILE_H

#define __XEM_NODEFLOWFILE_USE_NODEFLOWDOMFILE    1
#define __XEM_NODEFLOWFILE_USE_NODEFLOWSTREAMFILE 2
#define __XEM_NODEFLOWFILE_USE_NODEFLOWNULL       3

#define __XEM_NODEFLOWFILE_USE __XEM_NODEFLOWFILE_USE_NODEFLOWDOMFILE

#if __XEM_NODEFLOWFILE_USE == __XEM_NODEFLOWFILE_USE_NODEFLOWDOMFILE
#include <Xemeiah/nodeflow/nodeflow-dom-file.h>
namespace Xem
{
  typedef NodeFlowDomFile NodeFlowFile;
}
#elif __XEM_NODEFLOWFILE_USE == __XEM_NODEFLOWFILE_USE_NODEFLOWSTREAMFILE
#include <Xemeiah/nodeflow/nodeflow-stream-file.h>
namespace Xem
{
  typedef NodeFlowStreamFile NodeFlowFile;
}
#elif __XEM_NODEFLOWFILE_USE == __XEM_NODEFLOWFILE_USE_NODEFLOWNULL
#include <Xemeiah/nodeflow/nodeflow-null.h>
namespace Xem
{
  typedef NodeFlowNull NodeFlowFile;
}
#else

#endif

#endif // __XEM_NODEFLOW_FILE_H
