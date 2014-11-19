#include <Xemeiah/dom/nodeset.h>
#include <Xemeiah/dom/childiterator.h>
#include <Xemeiah/kern/volatiledocument.h>
#include <Xemeiah/xpath/xpath.h>

#include <Xemeiah/auto-inline.hpp>

#include <math.h>

#define Log_NodeSetTextCmp Debug
#define Log_NodeSetNumberCmp Debug
#define Log_NodeSetContains Debug
#define Log_NodeSetDump Debug

namespace Xem
{
  bool NodeSet::contains ( const ElementRef& _elt )
  {
    ElementRef elt = _elt;
    for ( iterator iter(*this) ; iter ; iter++ )
      {
        Log_NodeSetContains ( "[CONTAINS] : Add %llx:%s - at %llx:%s\n",
            elt.getElementId(), elt.getKey().c_str(),
            iter->isElement() ? iter->toElement().getElementId() : 0,
            iter->isElement() ? iter->toElement().getKey().c_str() : "(NA)" );
        if ( iter->isElement() && (iter->toElement() == elt) )
          {
            Log_NodeSetContains ( "[CONTAINS] Duplicate !\n" );
            return true;
          }
      }
    Log_NodeSetContains ( "[CONTAINS] : Clean.\n" );
    return false;  
  }

  bool NodeSet::contains ( const AttributeRef& _attr )
  {
    AttributeRef attr = _attr;
    for ( iterator iter(*this) ; iter ; iter++ )
      {
        Log_NodeSetContains ( "[CONTAINSA] : Add %llx:%s:%s - at %llx:%s:%s\n",
            attr.getElement().getElementId(),
            attr.getElement().getKey().c_str(),
            attr.getKey().c_str(),
            iter->isAttribute() ? iter->toAttribute().getElement().getElementId() : 0,
            iter->isAttribute() ? iter->toAttribute().getElement().getKey().c_str() : "(NA)",
            iter->isAttribute() ? iter->toAttribute().getKey().c_str() : "(NA)" );
        if ( iter->isAttribute() && (iter->toAttribute() == attr) )
          {
            Log_NodeSetContains ( "[CONTAINSA] : Duplicate !\n" );
            return true;
          }
      }
    Log_NodeSetContains ( "[CONTAINSA] : Clean.\n" );
    return false;  
  }

  /**
   * NodeSetTextComparator compares two NodeSet items according to their value converted as as String.
   */
  class NodeSetTextComparator
  {
  protected:
    XPath& sortXPath;
    bool orderAscending;
    bool caseOrderUpperFirst;
  public:
    NodeSetTextComparator (XPath& _sortXPath, bool _orderAscending, bool _caseOrderUpperFirst )
      : sortXPath(_sortXPath)
    {
      orderAscending = _orderAscending;
      caseOrderUpperFirst = _caseOrderUpperFirst;
    }
    ~NodeSetTextComparator() {}

    bool operator() ( Item* left, Item* right )
    {
      NodeRef& leftNode = left->toNode();
      NodeRef& rightNode = right->toNode();
      String leftValue = sortXPath.evalString ( leftNode );
      String rightValue = sortXPath.evalString ( rightNode );
      int result = stringComparator ( leftValue.c_str(), rightValue.c_str(), caseOrderUpperFirst );
      Log_NodeSetTextCmp ( "--> stringComparator returned %d\n", result );
      if ( result == 0 ) return false;
      return orderAscending ? result < 0 : result > 0;
    }
  };

  /**
   * NodeSetNumberComparator compares two NodeSet items according to their value converted as as Number.
   */
  class NodeSetNumberComparator
  {
  protected:
    XPath& sortXPath;
    bool orderAscending;
  public:
    NodeSetNumberComparator ( XPath& _sortXPath, bool _orderAscending )
      : sortXPath(_sortXPath)
    {
      orderAscending = _orderAscending;
    }
    ~NodeSetNumberComparator() {}

    bool operator() ( Item* left, Item* right )
    {
      NodeRef& leftNode = left->toNode();
      NodeRef& rightNode = right->toNode();
      Number leftValue = sortXPath.evalNumber ( leftNode );
      Number rightValue = sortXPath.evalNumber ( rightNode );
      Log_NodeSetNumberCmp ( "Comparing : left=%.32f, right=%.32f\n", leftValue, rightValue );
      if ( isnan(leftValue) )
        {
          leftValue = -INFINITY;
        }
      if ( isnan(rightValue) )
        {
          rightValue = -INFINITY;
        }
      if ( orderAscending )
        return ( leftValue < rightValue );
      else
        return ( leftValue > rightValue );
    }
  };


  void NodeSet::sort ( XPath& sortXPath,
          bool dataTypeText, 
          bool orderAscending,
          bool caseOrderUpperFirst)
  {
    if ( ! list ) return;
    if ( dataTypeText )
      {
        NodeSetTextComparator comparator( sortXPath, orderAscending, caseOrderUpperFirst);
        list->sort ( comparator );
      }
    else
      {
        NodeSetNumberComparator comparator( sortXPath, orderAscending);
        list->sort ( comparator );
      }
  }

  /**
   * Comparator class for sorting a NodeSet in document order.
   */
  class NodeSetDocumentOrderComparator
  {
  protected:
  public:
    NodeSetDocumentOrderComparator()
    {

    }
    ~NodeSetDocumentOrderComparator() {}

    bool operator() ( Item* left, Item* right )
    {
        return left->toNode().isBeforeInDocumentOrder ( right->toNode() );
    }

  };

  void NodeSet::sortInDocumentOrder ( )
  {
    if ( ! list ) return;
    NodeSetDocumentOrderComparator comparator;
    list->sort ( comparator );
  }

  bool NodeSet::checkDocumentOrderness ()
  {
    Item* last = NULL;
    for ( NodeSet::iterator iter(*this) ; iter ; iter++ )
      {
        if ( last )
          {
            if ( ! last->toNode().isBeforeInDocumentOrder ( iter->toNode() ) )
              {
                Warn ( "left item 0x%llx:%s is after right item 0x%llx:%s\n",
                    last->isElement() ? last->toElement().getElementId() : 0,
                    last->toNode().getKey().c_str(),
                    iter->isElement() ? iter->toElement().getElementId() : 0,
                    iter->toNode().getKey().c_str() );
                
              }
          }
        last = &(*iter);
      }
    return true;
  }

  void NodeSet::reverseOrder()
  {
    if ( isSingleton() ) return;
    if ( list ) list->reverse();
  }
  
  void NodeSet::copyTo ( NodeSet& result )
  {
    for ( iterator iter(*this) ; iter ; iter++ )
      {
        if ( iter->isNode() )
          result.pushBack ( iter->toNode(), false );
        else if ( iter->getItemType() == Item::Type_Bool )
          result.setSingleton ( (bool) iter->toBool() );
        else if ( iter->getItemType() == Item::Type_String )
          result.setSingleton ( iter->toString() );
        else if ( iter->getItemType() == Item::Type_Number )
          result.setSingleton ( (Number) iter->toNumber() );
        else if ( iter->getItemType() == Item::Type_Integer )
          result.setSingleton ( (Integer) iter->toInteger() );
        else
          {
            throwException ( Exception, "Invalid/not-handled type %x\n", iter->getItemType() );
          }
      }
  }

  NodeSet* NodeSet::copy ()
  {
    NodeSet* result = new NodeSet();
    copyTo ( *result );    
    return result;
  }

#ifdef __XEM_DOM_NODESET_DUMP
  void NodeSet::log ()
  {
    Log_NodeSetDump ( "NodeSet : '%lu' elements.\n", (unsigned long) size() );
    for ( iterator iter(*this) ; iter ; iter++ )
    {
        Item& item = *iter;
        switch ( item.getItemType() )
        {
        case Item::Type_Element:
            Log_NodeSetDump ( "\tElement %llx '%s'\n", item.toElement().getElementId(), item.toElement().getKey().c_str() );
            break;
        case Item::Type_Attribute:
            Log_NodeSetDump ( "\tAttribute '%s'='%s'\n", item.toNode().getKey().c_str(), item.toString().c_str() );
            break;
        case Item::Type_Bool:
            Log_NodeSetDump ( "\tBool '%s'\n", item.toString().c_str() );
            break;
        case Item::Type_Integer:
            Log_NodeSetDump ( "\tInteger '%s'\n", item.toString().c_str() );
            break;
        case Item::Type_Number:
            Log_NodeSetDump ( "\tNumber '%s' (%g)\n", item.toString().c_str(), item.toNumber() );
            break;
        case Item::Type_String:
            Log_NodeSetDump ( "\tString '%s'\n", item.toString().c_str() );
            break;
        default:
            NotImplemented ( "Item type : '%d'\n", item.getItemType() );
            break;
        }
    }
  
  }
#endif
};

