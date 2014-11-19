#include <Xemeiah/xemprocessor/xemprocessor.h>
#include <Xemeiah/xemprocessor/xemobjectmodule.h>
#include <Xemeiah/xemprocessor/xem-metaindexer.h>
#include <Xemeiah/xemprocessor/xemmoduleforge.h>

#include <Xemeiah/kern/volatiledocument.h>
#include <Xemeiah/kern/branchmanager.h>

#include <Xemeiah/dom/elementmapref.h>
#include <Xemeiah/dom/childiterator.h>

#include <Xemeiah/xprocessor/xprocessor.h>
#include <Xemeiah/xpath/xpath.h>
#include <Xemeiah/xpath/xpathparser.h>
#include <Xemeiah/nodeflow/nodeflow-file.h>
#include <Xemeiah/nodeflow/nodeflow-dom.h>
#include <Xemeiah/nodeflow/nodeflow-sequence.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_XemObject Debug

#define __XEM_XEMPROCESSOR_CHECK_CALLMETHOD_ARGUMENTS //< Option : check callMethod() arguments (strongly encouraged)

namespace Xem
{
  void
  XemProcessor::xemInstructionSetCurrentCodeScope(__XProcHandlerArgs__)
  {
    if ( item.hasAttr(xem.select() ) )
      {
        XPath currentCodeScopeXPath(getXProcessor(), item, xem.select());
        ElementRef currentCodeScope = currentCodeScopeXPath.evalElement();

        setCurrentCodeScope(currentCodeScope, true);
      }
    else if ( item.hasAttr(xem.branch() ) )
      {
        String branchName = item.getEvaledAttr(getXProcessor(),xem.branch());
        Document* document = getXProcessor().getStore().getBranchManager().openDocument(branchName, "explicit-read",
            KeyCache::getLocalKeyId(xem_role.code()) );
        if ( ! document )
          {
            throwException(Exception, "Could not open document from branch '%s'\n", branchName.c_str() );
          }
        getXProcessor().bindDocument(document, true);
        ElementRef currentCodeScope = document->getRootElement();
        setCurrentCodeScope(currentCodeScope,true);

        getXProcessor().setElement(xem_role.code(), currentCodeScope,true);
      }
    else
      {
      	throwException ( Exception, "Invalid call to xem:set-current-code-scope : no attribute 'select' nor 'branch'\n" );
      }
  }

  void
  XemProcessor::xemFunctionDefaultFunction(__XProcFunctionArgs__)
  {
    if (!isElementFunction)
      {
#if 0
        throwException ( Exception, "Not implemented : defaulted but not an Element Function : '%s' !\n",
            getKeyCache().dumpKey(functionKeyId).c_str() );
#else

        Log("Spurious : defaulted but not an Element Function : '%s' !\n",
            getKeyCache().dumpKey(functionKeyId).c_str() );
#endif
      }

    if ( KeyCache::getNamespaceId(functionKeyId) )
      {
        MetaIndexer metaIndexer = getMetaIndexer(node.getDocument(), functionKeyId);
        if ( metaIndexer )
          {
            metaIndexer.eval ( getXProcessor(), result, node.toElement(), *(args[0]) );
            Log_XemObject ( "Found %lu results\n", (unsigned long) result.size() );
            return;
          }

        throwException ( Exception, "Function '%s' has a namespace, but no MetaIndexer was found !\n",
            getKeyCache().dumpKey(functionKeyId).c_str() );
      }

    ElementRef codeScope = getCurrentCodeScope();
    ElementMapRef functionMap = codeScope.findAttr(xem.function_map(),
        AttributeType_SKMap);

    if (!functionMap)
      {
        throwException ( Exception, "Invalid or corrupted xem function map.\n" );
      }
    ElementRef thisElement = node.toElement();
    KeyId classId = getElementClass(thisElement);
    ElementRef function = XemProcessor::findFunction(functionMap, classId, functionKeyId);

    if (!function)
      {
        throwException ( Exception, "Could not find any function for object %s (%x) named %s (%x).",
            getKeyCache().dumpKey(classId).c_str(), classId,
            getKeyCache().dumpKey(functionKeyId).c_str(),functionKeyId);
      }

    getXProcessor().pushEnv();

    try
      {
        getXProcessor().setElement(__builtin.nons.this_(), thisElement);

        if (args.size() > 0)
          {
            XPath::FunctionArguments::size_type idx = 0;
            for (ChildIterator iter(function); iter; iter++)
              {
                if (iter.getKeyId() != xem.param())
                  continue;
                if (idx == args.size())
                  {
                    throwException ( Exception, "Not enough parameters !\n" );
                  }
                KeyId variableId = iter.getAttrAsKeyId(xem.name());
                NodeSet* var = getXProcessor().setVariable(variableId);
                args[idx]->copyTo(*var);
                idx++;
              }
            if (idx < args.size())
              {
                throwException ( Exception, "Too much parameters !\n" );
              }
          }

#if 1 // Use NodeFlowSequence
        ElementRef root = getXProcessor().createVolatileDocument(false);
        NodeFlowSequence nodeFlowSequence(getXProcessor(), root, result);
        getXProcessor().setNodeFlow(nodeFlowSequence);
        getXProcessor().processChildren(function);
#else
        String resultStr = getXProcessor().evalChildrenAsString ( function );
        result.setSingleton ( resultStr );
#endif
      }
    catch (Exception* e)
      {
        getXProcessor().popEnv();
        throw(e);
      }
    getXProcessor().popEnv();
  }

  void
  XemProcessor::xemInstructionProcedure(__XProcHandlerArgs__)
  {
    getXProcessor().processChildren(item);
  }

  void
  XemProcessor::xemInstructionMethod(__XProcHandlerArgs__)
  {
    if (item.getKeyId() != xem.method())
      {
        Bug ( "." );
      }
    getXProcessor().processChildren(item);
  }

  void
  XemProcessor::xemInstructionFunction(__XProcHandlerArgs__)
  {
    if (item.getKeyId() != xem.function())
      {
        Bug ( "." );
      }
    getXProcessor().processChildren(item);
  }

  void
  XemProcessor::xemInstructionProcess(__XProcHandlerArgs__)
  {
    Log_XemObject ( "XemInstructionProcess xem:process\n" );
    if (!item.hasAttr(xem.select()))
      {
        throwXProcessorException ( "xem:process has no xem:select attribute !\n" );
      }

    XPath sourceXPath(getXProcessor(), item, xem.select());

    NodeSet sourceNodeSet;
    sourceXPath.eval(sourceNodeSet);

    if (!sourceNodeSet.size())
      {
        Warn ( "Empty select expression for xem:process not found : '%s', select expression was : '%s'\n",
            sourceXPath.getExpression(),
            item.getAttr(xem.select()).c_str() );
        return;
        throwXProcessorException ( "Node not found : '%s', select expression was : '%s'\n",
            sourceXPath.getExpression(),
            item.getAttr(xem.select()).c_str() );
      }

    if (item.hasAttr(xem.type()))
      {
        String type = item.getEvaledAttr(getXProcessor(), xem.type());
        if (type == "xsl:stylesheet")
          {
            ElementRef source = sourceNodeSet.toElement();
#if 0
            if (source.getKeyId() != __builtin.xsl.stylesheet())
              {
                throwXProcessorException ( "Invalid node : '%s'\n", source.getKey().c_str() );
              }
#endif
          }
        else
          {
            throwXProcessorException ( "Invalid type '%s'\n", type.c_str() );
          }
      }

    if (item.hasAttr(xem.result_file()))
      {
        NodeFlowFile nodeFlow(getXProcessor());
        String resultFile = item.getEvaledAttr(getXProcessor(),
            xem.result_file());
        nodeFlow.setFile(resultFile);
        getXProcessor().setNodeFlow(nodeFlow);

        for (NodeSet::iterator iter(sourceNodeSet); iter; iter++)
          getXProcessor().process(iter->toElement());

        return;
      }

    if (item.hasAttr(xem.root()))
      {
        throwException ( Exception, "Not implemented : @root attribute of xem:process\n" );
      }
#if 0
    if ( item.hasAttr ( xem.root) ) )
      {
        XPath rootXPath ( item, xem.root) );
        ElementRef baseElement = rootXPath.evalElement ( getXProcessor(), currentNode );

        SubDocument subDocument ( baseElement );

        ElementRef rootElement = subDocument.getRootElement ();

        for ( NodeSet::iterator iter(sourceNodeSet); iter; iter++ )
        getXProcessor().process ( iter->toElement(), rootElement );
      }
    else
#endif
    for (NodeSet::iterator iter(sourceNodeSet); iter; iter++)
      getXProcessor().process(iter->toElement());

  }

  void
  XemProcessor::setCurrentCodeScope(ElementRef& codeScope, bool behind)
  {
    Log_XemObject ( "XProcessor %p : Setting currentCodeScope='%s', document=%p : [%llx:%llx] role='%s'\n",
        &getXProcessor(),
        codeScope.generateVersatileXPath().c_str(),
        &(codeScope.getDocument()),
        _brid(codeScope.getDocument().getBranchRevId()),
        codeScope.getDocument().getRole().c_str() );
    getXProcessor().setElement(xem.current_codescope(), codeScope, behind);
  }

  bool XemProcessor::hasCurrentCodeScope ()
  {
    return getXProcessor().hasVariable(xem.current_codescope());
  }

  ElementRef
  XemProcessor::getCurrentCodeScope()
  {
    if (!getXProcessor().hasVariable(xem.current_codescope()))
      {
        Bug ( "." );
        throwException ( Exception, "No xem:current-codescope variable defined !\n" );
      }
    ElementRef codeScope =
        getXProcessor().getVariable(xem.current_codescope())->toElement();
    return codeScope;
  }

  ElementRef
  XemProcessor::getObjectDefinition ( KeyId classId )
  {
    ElementMapRef objectMap = getCurrentCodeScope().findAttr(xem.object_map(),
        AttributeType_SKMap);
    ElementRef objectDefinition = objectMap.get(classId);
    return objectDefinition;
  }

  ElementRef
  XemProcessor::getObjectConstructor ( KeyId classId )
  {
    ElementMapRef constructorMap = getCurrentCodeScope().findAttr(xem.constructor_map(),
        AttributeType_SKMap);

    ElementRef objectConstructor = constructorMap.get(classId);
    return objectConstructor;
  }

  ElementRef
  XemProcessor::findMethod(KeyId classId, LocalKeyId methodId)
  {
    ElementRef rootElement = getCurrentCodeScope().getRootElement();

    Log_XemObject ( "findMethod : rootElement='%s', brId=[%llx:%llx]\n",
        rootElement.generateVersatileXPath().c_str(),
        _brid(rootElement.getDocument().getBranchRevId()) );

    ElementMapRef methodMap = rootElement.findAttr(xem.method_map(),
        AttributeType_SKMap);

    if (!methodMap)
      {
        rootElement.toXML(1, 0);
        throwException ( Exception, "Too late to build method map for document '%s'!\n", rootElement.getDocument().getRole().c_str() );
      }

    SKMapHash hash = hashMethodId(classId, methodId);

    Log_XemObject ( "==> %llx\n", hash );
    return methodMap.get(hash);
  }

  ElementRef
  XemProcessor::findFunction(ElementMapRef& functionMap, KeyId classId,
      LocalKeyId functionId)
  {
    SKMapHash hash = hashMethodId(classId, functionId);
    return functionMap.get(hash);
  }

  void
  XemProcessor::callMethod(ElementRef& caller, ElementRef& thisElement,
      ElementRef& method, bool setThis)
  {
    if (setThis)
      {
#if PARANOID
        AssertBug ( thisElement, "Non-defined this element !\n" );
#endif
        if (getXProcessor().hasVariable(__builtin.nons.this_()))
          {
            ElementRef superRef = getXProcessor().getVariable(
                __builtin.nons.this_())->toElement();
            getXProcessor().setElement(__builtin.nons.super(), superRef);
          }
        getXProcessor().setElement(__builtin.nons.this_(), thisElement);
      }
    if (caller)
      {
#ifdef __XEM_XEMPROCESSOR_CHECK_CALLMETHOD_ARGUMENTS
        typedef std::map<KeyId,ElementRef> ParamsMap;
        ParamsMap paramsMap;

        for (ChildIterator param(method); param; param++ )
          {
            if (param.getKeyId() != xem.param())
              continue;
            KeyId paramId = param.getAttrAsKeyId(xem.name());
            if ( paramsMap.find(paramId) != paramsMap.end() )
              {
                throwException ( Exception, "Duplicate name %s for method : %s\n",
                    getKeyCache().dumpKey(paramId).c_str(), method.generateVersatileXPath().c_str() );
              }
            std::pair<KeyId,ElementRef> p(paramId,param);
            paramsMap.insert(p);
          }
#endif // __XEM_XEMPROCESSOR_CHECK_CALLMETHOD_ARGUMENTS
        for (ChildIterator param(caller); param; param++ )
          {
            if (param.getKeyId() != xem.with_param())
              continue;
#ifdef __XEM_XEMPROCESSOR_CHECK_CALLMETHOD_ARGUMENTS
            KeyId paramId = param.getAttrAsKeyId(xem.name());
            ParamsMap::iterator paramIter = paramsMap.find(paramId);
            if ( paramIter == paramsMap.end() )
              {
                bool methodTakesExtraParams = method.hasAttr(xem.method_takes_extra_params())
                    && (method.getAttr(xem.method_takes_extra_params()) == "yes");
                if ( ! methodTakesExtraParams )
                  {
                    throwException ( Exception, "Parameter name %s not set on method : %s, caller : %s\n",
                        getKeyCache().dumpKey(paramId).c_str(), method.generateVersatileXPath().c_str(),
                        caller.generateVersatileXPath().c_str() );
                  }
              }
            else
              {
                paramsMap.erase(paramIter);
              }
#endif // __XEM_XEMPROCESSOR_CHECK_CALLMETHOD_ARGUMENTS
            getXProcessor().processInstructionVariable(param, xem.name(),xem.select(), false);
          }
#ifdef __XEM_XEMPROCESSOR_CHECK_CALLMETHOD_ARGUMENTS
        for ( ParamsMap::iterator iter = paramsMap.begin() ; iter != paramsMap.end() ; iter++ )
          {
            KeyId paramId = iter->first;

            bool assumeParamsAreInEnv = caller.hasAttr(xem.assume_params_are_in_env())
                && (caller.getAttr(xem.assume_params_are_in_env()) == "yes" );
            if ( assumeParamsAreInEnv && getXProcessor().hasVariable(paramId) )
              {
                continue;
              }

            ElementRef& param = iter->second;
            Log_XemObject ( "Setting param : %s\n", param.generateVersatileXPath().c_str() );
            if ( ! param.hasAttr(xem.select()) && ! param.getChild() )
              {
                throwException ( Exception, "Empty param value for parameter '%s' on method : %s, caller : %s\n",
                    getKeyCache().dumpKey(paramId).c_str(), method.generateVersatileXPath().c_str(),
                    caller.generateVersatileXPath().c_str() );
              }
            getXProcessor().processInstructionVariable(param, xem.name(),xem.select(), false);
          }
#endif // __XEM_XEMPROCESSOR_CHECK_CALLMETHOD_ARGUMENTS
      }
    else
      {
#ifdef __XEM_XEMPROCESSOR_CHECK_CALLMETHOD_ARGUMENTS
        for (ChildIterator param(method); param; param++ )
          {
            if (param.getKeyId() != xem.param())
              continue;
            KeyId paramId = param.getAttrAsKeyId(xem.name());
            if ( ! getXProcessor().hasVariable(paramId) )
              {
                throwException ( Exception, "No param set %s for no-caller method call : %s \n",
                    getKeyCache().dumpKey(paramId).c_str(), method.generateVersatileXPath().c_str() );
              }
          }
#endif // __XEM_XEMPROCESSOR_CHECK_CALLMETHOD_ARGUMENTS
      }
    getXProcessor().process(method);
  }

  void
  XemProcessor::callMethod(ElementRef& caller, ElementRef& thisElement,
      KeyId classId, LocalKeyId methodId, bool setThis)
  {
    if (!classId || !methodId)
      {
        throwException ( Exception, "Invalid zero classId or methodId provided : classId=%x, methodId=%x !\n",
            classId, methodId );
      }
    ElementRef method = findMethod(classId, methodId);

    if (!method)
      {
        KeyCache& keyCache = getXProcessor().getKeyCache();
        throwException ( Exception,
            "Could not get method '%s' for class %s (keys class=%x/method=%x/this=%x) in document '%s' (role '%s', brid=[%llx:%llx]).\n",
            keyCache.dumpKey(methodId).c_str(), keyCache.dumpKey(classId).c_str(),
            classId, methodId, thisElement.getKeyId(),
            getCurrentCodeScope().getDocument().getDocumentTag().c_str(), getCurrentCodeScope().getDocument().getRole().c_str(),
            _brid(getCurrentCodeScope().getDocument().getBranchRevId()) );
      }
    try
      {
        callMethod(caller, thisElement, method, setThis);
      }
    catch (Exception* e)
      {
        detailException ( e, "At class call : class='%s', method='%s'\n",
            getKeyCache().dumpKey(classId).c_str(),
            getKeyCache().dumpKey(methodId).c_str() );
        throw(e);
      }

  }

  void
  XemProcessor::doInstanciate(ElementRef& caller,
      ElementRef& constructorMethod, ElementRef& instance)
  {
    if (caller.hasAttr(xem.name()))
      {
        KeyId keyId = caller.getAttrAsKeyId(xem.name());
        getXProcessor().setElement(keyId, instance, true);
      }

    try
      {
        callMethod(caller, instance, constructorMethod, true);
      }
    catch (Exception* e)
      {
        detailException ( e, "Could not instanciate class '%s' from element : %s\n",
            getKeyCache().dumpKey(getElementClass(instance)).c_str(), caller.generateVersatileXPath().c_str() );
        throw(e);
      }
  }

  void XemProcessor::controlMethodArguments ( ElementRef& thisElement, KeyId methodId, KeyIdList& arguments )
  {
    KeyId classId = getElementClass(thisElement);
    controlMethodArguments(classId,methodId,arguments);
  }

  void XemProcessor::controlMethodArguments ( KeyId classId, KeyId methodId, KeyIdList& arguments )
  {
    ElementRef method = findMethod(classId, methodId);

    typedef std::map<KeyId,ElementRef> ParamsMap;
    ParamsMap paramsMap;

    for (ChildIterator param(method); param; param++ )
      {
        if (param.getKeyId() != xem.param())
          continue;
        KeyId paramId = param.getAttrAsKeyId(xem.name());
        if ( paramsMap.find(paramId) != paramsMap.end() )
          {
            throwException ( Exception, "Duplicate name %s for method : %s\n",
                getKeyCache().dumpKey(paramId).c_str(), method.generateVersatileXPath().c_str() );
          }
        std::pair<KeyId,ElementRef> p(paramId,param);
        paramsMap.insert(p);
      }
    for(KeyIdList::iterator arg = arguments.begin() ; arg != arguments.end() ; arg++ )
      {
        KeyId keyId = *arg;
        ParamsMap::iterator iter = paramsMap.find(keyId);
        if ( iter == paramsMap.end() )
          {
            throwException ( Exception, "Undefined caller argument : %s\n", getKeyCache().dumpKey(keyId).c_str() );
          }
        paramsMap.erase(iter);
      }
    for ( ParamsMap::iterator iter = paramsMap.begin() ; iter != paramsMap.end() ; iter++ )
      {
        Error ( "Undefined argument : %s\n", getKeyCache().dumpKey(iter->first).c_str() );
      }
    if ( paramsMap.size() )
      {
        throwException ( Exception, "At least one argument not set !\n" );
      }
  }

  void
  XemProcessor::xemInstructionInstance(__XProcHandlerArgs__)
  {
    LocalKeyId methodId =
        getXProcessor().getKeyCache().getBuiltinKeys().nons.Constructor();
    KeyId classId = getElementClass(item);

    if (!classId)
      {
        throwException ( Exception, "Element '%s' has invalid or no attribute 'xem:class'\n",
            item.generateVersatileXPath().c_str() );
      }
    ElementRef constructorMethod = findMethod(classId, methodId);
    if (!constructorMethod)
      {
        throwException ( Exception, "Could not instanciate '%s' : no constructor defined !\n",
            item.generateVersatileXPath().c_str() );
      }

    String instanceScope = "";
    if (item.hasAttr(xem.instance_scope()))
      instanceScope = item.getEvaledAttr(getXProcessor(), xem.instance_scope());
    if (constructorMethod.hasAttr(xem.instance_scope()))
      instanceScope = constructorMethod.getEvaledAttr(getXProcessor(),
          xem.instance_scope());

    if (instanceScope == "inline")
      {
        getNodeFlow().newElement(classId);
        for (AttributeRef attr = item.getFirstAttr(); attr; attr
            = attr.getNext())
          {
            if (attr.getNamespaceId() == xem.ns())
              continue;
            getNodeFlow().newAttribute(attr.getKeyId(), item.getEvaledAttr(
                getXProcessor(), attr.getKeyId()));
          }
        NodeFlowDom& nodeFlowDom = NodeFlowDom::asNodeFlowDom(getNodeFlow());
        ElementRef instance = nodeFlowDom.getCurrentElement();
        doInstanciate(item, constructorMethod, instance);

        getXProcessor().processChildren(item);
        getNodeFlow().elementEnd(classId);
        return;
      }
    /**
     * Default instanceScope : static
     */
    doInstanciate(item, constructorMethod, item);
#if 0
    if ( getXProcessor().hasVariable ( __builtin.nons.this_() ) )
      {
        ElementRef superRef = getXProcessor().getVariable ( __builtin.nons.this_() )->toElement();
        getXProcessor().setElement ( __builtin.nons.super(), superRef );
      }
#endif
  }

  KeyId
  XemProcessor::getElementClass(ElementRef& element)
  {
    KeyId classId = element.getKeyId();
    if (classId == xem.instance() || element.hasAttr(xem.class_()))
      classId = element.getAttrAsKeyId(xem.class_());

    if (classId == 0)
      {
        throwException ( Exception, "Invalid class for this=%s\n", element.generateVersatileXPath().c_str() );
      }
    return classId;
  }

  String
  XemProcessor::getElementClassName(ElementRef& element)
  {
    if (element.getKeyId() == xem.instance())
      return element.getAttr(xem.class_());
    else
      return element.getKey();
  }

  ElementRef
  XemProcessor::getThisFromCaller(ElementRef& caller)
  {
    if (caller.hasAttr(xem.this_()))
      {
        XPath thisXPath(getXProcessor(), caller, xem.this_());
        ElementRef eThis = thisXPath.evalElement();
        return eThis;
      }
    ElementRef eThis =
        getXProcessor().getVariable(__builtin.nons.this_())->toElement();
    return eThis;
  }

  void
  XemProcessor::callMethod(__XProcHandlerArgs__)
  {
    KeyId fullMethodId = item.getAttrAsKeyId(getXProcessor(), xem.method());

    ElementRef eThis = getThisFromCaller(item);
    KeyId classId = getElementClass(eThis);

    try
      {
        callMethod(item, eThis, classId, KeyCache::getLocalKeyId(fullMethodId),
            item.hasAttr(xem.this_()));
      }
    catch (Exception* e)
      {
        String methodName = item.getEvaledAttr(getXProcessor(), xem.method());
        detailException ( e, "Could not call method '%s':%s for this=%s\n",
            getKeyCache().dumpKey ( getElementClass ( eThis ) ).c_str(), methodName.c_str(),
            eThis.generateVersatileXPath().c_str() );
        throw(e);
      }
  }

  void XemProcessor::callElementConstructor ( ElementRef& thisElement )
  {
    if ( ! getXProcessor().hasVariable(xem.current_codescope()) )
      {
        Log_XemObject ( "Skipping Element Constructor : No Current code scope !\n" );
        return;
      }
    KeyId constructorKeyId = getKeyCache().getBuiltinKeys().nons.Constructor();

    NodeFlowDom nodeFlow(getXProcessor(), thisElement);

    getXProcessor().setNodeFlow(nodeFlow);

    callMethod(thisElement, constructorKeyId);
  }

  void
  XemProcessor::callMethod(ElementRef& thisElement, LocalKeyId methodId)
  {
    ElementRef nullCaller = ElementRef(thisElement.getDocument());
    callMethod(nullCaller, thisElement, getElementClass(thisElement), methodId,
        true);
  }

  void
  XemProcessor::callMethod(ElementRef& thisElement, const String& methodName)
  {
    LocalKeyId methodId = KeyCache::getLocalKeyId(getKeyCache().getKeyId(0,
        methodName.c_str(), false));
    if (!methodId)
      throwException ( Exception, "Invalid method name provided : '%s'\n", methodName.c_str() );
    callMethod(thisElement, methodId);
  }

  void
  XemProcessor::xemInstructionCall(__XProcHandlerArgs__)
  {
    callMethod(item);
  }

  void
  XemProcessor::xemFunctionGetQNameId ( __XProcFunctionArgs__ )
  {
    NodeSet& nodeSet = *(args[0]);
    if (nodeSet.size() != 1)
      {
        throwException ( Exception, "Error : xem:qname-id() called with multiple results in first argument !\n" );
      }
    KeyId classId = getElementClass(nodeSet.front().toElement());
    result.pushBack(classId);
  }

  void
  XemProcessor::xemFunctionGetObjectDefinition(__XProcFunctionArgs__)
  {
    KeyId classId;
    if (args.size() == 0)
      {
        ElementRef eThis =
            getXProcessor().getVariable(__builtin.nons.this_())->toElement();
        classId = getElementClass(eThis);
      }
    else if (args.size() == 1)
      {
        NodeSet& nodeSet = *(args[0]);
        if (nodeSet.size() != 1)
          {
            throwException ( Exception, "Error : object-definition() called with multiple results in first argument !\n" );
          }
        if (nodeSet.front().isElement())
          {
            classId = getElementClass(nodeSet.front().toElement());
          }
        else
          {
            try
            {
              String className = nodeSet.front().toString();
              Log_XemObject ( "CALLED object-definition('%s')\n", className.c_str() );
              ElementRef sourceElement = callingXPath.getSourceElementRef();
              classId = getKeyCache().getKeyIdWithElement(sourceElement,className);
            }
            catch ( Exception* e )
            {
              detailException(e, "Could not get class name '%s' because lost trace.\n",
                  nodeSet.front().toString().c_str() );
              throw ( e );
            }
          }
        if (!classId)
          {
            throwException ( Exception, "Error : object-definition() called with invalid class provided !\n" );
          }

      }
    else
      {
        throwException ( Exception, "Error : object-definition() called with multiple arguments !\n" );
      }
    ElementRef objectDefinition = getObjectDefinition(classId);
    if (!objectDefinition)
      {
        throwException ( Exception, "Could not get object definition for class='%s'\n", getKeyCache().dumpKey(classId).c_str() );
      }
    Log_XemObject ( "Returning class=%s (%x) => %s\n", getKeyCache().dumpKey(classId).c_str(), classId,
        objectDefinition.generateVersatileXPath().c_str() );
    result.pushBack(objectDefinition);
  }

  void
  XemProcessor::xemInstructionParam(__XProcHandlerArgs__)
  {
  }

  void
  XemProcessor::xemInstruction_triggerEvent(__XProcHandlerArgs__)
  {
    KeyId eventId = item.getAttrAsKeyId(getXProcessor(), xem.name());
    KeyIdList arguments;
    for ( ChildIterator param(item) ; param ; param ++ )
      {
        if ( param.getKeyId() != xem.with_param() ) continue;
        KeyId paramId = param.getAttrAsKeyId(xem.name());
#if 0
        if ( arguments.find(paramId) != arguments.end() )
          {
            throwException ( Excpetion, "Duplicate param '%s'\n", getKeyCache().dumpKey(paramId).c_str() );
          }
#endif
        getXProcessor().processInstructionVariable(param,xem.name(),xem.select(),false);
        arguments.push_back(paramId);
      }
    getXProcessor().triggerEvent(eventId,arguments);
  }

}
;

