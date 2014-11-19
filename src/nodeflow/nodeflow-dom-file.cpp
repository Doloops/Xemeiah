#include <Xemeiah/nodeflow/nodeflow-dom-file.h>
#include <Xemeiah/kern/store.h>
#include <Xemeiah/kern/volatiledocument.h>
#include <Xemeiah/xprocessor/xprocessor.h>

#include <Xemeiah/dom/elementref.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_NFDF Debug

namespace Xem
{

  NodeFlowDomFile::NodeFlowDomFile ( XProcessor& xproc )
  : NodeFlowDom ( xproc, xproc.createVolatileDocument(false) )
  {
    instanciatedDocument = &(baseElement.getDocument());
    instanciatedDocument->incrementRefCount();
    Log_NFDF ( "Instanciated Document at %p\n", instanciatedDocument );
    forbidAttributeCreationOutsideOfElements = true;
    mkdirEnabled = true;
  }
  
  NodeFlowDomFile::~NodeFlowDomFile ()
  {
    serialize ();
    if ( instanciatedDocument ) 
      {
        Log_NFDF ( "Deleting instanciated Document at %p\n", instanciatedDocument );
        Store& store = instanciatedDocument->getStore();
        store.releaseDocument(instanciatedDocument);
      }
  }

  void NodeFlowDomFile::serialize ()
  {
    if ( encoding == "" )
      {
        encoding = "UTF-8";
      }
    Log_NFDF ( "[NFDF] : Serialize to encoding='%s'\n", encoding.c_str() );

    currentElement.toXML ( writer,
          ElementRef::Flag_ChildrenOnly
        | ElementRef::Flag_SortAttributesAndNamespaces
        | ( omitXMLDeclaration ? 0 : ElementRef::Flag_XMLHeader )
        | ( indent ? 0 : ElementRef::Flag_NoIndent )
        | ( standalone ? ElementRef::Flag_Standalone : 0 )
        | ( ( outputMethod == OutputMethod_HTML ) ? ElementRef::Flag_Output_HTML : 0 )
        | ( ( outputMethod == OutputMethod_Text ) ? ElementRef::Flag_UnprotectText : 0 )
          , encoding
          );
  }
};
