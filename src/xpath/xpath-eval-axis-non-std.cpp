#include <Xemeiah/xpath/xpath.h>
#include <Xemeiah/xprocessor/xprocessor.h>
#include <Xemeiah/trace.h>
#include <Xemeiah/dom/blobref.h>

#include <Xemeiah/auto-inline.hpp>

/*
 * Non-standard Axes
 */
#define Log_Eval Debug

namespace Xem
{
    void
    XPath::evalAxisRoot ( __XPath_Functor_Args)
    {
        ElementRef current(node);
        ElementRef root = current.getRootElement();
        evalStep(result, root, step->nextStep);
    }

    void
    XPath::evalAxisHome ( __XPath_Functor_Args)
    {
        NotImplemented("Home.\n");
    }

    void
    XPath::evalAxisVariable ( __XPath_Functor_Args)
    {
        Log_Eval("XPathVariable : Getting variable keyid=%x\n", step->keyId);
        if (!xproc.hasVariable(step->keyId))
        {
            XPathUndefinedVariableException * xpe = new XPathUndefinedVariableException();
            // xpe->setSilent();
            xpe->undefinedVariable = step->keyId;
            detailException(xpe, "Environment has no variable %x '%s'\n", step->keyId,
                            xproc.getKeyCache().dumpKey(step->keyId).c_str());
            xproc.dumpEnv(xpe, true);
            throw(xpe);
        }
        NodeSet* variable = xproc.getVariable(step->keyId);
        AssertBug(variable != NULL, "NULL variable !\n");
        Log_Eval ( "[XPATHAXIS_VAR] variable size=%lu, %s\n", (unsigned long) variable->size(), step->nextStep == XPathStepId_NULL ? "terminal" : "continue" );
        evalNodeSetPredicate(result, *variable, step);
    }

    void
    XPath::evalAxisResource ( __XPath_Functor_Args)
    {
        /*
         * Be sure that the resource will be available after destruction of XPath
         * This is useless when resource is part of a comparator,
         * but it is necessary when xpath takes the form "'resource'".
         */
        if (temporaryParserForReadOnlyDocuments)
        {
            String str = stringFromAllocedStr(strdup(getResource(step->resource)));
            result.setSingleton(str);
            return;
        }
        /*
         * Optimize resource when the XPath is loaded from Store
         */
        String str = getResource(step->resource);

        result.setSingleton(str);
    }

    void
    XPath::evalAxisConstInteger ( __XPath_Functor_Args)
    {
        result.setSingleton(step->constInteger);
    }

    void
    XPath::evalAxisConstNumber ( __XPath_Functor_Args)
    {
        result.setSingleton(((Number) step->constNumber));
    }

    void
    XPath::evalAxisBlob ( __XPath_Functor_Args)
    {
        if (step->keyId == 0 || KeyCache::getLocalKeyId(step->keyId) == 0)
        {
            throwXPathException("Invalid key descriptor !\n");
        }
        BlobRef blobRef = node.toElement().findAttr(step->keyId, AttributeType_Blob);
        if (!blobRef)
            return;
        Log_Eval ( "evalAxisBlob() : selected '%s'\n", blobRef.generateVersatileXPath().c_str() );
        if (step->nextStep == XPathStepId_NULL)
            result.pushBack(blobRef, false);
        else
            evalStep(result, blobRef, step->nextStep);
    }
}
;
