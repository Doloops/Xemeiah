#include <Xemeiah/xpath/xpath.h>
#include <Xemeiah/xprocessor/xprocessor.h>
#include <Xemeiah/dom/elementmapref.h>
#include <Xemeiah/dom/documentmeta.h>

#include <Xemeiah/auto-inline.hpp>

#include <Xemeiah/version.h>


// #define __XEM_XPATH_FUNCTIONCALL_DELEGATE
#define Log_XpathEvalFunctionCall Debug

namespace Xem
{
  XPath::FunctionArguments::FunctionArguments()
  {
  
  }
  
  XPath::FunctionArguments::~FunctionArguments()
  {
    for ( size_type idx = 0 ; idx < size() ; idx++ )
      {
        delete ( (*this)[idx] );
      }
  }


  void XPath::evalFunctionArguments ( FunctionArguments& functionArguments, NodeRef& node, XPathStep * step )
  {
    for ( int arg = 0 ; arg < XPathStep_MaxFuncCallArgs ; arg++ )
      {
        if ( step->functionArguments[arg] == XPathStepId_NULL )
          break;
        NodeSet* nodeSet = new NodeSet();
        functionArguments.push_back ( nodeSet );
    
        evalStep (*nodeSet, node, step->functionArguments[arg] );
      }        
  }

  void XPath::evalFunctionFunctionCall ( __XPath_Functor_Args )
  {
    Log_XpathEvalFunctionCall ( "Calling XPath External function on %s (%x).\n",
        node.getStore().getKeyCache().dumpKey(step->keyId).c_str(),
        step->keyId );

    bool isElementFunction = (step->action == XPathFunc_ElementFunctionCall);

    XProcessor::XProcessorFunction functor = xproc.getXProcessorFunction ( step->keyId );
    if ( ! functor )
      {
        throwException ( Exception, "Could not get function for '%s':%s (%x) (%s).\n",
            xproc.getKeyCache().getNamespaceURL ( KeyCache::getNamespaceId ( step->keyId ) ),
            xproc.getKeyCache().getLocalKey ( KeyCache::getLocalKeyId ( step->keyId ) ),
            step->keyId, isElementFunction ? "Element function" : "Function" );
      }

    FunctionArguments functionArguments;

    NodeSet* tempResult = NULL;
    NodeSet* myResult = NULL;

    if ( step->nextStep != XPathStepId_NULL )
      {
        tempResult = new NodeSet();
        myResult = tempResult;
      }
    else
      {
        myResult = &result;
      }

    Exception* exception = NULL;
    try
      {
        evalFunctionArguments ( functionArguments, node, step );
        (functor.module->*functor.hook) ( *this, step->keyId, *myResult, node, functionArguments, isElementFunction );
      }
    catch ( Exception * e )
      {
        exception = e;      
      }
    
    /**
     * Throwing exception if any
     */
    if ( exception )
      {
        if ( tempResult ) delete ( tempResult );
        throw ( exception );
      }

    /**
     * Eval nextStep if any
     */      
    if ( step->nextStep != XPathStepId_NULL )
      {
        for ( NodeSet::iterator iter(*tempResult) ; iter ; iter++ )
          {
            if ( ! iter->isNode() )
              {
                Log_XpathEvalFunctionCall ( "Skipping cause iterator not a node !\n" );
                continue;
              }
            NodeRef& nodeRef = iter->toNode();
            evalStep (result, nodeRef, step->nextStep );
          }
        delete ( tempResult );
      }    
  }

  void XPath::evalFunctionElementFunctionCall ( __XPath_Functor_Args )
  {
    evalFunctionFunctionCall ( result, node, step );
  }

  void XPath::evalXProcessorFunction ( XProcessor& xproc, KeyId functionId, NodeRef& node, NodeSet& result, const String& value )
  {
    FunctionArguments functionArguments;
    NodeSet* nodeSet = new NodeSet();
    functionArguments.push_back ( nodeSet );
    nodeSet->setSingleton(value);

    XProcessor::XProcessorFunction functor = xproc.getXProcessorFunction ( functionId );
    if ( ! functor )
      {
        throwException ( Exception, "Could not get function for %s (%x).\n",
            xproc.getKeyCache().dumpKey(functionId).c_str(), functionId );
      }

    XPath fakeXPath(xproc);
    bool isElementFunction = true;
    (functor.module->*functor.hook) ( fakeXPath, functionId, result, node, functionArguments, isElementFunction );
  }
};
