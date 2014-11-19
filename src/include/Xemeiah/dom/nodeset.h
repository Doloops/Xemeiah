#ifndef __XEM_DOM_NODESET_H
#define __XEM_DOM_NODESET_H

#include <Xemeiah/kern/exception.h>
#include <Xemeiah/dom/item.h>
#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/dom/attributeref.h>

#include <Xemeiah/kern/poolallocator.h>


#include <list>

// #define __XEM_DOM_NODESET_STATIC_SINGLETON
// #define __XEM_DOM_NODESET_DUMP

namespace Xem
{
 
    XemStdException(IndexNotFoundException);
    XemStdException(DOMCastException);
    
    #define throwDOMCastException(...)  throwException ( DOMCastException, __VA_ARGS__ )

    /**
     * NodeSet represents a set of nodes or a singleton as returned by an XPath evaluation.
     * A NodeSet can only contain one of the 2 following :
     * -# Elements and Attributes (i.e. Nodes) 
     * -# Zero or one Singleton (where singletons are bool, Integer, Number and String).
     */
    class NodeSet
    {
    protected:
        /**
         * \todo Change this typedef name
         */
        typedef std::list<Item*, PoolAllocator<Item> > __list;
        __list* list;

        Item* singleton;

        INLINE void clear ( bool clearBindedDocument );
        INLINE void setAsList ();
        
        PoolAllocator<Item> defaultPoolAllocator;

    public:
        NodeSet();
        ~NodeSet();
      
        /**
         * Size of the node-set
         * @return the actual size of the node-set
         */
        INLINE size_t size();

        /**
         * PushBack functions for Nodes : Elements an Attributes
         * @param nodeRef the node to add
         * @param insertInDocumentOrder if set to true, NodeSet will insert the node in document order
         */
        INLINE void pushBack ( const NodeRef& nodeRef, bool insertInDocumentOrder = false );
      
      protected:
        /**
         * insertInDocumentOrder
         * @param nodeRef the node to add
         * @return true if the node was insert, false if it has to be inserted at the tail
         */
        INLINE bool insertNodeInDocumentOrder ( NodeRef& nodeRef );
        
      public:
        /*
         * Singleton (forced size() == 1) insertions
         */        
        INLINE bool isSingleton();
        INLINE Item* getSingleton();
        INLINE void setSingleton ( Item* );
        INLINE void setSingleton ( const String& );
        INLINE void setSingleton ( const char* );
        INLINE void setSingleton ( Integer );
        INLINE void setSingleton ( Number );
        INLINE void setSingleton ( bool );
        INLINE void setSingleton ( const ElementRef& );
        INLINE void setSingleton ( const AttributeRef& );        

        /**
         * Append an integer at the end of the list.
         * @param i the Integer to add.
         */
        INLINE void pushBack ( Integer i );

        /**
         * iterator over the list of nodes or atomic values of a NodeSet.
         */
        class iterator
        {
          protected:
          public:
#ifdef __XEM_DOM_NODESET_NODELIST_ARRAY  
            __ui64 idx;
#else
            __list::iterator __iterator;
#endif
            INLINE void __init() __FORCE_INLINE;
            bool iteratedOverSingleton;
            XProcessor* xproc;
            Integer pos;
            Item* toItem();
            NodeSet& result;
          public:
            INLINE iterator ( NodeSet& __r ) __FORCE_INLINE;
            INLINE iterator ( NodeSet& __r, XProcessor& xproc ) __FORCE_INLINE;
            INLINE ~iterator () __FORCE_INLINE;

            NodeSet& getNodeSet() const { return result; }

            Item* operator-> ();
            operator bool();
            operator Item* ();
            iterator& operator++(int u);

            Integer getPosition();
            Integer getLast();
            
            void setPosition ( Integer _pos ) { pos = _pos; }
            
            void insert ( Item* item );
            
            __list::iterator* iterator_ptr() { return &__iterator; }
            __list::iterator& iterator_obj() { return __iterator; }
        };

        INLINE Item& front();
        INLINE Item& back();
        
        bool contains ( const ElementRef& element );
        bool contains ( const AttributeRef& attribute );

        INLINE ElementRef& toElement ();
        INLINE AttributeRef& toAttribute ();
        INLINE NodeRef& toNode ();
        INLINE String toString ( );
        INLINE Integer toInteger ();
        INLINE Number toNumber ();
        INLINE bool isNaN ();        
        INLINE Integer toInt () { return toInteger(); }
        INLINE bool toBool ();

        INLINE bool isScalar () { return size() == 1; }
        INLINE bool isInteger () { return ( isScalar() && front().getItemType() == Item::Type_Integer ); }
        INLINE bool isNumber () { return ( isScalar() && front().getItemType() == Item::Type_Number ); }
        INLINE bool isBool () { return ( isScalar() && front().getItemType() == Item::Type_Bool ); }
        INLINE bool isNode () { return ( isScalar() && front().isNode() ); }

        /**
         * clear the NodeSet contents
         */
        INLINE void clear ();

        /**
         * Sort using a comparator class
         */
        template<class BinaryPredicate>
        void sort ( BinaryPredicate comp )
        {
          if ( ! list ) return;
          list->sort ( comp );
        }
        
        /**
         * Sort the result using an XPath expression.
         * @param xpath the XPath expression to use for sorting.
         * @param dataTypeText if true, data-type is "text", if false data-type is "number" ; data-type qname-but-not-ncname not implemented yet.
         * @param orderAscending if true, order is "ascending", otherwise order is "descending"
         * @param caseOrderUpperFirst if true, case-order is "upper-first", otherwise case-order is "lower-first"
         */
        void sort ( XPath& xpath, bool dataTypeText, bool orderAscending, bool caseOrderUpperFirst );

        /**
         * Sort the NodeSet in document order
         */
        void sortInDocumentOrder ();

        /**
         * Sort the NodeSet in document order
         */
        bool checkDocumentOrderness ();

        /**
         * Revert the NodeSet
         * That is, the first becomes last, the last becomes first.
         */
        void reverseOrder();
        
        /**
         * Returns a deep copy of the result
         */
        void copyTo ( NodeSet& nodeSet );
        
        /**
         * Returns a deep copy of the result
         * Generally speaking, the copy shall remains stable after a branch-forking operation.
         * @return a deep copy of the NodeSet
         */
        NodeSet* copy ();

        /**
         * Debug : log a XPath Result.
         */ 
#ifdef __XEM_DOM_NODESET_DUMP
        void log();
#else
        void log() {}
#endif
    };
};

#endif // __XEM_DOM_NODESET_H
