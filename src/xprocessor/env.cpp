#include <Xemeiah/xprocessor/env.h>
#include <Xemeiah/kern/store.h>
#include <Xemeiah/kern/document.h>
#include <Xemeiah/kern/volatiledocumentallocator.h>
#include <Xemeiah/trace.h>
#include <Xemeiah/nodeflow/nodeflow.h>

#include <Xemeiah/auto-inline.hpp>

#define Log_DA Debug
#define Log_Env Debug
#define Log_DumpEnv Info


namespace Xem
{
  Env::Env( Store& __s, EnvSettings& _envSettings )
    : store(__s), envSettings(_envSettings)
  {
    Log_Env ( "New env at %p\n", this );
    headEntry = NULL;
    currentEnvId = 0;
    fatherEnv = NULL;
    xpathBaseNode = NULL;
    currentDocumentAllocator = NULL;
    currentDocumentAllocatorEnvId = 0;
    
    /*
     * We push twice, in order for the popEnv() garbage collector to act also on behind for the first available currentEnvId (2)
     */
    pushEnv (); pushEnv ();
  }
  
#if 0  
  Env::Env( Store& __s, Env* _fatherEnv )
    : store(__s)
  {
    Bug ( "Not implemented : Env fork !\n" );
    Log_Env ( "New env at %p\n", this );
    headEntry = NULL;
    currentEnvId = 0;
    fatherEnv = _fatherEnv;

    /**
     * Copy the mapping facilities.
     */
    for ( ItemMap::iterator iter = fatherEnv->itemMap.begin () ;
        iter != fatherEnv->itemMap.end () ; iter++ )
      {
        itemMap[iter->first] = iter->second;
      }
    pushEnv (); pushEnv ();
  }
#endif

  Env::~Env()
  { 
    Log_Env ("Destroy env at %p\n", this );
    AssertBug ( currentEnvId == 2, "We have to have a currentEnvId==2 at Env destruction, but env=0x%lx.\n",
        currentEnvId );
    
    popEnv (); popEnv ();
    if ( currentEnvId )
        Warn ( "Deleting Env with a non-null currentEnvId=0x%lx\n", currentEnvId );
    Log_Env ( "At Env destruction : %lu documents in DocumentMap\n", (unsigned long) documentMap.size() );
    for ( DocumentMap::iterator iter = documentMap.begin () ; iter != documentMap.end() ; iter++ )
      {
        getStore().releaseDocument(iter->second);
        iter->second = NULL;
      }
      
#if 0
    AssertBug ( currentEnvId == 0, "Deleting Env with a non-null currentEnvId=0x%lx\n",
        currentEnvId );
#endif
  }

  DocumentAllocator& Env::getCurrentDocumentAllocator( bool behind )
  {
    EnvId maxDepthBetweenAllocators = envSettings.documentAllocatorMaximumAge;
    
    AssertBug ( currentEnvId > 1, "Invalid low currentEnvId=%lx\n", (unsigned long) currentEnvId );
    
    if ( behind )
      {
        EnvEntry* entry = getBehindEntry ();
        if ( entry->documentAllocator ) return *(entry->documentAllocator);
        if ( currentDocumentAllocator && entry->previousDocumentAllocator == currentDocumentAllocator )
          {
            if ( currentDocumentAllocatorEnvId + maxDepthBetweenAllocators < currentEnvId )
              {
                Log_DA ( "DA too old !!! currentDocumentAllocatorEnvId=%lx, currentEnvId=%lx\n",
                  (unsigned long) currentDocumentAllocatorEnvId, (unsigned long) currentEnvId );
              }
            else
              {
                return *currentDocumentAllocator;
              }
          }
          
        AssertBug ( headEntry == entry || headEntry->lastEntry == entry, "Non-contiguous ???\n" );

        Log_Env ( "New document allocator for PREVIOUS currentEnvId=%lx\n", (unsigned long) currentEnvId-1 );
        entry->documentAllocator = new VolatileDocumentAllocator ( store );
        if ( headEntry == entry )
          {
            currentDocumentAllocator = entry->documentAllocator;
            currentDocumentAllocatorEnvId = currentEnvId - 1;
          }
        else
          {
            headEntry->previousDocumentAllocator = entry->documentAllocator;
          }
        return *(entry->documentAllocator);
      }
    if ( currentDocumentAllocatorEnvId + maxDepthBetweenAllocators < currentEnvId )
      {
        Log_DA ( "DA too old !!! currentDocumentAllocatorEnvId=%lx, currentEnvId=%lx\n",
          (unsigned long) currentDocumentAllocatorEnvId, (unsigned long) currentEnvId );
        currentDocumentAllocator = NULL;
        currentDocumentAllocatorEnvId = 0;
      }
    if ( !currentDocumentAllocator )
      {
        EnvEntry* entry = getHeadEntry ();
        Log_Env ( "New document allocator for CURRENT currentEnvId=%lx\n", (unsigned long) currentEnvId );
        entry->documentAllocator = new VolatileDocumentAllocator ( store );
        currentDocumentAllocator = entry->documentAllocator;
        currentDocumentAllocatorEnvId = currentEnvId;
      }
    
    AssertBug ( currentDocumentAllocator, "No currentDocumentAllocator !\n" );
    return *currentDocumentAllocator;  
  }

  void Env::disableIterator ()
  {
    iteratorStack.push_front ( (NodeSet::iterator*) NULL );  
  }

  void Env::restoreIterator ()
  {
    AssertBug ( iteratorStack.size(), "Empty iterator stack !\n" );
    AssertBug ( iteratorStack.front() == NULL, "Illegal call to restoreIterator() : Current iterator is not NULL !\n" );
    iteratorStack.pop_front ();
  }

  NodeSet::iterator& Env::getCurrentIterator ()
  {
    AssertBug ( hasIterator(), "Current Env has no iterator !\n" );
    NodeSet::iterator* iter = iteratorStack.front ();
    return *iter;  
  }
  
  NodeSet& Env::getCurrentNodeSet ()
  {
    return getCurrentIterator().getNodeSet ();
  }


  Integer Env::getPosition ()
  {
#if 0
    AssertBug ( hasIterator(), "Current Env has no iterator !\n" );
    NodeSet::iterator* iter = iteratorStack.front ();
#endif
    return getCurrentIterator().getPosition ();  
  }

  Integer Env::getLast ()
  {
#if 0
    AssertBug ( hasIterator(), "Current Env has no iterator !\n" );
    NodeSet::iterator* iter = iteratorStack.front ();
#endif
    return getCurrentIterator().getLast ();  
  }

  Env::EnvEntry::EnvEntry ()
  {
    envId = 0;
    firstItem = NULL;
    lastEntry = NULL;
    bindedDocuments = NULL;
    documentAllocator = NULL;
    previousDocumentAllocator = NULL;
  }

  void Env::createEnvEntry()
  {
    /*
     * If the current headEntry has been created for the current envId,
     * I got nothing to do here.
     */
    if ( headEntry && headEntry->envId == currentEnvId )
      {
    	  Bug ( "Shall have checked this before !\n" );
      }
    Log_Env ( "%p Create entry %lx\n", this, currentEnvId );
    if ( ! headEntry )
      {
        headEntry = new EnvEntry();
        headEntry->lastEntry = NULL;
      }
    else
      {
        EnvEntry* lastEntry = headEntry;
        headEntry = new EnvEntry();
        headEntry->lastEntry = lastEntry;
      }
    headEntry->envId = currentEnvId;
    headEntry->firstItem = NULL;
    headEntry->previousDocumentAllocator = currentDocumentAllocator;
  }

  void Env::freeEnvEntry ()
  {
    Log_Env ( "%p Free entry %lx\n", this, currentEnvId );
    
    /* 
     * First, delete all bound variables for this Entry level
     */
    for ( EnvItem* item = headEntry->firstItem ; item ; )
      {
        EnvItem* next = item->nextInEntry;
        Log_Env ( "Item %p, next %p\n", item, next );
        AssertBug ( item != next, "Circular set !\n" );
        if ( item->previousItem )
          {
            itemMap[item->keyId] = item->previousItem;
          }
        else
          {
            ItemMap::iterator iter = itemMap.find ( item->keyId );
            if ( iter == itemMap.end() )
              {
                Warn ( "Could not find keyId '%x' (%s : %s) in map, maybe alloced but not assigned...\n", 
                    item->keyId, 
                    getStore().getKeyCache().getNamespaceURL ( KeyCache::getNamespaceId ( item->keyId ) ),
                    getStore().getKeyCache().getLocalKey ( KeyCache::getLocalKeyId ( item->keyId ) ) );
              }
            else
              {
                itemMap.erase ( iter );
              }
          }
        // delete ( item );
        item->clear ( );
        headEntry->releaseEnvItem ( item );
        item = next;
      }

    /*
     * Delete all fetched documents for this Entry level
     */
    if ( headEntry->bindedDocuments )
      {
        for ( EnvEntry::BindedDocuments::iterator iter = headEntry->bindedDocuments->begin() ;
          iter != headEntry->bindedDocuments->end() ; iter++ )
          {
            Document* doc = *iter;
            String url = doc->getDocumentURI ();
            
            if ( url.size() )
              {
                DocumentMap::iterator iter = documentMap.find ( url );
                
                AssertBug ( iter != documentMap.end(), "Could not find document '%s' for deletion !\n", url.c_str() );
                
                Log_Env ( "Env - fetched documents garbage collector : releasing '%s'\n", url.c_str() );
                
                documentMap.erase ( iter );
              }            
            // doc->release ();
            getStore().releaseDocument(doc);
          }
        delete ( headEntry->bindedDocuments );
      }
    currentDocumentAllocator = headEntry->previousDocumentAllocator;
    
    /*
     * Finally, replace the headEntry with the last one previously declared
     */      
    EnvEntry* lastEntry = headEntry->lastEntry;
    delete ( headEntry );
    headEntry = lastEntry;
  }

  String Env::getString ( KeyId keyId )
  {
    NodeSet* res = getVariable ( keyId );
    if ( ! res ) { Bug ( "." ); }
    return res->toString ();    
  }

  void Env::setString ( KeyId keyId, const String& value )
  { 
    NodeSet* res = setVariable ( keyId );
    res->setSingleton ( value );
  }

  void Env::setString ( KeyId keyId, const char* value )
  {
    setString ( keyId, String(value) );
  }

#if 0
  Env* Env::forkEnv ()
  {
    Bug ( "NOT IMPLEMENTED !\n" );
    Env* forkedEnv = NULL; // new Env ( store, this );
    
    forkedEnv->createEnvEntry ();
    for ( ItemMap::iterator iter = itemMap.begin () ; 
        iter != itemMap.end (); iter++ )
    {
        Info ( "KeyId=%x, EnvItem=%p\n", iter->first, iter->second );
        forkedEnv->setVariable ( forkedEnv->headEntry, iter->first, iter->second->variable->copy() );
    }  
    return forkedEnv;
  }
#endif

  void Env::dumpEnv ( )
  {
    if ( headEntry )
      {
        Log_DumpEnv ( "At envId=%lx, current env is :\n", currentEnvId );
        for ( EnvItem * item = headEntry->firstItem ;
              item ; item = item->nextInEntry )
          {
            NodeSet* res = item;
            String keyName = getKeyCache().dumpKey ( item->keyId );
            if ( res->size() == 0 )
            {
              Log_DumpEnv ( "\tVariable '%s' = '%s'\n", keyName.c_str(), "(Empty NodeSet)" );
            }
            else if ( res->isSingleton() )
            {
              Log_DumpEnv ( "\tVariable '%s' = '%s'\n", keyName.c_str(), res->toString().c_str() );
            }
            else     
            {
              Log_DumpEnv ( "\tVariable '%s' = '%s'\n", keyName.c_str(), "(Non-Scalar NodeSet)" );
            }
          }
        Log_DumpEnv ( "All variables are :\n" );
        for ( ItemMap::iterator iter = itemMap.begin() ; iter != itemMap.end() ; iter++ )
          {
            EnvItem* item = iter->second;
            AssertBug ( item->keyId == iter->first, "Corrupted Env map !\n" );
            String keyName = getKeyCache().dumpKey ( item->keyId );
            Log_DumpEnv ( "\titem=%p (envId=%lx), '%s' =\n", item, item->envId, keyName.c_str() );
            item->log ();          
          }
      }
  }

  void Env::dumpEnv ( Exception * xpe, bool fullDump )
  {
    if ( headEntry && ( fullDump || headEntry->envId == currentEnvId ) )
      {
        for ( EnvItem * item = headEntry->firstItem ;
              item ; item = item->nextInEntry )
          {
            NodeSet* res = item;
            String keyName = getKeyCache().dumpKey ( item->keyId );

            if ( res->size() == 0 )
              detailException ( xpe, "\tVariable '%s' = '%s'\n", keyName.c_str(), "(Empty NodeSet)" );
            else if ( res->isSingleton() )
              detailException ( xpe, "\tVariable '%s' (type %d) = '%s'\n", keyName.c_str(), res->front().getItemType(), res->toString().c_str() );
            else
            {
              detailException ( xpe, "\tVariable '%s' = (%ld elements)\n", keyName.c_str(), (unsigned long) res->size() );
              for ( NodeSet::iterator iter(*res) ; iter ; iter++ )
              {
                if ( iter->isNode() )
                  {
                    detailException ( xpe, "\t\t%s\n", iter->toNode().generateVersatileXPath().c_str() );
                    if ( iter->isElement() && iter->toElement().isText() )
                      {
                        detailException ( xpe, "\t\t\t%s\n", iter->toElement().getText().c_str() );
                      }
                  }
                else
                  {
                    detailException ( xpe, "\t\tType %d : %s\n", iter->getItemType(), iter->toString().c_str() );
                  }
              }
            }
          }
      }
  }

};
