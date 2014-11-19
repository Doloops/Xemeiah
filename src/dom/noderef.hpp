#include <Xemeiah/dom/noderef.h>
#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/dom/attributeref.h>

#define Log_Node Debug

namespace Xem
{
  __INLINE NodeRef::NodeRef ( Document & _document ) 
    : document(_document)
  {
    Log_Node ( "New node ref at %p, document=%p\n", this, &document );
  }

  __INLINE NodeRef::~NodeRef ()
  {
  }

#ifdef __XEM_DOM_NODEREF_OPERATOR_BOOL_HACK
  __INLINE NodeRef::operator bool() const
  {
    ElementRef* elt = (ElementRef*) this;
    return elt->getElementPtr() != NullPtr;
  }
#endif

#ifdef __XEM_DOM_NODEREF_GETKEYID_HACK
  __INLINE KeyId NodeRef::getKeyId()
  { 
#if 0 // PARANOID
    ElementRef* elt = (ElementRef*) this;
    AttributeRef* attr = (AttributeRef*) this;
    if ( elt->getKeyId() != attr->getKeyId() )
      {
        Warn ( "Diverging keyId ! elt=%x, attr=%x\n", elt->getKeyId(), attr->getKeyId() );
        Warn ( "ptrs : elt=%llx, attr=%llx\n", elt->getElementPtr(), attr->getAttributePtr() );
        Bug ( "." );
      }
#endif
    AssertBug ( nodePtr, "Invalid zero nodePtr !\n" );
    KeyId* keyId = getDocumentAllocator().getSegment<KeyId,Read> ( nodePtr, sizeof(KeyId) );
    return *keyId;
  }
#endif

  __INLINE bool NodeRef::operator== ( NodeRef& nodeRef )
  {
    if ( &document != &(nodeRef.document) )
      {
        return false;
      }
    return nodePtr == nodeRef.nodePtr;
  }
  
  __INLINE bool NodeRef::isRootElement ()
  {
    return nodePtr == document.rootElementPtr;
  }
};

