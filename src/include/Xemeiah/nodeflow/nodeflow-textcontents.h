#ifndef __XEM_NODEFLOW_TEXTCONTENTS_H
#define __XEM_NODEFLOW_TEXTCONTENTS_H

#include <Xemeiah/nodeflow/nodeflow-stream.h>

namespace Xem
{
  /**
   * NodeFlowTextContents is a restriction of NodeFlowStream where all non-text items are silently ignored.
   * The only usefull function, appendText(), is implemented in NodeFlowStream.
   */
  class NodeFlowTextContents : public NodeFlowStream
  {
  protected:
  
  public:
    NodeFlowTextContents ( XProcessor& xproc )
    : NodeFlowStream ( xproc ) 
    { 
      outputMethod = OutputMethod_Text;
    }
    ~NodeFlowTextContents () {}
  
    void newElement ( KeyId keyId, bool forceDefaultNamespace ) {}
    void newAttribute ( KeyId keyId, const String& value ) {}
    void elementEnd ( KeyId keyId ) {}
    
    void newComment ( const String& comment ) {}
    
    void newPI ( const String& name, const String& contents ) {}
  };
};

#endif // __XEM_NODEFLOW_TEXTCONTENTS_H

