#include <Xemeiah/nodeflow/nodeflow-stream-file.h>
#include <Xemeiah/io/filewriter.h>
#include <Xemeiah/dom/elementref.h>

#include <Xemeiah/auto-inline.hpp>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

namespace Xem
{
  NodeFlowStreamFile::NodeFlowStreamFile ( XProcessor& xproc )
  : NodeFlowStream ( xproc, new FileWriter() )
  {
  }

  NodeFlowStreamFile::~NodeFlowStreamFile ()
  {
    writer->flushBuffer();
  }

  void NodeFlowStreamFile::setFD ( int fd_ )
  {
    FileWriter* fileWriter = dynamic_cast<FileWriter*> ( writer );
    fileWriter->setFD(fd_);
  }

  void NodeFlowStreamFile::setFD ( FILE* fp )
  {
    setFD ( fileno(fp) );
  }

  void NodeFlowStreamFile::setFile ( const String& filepath )
  {
    FileWriter* fileWriter = dynamic_cast<FileWriter*> ( writer );
    fileWriter->setFile(filepath);
  }

};
