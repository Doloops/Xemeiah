#include <Xemeiah/xpath/xpath.h>
#include <Xemeiah/xpath/xpathparser.h>
#include <Xemeiah/xprocessor/xprocessor.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_XPathFinalStep Debug

namespace Xem
{
  XPath::~XPath ()
  {
    if ( temporaryParserForReadOnlyDocuments )
      delete ( temporaryParserForReadOnlyDocuments );
  }

  void XPath::parseFromAttribute ( ElementRef& ref, KeyId attrKeyId, bool isAVT )
  {
#ifdef __XEM_XPATH_RUNTIME_KEEP_NODEREF
    sourceNodeRef = &ref;
#endif

    if ( ref.getDocument().isWritable() )
      {
        XPathParser xpathParser ( ref, attrKeyId, isAVT );
        AttributeRef attrRef = xpathParser.saveToStore ( ref, attrKeyId );
        loadFromStore ( attrRef );
      }
    else
      {
        temporaryParserForReadOnlyDocuments = new XPathParser ( ref, attrKeyId, isAVT );
        loadFromPacked ( temporaryParserForReadOnlyDocuments->getPackedParsed() );
      }
  }


  AttributeRef XPath::copyTo ( ElementRef& elementRef, KeyId attrKeyId )
  {
    /*
     * TODO : mutualize code with generic copyAttribute()
     */
    for ( int i = 0 ; i < xpathSegment->nbStepId ; i++ )
      {
        XPathStep* step = getStep ( i );
        if ( step->action == XPathAxis_Resource )
          {
            NotImplemented ( "XPath has a resource !\n" );
          }
      }
    /*
     * Code stolen from XPathParser::saveToStore()
     */
    __ui64 nextResourceOffset = 0;
    __ui64 segmentSize = sizeof(XPathSegment) + xpathSegment->nbStepId * sizeof(XPathStep) + nextResourceOffset;

    AttributeRef attrRef = elementRef.addAttr ( attrKeyId, AttributeType_XPath, segmentSize );
    char * data = (char*) attrRef.getData<void,Write> ();
    attrRef.alterData ();
    memcpy ( data, xpathSegment, segmentSize );
    attrRef.protectData ();
    return attrRef;
  }

  ElementRef XPath::getSourceElementRef ()
  {
#ifdef __XEM_XPATH_RUNTIME_KEEP_NODEREF
    if ( ! sourceNodeRef )
      {
        throwXPathException ( "No sourceNodeRef provided !\n" );
      }
    return sourceNodeRef->isElement() ? sourceNodeRef->toElement() : sourceNodeRef->toAttribute().getElement();
#else
    throwXPathException ( "[Not Compiled] XPath does not keep trace of sourceNodeRef !\n" );    
    return *((ElementRef*)NULL);
#endif
  }
  
  
  void XPath::getUnresolvedVariableReferences ( std::list<KeyId>& variableReferences )
  {
    for ( XPathStepId stepId = 0 ; stepId < xpathSegment->nbStepId ; stepId++ )
      {
        XPathStep* step = getStep ( stepId );
        if ( step->action == XPathAxis_Variable )
          {
            KeyId variableKeyId = step->keyId;
            if ( ! xproc.hasVariable ( variableKeyId ) )
              {
                variableReferences.push_back ( variableKeyId );
              }
          }
      }
  }

  void XPath::buildFinalStepPredicate ( XPathFinalStep& finalStep, XPathStep* step )
  {
    if ( step->action == XPathAxis_Attribute )
      {
        finalStep.predicateAttributeIds.push_back ( step->keyId );
      }
    else if ( step->action == XPathFunc_Count
        || step->action == XPathComparator_Equals
        || step->action == XPathComparator_GreaterThan
        || step->action == XPathFunc_BooleanAnd )
      {

      }
    else
      {
        /*
         * TODO : Finish buildFinalStepPredicate() implementation !
         */
        Warn ( "NotImplemented : buildFinalStepPredicate(step->action=%x)\n",
            step->action );
      }
  }

  void XPath::buildFinalSteps ( XPathFinalSteps& finalSteps, XPathStep* step )
  {
    if ( step->action == XPathFunc_Union )
      {
        AssertBug ( step->nextStep == XPathStepId_NULL, "Bug !\n" );
        for ( int a = 0 ; a < XPathStep_MaxFuncCallArgs ; a++ )
          {
            if ( step->functionArguments[a] == XPathStepId_NULL ) break;
            buildFinalSteps ( finalSteps, getStep(step->functionArguments[a]) );
          }      
        return;
      }
    if ( step->nextStep != XPathStepId_NULL )
      {
        buildFinalSteps ( finalSteps, getStep(step->nextStep) );
        return;
      }


    Number priority = computeEffectivePriority ( step );
    Log_XPathFinalStep ( "Final step : action=%x, keyId=%x, p=%f\n", step->action, step->keyId, priority );
    
    XPathFinalStep finalStep;

    finalStep.elementOrAttribute = false;

    if ( step->action == XPathAxis_Attribute || step->action == XPathAxis_Namespace )
      {
        finalStep.keyId = step->keyId;
        finalStep.elementOrAttribute = true;
      }
    else if ( step->action == XPathAxis_Root )
      {
        finalStep.keyId = xproc.getKeyCache().getBuiltinKeys().xemint.root();
      }
    else if ( step->action == XPathFunc_Id || step->action == XPathFunc_Key )
      {
        finalStep.keyId = xproc.getKeyCache().getBuiltinKeys().xemint.element();
      }
    else if ( ( step->action & 0xf0 ) == 0xe0 ) // The Axis operators
      {
        finalStep.keyId = step->keyId;
        for ( int i = 0 ; i < XPathStep_MaxPredicates && step->predicates[i] != XPathStepId_NULL ; i++ )
          {
            XPathStep* pred = getStep(step->predicates[i]);
            buildFinalStepPredicate(finalStep,pred);
          }
      }
    else if ( ( step->action & 0xf0 ) == 0x10
        || ( step->action & 0xf0 ) == 0xa0 ) // All functions
      {
        for ( int i = 0 ; i < XPathStep_MaxFuncCallArgs && step->functionArguments[i] != XPathStepId_NULL ; i++ )
          {
            XPathStep* arg = getStep(step->functionArguments[i]);
            buildFinalSteps(finalSteps,arg);
          }
        return;
      }
    else if ( step->action == XPathAxis_Variable
        || step->action == XPathAxis_Resource
        || step->action == XPathAxis_ConstInteger
        || step->action == XPathAxis_ConstNumber )
      {
        return;
      }
    else
      {
        Bug ( "Not implemented ! action = %x\n", step->action );
        finalStep.keyId = 0;
      }
    finalStep.priority = priority;
    finalSteps.push_back ( finalStep );
  }
  
  
  void XPath::buildFinalSteps ( XPathFinalSteps& finalSteps )
  {
    buildFinalSteps ( finalSteps, getStep(xpathSegment->firstStep) );  
  }
};
  
