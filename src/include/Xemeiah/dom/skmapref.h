#ifndef __XEM_DOM_SKMAPREF_H
#define __XEM_DOM_SKMAPREF_H

#include <Xemeiah/kern/format/core_types.h>
#include <Xemeiah/kern/format/skmap.h>
#include <Xemeiah/dom/attributeref.h>

namespace Xem
{

  /**
   * Invalid use of a SKMap Exception
   */   
  XemStdException ( InvalidSKMapType );

  /**
   * SKMapRef is a scalar std::map<SKMapHash,SKMapValue> mapper, backed by a s DOM Attribute. 
   * Everything is done using the SKMapRef::iterator class.
   */
  class SKMapRef : public AttributeRef
  {
  public:
    INLINE static __ui64 hashString ( const String& str );
    
    /**
     * SKMapRef::iterator : iterates over a SMapRef
     */
    class iterator
    {
    protected:
      /**
       * Reference to the SKMapRef we are in.
       */
      SKMapRef& skMapRef;
      /**
       * The current Item we are at.
       */
      //      SKMapItem* currentItem;
      SegmentPtr currentItemPtr;
      
      /**
       * The trace buffer
       */
      SegmentPtr trace[SKMap_maxLevel];

      /**
       * Returns true if the iterator is positionned on a valid Item,
       * false if not.
       */
      INLINE bool isPositionned ();
      
      INLINE SKMapConfig* getConfig();
      
      Document& getDocument() const { return skMapRef.getDocument(); }
      DocumentAllocator& getDocumentAllocator() const { return getDocument().getDocumentAllocator(); }
      INLINE __ui64 getItemSize ( SKMapItem* skMapItem );
      
    public:
      iterator ( SKMapRef& skMapRef );
      // iterator ( SKMapRef& skMapRef, SKMapHash hash );
      ~iterator ();

      INLINE SKMapHash getHash ( );
      INLINE SKMapValue getValue ( );
      void setValue ( SKMapValue value );
      
      bool getNextHash ( SKMapHash& nextHash );
      bool getNextValue ( SKMapValue& nextValue );

      /**
       * Position the iterator to the closest Item
       * That is : each item before has a hash < hash, and each item after has a hash > hash.
       * If the item already exists for this hash, then iterator will be set on this item.
       * @param hash The Hash to find
       */
      void findClosest ( SKMapHash hash );
      
      /**
       * Insert a Hash value just after this position.
       * If the iterator is already positionned, it will be asserted that :
       * getHash () < hash < getNextHash ().
       */
      bool insert ( SKMapHash hash, SKMapValue );
      
      /**
       * Erase the item we are in.
       * The iterator will then be positionned on the next item (ie the next hash).
       */
      virtual bool erase ();
      
      /**
       * Returns true if the iterator is positionned on a valid Item,
       * false if not.
       */
      operator bool() { return isPositionned(); }
      
      iterator& operator++(int u);
      
      void reset ();
    };

  protected:
    INLINE SKMapConfig* getConfig();
    INLINE SKMapHeader* getHeader();
    INLINE void authorizeHeaderWrite();
    INLINE SKMapItem* getHead();
    INLINE void authorizeHeadWrite();

    INLINE __ui64 getItemSize ( __ui32 level );
    INLINE __ui64 getItemSize ( SKMapItem* skMapItem );

    INLINE SKMapItemPtr getNewItemPtr ( __ui32 level, SKMapHash hash, SKMapValue value );

    template<PageCredentials how>
    INLINE SKMapItem* getItem ( SKMapItemPtr skMapItemPtr );

    INLINE void alterItem ( SKMapItem* skMapItem );
    INLINE void protectItem ( SKMapItem* skMapItem );

    void __foo__ (); // Force template instanciation.

    SKMapRef ( const AttributeRef& attrRef )
    : AttributeRef(attrRef) {}

    /**
     * Delete a SKMap which only holds Items, no List
     */
    void deleteSKMapSingle ();
  public:
    /**
     * Default destructor
     */
    virtual ~SKMapRef () {}

    /**
     * Get the SKMap type.
     */
    INLINE SKMapType getSKMapType();
    
    /**
     * Check that the current SKMap has the given type
     * throw Exception if that's not the case.
     */
    INLINE void checkSKMapType ( SKMapType expectedType );
    
    /**
     * (Testing) Dump the SKMapRef
     */
    void dump ();

    /**
     * Provide a default SKMap configuration
     */
    static void initDefaultSKMapConfig ( SKMapConfig& config );
  };

  /**
   * SKMultiMapRef : a vectorial std::map<SKMapHash, std::list<SKMapValue> >
   * Everything is operated operated using the SKMultiMapRef::iterator.
   */
  class SKMultiMapRef : public SKMapRef
  {
  public:
    /**
     * multi-iterator iterates over multiple values for one key of the association.
     */
    class multi_iterator : public iterator
    {
    protected:
      /**
       * Current list we are working on.
       */
      SKMapListPtr currentListPtr;
      
      /**
       * Current position in list.
       */
      __ui32 currentIndex;
      
      /**
       * The hash that was requested for this iterator.
       */
      SKMapHash myHash;
      
      /**
       * Returns true if is positionned, false if not.
       */
      INLINE bool isPositionnedInList () __FORCE_INLINE;

      /**
       * Find the list corresponding to our current hash
       */
      INLINE void findList ( );

      template<PageCredentials how>
      INLINE SKMapList* getCurrentList () __FORCE_INLINE;

      /**
       * Template instanciation
       */
      void __foo__ ();

    public:
      INLINE multi_iterator ( SKMultiMapRef& skMultiMap, SKMapHash hash );
      INLINE ~multi_iterator ();

      /**
       * find a particular Hash
       */
      INLINE void findHash ( SKMapHash hash ) __FORCE_INLINE;
      
      INLINE SKMapValue getValue ();
      
      /**
       * Insert a value at the given Hash position
       */
      bool insert ( SKMapValue value );

      /**
       * Erase the value at the given Hash position
       * Erasing a value keeps the order of existing values intact
       */      
      bool erase ( SKMapValue value );
      
      /**
       * Erase all values for the given hash position
       */
      bool erase ();
      
      operator bool() { return isPositionnedInList(); }
      multi_iterator& operator++(int u);
      
      INLINE void reset ();
    };

    /**
     * Protected constructor : SKMultiMapRef is just an Interface.
     */
    SKMultiMapRef ( const AttributeRef& attrRef )
    : SKMapRef (attrRef) {}

    /**
     * Delete a SKMap List
     */
    void deleteSKMapList ( SegmentPtr listPtr );

    /**
     * deleteAttribute()
     */
    void deleteAttribute();

  public:
    /**
     * Virtual constructor
     */
    virtual ~SKMultiMapRef () {}
  
  };
};


#endif // __XEM_DOM_SKMAPREF_H
