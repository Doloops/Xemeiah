#ifndef __XEM_KERN_VOLATILE_DOCUMENT_H
#define __XEM_KERN_VOLATILE_DOCUMENT_H

#include <Xemeiah/kern/document.h>
#include <Xemeiah/kern/transactionlessdocument.h>

namespace Xem
{

  /**
   * VolatileDocument : in-memory representation of a document (not backed by any file).
   */
  class VolatileDocument : public TransactionlessDocument
  {
    friend class Store;
  protected:
    /**
     * Volatile Documents may not be indexed.
     */
    virtual bool mayIndex ();

    /**
     * VolatileDocument protected constructor : only Store is allowed to instanciate a VolatileDocument
     * use Store::createVolatileDocument() to instanciate a new volatile document
     */
    VolatileDocument ( Store& store, DocumentAllocator& allocator );

    /**
     * release document resources
     */
    void releaseDocumentResources ();
  public:
    /**
     * Virtual destructor
     */
    virtual ~VolatileDocument ();
  };
};

#endif //  __XEM_KERN_VOLATILE_DOCUMENT_H
