#include <Xemeiah/xprocessor/xprocessor.h>
#include <Xemeiah/xpath/xpath.h>
#include <Xemeiah/dom/nodeset.h>
#include <Xemeiah/dom/childiterator.h>
#include <Xemeiah/dom/elementmapref.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_EvalCL Debug

namespace Xem
{
  void XPath::evalFunctionChildLookup ( __XPath_Functor_Args )
  {
    Log_EvalCL ( "----------- CHILD LOOKUP '%s' (args=%u/%u/%u/%u, n=%u) --------------\n", getExpression(),
        step->functionArguments[0], step->functionArguments[1], step->functionArguments[2], 
        step->functionArguments[3], step->nextStep );
        
    NodeSet myResult;
    
    KeyId eltKeyId = evalStepAsKeyId ( node, step->functionArguments[0] );
    XPathStep* leftStep = getStep ( step->functionArguments[1] );
    XPathStep* rightStep = getStep ( step->functionArguments[2] );
    KeyId synthId = 0;
    
    if ( step->functionArguments[3] != XPathStepId_NULL )
      {
        synthId = getStep(step->functionArguments[3])->constInteger;
      }
    
    if ( leftStep->action != XPathAxis_Attribute )
      {
        Bug ( "." );
      }
    if ( rightStep->action != XPathAxis_Variable )
      {
        Bug ( "." );
      }

    NodeSet* variable = xproc.getVariable(rightStep->keyId);
    String s = variable->toString ();
    ElementRef father = node.toElement ();
    
    
    if ( synthId && getXProcessor().getSettings().xpathChildLookupEnabled )
      {
        ElementMultiMapRef map = father.findAttr ( synthId, AttributeType_SKMap );
        if ( map )
          {
            Log_EvalCL ( "Using provided map !\n" );
            SKMapHash hash = SKMapRef::hashString ( s );
            for ( SKMultiMapRef::multi_iterator iter(map, hash) ; iter ; iter++ )
              {
                ElementRef child = map.get ( iter );
                AttributeRef attr = child.findAttr ( leftStep->keyId );
                if ( !attr ) continue;
                if ( s == attr.toString() )
                  {
                    if ( step->predicates[0] != XPathStepId_NULL )
                      {
                        myResult.pushBack ( child );
                      }
                    else if ( step->nextStep != XPathStepId_NULL )
                      {
                        Log_EvalCL ( "Jumping to next %x\n", step->nextStep );
                        evalStep ( result, child, step->nextStep );
                      }
                    else
                      {
                        result.pushBack ( child );
                      }
                  }
              }
            if ( step->predicates[0] != XPathStepId_NULL )
              evalNodeSetPredicate ( result, myResult, step );
            return;
          }
      }
    
    __ui64 totalWalked = 0;
    __ui64 totalMatched = 0;
    for ( ChildIterator child(father) ; child ; child++ )
      {
        totalWalked ++;
        if ( child.getKeyId() != eltKeyId ) continue;
        AttributeRef attr = child.findAttr ( leftStep->keyId );
        if ( !attr ) continue;
        if ( s == attr.toString() )
          {
            totalMatched ++;
            if ( step->predicates[0] != XPathStepId_NULL )
              {
                myResult.pushBack ( child );
              }
            else if ( step->nextStep != XPathStepId_NULL )
              {
                Log_EvalCL ( "Jumping to next %x\n", step->nextStep );
                evalStep ( result, child, step->nextStep );
              }
            else
              {
                result.pushBack ( child );
              }
          }
      }

    if ( step->predicates[0] != XPathStepId_NULL )
      evalNodeSetPredicate ( result, myResult, step );
              
    Log_EvalCL ( "Walked for '%s' : matched/walked=%llu/%llu (synthId=%x, eltId=%x)\n", 
        getExpression(), totalMatched, totalWalked,
        synthId, eltKeyId );

    if ( synthId && getXProcessor().getSettings().xpathChildLookupEnabled &&
         totalWalked >= getXProcessor().getSettings().xpathChildLookupThreshold )
      {
        ElementMultiMapRef map = father.addSKMap ( synthId, SKMapType_ElementMultiMap );
        Log_EvalCL ( "Building childIterator map !\n" );
        for ( ChildIterator child(father) ; child ; child++ )
          {
            totalWalked ++;
            if ( child.getKeyId() != eltKeyId ) continue;
            AttributeRef attr = child.findAttr ( leftStep->keyId );
            if ( !attr ) continue;        
            SKMapHash hash = SKMapRef::hashString ( attr.toString() );
            map.put ( hash, child );          
          }
      }
  
  }

};
