#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/kern/store.h>
#include <Xemeiah/kern/document.h>
#include <Xemeiah/kern/format/journal.h>
#include <Xemeiah/dom/documentmeta.h>
#include <Xemeiah/dom/childiterator.h>
#include <Xemeiah/dom/attributeref.h>
#include <Xemeiah/xpath/xpath.h>
#include <Xemeiah/xprocessor/xprocessor.h> // For xsl:strip-spaces stuff.

#include <Xemeiah/auto-inline.hpp>

#undef __builtinKey
#define __builtinKey(__key) getKeyCache().getBuiltinKeys().__key()

#ifdef __XEM_DOM_ELEMENTREF_CHECKELEMENT
#define __checkElement(eltSeg) do { getStore().checkElement ( eltSeg ) } while (0)
#else
#define __checkElement(eltSeg) do {} while (0)
#endif

#define Log_Element Debug


namespace Xem
{
  String ElementRef::generateId ()
  {
    String id;
    stringPrintf ( id, "Ei-%s.%llx", getDocument().getRole().c_str(),
      getElementId() ? getElementId() : getElementPtr() );
    return id;
  }

  /*
   * Position retrieving functions.
   * \deprecated this is deprecated.
   */
  Integer ElementRef::getPosition()
  {
    Integer pos = 1;
    for ( ElementRef elder = getElder() ; elder ; elder = elder.getElder() )
      pos++;
    return pos;  
  }

  Integer ElementRef::getLastPosition()
  {
    // Warning, we may check against the rootElement() here !
    if ( getFather() ) return getFather().getNumberOfChildren();
    return 1;  
  }

  Integer ElementRef::getNumberOfChildren()
  {
    Integer pos = 0;
    for ( ChildIterator child(*this) ; child ; child++ )
      pos++;
    return pos;  
  }

  void ElementRef::rename ( KeyId newKeyId )
  {
    AssertBug ( newKeyId, "Provided a zeroed newKeyId !\n" );
    AssertBug ( getDocument().isWritable(), "Context is not writable !\n" );
    Log_Element ( "[ELEMENT RENAME] Old name : '%x'\n", getKeyId() );
    ElementSegment* me = getMe<Write> ();
    getDocumentAllocator().alter(me);
    me->keyId = newKeyId;
    getDocumentAllocator().protect(me);

    Log_Element ( "[ELEMENT RENAME] New name : '%x'\n", getKeyId() );
  }


  /*
   * *************************** Element Inserting ***************************
   */
  void ElementRef::insertBefore ( ElementRef& elementRef ) // Stub
  {
    AssertBug ( getFather(), "Could not add before on root element !\n" );
    if ( ! getElder() ) getFather().insertChild ( elementRef, false );
    else getElder().insertAfter ( elementRef, false );
    getDocument().appendJournal ( *this, JournalOperation_InsertBefore, elementRef, 0 );
  }

  void ElementRef::appendLastChild ( ElementRef& elementRef ) // Stub
  {
    ElementPtr mePtr = getElementPtr ();
    ElementSegment* me = getMe<Write> ();

#if PARANOID
    __checkElementFlag_HasAttributesAndChildren ( me->flags );
#endif

    ElementPtr newChildPtr = elementRef.getElementPtr();
    // ElementSegment* newChild = elementRef.getMe<Write> (); 
    ElementSegment* newChild = getDocumentAllocator().getSegment<ElementSegment, Write> 
              ( newChildPtr, sizeof(ElementSegment) );
    
    if ( me->attributesAndChildren.lastChildPtr )
      {
        getDocumentAllocator().alter ( newChild );
        newChild->fatherPtr = mePtr;
        newChild->elderPtr = me->attributesAndChildren.lastChildPtr;
        getDocumentAllocator().protect ( newChild );
        
        ElementSegment* lastChild = 
            getDocumentAllocator().getSegment<ElementSegment, Write> 
              ( me->attributesAndChildren.lastChildPtr );
        getDocumentAllocator().alter ( lastChild );
        lastChild->youngerPtr = newChildPtr;
        getDocumentAllocator().protect ( lastChild );
        
        getDocumentAllocator().alter ( me );
        me->attributesAndChildren.lastChildPtr = newChildPtr;
        getDocumentAllocator().protect ( me );
      }
    else
      {
        getDocumentAllocator().alter ( newChild );
        newChild->fatherPtr = mePtr;
        getDocumentAllocator().protect ( newChild );
        
        getDocumentAllocator().alter ( me );
        me->attributesAndChildren.childPtr = newChildPtr;
        me->attributesAndChildren.lastChildPtr = newChildPtr;
        getDocumentAllocator().protect ( me );
      }
    getDocument().appendJournal ( *this, JournalOperation_AppendChild, elementRef, 0 );
  }

  void ElementRef::insertAfter ( ElementRef& elementRef, bool updateJournal )
  {
#if PARANOID
    AssertBug ( getElementPtr(), "Null Pointer given !!!\n" );
#endif

    ElementSegment* me = getMe<Write> ();

#if PARANOID
    AssertBug ( me->fatherPtr, "Could not insertAfter() on root element !\n" );
#endif

    ElementRef fatherRef ( getDocument(), me->fatherPtr );
    
    ElementSegment* myFather = fatherRef.getMe<Read> ();

#if PARANOID
    AssertBug ( myFather, "No father for current node.. !\n" );
#endif

    ElementPtr newYoungerPtr = elementRef.getElementPtr();

    if ( me->youngerPtr )
      {
        ElementSegment* oldYounger = getDocumentAllocator().getElement<Write> ( me->youngerPtr );
        getDocumentAllocator().alter ( oldYounger );
        oldYounger->elderPtr = newYoungerPtr;
        getDocumentAllocator().protect ( oldYounger );
      }
    else
      {
#if PARANOID
        __checkElementFlag_HasAttributesAndChildren ( myFather->flags );
        AssertBug ( getElementPtr() == myFather->attributesAndChildren.lastChildPtr, "I got no younger, but am no lastChild of my own father...\n" );
#endif
        getDocumentAllocator().authorizeWrite ( fatherRef.getElementPtr(), myFather );
        getDocumentAllocator().alter ( myFather );
#if PARANOID
        __checkElementFlag_HasAttributesAndChildren ( myFather->flags );
#endif
        myFather->attributesAndChildren.lastChildPtr = newYoungerPtr;
        getDocumentAllocator().protect ( myFather );
      }

    ElementSegment* newYounger = elementRef.getMe<Write> ();

    getDocumentAllocator().alter ( newYounger );
    newYounger->fatherPtr = me->fatherPtr;
    newYounger->elderPtr = getElementPtr(); 

    if ( me->youngerPtr ) newYounger->youngerPtr = me->youngerPtr;
    getDocumentAllocator().protect ( newYounger );

    getDocumentAllocator().alter ( me );
    me->youngerPtr = newYoungerPtr;
    getDocumentAllocator().protect ( me );

    if ( updateJournal )
      getDocument().appendJournal ( *this, JournalOperation_InsertAfter, elementRef, 0 );

  }
  
  void ElementRef::insertChild ( ElementRef& elementRef, bool updateJournal )
  {
    AssertBug ( getElementPtr(), "Null Pointer given !!!\n" );

    ElementPtr newChildPtr = elementRef.getElementPtr();
    ElementSegment* me = getMe<Write> ();

    __checkElementFlag_HasAttributesAndChildren ( me->flags );
    ElementPtr oldChildPtr = me->attributesAndChildren.childPtr;

    if ( oldChildPtr )
      {
        __checkElementFlag_HasAttributesAndChildren ( me->flags );
        ElementSegment* oldChild = getDocumentAllocator().getElement<Write> ( me->attributesAndChildren.childPtr );
        AssertBug ( oldChild->fatherPtr == getElementPtr(),
		        "Inconsistency in father pointers : oldChild->father=%llx, me=%llx\n",
		        oldChild->fatherPtr, getElementPtr() );
        AssertBug ( oldChild->elderPtr == 0, "Child has a elder !!!\n" );

        getDocumentAllocator().alter (  oldChild );
        oldChild->elderPtr = newChildPtr;
        getDocumentAllocator().protect (  oldChild );
      }

    ElementSegment* newChild = elementRef.getMe<Write> ();

    getDocumentAllocator().alter ( newChild );
    newChild->fatherPtr = getElementPtr();
    if ( oldChildPtr ) newChild->youngerPtr = oldChildPtr;
    getDocumentAllocator().protect ( newChild );

    getDocumentAllocator().alter ( me );
    __checkElementFlag_HasAttributesAndChildren ( me->flags );
    me->attributesAndChildren.childPtr = newChildPtr; 
    if ( ! me->attributesAndChildren.lastChildPtr ) me->attributesAndChildren.lastChildPtr = newChildPtr; 
    getDocumentAllocator().protect ( me );

    if ( updateJournal )
      getDocument().appendJournal ( *this, JournalOperation_InsertChild, elementRef, 0 );

  }

  /*
   * **************************** Element Deletion ******************************
   */
  void ElementRef::deleteElement ()
  {
    Bug ( "This is deprecated !\n" );
  }

  void ElementRef::unlinkElementFromFather ()
  {
    ElementSegment* me = getMe<Write> ();

    AssertBug ( me->fatherPtr, "Could not deleteElement() on root element !\n" );

    ElementRef fatherRef ( getDocument(), me->fatherPtr );

    ElementSegment* myFather = fatherRef.getMe<Write> ();
    __checkElementFlag_HasAttributesAndChildren ( myFather->flags );

    if ( myFather->attributesAndChildren.childPtr == getElementPtr() )
      {
        getDocumentAllocator().authorizeWrite ( fatherRef.getElementPtr(), myFather );
        getDocumentAllocator().alter ( myFather );
        myFather->attributesAndChildren.childPtr = me->youngerPtr;
        getDocumentAllocator().protect ( myFather );
      }
    if ( myFather->attributesAndChildren.lastChildPtr == getElementPtr() )
      {
        getDocumentAllocator().authorizeWrite ( fatherRef.getElementPtr(), myFather );
        getDocumentAllocator().alter ( myFather );
        myFather->attributesAndChildren.lastChildPtr = me->elderPtr;
        getDocumentAllocator().protect ( myFather );
      }

    // Modify elder
    if ( me->elderPtr )
      {
        ElementSegment* elder = getDocumentAllocator().getElement<Write> ( me->elderPtr );
        getDocumentAllocator().alter ( elder );
        elder->youngerPtr = me->youngerPtr;
        getDocumentAllocator().protect ( elder );
      }
    if ( me->youngerPtr )
      {
        ElementSegment* younger = getDocumentAllocator().getElement<Write> ( me->youngerPtr );
        getDocumentAllocator().alter ( younger );
        younger->elderPtr = me->elderPtr;
        getDocumentAllocator().protect ( younger );
      }

    getDocumentAllocator().alter(me);
    me->fatherPtr = NullPtr;
    me->elderPtr = NullPtr;
    me->youngerPtr = NullPtr;
    getDocumentAllocator().protect(me);
  }

  void ElementRef::deleteChildren ( XProcessor& xproc )
  {
    ElementSegment* me = getMe<Write>();
    __checkElementFlag_HasAttributesAndChildren ( me->flags );

    ElementPtr childPtr = me->attributesAndChildren.childPtr;

    getDocumentAllocator().alter ( me );
    me->attributesAndChildren.childPtr = NullPtr;
    me->attributesAndChildren.lastChildPtr = NullPtr;
    getDocumentAllocator().protect ( me );

    for ( ; childPtr ; )
      {
        ElementRef child ( getDocument(), childPtr );
        childPtr = child.getYounger().getElementPtr();
        child.deleteElement ( xproc );
      }

  }

  void ElementRef::deleteElement ( XProcessor& xproc )
  {
    AssertBug ( getElementPtr(), "Null Pointer given !!!\n" );

    Log_Element ( "[ELT-DLT] Deleting element %llx\n", getElementPtr() );

    ElementSegment* me = getMe<Write>();
    
    if ( me->flags & ElementFlag_HasTextualContents )
      {
        /*
         * Trigger the eventDeleteElement
         */
        eventElement ( xproc, DomEventType_DeleteElement );

        getDocument().appendJournal(*this, JournalOperation_Remove, *this, 0);

        /*
         * This element is a textual contents, so delete contentsPtr if we have one.
         * Otherwise, do nothing, the content is embedded in ElementSegment structure.
         */
        if ( me->textualContents.size > me->textualContents.shortFormatSize )
          {
            getDocumentAllocator().freeSegment ( me->textualContents.contentsPtr, me->textualContents.size );
          }
      }
    else
      {
        /*
         * Delete whole hierarchy
         */
        deleteChildren ( xproc );

        /*
         * Delete my attributes
         */
        deleteAttributes ( xproc );

        /*
         * Trigger the eventDeleteElement
         */
        eventElement ( xproc, DomEventType_DeleteElement );

        /*
         * Remove contents
         */
        getDocument().appendJournal(*this, JournalOperation_Remove, *this, 0);

        /*
         * Finally, we can delete our relationship with our neighbours
         */
        unlinkElementFromFather();
      }      
    getDocument().unIndexElementById ( *this );

    getDocumentAllocator().alter ( me );
    memset ( me, 0, sizeof(ElementSegment) );
    getDocumentAllocator().protect ( me );
    
    getDocumentAllocator().freeSegment ( getElementPtr(), sizeof(ElementSegment) );

    setElementPtr ( NullPtr );
  }

  void ElementRef::appendText ( const String& text )
  {
    AssertBug ( isText() || isPI() || isComment(), "Not a text element !\n" );
    String s = getText();
    s += text;
    Log_Element ( "APPEND at ElementRef::appendText() : result string is '%s'\n", s.c_str() );
    
    setText ( s );
    Log_Element ( "APPEND after addAttr() : getText() is '%s'\n", getText().c_str() );
#if PARANOID
    AssertBug ( s == getText(), "Diverging texts : '%s' != '%s'\n", s.c_str(), getText().c_str() );
#endif
  }

  void ElementRef::copyContents ( XProcessor& xproc, ElementRef& eRef, bool onlyBaseAttributes )
  {
    Log_Element ( "copyContents from %s\n", eRef.generateVersatileXPath().c_str() );
    for ( AttributeRef attr = eRef.getFirstAttr() ; attr; attr = attr.getNext() )
      {
        Log_Element ( "copyContents at %s (k=%x/t=%x)\n", attr.getKey().c_str(), attr.getKeyId(), attr.getAttributeType() );
        if ( onlyBaseAttributes && !attr.isBaseType() )
          continue;
        NamespaceId nsId = attr.getNamespaceId();
        if ( nsId && ! getNamespacePrefix(nsId) )
          {
            addNamespacePrefix(eRef.getNamespacePrefix(nsId),nsId);
          }
        if ( attr.getAttributeType() == AttributeType_SKMap )
          {
            Warn ( "Skipping attribute '%s' : type SKMap\n", attr.generateVersatileXPath().c_str() );
            continue;
          }
        AttributeRef newAttr = copyAttribute(attr);
      }
    for ( ChildIterator child(eRef) ; child ; child++ )
      {
        ElementRef childCopy = getDocument().createElement ( *this, child.getKeyId() );
        appendLastChild ( childCopy );
        /*
         * We have to copy Namespace definition *after* appending,
         * because otherwise it would blur JournaledDocument Journal
         */
        NamespaceId nsId = child.getNamespaceId();
        if ( nsId && ! childCopy.getNamespacePrefix(nsId) )
          {
            childCopy.addNamespacePrefix(child.getNamespacePrefix(nsId),nsId);
          }
        childCopy.copyContents ( xproc, child, onlyBaseAttributes );
      }
  }

  bool ElementRef::mustSkipWhitespace ( XProcessor& xproc )
  {
    if ( ! isText() )       return false;
    if ( ! isWhitespace() ) return false;
    ElementRef father = getFather();
    AssertBug ( father, "no father !\n" );
    AttributeRef xmlSpace = father.findAttr ( __builtinKey(xml.space), AttributeType_String );
    Log_Element ( "[MSWS] : at '%s'\n", father.generateVersatileXPath().c_str() );
    if ( xmlSpace )
      {
        if ( xmlSpace.toString() == "preserve" ) return false;
      }

    return xproc.mustSkipWhitespace ( father.getKeyId() );
  }

  String ElementRef::toString()
  { 
    Log_Element ( "ElementRef::toString ! contents markup='%s'\n", getKey().c_str() );
    if ( isText() || isComment() || isPI() ) return getText();

    ElementRef child = getChild();
    if ( ! child ) return String();
    if ( child.isText() && ! child.getYounger() )
      {
        return child.getText();
      }

    Log_Element ( "Complex toString() : key='%s'\n", getKey().c_str() );

    String result;
    while ( true )
      {
        if ( child.isText() ) result += child.getText();
      	else if ( child.getChild() ) { child = child.getChild(); continue; }
      	while ( ! child.getYounger() )
      	  {
      	    child = child.getFather();
      	    if ( child == *this ) 
      	      {
      	        Log_Element ( "result = '%s'\n", result.c_str() );
      	        return result;
      	      }
      	  }
        child = child.getYounger();
      }
    return result;
  }

  String ElementRef::generateVersatileXPath ( )
  {
    String xpath = getDocument().getDocumentTag();
    if ( ! xpath.size() )
      {
        String brId;
        stringPrintf(brId,"(%p)[%llx:%llx]",&getDocument(),_brid(getDocument().getBranchRevId()));
        xpath += brId;
      }
    if ( !(*this) )
      {
        xpath += "/(#null)";
        return xpath;
      }
    ElementRef root = getRootElement();
    if ( root == *this )
      {
        xpath += "/";
        return xpath;
      }
    std::list<ElementRef> ancestors;
    for ( ElementRef ancestor = *this ; ancestor != root ; ancestor = ancestor.getFather() )
    {
      if ( !ancestor )
        {
          xpath += "(out-of-tree)";
          break;
        }
      ancestors.push_front ( ancestor );
      if ( ancestor == root ) break;
    }
    
    int depth = 0;
    
    while ( ancestors.size() )
    {
      ElementRef ancestor = ancestors.front ();
      ancestors.pop_front ();
      xpath += "/";
      if ( ancestor.getKeyId() == getKeyCache().getBuiltinKeys().xemint.textnode() )
        {
          xpath += "text()";
        }
      else
        {
          xpath += ancestor.getKey();
        }
        
      __ui64 pos = 1;
      for ( ElementRef elder = ancestor.getElder() ; elder ; elder = elder.getElder() )
        if ( elder.getKeyId() == ancestor.getKeyId() )
          pos++;
      xpath += "[";
      char nbStr[32];
      sprintf ( nbStr, "%llu", pos );
      xpath += nbStr;
      xpath += "]";

      if ( ancestor.getKeyId() == getKeyCache().getBuiltinKeys().xemint.textnode() )
        continue;
        
      for ( AttributeRef attrRef = ancestor.getFirstAttr() ; attrRef ; attrRef = attrRef.getNext() )
        {
          // We may filter some here...
          if ( attrRef.getType() != AttributeType_String )
            continue;
          if ( attrRef.getKeyId() == getKeyCache().getBuiltinKeys().xemint.document_base_uri() )
            continue;
          xpath += "[@";
          xpath += attrRef.getKey();
          xpath += "=\"";
          xpath += attrRef.toString ();
          xpath += "\"]";
        }
      depth++;
    }
    return xpath;
  }

  ElementRef ElementRef::lookup ( XProcessor& xproc, KeyId functionId, const String& value )
  {
    ElementRef result ( getDocument() );
    try
    {
      NodeSet nodeSet;
      XPath::evalXProcessorFunction(xproc,functionId,*this,nodeSet,value);

      if ( nodeSet.size() == 1 && nodeSet.front().getItemType() == Item::Type_Element )
        return nodeSet.front().toElement();

      return result;
    }
    catch ( Exception * e )
    {
      Warn ( "Could not lookup : Exception %s\n", e->getMessage().c_str() );
      delete ( e );
    }
    return result;
  }
};
