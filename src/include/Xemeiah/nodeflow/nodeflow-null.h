#ifndef __XEM_NODEFLOW_NULL_H
#define __XEM_NODEFLOW_NULL_H

#include <Xemeiah/nodeflow/nodeflow.h>
#include <Xemeiah/dom/elementref.h>

namespace Xem
{
  class NodeFlowNull : public NodeFlow
  {
  public:
    NodeFlowNull(XProcessor& xproc) : NodeFlow(xproc) {}
    ~NodeFlowNull() {}
  
    void setNamespacePrefix ( KeyId keyId, NamespaceId nsId, bool anticipated ) {}
  
    LocalKeyId getNamespacePrefix ( NamespaceId namespaceId ) { return 0; }
    
    NamespaceId getNamespaceIdFromPrefix ( LocalKeyId prefix, bool recursive ) { return 0; }
    
    NamespaceId getDefaultNamespaceId () { return 0; }
    
    void newElement ( KeyId keyId, bool forceDefaultNamespace ) {}
    
    void newAttribute ( KeyId keyId, const String& value ) {}
    
    void appendText ( const String& text, bool disableOutputEscaping ) {}
    
    void elementEnd ( KeyId keyId ) {}
    
    void newComment ( const String& comment ) {}
    
    void newPI ( const String& name, const String& contents ) {}
    
    ElementRef getCurrentElement() { return *((ElementRef*)NULL); }
    
    void setFile ( const String& filePath ) {}

    void setFD ( int fd ) {}

    void setFD ( FILE* fp ) {}

  };

};

#endif //  __XEM_NODEFLOW_NULL_H
