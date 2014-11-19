#include <Xemeiah/xsl/xslprocessor.h>
#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/dom/childiterator.h>
#include <Xemeiah/dom/elementmapref.h>
#include <Xemeiah/dom/integermapref.h>
#include <Xemeiah/xpath/xpath.h>
#include <Xemeiah/xpath/xpathparser.h>
#include <Xemeiah/nodeflow/nodeflow-stream.h>
#include <Xemeiah/nodeflow/nodeflow-textcontents.h>

#include <string.h>
#include <math.h>

#include <map>
#include <list>

#include <Xemeiah/auto-inline.hpp>

#define Log_XSL Debug
#define Log_XSLOpt Debug

#define __XEM_XSL_USE_TEMPLATE_FAST_TABLE
// #define __XEM_XSL_USE_TEMPLATE_FAST_TABLE_COMPARE

#define __XEM_XSL_TEMPLATE_FAST_USE_FIRST

namespace Xem
{
  ElementRef XSLProcessor::chooseTemplate ( __XProcHandlerArgs__ )
  { 
    Number initialPriority = -INFINITY;

    KeyId modeId = 0;

    if ( item.hasAttr ( xsl.mode() ) )
      {
        modeId = item.getAttrAsKeyId ( getXProcessor(), xsl.mode() );
        AssertBug ( modeId, "Invalid null modeId !\n" );
      }

    /*
     * Note : It is asserted that all xsl templates and consors are stored in the same document.
     */

    ElementRef stylesheet = getMainStylesheet ( );
#ifdef __XEM_XSL_USE_TEMPLATE_FAST_TABLE
    ElementRef foundTemplateFast = ( stylesheet.getDocument() );
#endif
	
#ifdef __XEM_XSL_USE_TEMPLATE_FAST_TABLE
    if ( getCurrentNode().isElement() )
      {
        Log_XSL ( "Optim : current='%s'\n", getCurrentNode().generateVersatileXPath().c_str() );
        ElementMultiMapRef modeTemplatesOpt = stylesheet.findAttr
          ( xslimpl.match_templates_opt(), AttributeType_SKMap );
  
        AssertBug ( modeTemplatesOpt, "No opt mode templates ?\n" );

        ElementMultiMapRef importedModeTemplatesOpt = stylesheet.findAttr
          ( xslimpl.imported_match_templates_opt(), AttributeType_SKMap );

        Number initialPriority = -INFINITY;
        foundTemplateFast = chooseTemplateOptimized ( item, modeTemplatesOpt, 
            modeId, initialPriority ); 

        if ( importedModeTemplatesOpt  )
          {
            ElementRef foundTemplateFastImported = chooseTemplateOptimized ( item, importedModeTemplatesOpt, 
              modeId, initialPriority ); 
            if ( ! foundTemplateFast  && foundTemplateFastImported )
              {
              Log_XSLOpt ( "Got from IMPORTED !\n" );
              foundTemplateFast = foundTemplateFastImported;
              }
          }

        Log_XSL ( "found templ : %s\n", foundTemplateFast.generateVersatileXPath().c_str() );        

#ifdef __XEM_XSL_USE_TEMPLATE_FAST_TABLE_COMPARE	

#else
        return foundTemplateFast;		
#endif 
      }
#endif

    ElementRef foundTemplate ( stylesheet.getDocument() );
    ElementRef importedTemplate ( stylesheet.getDocument() );
    ElementRef directTemplate ( stylesheet.getDocument() );

    Log_XSL ( "Oldschool template find for '%s'\n", 
      getCurrentNode().generateVersatileXPath().c_str() );
    ElementMultiMapRef importedModeTemplates = stylesheet.findAttr
      ( xslimpl.imported_match_templates(), AttributeType_SKMap );
	
    if ( importedModeTemplates )
      {
        importedTemplate = chooseTemplate ( item, importedModeTemplates, modeId, initialPriority ); 
      }

    ElementMultiMapRef modeTemplates = 
      stylesheet.findAttr ( xslimpl.match_templates(), AttributeType_SKMap );
	
    if ( modeTemplates )
      {
        directTemplate = chooseTemplate ( item, modeTemplates, modeId, initialPriority ); 
      }

    Log_XSL ( "imported=%s, direct=%s\n",
      importedTemplate.generateVersatileXPath().c_str(),
      directTemplate.generateVersatileXPath().c_str() );
    foundTemplate = directTemplate ? directTemplate : importedTemplate;

#ifdef __XEM_XSL_USE_TEMPLATE_FAST_TABLE_COMPARE	
    Info ( "Found template :\nfast   ='%s'\nregular='%s'\n", 
          foundTemplateFast.generateVersatileXPath().c_str(),
          foundTemplate.generateVersatileXPath().c_str() );
			
    if ( foundTemplate != foundTemplateFast )
      {
        Error ( "Found template : \nfast='%s'\nregular='%s'\n", 
          foundTemplateFast.generateVersatileXPath().c_str(),
          foundTemplate.generateVersatileXPath().c_str() );
        
        Bug ( "." );
      }
    // return foundTemplateFast;
#endif	  
    return foundTemplate;
  }

  ElementRef XSLProcessor::chooseTemplateOptimized ( ElementRef& item, 
      ElementMultiMapRef& optimizedModeTemplates, 
      KeyId modeId, Number& initialPriority )
  {
    NodeRef& currentNode = getCurrentNode ();
    KeyId keyId = currentNode.getKeyId ();
    KeyId wildcardId = __builtin.xemint.element ();

    SKMapHash hash = getNamedTemplateHash ( modeId, keyId );

	Log_XSLOpt ( "-----------------------------------------------------------------------\n" );
    Log_XSLOpt ( "Finding template for %s (using %s)\n",
      currentNode.getKey().c_str(), optimizedModeTemplates.getKey().c_str() );

    ElementMultiMapRef::multi_iterator iter ( optimizedModeTemplates, hash );
    
    if ( ! iter )
      {
        /*
         * We could not find direct iterator, so try using namespace wildcardId
         */
        hash = getNamedTemplateHash ( modeId, KeyCache::getKeyId(currentNode.getNamespaceId(), 0 ) );
        iter.findHash ( hash );
      }
    if ( ! iter && ( keyId != __builtin.xemint.textnode() ) && ( keyId != __builtin.xemint.comment() ) )
      {
        /*
         * We could not find namespace iterator, so try using general wildcardId
         */
        hash = getNamedTemplateHash ( modeId, wildcardId );
        iter.findHash ( hash );
      }
    if ( ! iter )
      {
        /**
         * We could not find wildcard, try using node()
         */
        hash = getNamedTemplateHash ( modeId, 0 );
        iter.findHash ( hash );
      }
    if ( ! iter )
      {
        Log_XSLOpt ( "Could not find any template on %s\n", optimizedModeTemplates.getKey().c_str() );
      }
    
    ElementRef foundTempl ( item.getDocument() );
	
    for ( ; iter ; iter ++ )
      {
        ElementRef templ = optimizedModeTemplates.get ( iter );
        XPath matchXPath ( getXProcessor(), templ, xsl.match() );
  
        Log_XSLOpt ( "At node=%s, match=%s, templ=%s\n",
          currentNode.getKey().c_str(), matchXPath.getExpression(),
          templ.generateVersatileXPath().c_str() );

        Number effectivePriority = 0;
        if ( ! matchXPath.matches ( getCurrentNode(), effectivePriority ) )
          {
            Log_XSLOpt ( "Template does not match !\n" );
            continue;
          }
        if ( templ.hasAttr ( xsl.priority() ) )
          {
            effectivePriority = templ.getAttrAsNumber ( xsl.priority() );
          }
        if ( effectivePriority > initialPriority )
          {
            Log_XSLOpt ( "Found for %s, template %s, initial=%f, effective=%f\n",
              currentNode.getKey().c_str(), matchXPath.getExpression(), 
              initialPriority, effectivePriority );
            initialPriority = effectivePriority;
			foundTempl = templ;
#ifdef __XEM_XSL_TEMPLATE_FAST_USE_FIRST			
            return templ;
#endif			
          }
        else
          {
            Log_XSLOpt ( "Found, but priority is smaller. Exiting.\n" );
#ifdef __XEM_XSL_TEMPLATE_FAST_USE_FIRST			
            break;
#endif
          }
      }    
    // Log_XSLOpt ( "Could not find any template for %s !\n",
    //    currentNode.getKey().c_str() );
	Log_XSLOpt ( "Quitting with match='%s'\n", foundTempl ? foundTempl.generateVersatileXPath().c_str() : "(none)" );
    return foundTempl; 
  }


  ElementRef XSLProcessor::chooseTemplate ( ElementRef& item, 
      ElementMultiMapRef& modeTemplates, KeyId modeId, Number& initialPriority )
  {
    AssertBug ( getCurrentNode(), "Called chooseTemplate on a null currentNode !\n" );

    Number currentPriority = initialPriority;
    ElementRef foundTemplate ( modeTemplates.getDocument() );
    ElementRef importedTemplate ( modeTemplates.getDocument() );

    Log_XSL ( "Getting template mode=%x\n\tcurrentNode=%s\n\titem=%s\n", 
        modeId, getCurrentNode().generateVersatileXPath().c_str(), 
        item.generateVersatileXPath().c_str() );
        

    if ( ! modeTemplates )
      {
        Log_XSL ( "Templates map not found !\n" );
        return foundTemplate;
      }

    __ui64 totalTemplates = 0, totalTemplatesMatched = 0;
    
    for ( ElementMultiMapRef::multi_iterator iter ( modeTemplates, modeId ) ; iter ; iter ++ )
      {
        ElementRef templ = modeTemplates.get ( iter );
        totalTemplates++;
#if PARANOID        
        if ( ! templ.hasAttr ( xsl.match() ) )
          {
            Bug ( "xsl:template has no match attribute !\n" );
          }
#endif          
        Log_XSL ( "Template [%s] (0x%llx) matches '%s' mode=%x, current prio=%g\n", 
            templ.generateVersatileXPath().c_str(), templ.getElementId(),
            templ.getAttr ( xsl.match() ).c_str(),
            templ.hasAttr(xsl.mode()) ? templ.getAttrAsKeyId ( xsl.mode() ) : 0,
            currentPriority );
#if PARANOID            
        if ( modeId && ( ! templ.hasAttr(xsl.mode()) || templ.getAttrAsKeyId ( xsl.mode() ) != modeId ) )
          {
            Bug ( "Invalid modeId !\n" );
          }
#endif          
        Number templatePriority = -INFINITY;
        bool hasTemplatePriority = false;
        if ( templ.hasAttr ( xsl.priority() ) )
          {
            hasTemplatePriority = true;
            templatePriority = templ.getAttrAsNumber ( xsl.priority() );
            Log_XSL ( "\tPriority : %g\n", templatePriority );
            if ( templatePriority < currentPriority )
              {
                Log_XSL ( "template pri=%g is less than current=%g, skipping this template.\n",
                  templatePriority, currentPriority );
                continue;
              }
          }
        XPath templXPath ( getXProcessor(), templ, xsl.match() );
        Number effectivePriority = 0;
        if ( ! templXPath.matches ( getCurrentNode(), effectivePriority ) )
          {
            Log_XSL ( "Template does not match !\n" );
            continue;
          }
        if ( hasTemplatePriority )
          effectivePriority = templatePriority;
          
        totalTemplatesMatched ++;
        Log_XSL ( "Template matches, effective=%g, current=%g\n", effectivePriority, currentPriority );
        if ( effectivePriority == currentPriority )
          {
            if ( getXProcessor().hasVariable ( xslimpl.warn_on_conflicts() ) )
              {
                Warn ( "Conflict between templates :\n\tNode '%s' : \n\ttemplate '%s'\n\tand '%s'\n\tboth match and have same priority %g\n",
                  getCurrentNode().generateVersatileXPath().c_str(), 
                  templ.generateVersatileXPath().c_str(),
                  foundTemplate.generateVersatileXPath().c_str(),
                  currentPriority );
              }              
            currentPriority = effectivePriority;
            foundTemplate = templ;
            // continue;
          }          
        else if ( effectivePriority > currentPriority )
          {
            currentPriority = effectivePriority;
            foundTemplate = templ;
          }
      }

    Log_XSL ( "!!! Checked %llu templates, %llu of them matched\n", totalTemplates, totalTemplatesMatched );
    if ( ! foundTemplate )
      {
        Log_XSL ( "------- Could not find any template ! -------\n" );
        return foundTemplate;
      }

    AssertBug ( foundTemplate.getKeyId() == xsl.template_(),
        "Invalid template %x:'%s'\n", foundTemplate.getKeyId(), foundTemplate.getKey().c_str() );
    Log_XSL ( "------------------------------ FINALLY : --------------------\n" );
    Log_XSL ( "Chosen template is %x : '%s'\n", foundTemplate.getKeyId(), foundTemplate.generateVersatileXPath().c_str() );
    return foundTemplate;
  }

  /**
   * evaluate the default template for a node.
   * This is to be done *only* when no template has been found for this node.
   */
  void XSLProcessor::defaultTemplate ( __XProcHandlerArgs__ )
  {
    String text;
    NodeRef& currentNode = getCurrentNode();
    if ( currentNode.isAttribute() )
      {
        text = currentNode.toAttribute().toString();    
      }
    else if ( currentNode.isElement() && currentNode.toElement().isText() )
      {
        text = currentNode.toElement().getText();
      }
    else
      {
        Log_XSL ( "DEFAULT TEMPLATE : Calling recursive on '%s'\n",
            currentNode.getKey().c_str() );
        NodeSet myChildren;
        for ( ChildIterator child(currentNode.toElement()) ; child ; child++ )
          {
            if ( child.mustSkipWhitespace ( getXProcessor() ) ) continue;
            myChildren.pushBack ( child );
          }
        for ( NodeSet::iterator iter( myChildren, getXProcessor() ) ; iter ; iter++ )
          {
            Log_XSL ( "Calling recursive on child=0x%llx:'%s'\n", 
                iter->toElement().getElementId(), iter->toNode().getKey().c_str() );
            ElementRef templ = chooseTemplate ( item );
            if ( templ )
              {
                Log_XSL ( "Calling recursive on child='%s', templ=0x%llx\n", 
                    iter->toNode().getKey().c_str(), templ.getElementId() );
                processTemplateArguments ( templ );
                getXProcessor().process ( templ );
              }
            else
              {
                defaultTemplate ( item );
              }
          }
        return;
      }
    getXProcessor().getNodeFlow().appendText ( text.c_str(), false );
  }
  
};
