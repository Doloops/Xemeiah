#include <Xemeiah/xemprocessor/xemprocessor.h>
#include <Xemeiah/xemprocessor/xemobjectmodule.h>

#include <Xemeiah/xprocessor/xprocessor.h>
#include <Xemeiah/xpath/xpath.h>
#include <Xemeiah/xpath/xpathparser.h>

#include <Xemeiah/dom/elementmapref.h>
#include <Xemeiah/xemprocessor/xemmoduleforge.h>

#include <Xemeiah/auto-inline.hpp>

namespace Xem
{
  SKMapHash XemProcessor::hashMethodId ( KeyId classId, KeyId methodId )
  {
    return ( (__ui64) classId << 16 ) + KeyCache::getLocalKeyId(methodId);
  }

  void XemProcessor::domEventObject ( DomEventType domEventType, ElementRef& domEventElement, NodeRef& nodeRef )
  {
    if ( domEventType == DomEventType_CreateElement )
      {
        ElementRef object = nodeRef.toElement();
        ElementRef rootElement = object.getRootElement();
        KeyId classId = object.getAttrAsKeyId ( xem.name() );

        ElementMapRef objectMap = rootElement.findAttr(xem.object_map(), AttributeType_SKMap );
        if ( ! objectMap ) objectMap = rootElement.addSKMap ( xem.object_map(), SKMapType_ElementMap );

        objectMap.put ( classId, object );
      }
    else
      {
        NotImplemented("DomEvent : %x\n", domEventType);
      }
  }

  void XemProcessor::domEventMethod ( DomEventType domEventType, ElementRef& domEventElement, NodeRef& nodeRef )
  {
    if ( domEventType == DomEventType_CreateElement )
      {
        ElementRef method = nodeRef.toElement();
        ElementRef rootElement = method.getRootElement();
        KeyId classId = method.getFather().getAttrAsKeyId ( xem.name() );

        KeyId methodId = method.getAttrAsKeyId ( xem.name() );

        if ( KeyCache::getNamespaceId(methodId) )
          {
            throwException ( Exception, "Invalid name for a method : '%s'\n", method.getAttr( xem.name() ).c_str() );
          }

        if ( methodId == getKeyCache().getBuiltinKeys().nons.Constructor() )
          {
            ElementMapRef constructorMap = rootElement.findAttr(xem.constructor_map(), AttributeType_SKMap );
            if (! constructorMap ) constructorMap = rootElement.addSKMap ( xem.constructor_map(), SKMapType_ElementMap );

            constructorMap.put ( classId, method );
          }
        ElementMapRef methodMap = rootElement.findAttr(xem.method_map(), AttributeType_SKMap );
        if ( ! methodMap ) methodMap = rootElement.addSKMap ( xem.method_map(), SKMapType_ElementMap );

        SKMapHash hash = hashMethodId ( classId, methodId );

        methodMap.put ( hash, method );
      }
    else
      {
        NotImplemented("DomEvent : %x\n", domEventType);

      }
  }

  void XemProcessor::domEventFunction ( DomEventType domEventType, ElementRef& domEventElement, NodeRef& nodeRef )
  {
    if ( domEventType == DomEventType_CreateElement )
      {
        ElementRef function = nodeRef.toElement();
        ElementRef rootElement = function.getRootElement();
        KeyId classId = function.getFather().getAttrAsKeyId ( xem.name() );

        KeyId functionId = function.getAttrAsKeyId ( xem.name() );

        if ( KeyCache::getNamespaceId(functionId) )
          {
            throwException ( Exception, "Invalid name for a function : '%s'\n", function.getAttr( xem.name() ).c_str() );
          }

        ElementMapRef functionMap = rootElement.findAttr(xem.function_map(), AttributeType_SKMap );
        if ( ! functionMap ) functionMap = rootElement.addSKMap ( xem.function_map(), SKMapType_ElementMap );

        SKMapHash hash = hashMethodId ( classId, functionId );

        functionMap.put ( hash, function);
      }
    else
      {
        NotImplemented("DomEvent : %x\n", domEventType);
      }
  }

};

