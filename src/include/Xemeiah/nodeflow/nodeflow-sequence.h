#ifndef __XEM_NODEFLOW_SEQUENCE_H
#define __XEM_NODEFLOW_SEQUENCE_H

#include <Xemeiah/nodeflow/nodeflow-dom.h>
#include <Xemeiah/dom/nodeset.h>

namespace Xem
{
  class NodeFlowSequence : public NodeFlowDom
  {
  protected:
    NodeSet& nodeSet;
    
  public:
    NodeFlowSequence ( XProcessor& xproc, const ElementRef& elementRef, NodeSet& nodeSet );
    ~NodeFlowSequence ();

    virtual void newElement ( KeyId keyId, bool forceDefaultNamespace );

    virtual void appendText ( const String& text, bool disableOutputEscaping  );

    virtual void processSequence ( XPath& xpath );
  };
};


#endif //  __XEM_NODEFLOW_SEQUENCE_H
