/*
 * elementref-documentorder.cpp
 *
 *  Created on: 15 déc. 2009
 *      Author: francois
 */

#include <Xemeiah/dom/elementref.h>
#include <Xemeiah/dom/attributeref.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_DocOrder Debug

namespace Xem
{
  bool ElementRef::isBeforeInDocumentOrder ( NodeRef& nRef )
  {
    /*
     * First, elude simple cases
     */
    if ( &(getDocument()) != &(nRef.getDocument()) )
      {
        if ( getDocument().getDocumentTag() != nRef.getDocument().getDocumentTag() )
          {
            Log_DocOrder ( "Compare docs : left='%s', right='%s'\n", getDocument().getDocumentTag().c_str(), nRef.getDocument().getDocumentTag().c_str() );
            return getDocument().getDocumentTag() < nRef.getDocument().getDocumentTag();
          }
        Warn ( "Comparing different documents, but with same documentTag !\n" );
        /*
         * When the documents are different but have same tags, sort by the document pointers...
         * Dummy, a bit stupid, but we don't have any other choice
         */
        return ( ( (long int) &(getDocument()) ) < ((long int) &(nRef.getDocument())) );
      }
    ElementRef eRef = nRef.isAttribute() ? nRef.toAttribute().getElement() : nRef.toElement();

    if ( (*this) == eRef )
      {
        if ( nRef.isAttribute() )
          return true;
#if 1
        Bug ( "DOM sorting in document : Comparing same elements ! this=0x%llx:%s, eRef=0x%llx:%s",
            getElementId(), getKey().c_str(), eRef.getElementId(), eRef.getKey().c_str() );
#else
        throwException ( Exception, "DOM sorting in document : Comparing same elements ! this=0x%llx:%s, eRef=0x%llx:%s",
            getElementId(), getKey().c_str(), eRef.getElementId(), eRef.getKey().c_str() );
#endif
      }

    if ( !getFather() ) return true;
    if ( !eRef.getFather() ) return false;

    /*
     *  Elude when I am older than eRef
     */
    if ( getYounger() == eRef ) return true;
    if ( getElder() == eRef ) return false;

    /*
     * First, we have to find the first commun ancestor
     */

    ElementRef myAncestor = *this;
    ElementRef eRefAncestor = eRef;

    if ( myAncestor.getFather() != eRefAncestor.getFather() )
      {
        bool found = false;
        for ( ElementRef hisAncestor = eRef.getFather() ; hisAncestor ; hisAncestor = hisAncestor.getFather() )
          {

            myAncestor = *this;
            if ( *this == hisAncestor ) return true;
            for ( ElementRef myAnces = *this ; myAnces ; myAnces = myAnces.getFather() )
              {
                Log_DocOrder ( "isBefore (%s, %s) : at (%s, %s)\n",
                  getKey().c_str(), eRef.getKey().c_str(),
                  myAnces.getKey().c_str(), hisAncestor.getKey().c_str() );
                if ( myAnces == eRef )
                    return false;
                if ( myAnces == hisAncestor )
                  {
                    found = true;
                    break;
                  }
                myAncestor = myAnces;
              }
            if ( found ) break;
            eRefAncestor = hisAncestor;
          }
        AssertBug ( found, "Could not find common ancestor !\n" );
        Log_DocOrder ( "Found myAncestor=%s, hisAncestor=%s\n",
            myAncestor.getKey().c_str(), eRefAncestor.getKey().c_str() );

#if PARANOID
        AssertBug ( myAncestor.getFather() == eRefAncestor.getFather(), "Could not find common ancestors !\n" );
        AssertBug ( myAncestor != eRefAncestor, "Computed same ancestors !\n" );
#endif

      }
    /*
     * Then, as we have the last commun ancestor and both ancestors, sons of this commun ancestor,
     * We have to compare them.
     */
    for ( ElementRef elt = myAncestor ; elt ; elt = elt.getYounger() )
        if ( elt == eRefAncestor ) return true;
    for ( ElementRef elt = eRefAncestor ; elt ; elt = elt.getYounger() )
        if ( elt == myAncestor ) return false;
    Bug ( "Shall not be here !\n" );
    return false;
  }



};
