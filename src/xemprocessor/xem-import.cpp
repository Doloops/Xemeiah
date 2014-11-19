#include <Xemeiah/xemprocessor/xemprocessor.h>
#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/parser/parser.h>

#include <Xemeiah/auto-inline.hpp>

#define throwXemException(...) throwException ( Exception, __VA_ARGS__ )

#define Log_XemImport Debug

namespace Xem
{
  /*
   * xem Extensions : importing...
   */

  void XemProcessor::xemInstructionImportDocument ( __XProcHandlerArgs__ )
  {
    String href = item.getEvaledAttr ( getXProcessor(), xem.href() );
    String keepTextMode = "xsl";
    if ( item.hasAttr ( xem.keep_text_mode() ) )
      {
        keepTextMode =  item.getEvaledAttr ( getXProcessor(), xem.keep_text_mode() );
      }
    ElementRef resultElement = getNodeFlow().getCurrentElement ();
    try
      {
      	Parser::parseFile ( getXProcessor(), resultElement, href, keepTextMode );
      }
    catch ( Exception * e)
      {
        detailException ( e, "xem:import-document :  could not import '%s' \n", href.c_str() );
        throw ( e );
      }
    ElementRef importedElement = resultElement.getLastChild();

    for ( AttributeRef attr = item.getFirstAttr() ; attr ; attr = attr.getNext() )
      {
        if ( attr.getNamespaceId() && attr.getNamespaceId() != item.getNamespaceId()
            && attr.isBaseType() && attr.getNamespaceId() != getKeyCache().getBuiltinKeys().xmlns.ns() )
          {
            // item.ensureNamespaceDeclaration(nodeFl, attr.getNamespaceId(), false );
            if ( !importedElement.getNamespacePrefix(attr.getNamespaceId(), false ) )
              {
                importedElement.addNamespacePrefix(item.getNamespacePrefix(attr.getNamespaceId()), attr.getNamespaceId() );
              }
            importedElement.addAttr(getXProcessor(), attr.getKeyId(), attr.toString());
          }
      }
  }

  void XemProcessor::xemInstructionImportFolder ( __XProcHandlerArgs__ )
  {
    throwXemException ( "xem:import-folder is not implemented !\n" );
#if 0
    ElementRef elt = getXProcessor().getNodeFlow().getCurrentElement ();
    
    String href = item.getEvaledAttr ( getXProcessor(), xem.href() );
    if ( ! importDir ( elt, href.c_str() ) )
      {
      	throwXemException ( "xem:import-folder :  could not import '%s'!\n", href.c_str() );
      }
#endif
  }

  void XemProcessor::xemInstructionImportZip ( __XProcHandlerArgs__ )
  {
    throwXemException ( "xem:import-zip is not implemented !\n" );
#if 0
    if ( item.getChild() )
      {
        throwXemException ( "xem:import-zip must not have a child !\n" );
      }
    ElementRef elt = getXProcessor().getNodeFlow().getCurrentElement();
    String href = item.getEvaledAttr ( getXProcessor(), xem.href() );
    /**
     * \todo : Implement cleany a excludedFiles pattern !
     */
    std::list<String*> excludedFiles;
    if ( item.hasAttr ( xem.exclude() ) )
      {
        String * excluded = new String ( item.getAttr ( xem.exclude() ) );
        excludedFiles.push_back ( excluded );
        Log_XemImport ( "Excluded file : '%s'\n", excluded->c_str() );
      }
    if ( ! importZip ( elt, href.c_str(), excludedFiles ) )
      {
        throwXemException ( "xem:import-folder :  could not import '%s'!\n", href.c_str() );
      }
    elt.getDocument().housewife ();
#endif
  }
};

