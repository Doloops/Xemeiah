#include <Xemeiah/kern/store.h>
#include <Xemeiah/kern/document.h>
#include <Xemeiah/kern/subdocument.h>
#include <Xemeiah/kern/volatiledocument.h>
#include <Xemeiah/trace.h>

#include <Xemeiah/parser/parser.h>

#include <Xemeiah/xprocessor/xprocessor.h>
#include <Xemeiah/xprocessor/env.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_EnvDoc Debug

namespace Xem
{
  void
  Env::setDocument(const String& url, Document& document)
  {
    if (documentMap.find(url) != documentMap.end())
      {
        releaseDocument(url);
      }
    /*
     * We must be really shure of the perenity of the String used as a key.
     */
    String sUrl = stringFromAllocedStr(strdup(url.c_str()));
    documentMap[sUrl] = &document;

    Log_EnvDoc ( "[XEM:ENV] Document %s mapped to %p\n", sUrl.c_str(), &document );
  }

  void
  Env::bindDocument(Document* document, bool bindBehind)
  {
    /*
     * Record this document as being fetched for the entry behind this one
     * Automatic garbage collecting will be before just after.
     */
    EnvEntry* entry = bindBehind ? getBehindEntry() : getHeadEntry();

    if (!entry->bindedDocuments)
      {
        entry->bindedDocuments = new EnvEntry::BindedDocuments();
      }
    document->incrementRefCount();
    Log_EnvDoc ( "Env %p : binding document %p : set refCount=%llx\n",
        this, document, document->getRefCount() );
    entry->bindedDocuments->push_back(document);
  }

  ElementRef
  Env::createVolatileDocument ( bool behind )
  {
    DocumentAllocator& allocator = getCurrentDocumentAllocator(behind);
    Document* document = getStore().createVolatileDocument(allocator);
    bindDocument(document, behind);
    return document->getRootElement();
  }

  ElementRef
  Env::fetchDocumentRoot(const String& effectiveURL)
  {
    bool behind = true;

    ElementRef rootElement = createVolatileDocument(behind);
    // registerEvents (rootElement.getDocument());

#ifdef __XEM_ENV_GETDOCUMENT_TIME
    NTime begin = getntime ();
#endif // __XEM_ENV_GETDOCUMENT_TIME
    String keepTextMode = "normal";

    rootElement.getDocument().setDocumentURI(effectiveURL);
    setDocument(effectiveURL, rootElement.getDocument());

#ifdef __XEM_DOCUMENT_HAS_DOCUMENTTAG
    String documentTag = "document(\"";
    documentTag += effectiveURL;
    documentTag += "\")";

    rootElement.getDocument().setDocumentTag(documentTag);
#endif

#if 1
    if (stringEndsWith(effectiveURL, ".xsl")
        || stringEndsWith(effectiveURL, ".xup")
        || stringEndsWith(effectiveURL, ".xem.xml")
        || stringEndsWith(effectiveURL, ".xupdate.xml") )
      {
        keepTextMode = "xsl";
        registerEvents (rootElement.getDocument());
      }
#endif

    XProcessor& xproc = dynamic_cast<XProcessor&> (*this);
    Parser::parseFile(xproc, rootElement, effectiveURL, keepTextMode );

#ifdef __XEM_ENV_GETDOCUMENT_TIME    
    NTime end = getntime ();
    LogTime ( "Time spent importing : ", begin, end );
#endif // __XEM_ENV_GETDOCUMENT_TIME
    return rootElement;
  }

  ElementRef
  Env::getDocumentRoot(const String& url, const String* baseURI)
  {
    String documentURL = "";
    if (baseURI)
      {
        documentURL = *baseURI;
      }
    else
      {
        documentURL = getBaseURI();
      }
    documentURL += url;

    Log_EnvDoc ( "Effective documentURL = '%s'\n", documentURL.c_str() );

    DocumentMap::iterator iter = documentMap.find(documentURL);
    if (iter != documentMap.end())
      return iter->second->getRootElement();

    Log_EnvDoc ( "--> Document '%s' does not exist in cache, importing.\n", documentURL.c_str() );

    ElementRef rootElement = fetchDocumentRoot(documentURL);

    return rootElement;
  }

  /*
   * Stuff not implemented or deprecated
   * This may come back to the surface one day or another...
   */

  void
  Env::releaseDocument(const String& url)
  {
    NotImplemented ( "Explicit document release is deprecated now !\n" );

    DocumentMap::iterator iter = documentMap.find(url);
    if (iter != documentMap.end())
      {
        Log_EnvDoc ( "Releasing document '%s'\n", url.c_str() );
        getStore().releaseDocument ( iter->second );
        // delete (iter->second);
        documentMap.erase(iter);
      }
    else
      {
        Warn ( "[ENV-RELEASEDOCUMENT] Could not release document : '%s'\n", url.c_str() );
      }
  }

  void
  Env::setDocument(const String& url, ElementRef& documentRoot)
  {
    NotImplemented ( "Env : setDocument() from a subdocument is not implemented !\n" );
  }
}
;

