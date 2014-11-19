#include <Xemeiah/xpath/xpath.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_XPath_InitStatic Debug

namespace Xem
{
  XPath::Functor XPath::actionMap[256];
  
  void XPath::initStatic ()
  {
    static bool isStaticInited = false;
    AssertBug ( ! isStaticInited, "XPath static initialization already occured !\n" );
    isStaticInited = true;
    Log_XPath_InitStatic ( "actionMap at %p\n", actionMap );

    memset ( actionMap, 0, sizeof ( actionMap ) );

    Log_XPath_InitStatic ( "Building axis map ! \n" );
#define __XPath_Axis(__name,__id)					\
    actionMap[XPathAxis_##__id] = &XPath::evalAxis##__id
#include <Xemeiah/xpath/xpath-axis.h>
#undef __XPath_Axis

    Log_XPath_InitStatic ( "Building comparator map !\n" );
    actionMap[XPathComparator_Equals] = &XPath::evalActionComparator;
    actionMap[XPathComparator_NotEquals] = &XPath::evalActionComparator;
    actionMap[XPathComparator_LessThan] = &XPath::evalActionComparator;
    actionMap[XPathComparator_LessThanOrEquals] = &XPath::evalActionComparator;
    actionMap[XPathComparator_GreaterThan] = &XPath::evalActionComparator;
    actionMap[XPathComparator_GreaterThanOrEquals] = &XPath::evalActionComparator;

    Log_XPath_InitStatic ( "Building function map !\n" );
#define __XPath_Func(__name,__func,__cardinality,__defaultsToSelf)  \
    actionMap[XPathFunc_##__func] = &XPath::evalFunction##__func;

#include <Xemeiah/xpath/xpath-funcs.h>
#undef __XPath_Func
  } 

  void XPath::evalFunctionXSLNumbering ( __XPath_Functor_Args ) {}
  void XPath::evalFunctionXSLNumbering_SingleCharacterConverter ( __XPath_Functor_Args ) {}
  void XPath::evalFunctionXSLNumbering_IntegerConverter ( __XPath_Functor_Args ) {}

  class XPathStaticInit
  {
  public:
    XPathStaticInit() { XPath::initStatic(); }
  };

  XPathStaticInit __XPathStaticInit;
};
