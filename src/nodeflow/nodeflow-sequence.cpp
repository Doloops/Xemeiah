#include <Xemeiah/nodeflow/nodeflow-sequence.h>
#include <Xemeiah/xpath/xpath.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_Seq Debug

namespace Xem
{

  NodeFlowSequence::NodeFlowSequence ( XProcessor& xproc, const ElementRef& _elementRef, NodeSet& _nodeSet )
  : NodeFlowDom(xproc, _elementRef), nodeSet(_nodeSet)
  {
    
  }
  
  NodeFlowSequence::~NodeFlowSequence ()
  {
  
  }

  void NodeFlowSequence::newElement ( KeyId keyId, bool forceDefaultNamespace )
  {
    bool maySequenceThis = (baseElement == currentElement);
    NodeFlowDom::newElement ( keyId, forceDefaultNamespace );

    Log_Seq ( "New element '%s', maySequence=%d\n", getCurrentElement().generateVersatileXPath().c_str(), maySequenceThis );

    if ( maySequenceThis )
      {
        ElementRef node = getCurrentElement();
        nodeSet.pushBack ( node );
      }
  }
  

  void NodeFlowSequence::appendText ( const String& text, bool disableOutputEscaping  )
  {
    bool maySequenceThis = (baseElement == currentElement);
    Log_Seq ( "New text '%s', maySequence=%d\n", text.c_str(), maySequenceThis );
    if ( maySequenceThis )
      {
        // Append
        if ( nodeSet.isScalar() )
          {
            String current = nodeSet.toString ();
            nodeSet.clear ();
            current += text;
            nodeSet.setSingleton ( current );
            
            Log_Seq ( "--> Concatenated : '%s'\n", nodeSet.toString().c_str() );
          }
        else
          {
            nodeSet.setSingleton ( text );
          }
      }    
    else
      {
        NodeFlowDom::appendText ( text, disableOutputEscaping );
      }
  }


  void NodeFlowSequence::processSequence ( XPath& xpath )
  {
    bool maySequenceThis = (baseElement == currentElement);

    if ( maySequenceThis )
      {
        Log_Seq ( "--> Processing xpath eval for sequence appending !\n" );
        xpath.eval ( nodeSet );
      }
    else
      {
        /*
         * Not pretty sure if we have to fallback to NodeFlow implementation here...
         */
        NodeFlow::processSequence ( xpath );      
      }  
  }
};

