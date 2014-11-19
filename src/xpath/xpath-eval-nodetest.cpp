#include <Xemeiah/xpath/xpath.h>
#include <Xemeiah/xprocessor/xprocessor.h>
#include <Xemeiah/trace.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_Eval Debug

namespace Xem
{
  /**
   * Evaluate the predicate-part of an Axis Step.
   * Predicates are only filtering operations, which take as input the NodeSet computed from the Axis and pre-filtered by the Key comparison,
   * and returns a NodeSet filtered by the different levels of predicates.
   * Each level of predicate has its own operating nodeset (result of the n-1 predicate).
   * @param xproc Environment - Evaluation Context.
   * @param targetNodeSet The final NodeSet for the XPath.
   * @param nodeset The nodeset to operate on, i.e. the resulting nodeset of the Axis + Key evaluation, as an input of the predicate.
   * @param step the current XPathStep we are evaluating.
   * @param revertNodeSet May revert the NodeSet after predicate filtering.
   */
  void XPath::evalNodeSetPredicate (  NodeSet& targetNodeSet, NodeSet& nodeset, XPathStep* step, bool revertNodeSet )
  {
    NodeSet *iterateNodeSet = &nodeset;
    bool mustFreeIterateNodeSet = false;

#ifdef __XEM_XPATH_LOG_NODESET
    Log_Eval ( "evalNodeSetPredicate : preliminary nodeset is :\n" );
    nodeset.log ();
#endif //   __XEM_XPATH_LOG_NODESET
    for ( int predicateId = 0 ; step->predicates[predicateId] != XPathStepId_NULL ; predicateId++ )
      {
        NodeSet* myNodeSet = new NodeSet();
        Log_Eval ( "* At predicateId = '%d'\n", predicateId );
        for ( NodeSet::iterator iter(*iterateNodeSet,xproc) ; iter ; iter++ )
          {
            if ( ! iter->isNode() )
              {
                NotImplemented ( "Predicate on a non-node element !\n" );
              }
            NodeSet predicateNodeSet; evalStep ( predicateNodeSet, iter->toNode(), step->predicates[predicateId] );
                            
            if ( predicateNodeSet.isNumber() || predicateNodeSet.isInteger() )
              {
                Integer askedPosition = predicateNodeSet.toInteger();
                Log_Eval ( "Position predicate : pos = %lld, asked = %lld\n", iter.getPosition(), askedPosition );
                if ( askedPosition != iter.getPosition() )
                    continue;
              }
            else
              {
                Log_Eval ( "Non-positionnal predicate. nodeset = '%d'\n",
                predicateNodeSet.toBool() );
                if ( ! predicateNodeSet.toBool() )
                  continue;
              }

            myNodeSet->pushBack ( iter->toNode(), false );
          }
#ifdef __XEM_XPATH_LOG_NODESET
        Log_Eval ( "After predicate %d : myNodeSet is :\n", predicateId );
        myNodeSet->log ();
#endif // __XEM_XPATH_LOG_NODESET
        if ( mustFreeIterateNodeSet ) delete ( iterateNodeSet );
        iterateNodeSet = myNodeSet;
        mustFreeIterateNodeSet = true;
        if ( predicateId == XPathStep_MaxPredicates ) break;
      }


    if ( revertNodeSet )
        iterateNodeSet->reverseOrder ();
#if PARANOID
    iterateNodeSet->checkDocumentOrderness ();
#endif
#ifdef __XEM_XPATH_LOG_NODESET
    Log_Eval ( "Final iterateNodeSet is :\n" );
    iterateNodeSet->log ();
#endif // __XEM_XPATH_LOG_NODESET
    if ( iterateNodeSet->isSingleton() )
      {
        switch ( iterateNodeSet->getSingleton()->getItemType() )
        {
        case Item::Type_String:
            targetNodeSet.setSingleton ( iterateNodeSet->getSingleton()->toString() );
            break;
        case Item::Type_Integer:
            targetNodeSet.setSingleton ( iterateNodeSet->getSingleton()->toInteger() );
            break;
        case Item::Type_Bool:
            targetNodeSet.setSingleton ( iterateNodeSet->getSingleton()->toBool() );
            break;
        case Item::Type_Number:
            targetNodeSet.setSingleton ( iterateNodeSet->getSingleton()->toNumber() );
            break;
        case Item::Type_Element:
            targetNodeSet.setSingleton ( iterateNodeSet->getSingleton()->toElement() );
            break;
        case Item::Type_Attribute:
            targetNodeSet.setSingleton ( iterateNodeSet->getSingleton()->toAttribute() );
            break;
        default:
            NotImplemented ( "Singleton type %d\n", iterateNodeSet->getSingleton()->getItemType() );
        }    
      }
    else
      {
        for ( NodeSet::iterator iter(*iterateNodeSet,xproc) ; iter ; iter++ )
          {
            switch ( iter->getItemType() )
            {
            case Item::Type_Element:
            case Item::Type_Attribute:
                evalStep ( targetNodeSet, iter->toNode(), step->nextStep );
                break;
            default:
                throwXPathException ( "Post-Predicate nodeset(size=%lu) is not a Node ! type=%d, value='%s'\n", 
                  (unsigned long) iterateNodeSet->size(),
                  iter->getItemType(), iter->toString().c_str() );
                AssertBug ( step->nextStep == XPathStepId_NULL, "Non-node item, but step has a next !\n" );
                break;
            }
          }
      }
    if ( mustFreeIterateNodeSet ) delete ( iterateNodeSet );
  }
#undef __keyCache
};

