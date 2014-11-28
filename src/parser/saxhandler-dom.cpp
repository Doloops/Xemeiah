#include <Xemeiah/parser/saxhandler-dom.h>
#include <Xemeiah/dom/string.h>
#include <Xemeiah/dom/attributeref.h>
#include <Xemeiah/xpath/xpath.h>
#include <Xemeiah/xpath/xpathparser.h> //< Used to synthetise DOCTYPE ID attributes XPaths
#include <Xemeiah/xprocessor/xprocessor.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_SHD Debug
#define Warn_SHD Warn

#define __builtinKey(__key) ( getKeyCache().getBuiltinKeys().__key() )

#define __XEM_SHD_TEMP_CREATE_ATTRIBUTES

namespace Xem
{

    SAXHandlerDom::SAXHandlerDom (XProcessor& _xproc, ElementRef& __element) :
            xproc(_xproc), rootElement(__element), currentElement(__element), namespaceAlias(__element.getKeyCache())
    {
        keepTextMode = KeepTextMode_All;
        keepTextAtRootElement = false;
        elementNamespaceWasDeferred = false;
        elementPrefixId = 0;

        parsedAttributes_number = 0;
        parsedAttributes_alloced = 32;
        parsedAttributes = (ParsedAttribute*) malloc(parsedAttributes_alloced * sizeof(ParsedAttribute));
    }

    SAXHandlerDom::~SAXHandlerDom ()
    {
        free(parsedAttributes);
    }

    KeyCache&
    SAXHandlerDom::getKeyCache ()
    {
        return xproc.getKeyCache();
    }

    bool
    SAXHandlerDom::setKeepTextMode (const String& mode)
    {
        if (mode == "all")
            keepTextMode = KeepTextMode_All;
        else if (mode == "normal")
            keepTextMode = KeepTextMode_All;
        else if (mode == "xsl")
        {
            keepTextMode = KeepTextMode_XSL;
            xslTextKeyId = getKeyCache().getKeyId(getKeyCache().getNamespaceId("http://www.w3.org/1999/XSL/Transform"),
                                                  "text", true);

        }
        else if (mode == "none")
            keepTextMode = KeepTextMode_None;
        else
        {
            Warn("Invalid text mode '%s'\n", mode.c_str());
            return false;
        }
        Log_SHD ( "SHD : Setting Text mode to '%s' -> %d\n", mode.c_str(), keepTextMode );
        return true;
    }

    bool
    SAXHandlerDom::setKeepTextAtRootElement (bool value)
    {
        keepTextAtRootElement = value;
        return keepTextAtRootElement;
    }

#ifdef __XEM_SHD_TEMP_CREATE_ATTRIBUTES
    inline SAXHandlerDom::ParsedAttribute*
    SAXHandlerDom::getNewParsedAttribute ()
    {
        if (parsedAttributes_number == parsedAttributes_alloced)
        {
            NotImplemented("Out of place to store ParsedAttribute ! Must realloc here !\n");
        }
        return &(parsedAttributes[parsedAttributes_number++]);
    }
#endif // __XEM_SHD_TEMP_CREATE_ATTRIBUTES

    void
    SAXHandlerDom::dumpContext (Exception* e)
    {
        for (ElementRef parent = currentElement; parent; parent = parent.getFather())
        {
            detailException(e, "\tAt Element : '%s'\n", parent.getKey().c_str());
            for (AttributeRef attr = parent.getFirstAttr(); attr; attr = attr.getNext())
            {
                detailException(e, "\t\tAttribute %s=\"%s\"\n", attr.getKey().c_str(), attr.toString().c_str());
            }
            if (parent == rootElement)
                break;
        }
    }

    void
    SAXHandlerDom::eventElement (const char* ns, const char *name)
    {
        Log_SHD ( "New element : '%s:%s'\n", ns ? ns : "", name );
        namespaceAlias.push();
#if PARANOID
        AssertBug ( elementPrefixId == 0, "elementPrefixId already defined !\n" );
        AssertBug ( elementNamespaceWasDeferred == 0, "elementPrefixId already defined !\n" );
#ifdef __XEM_SHD_TEMP_CREATE_ATTRIBUTES
        AssertBug ( parsedAttributes_number == 0, "parsedAttributes already defined !\n" );
#endif // __XEM_SHD_TEMP_CREATE_ATTRIBUTES
#endif
        LocalKeyId localKeyId = getKeyCache().getKeyId(0, name, true);
        NamespaceId nsId = 0;
        /*
         * Check our element prefix.
         */
        if (ns && *ns)
        {
            elementPrefixId = getKeyCache().getKeyId(0, ns, true);

            if ((nsId = namespaceAlias.getNamespaceIdFromPrefix(elementPrefixId)) == 0)
            {
                Log_SHD ( "Could not get namespace for prefix '%s' yet, deferring namespace lookup.\n", ns );
                elementNamespaceWasDeferred = true;
            }
        }
        else
        {
            nsId = namespaceAlias.getDefaultNamespaceId();
        }
        KeyId keyId = getKeyCache().getKeyId(nsId, localKeyId);

        Log_SHD ( "\t==> keyId = %x\n", keyId );

        ElementRef newChild = currentElement.getDocument().createElement(currentElement, keyId);
        currentElement.appendLastChild(newChild);
        currentElement = newChild;
    }

    void
    SAXHandlerDom::eventAttr (const char* ns, const char *name, const char *value)
    {
        Log_SHD ( "New attr '%s:%s'='%s'\n", ns ? ns : "", name, value );
        /*
         * Local part of the key
         */
        LocalKeyId localKeyId = getKeyCache().getKeyId(0, name, true);
        /*
         * Prefix of the key (ns value).
         */
        LocalKeyId prefixId = 0;
        /*
         * Resolved namespace.
         */
        NamespaceId nsId = 0;
        /*
         * Target namespace for xmlns:(namespace) and xmlns attributes.
         */
        NamespaceId targetNamespaceId = 0;

        /*
         * Check if this attribute has a namespace
         */
        if (ns && *ns)
        {
            prefixId = getKeyCache().getKeyId(0, ns, true);
            if (prefixId == getKeyCache().getBuiltinKeys().nons.xmlns())
                nsId = getKeyCache().getBuiltinKeys().xmlns.ns();
            else
                nsId = namespaceAlias.getNamespaceIdFromPrefix(prefixId);
        }
        if (nsId == getKeyCache().getBuiltinKeys().xmlns.ns())
        {
            KeyId declarationId = getKeyCache().getKeyId(nsId, localKeyId);
            targetNamespaceId = getKeyCache().getNamespaceId(value);
            AssertBug(targetNamespaceId, "Invalid null namespace : '%s'\n", value);
            namespaceAlias.setNamespacePrefix(declarationId, targetNamespaceId);
            if (localKeyId == elementPrefixId)
            {
                /*
                 * Here we rename the element.
                 * The element prefix resolving may have been deferred (elementNamespaceWasDeferred=true), change it.
                 * The element may already have been resolved in a wrong namespace, also change it.
                 */
                LocalKeyId elementLocalKeyId = getKeyCache().getLocalKeyId(currentElement.getKeyId());
                KeyId elementKeyId = getKeyCache().getKeyId(targetNamespaceId, elementLocalKeyId);
                currentElement.rename(elementKeyId);
                if (!elementNamespaceWasDeferred)
                {
                    Log_SHD ( "Element has changed namespaces : now '%x'\n", targetNamespaceId );
                }
                else
                {
                    Log_SHD ( "Element namespace was resolved : now '%x'\n", targetNamespaceId );
                }
                elementNamespaceWasDeferred = false;
            }
            currentElement.addNamespaceAlias(declarationId, targetNamespaceId);
            return;
        }
        else if (localKeyId == getKeyCache().getBuiltinKeys().nons.xmlns())
        {
            targetNamespaceId = currentElement.getKeyCache().getNamespaceId(value);
            namespaceAlias.setDefaultNamespaceId(targetNamespaceId);
            if (!elementPrefixId)
            {
                /*
                 * We have to rename the element.
                 * The element may already have been assigned a namespace because of a previous default namespace declaration,
                 * but as long as it has no elementPrefixId, it is legitimate for us to rename it.
                 */
                KeyId keyId = getKeyCache().getKeyId(targetNamespaceId,
                                                     getKeyCache().getLocalKeyId(currentElement.getKeyId()));
                currentElement.rename(keyId);
            }
            currentElement.addNamespaceAlias(localKeyId, targetNamespaceId);
            Log_SHD ( "Set default targetNamespaceId to '%x'\n", targetNamespaceId );
            return;
        }

        KeyId attrKeyId = KeyCache::getKeyId(nsId, localKeyId);

#ifdef __XEM_SHD_TEMP_CREATE_ATTRIBUTES
        ParsedAttribute* parsedAttribute = getNewParsedAttribute();
        parsedAttribute->prefixId = prefixId;
        parsedAttribute->localKeyId = localKeyId;

        AttributeRef attrRef(currentElement.getDocument(), currentElement.getAllocationProfile(), attrKeyId, value);
        parsedAttribute->attrPtr = attrRef.getAttributePtr();
#else
        currentElement.addAttr ( attrKeyId, value );
#endif
    }

    void
    SAXHandlerDom::eventAttrEnd ()
    {
        Log_SHD ( "eventAttrEnd for '%s'\n", currentElement.getKey().c_str() );
#ifdef __XEM_SHD_TEMP_CREATE_ATTRIBUTES
        if (elementNamespaceWasDeferred)
        {
            throwException(UndefinedNamespaceException, "Could not resolve element namespace prefix : '%s'\n",
                           getKeyCache().dumpKey(elementPrefixId).c_str());
        }
        for (__ui32 parsedAttributes_index = 0; parsedAttributes_index < parsedAttributes_number;
                parsedAttributes_index++)
        {
            ParsedAttribute* parsedAttribute = &(parsedAttributes[parsedAttributes_index]);
            AttributeRef attrRef(currentElement.getDocument(), NullPtr, parsedAttribute->attrPtr);
            NamespaceId nsId = 0;
            if (parsedAttribute->prefixId)
            {
                nsId = namespaceAlias.getNamespaceIdFromPrefix(parsedAttribute->prefixId);
                if (!nsId)
                {
                    throwException(UndefinedNamespaceException, "Could not resolve element namespace prefix : '%s'\n",
                                   getKeyCache().dumpKey(parsedAttribute->prefixId).c_str());
                }
            }
            KeyId attrKeyId = KeyCache::getKeyId(nsId, parsedAttribute->localKeyId);
            attrRef.rename(attrKeyId);
            currentElement.addAttr(attrRef);
            currentElement.eventAttribute(xproc, DomEventType_CreateAttribute, attrRef);
        }
        parsedAttributes_number = 0;
#endif // __XEM_SHD_TEMP_CREATE_ATTRIBUTES

        elementNamespaceWasDeferred = false;
        elementPrefixId = 0;
    }

    void
    SAXHandlerDom::eventElementEnd (const char* ns, const char* name)
    {
        Log_SHD ( "Event end '%s:%s'\n", ns ? ns : "", name );
        /*
         * I am permissive : if the parse gives NULL, I don't test anything.
         */
        if (name)
        {
            NamespaceId nsId = 0;
            if (ns && *ns)
            {
                LocalKeyId prefixId = getKeyCache().getKeyId(0, ns, false);
                if (!prefixId)
                {
                    throwException(Exception, "Could not get LocalKeyId from prefix '%s'\n", ns);
                }
                nsId = namespaceAlias.getNamespaceIdFromPrefix(prefixId);
                if (!nsId)
                {
                    throwException(Exception, "Could not get NamespaceId from prefix '%s'\n", ns);
                }
            }
            else
            {
                nsId = namespaceAlias.getDefaultNamespaceId();
            }
            LocalKeyId localKeyId = getKeyCache().getKeyId(0, name, false);

            KeyId keyId = getKeyCache().getKeyId(nsId, localKeyId);
            if (keyId != currentElement.getKeyId())
            {
                EventHandlerException * e = new EventHandlerException();
                detailException(e, "End element did not match ! I am on %x:'%s', parser tells %x:'%s:%s'\n",
                                currentElement.getKeyId(),
                                currentElement.getKeyCache().getKey(namespaceAlias, currentElement.getKeyId()), keyId,
                                ns ? ns : "", name);
                dumpContext(e);
                throw(e);
            }
        }
        namespaceAlias.pop();
        currentElement.eventElement(xproc, DomEventType_CreateElement);

        if (!currentElement.getFather())
        {
            throwException(EventHandlerException, "Element has no father !\n");
        }
        currentElement = currentElement.getFather();

    }

    inline bool
    string_isspace (const char* text)
    {
        bool sp = true;
        for (const char* c = text; *c; c++)
            if (!isspace(*c))
            {
                sp = false;
                break;
            }
        return sp;
    }

    void
    SAXHandlerDom::eventText (const char *text)
    {
        Log_SHD ( "eventText [%s]\n", text );
        if (rootElement == currentElement && !keepTextAtRootElement)
        {
            bool sp = string_isspace(text);
            if (!sp)
            {
                Warn_SHD ( "At root element : Skipping text [%s]\n", text );
                if ( *text == '>' ) Bug ( "." );
            }
            return;
        }
        if (keepTextMode != KeepTextMode_All)
        {
#define __isKey(__key) ( currentElement.getKeyId() == __builtinKey ( __key ) )

            bool sp = string_isspace(text);
            if (sp)
            {
                if (__isKey(xml.text))
                {
                    goto eventText_ImportSpacedText;
                }
                AttributeRef xmlSpace = currentElement.findAttr(__builtinKey(xml.space), AttributeType_String);
                if (xmlSpace && xmlSpace.toString() == "preserve")
                    goto eventText_ImportSpacedText;

                if (keepTextMode == KeepTextMode_XSL && xslTextKeyId && currentElement.getKeyId() == xslTextKeyId) // __isKey(xsl.text) )
                {
                    goto eventText_ImportSpacedText;
                }
                if (keepTextMode == KeepTextMode_None)
                {
                }
                Log_SHD ( "Skipping text [%s] (keepTextMode=%d, key=%s)\n",
                        text, keepTextMode, currentElement.getKey().c_str() );
                return;
            }
        }
        eventText_ImportSpacedText:
        Log_SHD ( "New text '%s'\n", text );
        ElementRef textNode = currentElement.getDocument().createTextNode(currentElement, text);
        currentElement.appendLastChild(textNode);
    }

    void
    SAXHandlerDom::eventComment (const char *comment)
    {
        Log_SHD ( "eventComment [%s]\n", comment );
        ElementRef commentNode = currentElement.getDocument().createCommentNode(currentElement, comment);
        currentElement.appendLastChild(commentNode);
    }

    void
    SAXHandlerDom::eventProcessingInstruction (const char* name, const char * contents)
    {
        Log_SHD ( "Inserting PI '%s'='%s'\n", name, contents );
        if (strcmp(name, "xml") == 0)
        {
            Log_SHD ( "Skipping XML PI\n" );
            return;
        }
        AssertBug(name, "PI Created with no name !\n");
        AssertBug(name[0], "Empty PI name !\n");

        ElementRef piNode = currentElement.getDocument().createPINode(currentElement, name, contents);
        currentElement.appendLastChild(piNode);
    }

    void
    SAXHandlerDom::eventEntity (const char* entityName, const char* entityValue)
    {
        Log_SHD ( "ENTITY : '%s' = '%s'\n", entityName, entityValue );
        currentElement.getDocument().setUnparsedEntity(entityName, entityValue);
    }

    void
    SAXHandlerDom::eventNDataEntity (const char* entityName, const char* ndata)
    {
        Log_SHD ( "ENTITY NDATA : '%s' = '%s'\n", entityName, ndata );
    }

    void
    SAXHandlerDom::eventDoctypeMarkupDecl (const char* markupName, const char* value)
    {
        Log_SHD ( "DOCTYPE markup : '%s' [%s]\n", markupName, value );
        if (strcmp(markupName, "ATTLIST") == 0)
        {
            std::list<String> tokens;
            String(value).tokenize(tokens);

            String elementName = tokens.front();
            tokens.pop_front();

            Log_SHD ( "DOCTYPE ATTLIST : Element '%s'\n", elementName.c_str() );

            while (tokens.size())
            {
                String attributeName = tokens.front();
                tokens.pop_front();

                String attributeType = tokens.front();
                tokens.pop_front();

                String defaultDecl = tokens.front();
                tokens.pop_front();

                String attrValue;
                if (defaultDecl == "#FIXED"
                        || (tokens.size() && (tokens.front().at(0) == '"' || tokens.front().at(0) == '\'')))
                {
                    attrValue = tokens.front();
                    tokens.pop_front();
                }

                Log_SHD ( "DOCTYPE ATTLIST : Element='%s', Attribute='%s', type='%s', decl='%s', value='%s'\n",
                        elementName.c_str(), attributeName.c_str(), attributeType.c_str(), defaultDecl.c_str(), attrValue.c_str()
                );

                if (attributeType == "ID")
                {
                    if (rootElement.hasAttr(getKeyCache().getBuiltinKeys().xemint.id_match(), AttributeType_XPath))
                    {
                        Log_SHD ( "Already have an ID specificator !\n" );
                    }
                    else
                    {
                        Log_SHD ( "Set attribute 'id' as ID specificator !\n" );
                        NamespaceAlias nsAlias(getKeyCache());
                        XPathParser xpathIdMatch(getKeyCache(), nsAlias, "*[@id]", false);
                        XPathParser xpathIdUse(getKeyCache(), nsAlias, "@id", false);
                        xpathIdMatch.saveToStore(rootElement, getKeyCache().getBuiltinKeys().xemint.id_match());
                        xpathIdUse.saveToStore(rootElement, getKeyCache().getBuiltinKeys().xemint.id_use());
                    }
                }
            }
        }
    }

    void
    SAXHandlerDom::parsingFinished ()
    {
        Log_SHD ( "End of parsing : rootElement = %s, currentElement = %s\n",
                rootElement.getKey().c_str(), currentElement.getKey().c_str() );
        if (rootElement != currentElement)
        {
            EventHandlerException* e = new EventHandlerException();
            detailException(e, "At end of parsing : unclosed markups !\n");
            dumpContext(e);
            throw(e);
        }
    }
}
