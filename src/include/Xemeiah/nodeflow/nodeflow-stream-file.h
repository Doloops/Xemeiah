#ifndef __XEM_NODEFLOW_FILE_H
#define __XEM_NODEFLOW_FILE_H

#include <Xemeiah/nodeflow/nodeflow-stream.h>

namespace Xem
{
  /**
   * NodeFlowStreamFile : NodeFlowStream with a FileWriter output
   */
  class NodeFlowStreamFile : public NodeFlowStream
  {
  public:
    /**
     * Alternate Constructor
     */
    NodeFlowStreamFile ( XProcessor& xproc );
    
    /**
     * Destructor
     */
    virtual ~NodeFlowStreamFile();
     
    /**
     * Set an output target
     * @param fd the file descriptor to write to.
     */
    void setFD ( int fd );
    
    /**
     * Set an output target
     * @param fp the FILE* to write to.
     */
    void setFD ( FILE* fp );
    
    /**
     * Set an output target
     * @param file the File target.
     */
    void setFile ( const String& filePath );
  };
};

#endif // __XEM_NODEFLOW_FILE_H

