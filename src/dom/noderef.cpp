#include <Xemeiah/kern/store.h>
#include <Xemeiah/kern/document.h>
#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/dom/attributeref.h>
#include <Xemeiah/xpath/xpath.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_ParseNodeId Debug

namespace Xem
{
    /*
     * Accessor Functions
     */
    ElementRef
    NodeRef::getRootElement ()
    {
#ifdef __XEM_DOM_NODEREF_CHROOT_USING_XEMINT_ROOT
        if (isAttribute())
        {
            ElementRef parent(getDocument(), toAttribute().getParentElementPtr());
            return parent.getRootElement();
        }
        AssertBug(isElement(), "Should be an element here !");
        for (ElementRef ancestor = toElement(); ancestor; ancestor = ancestor.getFather())
        {
            if (ancestor.getKeyId() == getKeyCache().getBuiltinKeys().xemint.root())
            {
                return ancestor;
            }
        }
        return toElement();
#else // __XEM_DOM_NODEREF_CHROOT_USING_XEMINT_ROOT
        return getDocument().getRootElement();
#endif // __XEM_DOM_NODEREF_CHROOT_USING_XEMINT_ROOT
    }

    bool
    NodeRef::isValidNodeId (const String& eltId)
    {
        if (eltId.c_str() && eltId.c_str()[0] == 'E' && eltId.c_str()[1] == 'i' && eltId.c_str()[2] == '-')
            return true;
        return false;
    }

    bool
    NodeRef::parseNodeId (const String& strNodeId, String& role, ElementId& elementId, KeyId& attributeKeyId)
    {
        attributeKeyId = 0;
        if (strNodeId.size() < 3)
        {
            throwException(Exception, "Invalid short NodeId too narrow '%s'\n", strNodeId.c_str());
        }
        if (strchr(strNodeId.c_str(), ':'))
        {
            NotImplemented("Yet : nodeId in Attribute format !\n");
        }
        if (!isValidNodeId(strNodeId))
            return false;

        const char* sx = &(strNodeId.c_str()[3]);

        /*
         * TODO : Replace this by a no-copy role string constructor
         */
        for (; *sx; sx++)
        {
            if (*sx == '.')
                break;
            role += *sx;
        }
        if (!*sx)
        {
            return false;
        }
        sx++;

        char* postP = NULL;
        elementId = strtoull(sx, &postP, 16);

        if (elementId == 0)
        {
            throwException(Exception, "Invalid zero ElementId : '%s' !\n", strNodeId.c_str());
        }

        Log_ParseNodeId ( "NodeId str='%s', role=%s, elementId=%llx\n", strNodeId.c_str(), role.c_str(), elementId );

        if (postP && *postP)
        {
            if (*postP != '!')
            {
                throwException(Exception, "Invalid charater after element : '%c' in '%s'\n", *postP, strNodeId.c_str());
            }
            postP++;
            char* postA = NULL;
            attributeKeyId = strtol(postP, &postA, 16);
            if (attributeKeyId == 0 || (postA && *postA))
            {
                throwException(Exception, "Invalid charater after attribute : '%c' in '%s'\n", *postA,
                               strNodeId.c_str());
            }
        }
        Log_ParseNodeId ( "NodeId str='%s', role=%s, elementId=%llx, attributeKeyId=%x\n", strNodeId.c_str(), role.c_str(), elementId, attributeKeyId );
        return true;
    }

}
;
