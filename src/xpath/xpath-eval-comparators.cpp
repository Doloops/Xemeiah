#include <Xemeiah/xpath/xpath.h>
#include <Xemeiah/xprocessor/xprocessor.h>

#include <Xemeiah/auto-inline.hpp>

namespace Xem
{

// #define __XEM_XPATH_COMPARATOR_CONSTINTEGER_SHORTCUT //< Option : Shortcut when operator is constInteger

#define Log_XPathComparator Debug

#define __bothOperandsHaveType(__type) ( ( leftItem.getItemType() == __type ) && ( rightItem.getItemType() == __type ) )
#define __oneOperandHasType(__type) ( ( leftItem.getItemType() == __type ) || ( rightItem.getItemType() == __type ) )

#if 0

     Log_XPathComparator ( "True types : left='" __printfSwitch "',right='" __printfSwitch "'\n", \
      leftItem.to##__typeT () __postOp, rightItem.to##__typeT () __postOp );
      
#endif

#define __doCompareTyped(__typeT,__operand,__printfSwitch, __leftOp, __rightOp,__postOp) \
     result = (leftItem.to##__typeT () __operand rightItem.to##__typeT ()); \
     Log_XPathComparator ( "Comparing with operand='%s', type='%s', result='%d'\n", #__operand,#__typeT, result ); \
     Log_XPathComparator ( "Left='%s' (sz %lu), Right='%s' (sz %lu)\n", leftItem.toString().c_str(), leftItem.toString().size(), \
        rightItem.toString().c_str(), rightItem.toString().size() ); \
     return result;

#define __doCompareType(__typeT,__operand,__printfSwitch, __leftOp, __rightOp,__postOp) \
   case Item::Type_##__typeT: \
    __doCompareTyped ( __typeT, __operand, __printSwitch, __leftOp, __rightOp, __postOp )
    
#define __doCompare(__operand, __type, __leftOp, __rightOp) \
   do { switch ( __type ) { \
	__doCompareType(Bool,__operand,"%d",__leftOp, __rightOp,);  \
	__doCompareType(Integer,__operand,"%lld",__leftOp, __rightOp,);  \
	__doCompareType(Number,__operand,"%g",__leftOp, __rightOp,);  \
	__doCompareType(String,__operand,"%s",__leftOp, __rightOp,.c_str()); \
	default: Bug ( "Invalid type '%d'\n", type ); \
    } } while (0)
    
#define __doCompareOp(__func,__operand, __type, __leftOp, __rightOp) \
    else if ( __func == comparator ) { __doCompare(__operand, __type, __leftOp, __rightOp); }

#define __doCompareOpList(__type, __leftOp, __rightOp) \
    if ( 0 ) {} \
    __doCompareOp ( XPathComparator_Equals, ==,              __type, __leftOp, __rightOp ) \
    __doCompareOp ( XPathComparator_NotEquals, !=,           __type, __leftOp, __rightOp ) \
    __doCompareOp ( XPathComparator_LessThan, <,             __type, __leftOp, __rightOp ) \
    __doCompareOp ( XPathComparator_LessThanOrEquals, <=,    __type, __leftOp, __rightOp ) \
    __doCompareOp ( XPathComparator_GreaterThan, >,          __type, __leftOp, __rightOp ) \
    __doCompareOp ( XPathComparator_GreaterThanOrEquals, >=, __type, __leftOp, __rightOp )

  /*
   * Put the result of comparison in result.
   * If the loop must stop return true.
   * If the loop must continue return false.
   */
  bool XPath::evalComparatorHelper ( bool& result, XPathAction comparator, Item& leftItem, Item& rightItem )
  {
    /*
     * First, qualify the ItemType to use for comparison
     */ 
    Item::ItemType type = Item::Type_Null; 
    if ( ( comparator == XPathComparator_Equals ) && ( __bothOperandsHaveType ( Item::Type_Element ) ) )
      {
        result = (leftItem.toElement() == rightItem.toElement());
        if (result) return result;
        /*
         * TODO : This is flawed : we must be able to compare element contents ! Check this better.
         */
        result = (leftItem.toString() == rightItem.toString());
        Log_XPathComparator ( "Compare elements as strings : left='%s', right='%s', res=%d\n",
            leftItem.toString().c_str(), rightItem.toString().c_str(), result );
        return result;
      }
    else if ( ( comparator == XPathComparator_Equals ) && ( __bothOperandsHaveType ( Item::Type_Attribute ) ) )
      {
        result = ( leftItem.toAttribute() == rightItem.toAttribute() ); if ( result ) return result;
      }
    if ( __bothOperandsHaveType(Item::Type_Integer) ) type = Item::Type_Integer;
    else if ( __oneOperandHasType(Item::Type_Bool) && 
      ( comparator == XPathComparator_Equals || comparator == XPathComparator_NotEquals) ) type = Item::Type_Bool;
    else if ( __oneOperandHasType(Item::Type_Number) ) type = Item::Type_Number;
    else if ( __oneOperandHasType(Item::Type_Integer) ) type = Item::Type_Number;
    else if ( __oneOperandHasType(Item::Type_String) ) type = Item::Type_String;
    else
      {
        type = Item::Type_String;
      }

    __doCompareOpList ( type, leftItem, rightItem );
    throwXPathException ( "Invalid comparison func : '%d'\n", comparator );
    
    return false;
  }


  bool XPath::evalComparator ( XPathAction comparator, NodeRef& node, XPathStepId leftStepId, XPathStepId rightStepId )
  {
    NodeSet leftNodeSet, rightNodeSet;
    
    evalStep ( leftNodeSet, node, leftStepId );

#ifdef __XEM_XPATH_COMPARATOR_CONSTINTEGER_SHORTCUT
    XPathStep* rightStep = getStep ( rightStepId );
    
    if ( rightStep->action == XPathAxis_ConstInteger )
      {
        ItemImpl<Integer> leftInteger = leftNodeSet.toInteger();
        ItemImpl<Integer> rightInteger = rightStep->constInteger;
        bool result = false;
        evalComparatorHelper ( result, comparator, leftInteger, rightInteger );
        return result;
      }
#endif // __XEM_XPATH_COMPARATOR_CONSTINTEGER_SHORTCUT

    evalStep ( rightNodeSet, node, rightStepId );

    if ( leftNodeSet.size() == 0 && rightNodeSet.size() == 0 )
      {
        return false;
      }
    if ( leftNodeSet.size() == 0 || rightNodeSet.size() == 0 )
     {
        Log_XPathComparator ( "Empty result for left has %lu, right has %lu (expr='%s')\n",
            (unsigned long) leftNodeSet.size(), (unsigned long) rightNodeSet.size(), expression );
        NodeSet& nonEmptyNodeSet = leftNodeSet.size() ? leftNodeSet : rightNodeSet;
        
        AssertBug ( nonEmptyNodeSet.size(), "Internal bug : non-empty nodeSet is empty...\n" );
        
        if ( nonEmptyNodeSet.size() > 1 )
          return false;
          
        /**
         * Ok, now we know that nonEmptyNodeSet is a singleton. Try to check against this.
         */
        Item& item = nonEmptyNodeSet.front ();
        switch ( item.getItemType() )
        {
        case Item::Type_Bool:
          {
            bool leftOp = leftNodeSet.toBool ();
            bool rightOp = rightNodeSet.toBool ();
            switch ( comparator )
            {
            case XPathComparator_Equals: return leftOp == rightOp;
            case XPathComparator_NotEquals: return leftOp != rightOp;
            case XPathComparator_LessThan: return leftOp < rightOp;
            case XPathComparator_LessThanOrEquals: return leftOp <= rightOp;
            case XPathComparator_GreaterThan: return leftOp > rightOp;
            case XPathComparator_GreaterThanOrEquals: return leftOp >= rightOp;
            }
          }
        default:
          return false;
        }
      }
    bool result = false;
    for ( NodeSet::iterator leftIter(leftNodeSet) ; leftIter ; leftIter++ )
      for ( NodeSet::iterator rightIter(rightNodeSet) ; rightIter; rightIter++ )
        {
          evalComparatorHelper ( result, comparator, *leftIter, *rightIter );
          if ( result )
            return result;
        }
    return result;
  }

#undef __doCompareOp
#undef __doCompare
#undef __oneOperandHasType

  void XPath::evalActionComparator ( __XPath_Functor_Args )
  {
    Log_XPathComparator ( "At expression : '%s'\n", expression );
    bool boolResult = evalComparator ( step->action, node, step->functionArguments[0], step->functionArguments[1] );
    Log_XPathComparator ( "=> Final result=%s\n", boolResult ? "true" : "false");
    result.setSingleton ( boolResult );
  }

};

