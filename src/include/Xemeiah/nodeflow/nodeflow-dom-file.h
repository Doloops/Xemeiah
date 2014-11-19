#ifndef __XEM_NODEFLOW_DOM_FILE_H
#define __XEM_NODEFLOW_DOM_FILE_H

#include <Xemeiah/nodeflow/nodeflow-dom.h>
#include <Xemeiah/io/filewriter.h>

#include <stdio.h>

namespace Xem
{
  class Store;
  
  /**
   * NodeFlow to a file, using a temporary DOM before serializing
   */
  class NodeFlowDomFile : public NodeFlowDom
  {
  protected:
    /**
     * The document we had to create before writing to a file
     */
    Document* instanciatedDocument;

    /**
     * Our file writer
     */
    FileWriter writer;

    /**
     * Is mkdir enabled
     */
    bool mkdirEnabled;

    /**
     * Serialize contents
     */
    void serialize ();  

  public:
    /**
     * Single constructor
     */
    NodeFlowDomFile ( XProcessor& xproc );

    /**
     * Single destructor
     */
    ~NodeFlowDomFile ();

    /**
     * Set file destricptor
     */
    void setFD ( int _fd ) { writer.setFD(_fd); }
    void setFD ( FILE* fp ) { setFD ( fileno(fp) ); }
  
    /**
     * disable making directories
     */
    void disableMkdir () { mkdirEnabled = false; }

    /**
     * Set an output target
     * @param file the File target.
     */
    void setFile ( const String& filePath ) { writer.setFile(filePath, mkdirEnabled); }
  };


};
#endif //  __XEM_NODEFLOW_DOM_FILE_H

