#include <Xemeiah/dom/nodeset.h>

#include <Xemeiah/dom/item-base.h>

#include <Xemeiah/xprocessor/xprocessor.h>

#include <math.h>

#define Log_NodeSetHPP Debug
#define __XEM_NODESET_FAST_NODEREF_DUPLICATE
// #define __XEM_NODESET_PUSHBACK_LOG_NODESET
// #define __XEM_NODESET_NO_EXPLICIT_ITEM_CLEARCONTENTS

namespace Xem
{

  __INLINE NodeSet::NodeSet()
  {
#ifdef __XEM_DOM_NODESET_HAS_BINDEDDOCUMENT
    bindedDocument = NULL;
#endif

    list = NULL;
    
    singleton = NULL;

#ifdef XEM_XPATH_ITEM_COUNT
    __allNodeSets.push_back ( this );
    XPathNodeSetCountLog_NodeSetHPP ( "NodeSet nbCounts : %lx\n", __allNodeSets.size() );
#endif
  }

  __INLINE NodeSet::~NodeSet()
  {
    clear ();
  }

#ifdef __XEM_DOM_NODESET_HAS_BINDEDDOCUMENT
  __INLINE void NodeSet::bindDocument ( Document* document )
  {
    if ( bindedDocument )
      {
        Bug ( "NodeSet already has a binded context !\n" );
      }
    bindedDocument = document;
  }
#endif

  __INLINE void NodeSet::clear ( bool clearBindedDocument )
  {
    if ( singleton )
      {
#ifndef __XEM_NODESET_NO_EXPLICIT_ITEM_CLEARCONTENTS
        singleton->clearContents();
#endif //  __XEM_NODESET_NO_EXPLICIT_ITEM_CLEARCONTENTS
        singleton = NULL;
      }
    else if ( list )
      {
        /*
         * \todo Think about clearing all contents using clearContents();
         */
#ifndef __XEM_NODESET_NO_EXPLICIT_ITEM_CLEARCONTENTS
        for ( __list::iterator i = list->begin() ; i != list->end() ; i++ )
          {
            (*i)->clearContents ();
          }
#endif //  __XEM_NODESET_NO_EXPLICIT_ITEM_CLEARCONTENTS
        delete ( list );
        list = NULL;
      }
  }

  __INLINE void NodeSet::setAsList ()
  {
    AssertBug ( !isSingleton(), "NodeSet is already set as a singleton !\n" );

    if ( ! list ) 
      {
        list = new __list(defaultPoolAllocator);
        Log_NodeSetHPP ( "For nodeset.h : defaultPoolAllocator at %p\n", &defaultPoolAllocator );
      }
  }
  
  __INLINE bool NodeSet::isSingleton()
  {
    return (singleton != NULL);  
  }
  
  __INLINE Item* NodeSet::getSingleton()
  {
    AssertBug ( isSingleton(), "NodeSet is not a singleton !\n" );
    return singleton;
  }  

#if 1
#define __itemAllocator(__Type,__Source) \
  new(defaultPoolAllocator.alloc(sizeof(__Type))) __Type(__Source)
#else
#define __itemAllocator(__Type,__Source) \
  new __Type(__Source)
#endif

  __INLINE void NodeSet::pushBack ( Integer i ) 
  { 
    if ( size() == 0 ) setSingleton ( i );
    else
      {
        setAsList ();
        list->push_back ( __itemAllocator(ItemImpl<Integer>,i) );
      }
  }

  
  __INLINE void NodeSet::clear ()
  {
    clear ( true );
  }

  
  __INLINE size_t NodeSet::size ()
  {
    if ( isSingleton() ) return 1;
    if ( ! list ) return 0;
    return list->size();
  }

  __INLINE Item& NodeSet::front ()
  {
    if ( isSingleton() ) return *getSingleton();
    AssertBug ( list, "NodeSet : calling front() on an empty NodeSet !\n" );
    return *(list->front());
  }

  __INLINE Item& NodeSet::back ()
  {
    if ( isSingleton() ) return *getSingleton();
    AssertBug ( list, "NodeSet : calling back() on an empty NodeSet !\n" );
    return *(list->back());
  }
  
  /**
   * Iterator stuff.
   */
  __INLINE void NodeSet::iterator::__init()
  {
    if ( result.list ) __iterator = result.list->begin();
    pos = 1;
    iteratedOverSingleton = false;
  }
  __INLINE NodeSet::iterator::iterator(NodeSet& __r)
    : result(__r)
  {
    xproc = NULL;
    __init();
  }

  __INLINE NodeSet::iterator::iterator(NodeSet& __r, XProcessor& __xproc )
    : result(__r)
  {
    __init();
    xproc = &__xproc;
    xproc->pushIterator ( *this );
  }

  __INLINE NodeSet::iterator::~iterator ()
  {
    if ( xproc ) xproc->popIterator ( *this );
  }

  __INLINE void NodeSet::iterator::insert ( Item* item )
  {
    AssertBug ( result.list, "Empty list.\n" );
    result.list->insert ( __iterator, item );  
  }
  
  __INLINE Item* NodeSet::iterator::toItem()
  {
    if ( result.isSingleton() )
    {
        AssertBug ( !iteratedOverSingleton, "Already iterated over singleton !\n" );
        return result.getSingleton();
    }
    AssertBug ( result.list, "Empty list.\n" );
    AssertBug ( __iterator != result.list->end(), "Dereference at end of iterator !\n" );
    return *(__iterator);
  }

  __INLINE NodeSet::iterator::operator bool()
  {
    if ( result.isSingleton() ) return !iteratedOverSingleton;
    if ( ! result.list ) return false;
    AssertBug ( result.list, "Empty list.\n" );
    return (__iterator != result.list->end() );
  }

  __INLINE Item* NodeSet::iterator::operator-> ()
  {
    return toItem();
  }

  __INLINE NodeSet::iterator::operator Item* ()
  {
    return toItem();
  }

  __INLINE NodeSet::iterator& NodeSet::iterator::operator++(int u)
  {
    if ( result.isSingleton() )
      {
        AssertBug ( !iteratedOverSingleton, "Already iterated over singleton !\n" );
        iteratedOverSingleton = true;
        return *this;
      }
    AssertBug ( result.list, "Empty list.\n" );
    __iterator++;
    pos++;
    return *this;
  }
	
  __INLINE Integer NodeSet::iterator::getPosition ()
  {
    return pos;
  }

  __INLINE Integer NodeSet::iterator::getLast ()
  {
    return (result.size());
  }

  /**
   * Node-Set related functions
   */
   
  __INLINE bool NodeSet::insertNodeInDocumentOrder ( NodeRef& nodeRef )
  {
    NodeRef& lastNodeRef = back().toNode();
    Log_NodeSetHPP ( "lastNodeRef=%s (0x%llx), nodeRef=%s (0x%llx)\n",
        lastNodeRef.getKey().c_str(), lastNodeRef.isElement() ? lastNodeRef.toElement().getElementId() : 0,
        nodeRef.getKey().c_str(), nodeRef.isElement() ? nodeRef.toElement().getElementId() : 0 );
    if ( nodeRef == lastNodeRef ) 
      {
        Log_NodeSetHPP ( "Equals to tail !!!\n" );
        return true;
      }
    if ( lastNodeRef.isBeforeInDocumentOrder ( nodeRef ) )
      {
        Log_NodeSetHPP ( "Well sorted !!!\n" );
        return false;
      }
    Log_NodeSetHPP ( "DOM_INSERT_DOC_ORDER_FORCE\n" );
    for ( iterator iter(*this) ; iter ; iter++ )
      {
        Log_NodeSetHPP ( "INSERT : e=%s, iter=%s\n",
            nodeRef.getKey().c_str(), iter->toNode().getKey().c_str() );
        if ( nodeRef == iter->toNode() )
            return true;
        if ( nodeRef.isBeforeInDocumentOrder ( iter->toNode() ) ) // 
          {
            Log_NodeSetHPP ( "is before !\n" );
            Item* newItem;  

            if ( nodeRef.isElement() )
                newItem = __itemAllocator ( ElementRef, nodeRef.toElement() );
            else
                newItem = __itemAllocator ( AttributeRef, nodeRef.toAttribute() );
                
            Log_NodeSetHPP ( "[NS] Insert : newItem=%p\n", newItem );
            iter.insert ( newItem );
            return true;
          }
      }
    Log_NodeSetHPP ( "lastNodeRef=%s is before nodeRef=%s\n",
      lastNodeRef.getKey().c_str(), nodeRef.getKey().c_str() );
    return false;  
  }
    
  __INLINE void NodeSet::pushBack ( const NodeRef& nodeRef, bool insertInDocumentOrder )
  { 
    if ( ! list )
      setAsList ();
    
    /**
     * Optimize this a lot !
     * First, we *may* need a generic pushBack ( const NodeRef& e ) (/!\ typing problems)
     * 
     * Compare the document-orderness with the tail of the NodeSet
     * If it is before the tail, then we have to enter the complex way
     * If not, we also have done the duplicate checking !
     */
#ifdef __XEM_NODESET_PUSHBACK_LOG_NODESET
    Log_NodeSetHPP ( "pushBack : pushing node=%llx:%s, insertInDocumentOrder=%s, size=%lx\n",
      nodeRef.isElement() ? nodeRef.toElement().getElementId() : 0,
      nodeRef.getKey(), insertInDocumentOrder ? "true" : "false", size() );
    Log_NodeSetHPP ( "****\n" );
    Log_NodeSetHPP ();
    Log_NodeSetHPP ( "****\n" );
#endif // __XEM_NODESET_PUSHBACK_LOG_NODESET
    if ( insertInDocumentOrder && ! list->empty() )
      {
        Log_NodeSetHPP ( "Inserting in document order...\n" );
        if ( insertNodeInDocumentOrder ( (NodeRef&) nodeRef ) )
          return;
      }
#if PARANOID
// #define __XEM_NODESET_PUSHBACK_CHECK_DUPLICATES
#endif

    Item* newItem;
#ifdef __XEM_NODESET_FAST_NODEREF_DUPLICATE
            // new(defaultPoolAllocator.alloc(sizeof(__Type))) __Type(__Source)
    newItem = defaultPoolAllocator.alloc(sizeof(AttributeRef));
    memcpy ( newItem, &nodeRef, sizeof(AttributeRef) );

#else
    if ( nodeRef.isElement() )
      {
#ifdef __XEM_NODESET_PUSHBACK_CHECK_DUPLICATES
        if ( contains(nodeRef.toElement()) )
          {
            Error ( "Already has this element : '%s'\n", nodeRef.getKey() );
            Log_NodeSetHPP ();
            Bug ( "." );
          }
#endif
        newItem = __itemAllocator ( ElementRef, nodeRef.toElement() );
      }
    else
      {
#ifdef __XEM_NODESET_PUSHBACK_CHECK_DUPLICATES
        AssertBug ( ! contains(nodeRef.toAttribute()), "Already has this element !\n" );
#endif
        newItem = __itemAllocator ( AttributeRef, nodeRef.toAttribute() );
      }
#endif

      Log_NodeSetHPP ( "[NS] Insert : newItem=%p\n", newItem );
      list->push_back ( newItem );
    }

#define __setSingletonFunc(__CallType,__ItemType) \
 __INLINE void NodeSet::setSingleton ( __CallType e ) \
 { \
   if ( singleton ) throwException(Exception,"Singleton already set for type %s\n", STRINGIFY(__ItemType) ); \
   AssertBug ( ! singleton, "Singleton already set !\n" ); \
   singleton = __itemAllocator( __ItemType, e); \
 }

  __setSingletonFunc ( const char*, String )
  __setSingletonFunc ( const String&, String )
  __setSingletonFunc ( Integer, ItemImpl<Integer> )
  __setSingletonFunc ( Number, ItemImpl<Number> )
  __setSingletonFunc ( bool, ItemImpl<bool> )
  __setSingletonFunc ( const ElementRef&, ElementRef )
  __setSingletonFunc ( const AttributeRef&, AttributeRef )
  
  __INLINE ElementRef& NodeSet::toElement ()
  {
    if ( size() == 1 && front().getItemType() == Item::Type_Element )
      return front().toElement();

    DOMCastException *e = new DOMCastException();
    detailException ( e, "DOMCastException : Non-scalar result can not be casted to an Element ! (size : %lu)\n", (unsigned long) size() );

    for ( iterator iter(*this) ; iter ; iter++ )
      {
        if ( iter->isElement() )
          {
            detailException ( e, "\tElement '%s' (id: %llx)\n", 
                      iter->toElement().getKey().c_str(),
                      iter->toElement().getElementId() );
            for ( AttributeRef attr = iter->toElement().getFirstAttr() ;
              attr ; attr = attr.getNext() )
              {
                detailException ( e, "\t\tAttribute '%s' : '%s'\n",
                          attr.getKey().c_str(), attr.toString().c_str() );
              }
          } 
        else if ( iter->isAttribute() )
          {
            AttributeRef attr = iter->toAttribute ();
            detailException ( e, "\tAttribute '%s' : '%s'\n",
                      attr.getKey().c_str(), attr.toString().c_str() );
          
          }
        else
          {
            detailException ( e, "\tSingleton type=%d : '%s'\n", iter->getItemType(), iter->toString().c_str() );
          }
      }
    throw ( e );
    return ElementRefNull;
  }

  __INLINE AttributeRef& NodeSet::toAttribute ()
  {
    if ( size() == 1 && front().getItemType() == Item::Type_Attribute )
      return front().toAttribute();
    throwException(DOMCastException, "Could not cast node set to attribute !" );
    return AttributeRefNull;
  }

  __INLINE NodeRef& NodeSet::toNode ()
  {
    if ( size() == 1 && front().isNode() )
      return front().toNode();
    Log_NodeSetHPP ( "Invalid NodeSet cast to NodeRef :\n" );
    log();
    throwException ( DOMCastException, "Non-scalar result can not be casted to a Node ! (size=%lu)\n", (unsigned long) size() );
    return ElementRefNull;
  }
  
  __INLINE String NodeSet::toString ( )
  {
    Log_NodeSetHPP ( "toString, size=%lu\n", (unsigned long) size() );
    if ( size() == 0 ) 
      {
        return String();
      }
    Log_NodeSetHPP ( "toString, front type=%x\n", front().getItemType() );
    return front().toString ();
  }

  __INLINE Integer NodeSet::toInteger ()
  {
    if ( size() == 0 ) return (Integer)NAN;
    return front().toInteger ();
  }

  __INLINE Number NodeSet::toNumber ()
  {
    if ( size() == 0 ) return (Number)NAN;
    return front().toNumber ();
  }
  
  __INLINE bool NodeSet::isNaN ()
  {
    if ( size() == 0 ) return true;
    return front().isNaN ();
  }

  __INLINE bool NodeSet::toBool ()
  {
    if ( size() == 0 )
      return false;
    if ( size() == 1 )
      {
        return front().toBool ();
      }
    Log_NodeSetHPP ( "Non-scalar !\n" );
    Item& item = front ();
#if 1
    switch ( item.getItemType () )
      {
      case Item::Type_Element:
        if ( item.toElement().isText() )
          return ( item.toElement().getText().size() > 0 );
        return true; 
      case Item::Type_Attribute:
        return ( item.toAttribute().toString().size() > 0 );
      case Item::Type_String:
        return ( item.toString().size() > 0 );
      default:
        break;
      }
    Warn ( "Dumping NodeSet\n" );
    for ( iterator iter(*this) ; iter ; iter++ )
      {
        Item& item = *iter;
        Warn ( "\tItem type=%d\n", item.getItemType() );

      }
    NotImplemented ( "Not handled : size=%lu, itemType = '%d'\n", (unsigned long) size(), item.getItemType() );
#endif
    return false;
  }
};
