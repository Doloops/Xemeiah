#include <Xemeiah/xsl/xslprocessor.h>
#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/dom/childiterator.h>
#include <Xemeiah/dom/elementmapref.h>
#include <Xemeiah/dom/integermapref.h>
#include <Xemeiah/xpath/xpath.h>
#include <Xemeiah/xpath/xpathparser.h>
#include <Xemeiah/nodeflow/nodeflow-stream.h>
#include <Xemeiah/nodeflow/nodeflow-textcontents.h>

#include <Xemeiah/parser/parser.h>

#include <math.h> //< for INFINTY
#include <map>
#include <list>

#include <Xemeiah/auto-inline.hpp>

#define Log_XSL Debug
#define Log_XSLOpt Debug

namespace Xem
{
  void
  XSLProcessor::prepareStylesheet(ElementRef& stylesheet)
  {
    Log_XSL ( "prepareStylesheet %s, ...\n", stylesheet.generateVersatileXPath().c_str() );
    NodeSet stripSpaces;

    prepareStylesheet(stylesheet, stylesheet, stripSpaces, false);

    processStylesheetStripSpaces(stylesheet, stripSpaces);
    Log_XSL ( "prepareStylesheet %s, OK\n", stylesheet.generateVersatileXPath().c_str() );

    /*
     * We have to force 'xsl' out of
     */
    LocalKeyId prefixId = stylesheet.getNamespacePrefix(xsl.ns(), true);
    const char* prefix = getKeyCache().getLocalKey(prefixId);

    AttributeRef excluded = stylesheet.findAttr(xsl.exclude_result_prefixes(), AttributeType_String);
    if ( excluded )
      {
        String excludedStr = excluded.toString();
        excludedStr += " ";
        excludedStr += prefix;
        stylesheet.addAttr(getXProcessor(), xsl.exclude_result_prefixes(), excludedStr);
      }
    else
      stylesheet.addAttr(getXProcessor(), xsl.exclude_result_prefixes(), prefix);
  }

  void
  XSLProcessor::processStylesheetStripSpaces(ElementRef& stylesheet,
      NodeSet& stripSpaces)
  {
    if (stripSpaces.size())
      {
        IntegerMapRef stripMap = stylesheet.findAttr(
            xslimpl.strip_spaces_map(), AttributeType_SKMap);
        if (!stripMap)
          {
            stripMap = stylesheet.addSKMap(
                xslimpl.strip_spaces_map(), SKMapType_IntegerMap);
          }
        for (NodeSet::iterator nIter(stripSpaces); nIter; nIter++)
          {
            ElementRef& stripDecl = nIter->toElement();
            std::list<String> list;

            String elements = stripDecl.getAttr(xsl.elements());
            Log_XSL ( "Elements : '%s'\n", elements.c_str() );

            // tokensToTokenList ( list, elements );
            elements.tokenize(list);

            bool isStrip =
                (stripDecl.getKeyId() == xsl.strip_space());
            if (!isStrip)
              AssertBug (stripDecl.getKeyId() == xsl.preserve_space(),
                  "Internal error : invalid element '%s'\n", nIter->toElement().getKey().c_str() );
            for (std::list<String>::iterator sIter = list.begin(); sIter
                != list.end(); sIter++)
              {
                const char* qname = (*sIter).c_str();
                KeyId keyId = 0;
                if (strstr(qname, ":*"))
                  {
                    char* ns = strdup(qname);
                    char* delim = strchr(ns, '*');
                    delim[0] = 'k';
                    keyId = getKeyCache().getKeyIdWithElement(stripDecl, ns);
                    keyId = KeyCache::getKeyId(KeyCache::getNamespaceId(keyId),
                        0);
                    delete (ns);
                  }
                else if (strcmp(qname, "*") == 0)
                  {
                    keyId = __builtin.xemint.element();
                  }
                else if (strchr(qname, '*'))
                  {
                    NotImplemented ( "qname has a wildcard.\n" );
                  }
                else
                  {
                    keyId = getKeyCache().getKeyIdWithElement ( stripDecl, qname );
                  }
                Log_XSL ( "[STRIP] Pushing %x:'%s' to %s list\n", keyId, sIter->c_str(), isStrip ? "strip" : "preserve" );

                stripMap.put(keyId, isStrip ? 2 : 1);
              }
          }
      }
  }

  String
  XSLProcessor::resolveStylesheetURI(ElementRef& importItem,
      ElementRef& mainStylesheet)
  {
    String url = importItem.getAttr(xsl.href());

    if (!url.size())
      {
        throwXSLException ( "Invalid empty href for xsl:import/xsl:include : at %s\n",
            importItem.generateVersatileXPath().c_str() );
      }

    if (strncmp(url.c_str(), "file:", 5) == 0)
      {
        url = &(url.c_str()[5]);
      }

    Log_XSL ( "Relative URI for imp/incl stylesheet is '%s'\n", url.c_str() );
    /*
     * First, compute the current relative path
     */
    ElementRef currentStylesheet = importItem.getFather();
    AssertBug ( currentStylesheet.getKeyId() == xsl.stylesheet(),
        "Wrong father for import/include element : '%s'\n", currentStylesheet.getKey().c_str() );
    String currentBaseURI = currentStylesheet.getFather().getAttr(
        __builtin.xemint.document_base_uri());

    Log_XSL ( "Current relative baseURI is '%s'\n", currentBaseURI.c_str() );

    String completeURI = currentBaseURI + url;
    return completeURI;
  }

  void
  XSLProcessor::checkStylesheetImportCycle(ElementRef& importItem,
      const String& completeURI)
  {
    for (ElementRef hierarchy = importItem.getFather(); hierarchy; hierarchy
        = hierarchy.getFather())
      {
        if (hierarchy.hasAttr(__builtin.xemint.document_uri()))
          {
            String hierarchyURI = hierarchy.getAttr(
                __builtin.xemint.document_uri());
            if (hierarchyURI == completeURI)
              {
                throwXSLException ( "Cycles while importing : importing '%s'\n", completeURI.c_str() );
              }
          }
      }
  }

  ElementRef
  XSLProcessor::importStylesheet(ElementRef& importItem,
      ElementRef& mainStylesheet)
  {
    Log_XSL ( "importStylesheet, importItem = %s\n", importItem.generateVersatileXPath().c_str() );

    /*
     * We have a strong pre-requisite for using ElementMapRef on templates and top-level params :
     * All imported and included stylesheets must be part of the mainStylesheet document.
     */

    /*
     * First, fetch the complete URI for this stylesheet
     */
    String completeURI = resolveStylesheetURI(importItem, mainStylesheet);

    Log_XSL ( "completeURI = '%s'\n", completeURI.c_str() );

    ElementRef rootElement(importItem.getDocument());

    if (importItem.getChild())
      {
        Log_XSL ( "Import item already has a child for '%s'\n", completeURI.c_str() );
        rootElement = importItem.getChild();
        String existingURI = rootElement.getAttr(
            __builtin.xemint.document_uri());
        if (existingURI != completeURI)
          {
            Bug ( "Internal bug : was searching for '%s', but imported stylesheet has recorded '%s'\n",
                completeURI.c_str(), existingURI.c_str() );
          }
      }
    else
      {
        checkStylesheetImportCycle(importItem, completeURI);
        /*
         * Ok, now create a rootElement as son of the importItem we are on.
         */
        rootElement = importItem.getDocument().createElement(importItem,
            __builtin.xemint.root());
        importItem.appendLastChild(rootElement);

        rootElement.addAttr(__builtin.xemint.document_uri(), completeURI);
        if (strchr(completeURI.c_str(), '/'))
          {
            char* baseURI = strdup(completeURI.c_str());
            char* separator = strrchr(baseURI, '/');
            separator[1] = '\0';
            rootElement.addAttr(__builtin.xemint.document_base_uri(), baseURI);
            free(baseURI);
          }
        else
          rootElement.addAttr(__builtin.xemint.document_base_uri(), "");

        String keepTextMode = "xsl";

        try
          {
            Parser::parseFile (getXProcessor(), rootElement, completeURI, keepTextMode);
          }
        catch (Exception* e)
          {
            detailException ( e, "While importing file : '%s'\n", completeURI.c_str() );
            throw(e);
          }

      }
    ElementRef stylesheet(rootElement.getDocument());

    Log_XSL ( "rootElement = '%s'\n", rootElement.generateVersatileXPath().c_str() );

    for (ChildIterator child(rootElement); child; child++)
      {
        if (child.getKeyId() == xsl.stylesheet())
          {
            stylesheet = child;
          }
      }

    if (!stylesheet)
      {
        throwXSLException ( "Included / imported stylesheet '%s' has no xsl:stylesheet declared !\n", completeURI.c_str() );
      }
    return stylesheet;
  }

  void
  XSLProcessor::buildTopLevelElements(ElementRef& currentStylesheet,
      ElementRef& mainStylesheet, NodeSet& stripSpacesDeclarations,
      NodeSet& myXSLDeclarations)
  {
    for (ChildIterator child(currentStylesheet); child; child++)
      {
        if (child.getKeyId() == xsl.import())
          {
            String href = child.getAttr(xsl.href());
            Log_XSL ( "XSLProcessor : Stylesheet : Importing '%s'\n", href.c_str() );
            ElementRef importedStylesheet = importStylesheet(child,
                currentStylesheet);

            prepareStylesheet(importedStylesheet, mainStylesheet,
                stripSpacesDeclarations, true);
          }
        else if (child.getKeyId() == xsl.include())
          {
            String href = child.getAttr(xsl.href());
            Log_XSL ( "XSLProcessor : Stylesheet : Including '%s'\n", href.c_str() );
            ElementRef importedStylesheet = importStylesheet(child,
                currentStylesheet);
            buildTopLevelElements(importedStylesheet, mainStylesheet,
                stripSpacesDeclarations, myXSLDeclarations);
          }
        else if (child.getNamespaceId() == xsl.ns())
          {
            myXSLDeclarations.pushBack(child);
          }
        else if (child.isText())
          {
            Warn ( "Top-level text ignored : '%s'\n", child.getText().c_str() );
          }
        else if (child.isComment())
          {

          }
        else
          {
            Log_XSL ( "Ignored top-level element : '%s'\n", child.getKey().c_str() );
          }
      }

  }

  void
  XSLProcessor::addTopLevelElement(ElementRef& mainStylesheet,
      ElementRef& topLevelElement)
  {
    ElementMultiMapRef topLevelElements = mainStylesheet.findAttr(
        xslimpl.toplevel_elements(), AttributeType_SKMap);
    if (!topLevelElements)
      {
        topLevelElements = mainStylesheet.addSKMap(
            xslimpl.toplevel_elements(), SKMapType_ElementMultiMap);
      }
    topLevelElements.put(topLevelElement.getKeyId(), topLevelElement);
    Log_XSL ( "==> topLevelElements='%s', add '%s'\n",
        topLevelElements.generateVersatileXPath().c_str(),
        topLevelElement.generateVersatileXPath().c_str() );
  }

  void
  XSLProcessor::prepareStylesheet(ElementRef& currentStylesheet,
      ElementRef& mainStylesheet, NodeSet& stripSpacesDeclarations,
      bool isImported)
  {
    Log_XSL ( "----------------------------------------------------------------------------\n" );
    Log_XSL ( "setStylesheet contents : \n\tcurrent=%s\n\tmain=%s\n\n",
        currentStylesheet.generateVersatileXPath().c_str(),
        mainStylesheet.generateVersatileXPath().c_str() );
#if PARANOID
    AssertBug ( &(currentStylesheet.getDocument()) == &(mainStylesheet.getDocument()), "Invalid stylesheets from different documents !\n" );
#endif

    NodeSet myImportedStylesheets;
    NodeSet myXSLDeclarations;

    buildTopLevelElements(currentStylesheet, mainStylesheet,
        stripSpacesDeclarations, myXSLDeclarations);

    Log_XSL ( "Processing other XSL Declarations.\n" );
    for (NodeSet::iterator childIter(myXSLDeclarations); childIter; childIter++)
      {
        ElementRef& child = childIter->toElement();
        Log_XSL ( "At xsl declaration : '%s'\n", child.generateVersatileXPath().c_str() );

        if (child.getKeyId() == xsl.variable() || child.getKeyId()
            == xsl.param())
          {
            ElementRef& param = child;
            if (!param.hasAttr(xsl.name()))
              throwXProcessorException ( "Param or Variable has no name attribute.\n" );
            KeyId nameKeyId = param.getAttrAsKeyId(xsl.name());
            ElementMapRef topLevelParams = mainStylesheet.findAttr(
                xslimpl.toplevel_params(), AttributeType_SKMap);
            if (!topLevelParams)
              {
                topLevelParams = mainStylesheet.addSKMap(
                    xslimpl.toplevel_params(), SKMapType_ElementMap);
              }
            topLevelParams.put(nameKeyId, child);
          }
        else if (child.getKeyId() == xsl.template_())
          {
            Log_XSL ( "New template : \n\tTemplate=[%s]\n\ton [%s]\n\tmatches='%s', mode='%s', name='%s', imported=%s\n",
                child.generateVersatileXPath().c_str(), mainStylesheet.generateVersatileXPath().c_str(),
                child.hasAttr ( xsl.match() ) ? child.getAttr ( xsl.match() ).c_str() : "(none)",
                child.hasAttr ( xsl.mode() ) ? child.getAttr ( xsl.mode() ).c_str() : "(none)",
                child.hasAttr ( xsl.name() ) ? child.getAttr ( xsl.name() ).c_str() : "(none)",
                isImported ? "yes" : "no"
            );
            KeyId modeId = 0;
            if (child.hasAttr(xsl.mode()))
              modeId = child.getAttrAsKeyId(xsl.mode());
            bool inserted = false;
            if (child.hasAttr(xsl.match()))
              {
                if (isImported)
                  {
                    ElementMultiMapRef importedModeTemplates =
                        mainStylesheet.findAttr(
                            xslimpl.imported_match_templates(),
                            AttributeType_SKMap);
                    if (!importedModeTemplates)
                      {
                        importedModeTemplates = mainStylesheet.addSKMap(
                            xslimpl.imported_match_templates(),
                            SKMapType_ElementMultiMap);
                      }
                    importedModeTemplates.put(modeId, child);
                  }
                else
                  {
                    ElementMultiMapRef modeTemplates = mainStylesheet.findAttr(
                        xslimpl.match_templates(),
                        AttributeType_SKMap);
                    if (!modeTemplates)
                      {
                        modeTemplates = mainStylesheet.addSKMap(
                            xslimpl.match_templates(),
                            SKMapType_ElementMultiMap);
                      }
                    modeTemplates.put(modeId, child);
                  }
                inserted = true;
              }
            if (child.hasAttr(xsl.name()))
              {
                KeyId nameId = child.getAttrAsKeyId(xsl.name());
                /* 
                 * Oasis considers that mode must be skipped for a named template
                 */
                modeId = 0;
                SKMapHash hash = getNamedTemplateHash(modeId, nameId);
                ElementMapRef nameTemplates = mainStylesheet.findAttr(
                    xslimpl.named_templates(), AttributeType_SKMap);
                if (!nameTemplates)
                  {
                    nameTemplates = mainStylesheet.addSKMap(
                        xslimpl.named_templates(),
                        SKMapType_ElementMap);
                  }
                nameTemplates.put(hash, child);
                inserted = true;
              }
            if (!inserted)
              {
                throwXSLException ( "Invalid template which has no name nor match.\n" );
              }
          }
        else if ((child.getKeyId() == xsl.strip_space())
            || (child.getKeyId() == xsl.preserve_space()))
          {
            if (!child.hasAttr(xsl.elements()))
              {
                throwXSLException ( "%s has no 'elements' attribute !\n", child.getKey().c_str() );
              }
            stripSpacesDeclarations.pushBack(child);
          }
        else if (child.getKeyId() == xsl.namespace_alias())
          {
            String fromPrefix =
                child.getAttr(xsl.stylesheet_prefix());
            String toPrefix = child.getAttr(xsl.result_prefix());

            Log_XSL ( "NS Alias : '%s' -> '%s'\n", fromPrefix.c_str(), toPrefix.c_str() );

            NamespaceId fromNamespaceId, toNamespaceId;
            if (fromPrefix == "#default")
              {
                fromNamespaceId = child.getNamespaceAlias(
                    __builtin.nons.xmlns(), true);
              }
            else
              {
                LocalKeyId fromPrefixId = child.getKeyCache().getKeyId(0,
                    fromPrefix.c_str(), true);
                KeyId declarationId =
                    child.getKeyCache().buildNamespaceDeclaration(fromPrefixId);
                fromNamespaceId = child.getNamespaceAlias(declarationId, true);
              }

            if (toPrefix == "#default")
              {
                toNamespaceId = child.getNamespaceAlias(__builtin.nons.xmlns(),
                    true);
              }
            else
              {
                LocalKeyId toPrefixId = child.getKeyCache().getKeyId(0,
                    toPrefix.c_str(), true);
                KeyId declarationId =
                    child.getKeyCache().buildNamespaceDeclaration(toPrefixId);
                toNamespaceId = child.getNamespaceAlias(declarationId, true);
              }

            Log_XSL ( "NS Alias : '%x' -> '%x'\n", fromNamespaceId, toNamespaceId );
            Log_XSL ( "NS Alias : '%s' -> '%s'\n",
                child.getKeyCache().getNamespaceURL ( fromNamespaceId ),
                child.getKeyCache().getNamespaceURL ( toNamespaceId ) );

            if (!fromNamespaceId || !toNamespaceId || fromNamespaceId
                == toNamespaceId)
              {
                throwXSLException ( "Invalid xsl:namespace-alias from namespace prefix '%s' to namespace prefix '%s'\n",
                    fromPrefix.c_str(), toPrefix.c_str() );
              }

            IntegerMapRef nsAliasMap = mainStylesheet.findAttr(
                xslimpl.namespace_aliases(), AttributeType_SKMap);
            if (!nsAliasMap)
              {
                nsAliasMap
                    = mainStylesheet.addSKMap(
                        xslimpl.namespace_aliases(),
                        SKMapType_IntegerMap);
              }

            nsAliasMap.put(fromNamespaceId, toNamespaceId);
          }
        else if (child.getKeyId() == xsl.key())
          {
            ElementRef& instructKey = child;
            /*
             * We don't build the mapping right now, we just parse name and XPath expressions.
             * Mapping building is done on-demand in XPath::evalFunctionKey().
             */
            KeyId keyNameId = instructKey.getAttrAsKeyId(xsl.name());
            XPath matchXPath(getXProcessor(), instructKey,
                xsl.match());
            XPath useXPath(getXProcessor(), instructKey, xsl.use());

            ElementMapRef keysMap = mainStylesheet.findAttr(
                xslimpl.key_definitions(), AttributeType_SKMap);
            if (!keysMap)
              {
                keysMap = mainStylesheet.addSKMap(
                    xslimpl.key_definitions(), SKMapType_ElementMap);
              }
            keysMap.put(keyNameId, child);
          }
        else if (child.getKeyId() == xsl.include())
          {
            Bug ( "xsl:include too late !\n" );
          }
        else if (child.getKeyId() == xsl.decimal_format())
          {
            KeyId decimalFormatId = 0;
            if (child.hasAttr(xsl.name()))
              {
                decimalFormatId = child.getAttrAsKeyId(xsl.name());
              }

            ElementMapRef decimalFormatsMap = mainStylesheet.findAttr(
                xslimpl.decimal_formats(), AttributeType_SKMap);
            if (!decimalFormatsMap)
              {
                decimalFormatsMap = mainStylesheet.addSKMap(
                    xslimpl.decimal_formats(), SKMapType_ElementMap);
              }
            decimalFormatsMap.put(decimalFormatId, child);
          }
        else if (child.getKeyId() == xsl.output())
          {
            Log_XSL ( "[XSL OUTPUT] xsl:output : '%s'\n", child.generateVersatileXPath().c_str() );
            XSLProcessor::addTopLevelElement(mainStylesheet, child);
          }
        else if (child.getKeyId() == xsl.attribute_set())
          {
            Log_XSL ( "[XSL] xsl:attribute-set : '%s'\n", child.generateVersatileXPath().c_str() );
            KeyId attributeSetId = child.getAttrAsKeyId(xsl.name());
            ElementMultiMapRef attributeSetMap = mainStylesheet.findAttr(
                xslimpl.attribute_set_map(), AttributeType_SKMap);
            if ( !attributeSetMap )
              {
                attributeSetMap = mainStylesheet.addSKMap(xslimpl.attribute_set_map(),
                        SKMapType_ElementMultiMap);
              }
            attributeSetMap.put(attributeSetId, child);
            XSLProcessor::addTopLevelElement(mainStylesheet, child);
#if 0
            for ( ChildIterator attr(child) ; attr ; attr++ )
              {
                if ( attr.getKeyId() == xsl.attribute() )
                  {
                    ElementRef _root = attr.getRootElement();
                    NodeSet ns; ns.pushBack(_root,false);
                    NodeSet::iterator iter(ns,getXProcessor());
                    String value = getXProcessor().evalChildrenAsString(attr);
                    Log ( "[XSL] xsl:attribute-set : '%s' => '%s'\n",
                        attr.generateVersatileXPath().c_str(), value.c_str() );
                    attr.addAttr(xslimpl.attribute_value(), value);
                  }
              }
#endif
          }
        else
          {
            Warn ( "xsl top-level element not implemented : %s.\n", child.getKey().c_str() );
#if 1
            Warn ( "\tat %s.\n", child.generateVersatileXPath().c_str() );
#endif
          }
      }

    if (currentStylesheet == mainStylesheet)
      {
        ElementMultiMapRef nullAttr(currentStylesheet.getDocument());

        Log_XSLOpt ( "---------------- BUILDING %s ---------------\n",
            currentStylesheet.generateVersatileXPath().c_str() );
        ElementMultiMapRef modeTemplates = currentStylesheet.findAttr(
            xslimpl.match_templates(), AttributeType_SKMap);
        ElementMultiMapRef importedModeTemplates = currentStylesheet.findAttr(
            xslimpl.imported_match_templates(), AttributeType_SKMap);

        if (modeTemplates || importedModeTemplates)
          {
            ElementMultiMapRef optimizedModeTemplates =
                currentStylesheet.findAttr(
                    xslimpl.match_templates_opt(),
                    AttributeType_SKMap);
            AssertBug ( !optimizedModeTemplates, "Already built match-templates-optimized !\n" );
            optimizedModeTemplates = currentStylesheet.addSKMap(
                xslimpl.match_templates_opt(),
                SKMapType_ElementMultiMap);
            Log_XSLOpt ( "//// Building direct opts...\n" );
#if 1
            prepareOptimizedMatchTemplates(modeTemplates, nullAttr,
                optimizedModeTemplates);

#else
            prepareOptimizedMatchTemplates ( modeTemplates,
                importedModeTemplates, optimizedModeTemplates );
#endif
          }

        if (importedModeTemplates)
          {
            ElementMultiMapRef optimizedImportedModeTemplates =
                currentStylesheet.findAttr(
                    xslimpl.imported_match_templates_opt(),
                    AttributeType_SKMap);
            AssertBug ( !optimizedImportedModeTemplates, "Already built imported-match-templates !\n" );
            optimizedImportedModeTemplates = currentStylesheet.addSKMap(
                xslimpl.imported_match_templates_opt(),
                SKMapType_ElementMultiMap);

            Log_XSLOpt ( "//// Building import opts...\n" );
            prepareOptimizedMatchTemplates(importedModeTemplates, nullAttr,
                optimizedImportedModeTemplates);
          }

      }
    Log_XSL ( "setStylesheet contents at '0x%llx'... OK\n", currentStylesheet.getElementId() );
  }

  inline void
  XSLProcessor::copyList(TemplateInfoList& list2, TemplateInfoList& list1)
  {
    for (TemplateInfoList::iterator iter = list1.begin(); iter != list1.end(); iter++)
      {
        TemplateInfo info = *iter;
        list2.push_back(info);
      }
  }

  void
  XSLProcessor::insertMatchTemplate(TemplateInfoList& list, TemplateInfo& info)
  {
    if (list.size() == 0)
      {
        list.push_back(info);
        return;
      }
    for (TemplateInfoList::iterator iter = list.begin(); iter != list.end(); iter++)
      {
        if (info.priority < iter->priority)
          continue;
#if 0
        if ( info.priority == iter->priority )
          {
            Log_XSLOpt ( "Conflict : info=%s, iter=%s\n",
                info.templ.generateVersatileXPath().c_str(),
                iter->templ.generateVersatileXPath().c_str() );
            if ( iter->templ.isBeforeInDocumentOrder ( info.templ ) )
            continue;
          }
#endif
        list.insert(iter, info);
        return;
      }
    list.push_back(info);
  }

  void
  XSLProcessor::mergeTemplateInfoMap(
      ElementMultiMapRef& optimizedModeTemplates,
      TemplateInfoMap& templateInfoMap, KeyId modeId)
  {
    for (TemplateInfoMap::iterator iter = templateInfoMap.begin(); iter
        != templateInfoMap.end(); iter++)
      {
        KeyId keyId = iter->first;
#if 0 // DEPRECATED : Zero KeyId is supposed to work
        if (!keyId)
          {
            Warn ( "!!!!!!!!!! Zero keyId !\n");
            for (TemplateInfoList::iterator iter2 = iter->second.begin(); iter2 != iter->second.end(); iter2++)
              {
                Warn ( "\tkey=0, mode=%s (%x), p=%f, templ='%s' at %s\n",
                    getKeyCache().dumpKey(modeId).c_str(), modeId,
                    iter2->priority,
                    iter2->templ.getAttr(xsl.match()).c_str(),
                    iter2->templ.generateVersatileXPath().c_str() );

              }
            // continue;
          }
        // AssertBug ( keyId, "Zero keyId !\n" );
#endif
        TemplateInfoList& list = iter->second;
        for (TemplateInfoList::iterator iter2 = list.begin(); iter2
            != list.end(); iter2++)
          {
            Log_XSLOpt ( "key=%s (%x), mode=%s (%x), p=%f, templ='%s' at %s\n",
                iter->first ? getKeyCache().dumpKey(iter->first).c_str() : "(nil)", iter->first,
                getKeyCache().dumpKey(modeId).c_str(), modeId,
                iter2->priority,
                iter2->templ.getAttr(xsl.match()).c_str(),
                iter2->templ.generateVersatileXPath().c_str() );

            SKMapHash hash = getNamedTemplateHash(modeId, keyId);
            optimizedModeTemplates.put(hash, iter2->templ);
          }
      }
  }

  void
  XSLProcessor::mergeFinalSteps(TemplateInfoMap& templateInfoMap,
      XPath::XPathFinalSteps& finalSteps, ElementRef& templ,
      bool hasTemplatePriority, Number templatePriority)
  {
    KeyId wildcardId = __builtin.xemint.element();
    KeyId textId = __builtin.xemint.textnode();
    for (XPath::XPathFinalSteps::iterator iter = finalSteps.begin(); iter
        != finalSteps.end(); iter++)
      {
        if ( iter->elementOrAttribute )
          {
            // Skip final attributes (yet)
            continue;
          }
        KeyId keyId = iter->keyId;

        TemplateInfo info(templ, hasTemplatePriority ? templatePriority
            : iter->priority);
        TemplateInfoList& lst = templateInfoMap[keyId];

        if (keyId == wildcardId || keyId == 0)
          {
            for (TemplateInfoMap::iterator witer = templateInfoMap.begin(); witer
                != templateInfoMap.end(); witer++)
              {
                if (keyId == wildcardId && witer->first == textId)
                  continue;
                if (witer->first != keyId)
                  {
                    // TemplateInfoList& list = iter->second;
                    insertMatchTemplate(witer->second, info);
                  }
              }
          }
        else if (keyId == textId)
          {
            if (lst.size() == 0)
              {
                copyList(lst, templateInfoMap[0]);
              }
          }
        else if (KeyCache::getNamespaceId(keyId) && !KeyCache::getLocalKeyId(
            keyId))
          {
            for (TemplateInfoMap::iterator witer = templateInfoMap.begin(); witer
                != templateInfoMap.end(); witer++)
              {
                if (KeyCache::getNamespaceId(witer->first)
                    == KeyCache::getNamespaceId(keyId)
                    && KeyCache::getLocalKeyId(witer->first))
                  {
                    insertMatchTemplate(witer->second, info);
                  }
              }
          }
        else
          {
            if (lst.size() == 0)
              {
                // Initialize with already-found wildcards
                KeyId nsId = KeyCache::getKeyId(
                    KeyCache::getNamespaceId(keyId), 0);
                if (templateInfoMap.find(nsId) != templateInfoMap.end())
                  {
                    copyList(lst, templateInfoMap[nsId]);
                  }
                else
                  copyList(lst, templateInfoMap[wildcardId]);
              }
          }
        insertMatchTemplate(lst, info);
      }
  }

  void
  XSLProcessor::buildTemplateInfoMap(TemplateInfoMap& templateInfoMap,
      ElementMultiMapRef& modeTemplates, KeyId modeId)
  {
    for (ElementMultiMapRef::multi_iterator iter(modeTemplates, modeId); iter; iter++)
      {
        ElementRef templ = modeTemplates.get(iter);
        Number templatePriority = -INFINITY;
        bool hasTemplatePriority = false;
        if (templ.hasAttr(xsl.priority()))
          {
            templatePriority = templ.getAttrAsNumber(xsl.priority());
            hasTemplatePriority = true;
          }

        Log_XSLOpt ( "At template : '%s'\n", templ.generateVersatileXPath().c_str() );
        XPath templXPath(getXProcessor(), templ, xsl.match());
        XPath::XPathFinalSteps finalSteps;
        templXPath.buildFinalSteps(finalSteps);

        mergeFinalSteps(templateInfoMap, finalSteps, templ,
            hasTemplatePriority, templatePriority);
      }
  }

  void
  XSLProcessor::prepareOptimizedMatchTemplates(
      ElementMultiMapRef& modeTemplates,
      ElementMultiMapRef& importedModeTemplates,
      ElementMultiMapRef& optimizedModeTemplates)
  {
    std::map<KeyId, bool> processedModes;
    if ( modeTemplates )
      {
        Log_XSLOpt ( " -------- BUILDING DIRECT ----------------\n" );
        for (SKMapRef::iterator modeIter(modeTemplates); modeIter; modeIter++)
          {
            TemplateInfoMap templateInfoMap;
            Log_XSLOpt ( "At '%s' modeId=0x%llx, mode=%s\n",
                modeTemplates.generateVersatileXPath().c_str(),
                modeIter.getHash(),
                modeIter.getHash() ? getKeyCache().dumpKey(modeIter.getHash()).c_str() : "(#default)" );
            KeyId modeId = modeIter.getHash();
            processedModes[modeId] = true;
            if (importedModeTemplates)
              {
                Log_XSLOpt ( "*** BUILD IMPORT\n" );
                buildTemplateInfoMap(templateInfoMap, importedModeTemplates, modeId);
              }

            Log_XSLOpt ( "*** BUILD DIRECT\n" );
            buildTemplateInfoMap(templateInfoMap, modeTemplates, modeId);
            Log_XSLOpt ( "*** MERGE\n" );
            mergeTemplateInfoMap(optimizedModeTemplates, templateInfoMap, modeId);
          }
      }
    if ( importedModeTemplates )
      {
        Log_XSLOpt ( " -------- BUILDING IMPORTS ----------------\n" );
        for (SKMapRef::iterator modeIter(importedModeTemplates); modeIter; modeIter++)
          {
            KeyId modeId = modeIter.getHash();
            if (processedModes.find(modeId) != processedModes.end())
              {
                Log_XSLOpt ( "Skipping mode %s (%x), already processed.\n",
                    getKeyCache().dumpKey(modeId).c_str(), modeId );
                continue;
              }
            TemplateInfoMap templateInfoMap;
            buildTemplateInfoMap(templateInfoMap, importedModeTemplates, modeId);
            mergeTemplateInfoMap(optimizedModeTemplates, templateInfoMap, modeId);
          }
      }
  }
}
;

