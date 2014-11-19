#include <Xemeiah/nodeflow/nodeflow-dom.h>
#include <Xemeiah/xprocessor/xprocessor.h>
#include <Xemeiah/dom/attributeref.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_NodeFlowDom Debug
#define Log_NodeFlowDom_NS Debug

namespace Xem
{
  NodeFlowDom::NodeFlowDom ( XProcessor& xproc, const ElementRef& elementRef )
  : NodeFlow(xproc), baseElement(elementRef), currentElement(elementRef)
  {
    allowAttributes = false;
    forbidAttributeCreationOutsideOfElements = false;
    Log_NodeFlowDom ( "New NodeFlowDom at %p\n", this );
  }

  NodeFlowDom::~NodeFlowDom ()
  {
    Log_NodeFlowDom ( "Delete NodeFlowDom at %p\n", this );
  }

  KeyCache& NodeFlowDom::getKeyCache ()
  {
    return currentElement.getKeyCache();
  }
  void NodeFlowDom::serializeDocType ( KeyId rootKeyId )
  {
    if ( docTypePublic.size() || docTypeSystem.size() )
      {
        String textDocType = "<!DOCTYPE ";
        if ( outputMethod == OutputMethod_HTML )
          textDocType += "HTML";
        else
          {
            if ( KeyCache::getNamespaceId(rootKeyId) && KeyCache::getNamespaceId(rootKeyId) != __builtin.xhtml.ns() )
              {
                throwException ( Exception, "Invalid : outputting a DOCTYPE with a namespace set !\n" );
              }
            textDocType += getKeyCache().getLocalKey ( KeyCache::getLocalKeyId(rootKeyId) );
          }

        if ( docTypePublic.size() )
          {
            textDocType += " PUBLIC";
            textDocType += " \"";
            textDocType += docTypePublic;
            textDocType += "\"";
          }
        else
          {
            textDocType += " SYSTEM";
          }

        if ( docTypeSystem.size() )
          {
            textDocType += " \"";
            textDocType += docTypeSystem;
            textDocType += "\"";
          }
        textDocType += ">\n";
        Log_NodeFlowDom ( "DOCTYPE : '%s'\n", textDocType.c_str() );
        ElementRef docTypeDeclaration = baseElement.getDocument().createTextNode ( baseElement, textDocType.c_str() );
        docTypeDeclaration.setDisableOutputEscaping ();
        baseElement.insertChild ( docTypeDeclaration );
      }

  }

  void NodeFlowDom::newElement ( KeyId keyId, bool forceDefaultNamespace )
  {
    if ( isRestrictToText() ) return;

    if ( baseElement == currentElement )
      {
        requalifyOutput(keyId);
        serializeDocType(keyId);
      }
    
    ElementRef newElement = currentElement.getDocument().createElement ( currentElement, keyId );
    currentElement.appendLastChild ( newElement );
    currentElement = newElement;
    
    if ( KeyCache::getNamespaceId(keyId) == 0 
        && newElement.getNamespaceAlias ( getKeyCache().getBuiltinKeys().nons.xmlns(), true ) )
      {
        newElement.addNamespaceAlias ( getKeyCache().getBuiltinKeys().nons.xmlns(), 0 );
      }
      
    while ( ! anticipatedNamespaces.empty() )
      {
        AnticipatedNamespace aNs = anticipatedNamespaces.front ();
        anticipatedNamespaces.pop_front ();

        Log_NodeFlowDom_NS ( "Set anticipated (%s) : %s => %s\n",
            newElement.generateVersatileXPath().c_str(),
            getKeyCache().dumpKey(aNs.nsDeclaration).c_str(), getKeyCache().getNamespaceURL(aNs.nsId));
        newElement.addNamespaceAlias ( aNs.nsDeclaration, aNs.nsId );
      }
      
    allowAttributes = true;
  }
  
  void NodeFlowDom::newAttribute ( KeyId keyId, const String& value )
  {
    if ( isRestrictToText() ) return;

    if ( !allowAttributes && forbidAttributeCreationOutsideOfElements )
      {
        Warn ( "Illegal attribute here !\n" );
        return;
      }

    if ( mustTriggerEvents() )
      currentElement.addAttr ( xprocessor, keyId, value );
    else
      currentElement.addAttr ( keyId, value );
  }
  
  void NodeFlowDom::setNamespacePrefix ( KeyId keyId, NamespaceId nsId, bool anticipated )
  {
    if ( isRestrictToText() ) return;
    
    Log_NodeFlowDom_NS ( "[NSALIAS] currentElement=%s, Setting : %s (%x) -> %s (%x) %s\n",
        currentElement.generateVersatileXPath().c_str(),
        getKeyCache().getLocalKey(keyId), keyId, 
        getKeyCache().getNamespaceURL(nsId), nsId, anticipated ? "(anticipated)" : "" );

    if ( anticipated )
      {
        if ( currentElement.getNamespaceAlias ( keyId, true ) == nsId )
          {
            Log_NodeFlowDom_NS ( "[NSALIAS][DUPLICATE] Already set : %s (%x) -> %s (%x)\n",
                getKeyCache().getLocalKey(keyId), keyId, 
                getKeyCache().getNamespaceURL(nsId), nsId );          
            return;
          }
        for ( std::list<AnticipatedNamespace>::iterator iter = anticipatedNamespaces.begin() ; iter != anticipatedNamespaces.end() ; iter++ )
          {
            if ( iter->nsDeclaration == keyId )
              {
                Log_NodeFlowDom_NS ( "[NSALIAS][DUPLICATE] Already set in anticipated : %s (%x) -> %s (%x)\n",
                    getKeyCache().getLocalKey(keyId), keyId, 
                    getKeyCache().getNamespaceURL(nsId), nsId );
                if ( iter->nsId != nsId )
                  {
                    Log_NodeFlowDom_NS ( "[NSALIAS][DUPLICATE][DIVERGING] Diverging ! Already set as ns %s (%x)\n",
                        getKeyCache().getNamespaceURL(iter->nsId), iter->nsId );
                  }
              }
          }
        AnticipatedNamespace aNs = { keyId, nsId };
        anticipatedNamespaces.push_back ( aNs );
        Log_NodeFlowDom_NS ( "At %s : anticipated : nsPrefix '%s' (%x) -> '%s' (%x)\n",
            currentElement.generateVersatileXPath().c_str(),
            getKeyCache().getLocalKey ( KeyCache::getLocalKeyId(keyId) ), keyId,
            getKeyCache().getNamespaceURL ( nsId ), nsId );
        return;
      }
    if ( ! allowAttributes )
      {
        Log_NodeFlowDom_NS ( "Illegal namespace attribute here (not anticpated) !\n" );
        return;
      }
    Log_NodeFlowDom_NS ( "Add : %s => %s\n", getKeyCache().dumpKey(keyId).c_str(), getKeyCache().getNamespaceURL(nsId) );
    currentElement.addNamespaceAlias ( keyId, nsId );
  }
  
  void NodeFlowDom::removeAnticipatedNamespacePrefix ( KeyId keyId )
  {
    for ( std::list<AnticipatedNamespace>::iterator iter = anticipatedNamespaces.begin () ;
        iter != anticipatedNamespaces.end() ; iter++ )
      {
        if ( iter->nsDeclaration != keyId ) continue;
        anticipatedNamespaces.erase ( iter );
        Log_NodeFlowDom ( "Remove anticipated %x\n", keyId );
        return;
      }
    Log_NodeFlowDom ( "Did not find anticipated %x\n", keyId );
  }
  
  void NodeFlowDom::removeAnticipatedNamespaceId ( NamespaceId nsId )
  {
    for ( std::list<AnticipatedNamespace>::iterator iter = anticipatedNamespaces.begin () ;
        iter != anticipatedNamespaces.end() ; iter++ )
      {
        if ( iter->nsId != nsId ) continue;
        anticipatedNamespaces.erase ( iter );
        Log_NodeFlowDom ( "Remove anticipated %x\n", nsId );
        return;
      }
    Log_NodeFlowDom ( "Did not find anticipated %x\n", nsId );
  }

  NamespaceId NodeFlowDom::getDefaultNamespaceId ()
  {
    return currentElement.getNamespaceAlias ( getKeyCache().getBuiltinKeys().nons.xmlns(), true );
  }
  
  LocalKeyId NodeFlowDom::getNamespacePrefix ( NamespaceId namespaceId )
  {
    return currentElement.getNamespacePrefix ( namespaceId, true );
  }

  NamespaceId NodeFlowDom::getNamespaceIdFromPrefix ( LocalKeyId prefix, bool recursive )
  {
    Log_NodeFlowDom_NS ( "=> finding from %s\n", currentElement.generateVersatileXPath().c_str() );
    KeyId declarationId = getKeyCache().buildNamespaceDeclaration ( prefix );
    return currentElement.getNamespaceAlias ( declarationId, recursive );
  }
  
  bool NodeFlowDom::isSafeAttributePrefix ( LocalKeyId prefixId, NamespaceId nsId )
  {
    LocalKeyId currentNsId = getNamespaceIdFromPrefix ( prefixId, true );
    if ( currentNsId == 0 )
      {
        KeyId declarationId = getKeyCache().buildNamespaceDeclaration ( prefixId );
        setNamespacePrefix ( declarationId, nsId, false );
        Log_NodeFlowDom_NS ( "=> Not set !\n" );
        return true;
      }
    else if ( currentNsId == nsId )
      {
        Log_NodeFlowDom_NS ( "=> Already set !\n" );
        return true;
      }
    AssertBug ( currentNsId && currentNsId != nsId, "Not Diverging ?\n" );
    Log_NodeFlowDom_NS ( "Prefix already set : prefix='%s', already mapped to '%s' (whereas namespace shall be '%s')\n",
        getKeyCache().getLocalKey(prefixId),
        getKeyCache().getNamespaceURL(currentNsId),
        getKeyCache().getNamespaceURL(nsId) );

    LocalKeyId currentLevelNsId = getNamespaceIdFromPrefix ( prefixId, false );
    if ( currentLevelNsId && currentLevelNsId != nsId )
      {
        /*
         * Set at this element, so we can't use it
         */
        Log_NodeFlowDom_NS ( "=> set at this level !\n" );
        return false;
      }
    if ( currentElement.getNamespaceId() == currentNsId )
      {
        Log_NodeFlowDom_NS ( "=> set for this element !\n" );
        return false;
      }
    Log_NodeFlowDom_NS ( "=> Safe, setting this !\n" );
    KeyId declarationId = getKeyCache().buildNamespaceDeclaration ( prefixId );
    setNamespacePrefix ( declarationId, nsId, false );
    return true;
  }

  void NodeFlowDom::elementEnd ( KeyId keyId )
  {  
    if ( isRestrictToText() ) return;
    
    while ( ! anticipatedNamespaces.empty() )
      {
        AnticipatedNamespace aNs = anticipatedNamespaces.front ();
        anticipatedNamespaces.pop_front ();
        
        currentElement.addNamespaceAlias ( aNs.nsDeclaration, aNs.nsId );
      }
    if ( mustTriggerEvents() )
      currentElement.eventElement ( xprocessor, DomEventType_CreateElement );
    currentElement = currentElement.getFather ();
    allowAttributes = false;
  }
  
  void NodeFlowDom::appendText ( const String& text, bool disableOutputEscaping )
  {
    /*
     * First, elude cases with an empty string
     */
    if ( ! text.c_str() || ! text.c_str()[0] ) return;
    Log_NodeFlowDom ( "NodeFlowDom::appendText : APPEND '%s'\n", text.c_str() );
    ElementRef lastChild = currentElement.getLastChild ();
    if ( ! lastChild || ! lastChild.isText() )
      {
        Log_NodeFlowDom ( "APPEND --> NewString\n" );
        ElementRef newElement = currentElement.getDocument().createTextNode ( currentElement, text.c_str() );
        if ( disableOutputEscaping )
          {
            newElement.setDisableOutputEscaping ( disableOutputEscaping );
          }
        else if ( isCDataSectionElement ( currentElement.getKeyId() ) )
          {
            newElement.setWrapCData ();
          }
        currentElement.appendLastChild ( newElement );
      }
    else
      {
        if ( disableOutputEscaping && ! lastChild.getDisableOutputEscaping() )
          {
            Warn ( "At appendText() : disabling output escaping on a text continuation.\n" );
          }
        Log_NodeFlowDom ( "APPEND --> Concatenating with existing string '%s'.\n", lastChild.getText().c_str() );
        lastChild.appendText ( text );
      }  
    allowAttributes = false;
  }
  
  void NodeFlowDom::newComment ( const String& comment )
  {
    if ( isRestrictToText() ) return;
    
    ElementRef newElement = currentElement.getDocument().createCommentNode ( currentElement, comment.c_str() );
    currentElement.appendLastChild ( newElement );
    allowAttributes = false;
  }
  
  void NodeFlowDom::newPI ( const String& name, const String& contents )
  {
    if ( isRestrictToText() ) return;
    
    ElementRef newElement = currentElement.getDocument().createPINode ( currentElement, name.c_str(), contents.c_str() );
    currentElement.appendLastChild ( newElement );
    allowAttributes = false;
  }
  
  ElementRef NodeFlowDom::getCurrentElement()
  {
    return currentElement;
  }
};

