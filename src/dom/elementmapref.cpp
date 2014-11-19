#include <Xemeiah/dom/elementmapref.h>
#include <Xemeiah/dom/nodeset.h>
#include <Xemeiah/xpath/xpath.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_EMR Debug

namespace Xem
{
  void ElementMultiMapRef::insert ( ElementRef& eltRef, XPath& useXPath )
  {
    NodeSet useNodeSet;
    useXPath.eval ( useNodeSet, eltRef );
    if ( useNodeSet.size() == 0 )
      {
        Log_EMR ( "Empty use xpath '%s' for element '%s'\n", useXPath.getExpression(),
            eltRef.generateVersatileXPath().c_str() );
        return;
      }
    for ( NodeSet::iterator iter ( useNodeSet ) ; iter ; iter++ )
      {
        String value = iter->toString();
        __ui64 hash = hashString ( value );

        Log_EMR ( "Element 0x%llx:%s matches, value='%s', hash='0x%llx'\n", 
            eltRef.getElementId(), eltRef.getKey().c_str(), value.c_str(), hash );

        put ( hash, eltRef );
      }            
  }

  void ElementMultiMapRef::remove ( ElementRef& eltRef, XPath& useXPath )
  {
    NodeSet useNodeSet;
    useXPath.eval ( useNodeSet, eltRef );
    if ( useNodeSet.size() == 0 )
      {
        Log_EMR ( "Empty use xpath '%s' for element '%s'\n", useXPath.getExpression(),
            eltRef.generateVersatileXPath().c_str() );
        return;
      }
    for ( NodeSet::iterator iter ( useNodeSet ) ; iter ; iter++ )
      {
        String value = iter->toString();
        __ui64 hash = hashString ( value );

        Log_EMR ( "Element 0x%llx:%s matches, value='%s', hash='0x%llx'\n", 
            eltRef.getElementId(), eltRef.getKey().c_str(), value.c_str(), hash );

        remove ( hash, eltRef );
      }            
  }
  
};


