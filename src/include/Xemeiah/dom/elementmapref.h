#ifndef __XEM_DOM_ELEMENTMAPREF_H
#define __XEM_DOM_ELEMENTMAPREF_H

#include <Xemeiah/dom/skmapref.h>

namespace Xem
{
  class XPath;
  
  /**
   * ElementMapRef is a specialized container, in the form std::map<__ui64,ElementRef>
   * Each ElementMapRef is backed by an AttributeRef of a given element, mapping is persistent.
   * Warning : Each Element stored in the mapping must be part of the same document !
   */
  class ElementMapRef : public SKMapRef
  {
  public:
    /**
     * ElementMapRef instanciator from an AttributeRef
     */
    ElementMapRef ( const AttributeRef& attrRef )
    : SKMapRef ( attrRef ) 
    {
      checkSKMapType ( SKMapType_ElementMap );
    }
    
    /**
     * Instance destructor.
     */
    ~ElementMapRef () {}
    
    /**
     * Add a value to the mapping.
     * If the mapping already has elements for the hash, then the element replaces existing values,
     * otherwise, the key is just inserted.
     * @param hash the key for the mapping
     * @param eltRef the element to store.
     */
    void put ( SKMapHash hash, ElementRef& eltRef )
    {
      AssertBug ( &(getDocument()) == &(eltRef.getDocument()), "Can not insert foreign element !\n" );
      iterator iter(*this);
      iter.findClosest ( hash );
      if ( iter && iter.getHash() == hash )
        {
          iter.setValue ( eltRef.getElementPtr() );
        }
      else
        {
          iter.insert ( hash, eltRef.getElementPtr() );
        }
    }

    /**
     * Get an ElementRef from a current hash value.
     * @param hash value the key for the mapping.
     * @return an ElementRef for the iterator.
     */
    ElementRef get ( SKMapHash hash )
    {
      iterator iter(*this);
      iter.findClosest ( hash );
      
      if ( ! iter || iter.getHash() != hash )
        return ElementRef ( getDocument(), NullPtr );
      return ElementRef ( getDocument(), iter.getValue() );
    }
    
    /**
     * Get an ElementRef from a current hash value.
     * @param hash value the key for the mapping.
     * @return an ElementRef for the iterator.
     */
    bool has ( SKMapHash hash )
    {
      iterator iter(*this);
      iter.findClosest ( hash );

      if ( ! iter || iter.getHash() != hash )
        return false;
      return true;
    }

    /**
     * Get an ElementRef from a current iterator position.
     * @param iter an iterator of the mapping.
     * @return an ElementRef for the iterator.
     */
    ElementRef get ( iterator& iter )
    {
      AssertBug ( iter, "Null iterator !\n" );
      return ElementRef ( getDocument(), iter.getValue() );
    }
    
    /**
     * Removes an ElementRef from the mapping
     */
    void erase ( SKMapHash hash, ElementRef& eltRef )
    {
      NotImplemented ( "ElementMapRef::erase()\n" );
    }
    
    /**
     * Element Iterator
     */
    class element_iterator : public iterator
    {
    protected:
      ElementRef element;
    public:
      element_iterator ( ElementMapRef& _elementMapRef )
      : iterator(_elementMapRef), element(_elementMapRef.getDocument())
      {
        element = _elementMapRef.get ( *this );
      }
      ~element_iterator () {}
    
      ElementRef toElement ()
      {
        ElementMapRef& elementMapRef = dynamic_cast<ElementMapRef&> ( skMapRef );
        return elementMapRef.get ( *this );
      }
    };
  };

  /**
   * ElementMultiMapRef is a specialized container, in the form std::map<__ui64, std::list<ElementRef> >
   * Each ElementMultiMapRef is backed by an AttributeRef of a given element, mapping is persistent.
   * Warning : Each Element stored in the mapping must be part of the same document !
   */
  class ElementMultiMapRef : public SKMultiMapRef
  {
  public:
    /**
     * ElementMultiMapRef instanciator from an AttributeRef
     */
    ElementMultiMapRef ( const AttributeRef& attrRef )
    : SKMultiMapRef ( attrRef )
    {
      checkSKMapType ( SKMapType_ElementMultiMap );
    }
    
    
    /**
     * Instance destructor.
     */
    ~ElementMultiMapRef() {}
    
    /**
     * Get an ElementRef from a current iterator position.
     * @param iter an iterator over a given hash value.
     * @return an ElementRef for the iterator.
     */
    ElementRef get ( multi_iterator& iter )
    {
      AssertBug ( iter, "Null iterator !\n" );
      return ElementRef ( getDocument(), iter.getValue() );
    }
    
    /**
     * Add a value to the mapping.
     * If the mapping already has elements for the hash, then the element is appended at the end of this list,
     * otherwise a new list is created.
     * @param hash the key for the mapping
     * @param eltRef the element to store.
     */
    void put ( SKMapHash hash, ElementRef& eltRef )
    {
      AssertBug ( &(getDocument()) == &(eltRef.getDocument()), "Can not insert foreign element !\n" );
      multi_iterator iter(*this, hash);
      iter.insert ( eltRef.getElementPtr() );
    }
    
    /**
     * Remove a value pair from the mapping.
     */
    void remove ( SKMapHash hash, ElementRef& eltRef )
    {
      multi_iterator iter(*this, hash);
      AssertBug ( iter, "Hash %llx is not part of the SKMultiMap !\n", hash );
      iter.erase ( eltRef.getElementPtr() );
    }
    
    /**
     * Remove a whole value hash from the mapping
     */
    void remove ( SKMapHash hash )
    {
      iterator iter(*this);
      iter.findClosest(hash);
      if ( ! iter || iter.getHash() != hash )
        {
          throwException ( Exception, "Could not find hash %llx\n", hash );
        }
      deleteSKMapList(iter.getValue());
      iter.erase();
    }

    /**
     * Higher level function : insert using an XPath to eval key
     */
    void insert ( ElementRef& eltRef, XPath& useXPath );
    
    /**
     * Higher level function : remove using an XPath to eval key
     */
    void remove ( ElementRef& eltRef, XPath& useXPath );

    /**
     * Delete this attribute
     */
    virtual void deleteAttribute ()
    {
      deleteSKMapSingle ();
    }
  };

};

#endif // __XEM_DOM_ELEMENTMAPREF_H
