#ifndef __XEM_DOM_INTEGERMAPREF_H
#define __XEM_DOM_INTEGERMAPREF_H

#include <Xemeiah/dom/skmapref.h>

namespace Xem
{
  /**
   * IntegerMapRef is a specialized container, in the form std::map<Integer,Integer>
   * Each IntegerMapRef is backed by an AttributeRef of a given element, mapping is persistent.
   */
  class IntegerMapRef : public SKMapRef
  {
  public:
    /**
     * IntegerMapRef instanciator from an AttributeRef
     */
    IntegerMapRef ( const AttributeRef& attrRef )
    : SKMapRef ( attrRef ) 
    {
      checkSKMapType ( SKMapType_IntegerMap );
    }
    
    /**
     * Instance destructor.
     */
    ~IntegerMapRef () {}
    
    /**
     * Add a value to the mapping.
     * If the mapping already has elements for the hash, then the value replaces existing values,
     * otherwise, the key is just inserted.
     * @param key the key for the mapping
     * @param value the corresponding key.
     */
    void put ( Integer key, Integer value )
    {
      SKMapHash hash = (Integer) key;
      SKMapValue val = (Integer) value;
      iterator iter(*this);
      iter.findClosest ( hash );
      if ( iter && iter.getHash() == hash )
        {
          iter.setValue ( val );
        }
      else
        {
          iter.insert ( hash, val );
        }
    }
    
    /**
     * Get a value from a key.
     * @param key the key for the mapping.
     * @return the corresponding value.
     */
    Integer get ( Integer key )
    {
      iterator iter(*this);
      SKMapHash hash = (SKMapHash) key;
      iter.findClosest ( hash );

      if ( ! iter || iter.getHash() != hash )
        return (Integer) 0;
      return (Integer) iter.getValue();
    }

    /**
     * Check if a value is part of the mapping
     * @param key the key for the mapping
     * @return true if a value has been defined for this key, false otherwise
     */
    Integer has ( Integer key )
    {
      iterator iter(*this);
      SKMapHash hash = (SKMapHash) key;
      iter.findClosest ( hash );

      if ( ! iter || iter.getHash() != hash )
        return false;
      return true;    
    }

    /**
     * Deletes attribute
     */
    virtual void deleteAttribute ()
    {
      deleteSKMapSingle ();
    }
  };
};

#endif // __XEM_DOM_INTEGERMAPREF_H

