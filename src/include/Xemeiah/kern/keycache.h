#ifndef __XEM_KERN_KEYCACHE_H
#define __XEM_KERN_KEYCACHE_H

#include <Xemeiah/dom/string.h>
#include <Xemeiah/kern/exception.h>
#include <Xemeiah/kern/format/core_types.h>
#include <Xemeiah/kern/qnamemap.h>
#include <Xemeiah/kern/hashbucket.h>
#include <Xemeiah/kern/mutex.h>

#include <map>
#include <list>

namespace Xem
{
  class Store;
  class ElementRef;
  class NamespaceAlias;

  XemStdException ( InvalidKeyException );
  XemStdException ( UnresolvableNamespaceException );
  XemStdException ( InexistantKeyException );
  
  typedef ArrayMap<KeyId,const char*> LocalKeyIdToKeyMap;

  class KeyCache;
  
  /**
   * Minimal KeyClass definition for builtin-keys
   */
  class BuiltinKeyClass
  {
  public:
    virtual ~BuiltinKeyClass () {}
    virtual bool __build ( KeyCache& keyCache ) = 0;
  };

  /**
   * Define the BuiltinKey classes
   */
#include <Xemeiah/kern/builtin_keys_prolog.h>
#include <Xemeiah/kern/builtin_keys.h>
#include <Xemeiah/kern/builtin_keys_postlog.h>

  /**
   * The KeyCache class is responsible for converting char* representations of non-prefixed keys 
   * to their corresponding unique integer representation (LocalKeyId).
   * It is also responsible for converting Namespace URLs to their corresponding unique NamespaceIds.
   *
   * Fully-qualified names (i.e. prefixed names with a Namespace binding) are parsed using the NamespaceAlias class.
   * Refer to this class for further information on NamespaceId binding.
   */
  class KeyCache
  {
    friend class Store;
    friend class PersistentStore;
    friend class NetStore;
    friend class NamespaceAlias;
  protected:
    /**
     * Reference to our Store, mainly used in case of a new Key or Namespace creation
     */
    Store& store;
    
    /**
     * Thread protection lock, locked only in write-scenarii
     */
    Mutex writeMutex;

    /**
     * KeyCache constructor.
     * @param store a reference to the Store class.
     */
    KeyCache ( Store& store );
    
    /**
     * KeyCache destructor.
     */
    ~KeyCache ();

    /**
     * HashBucket : a fast char* to LocalKeyId mapping class.
     *
     * This is used both for local key mapping and namespace URL mapping.
     */
    typedef HashBucketTemplate<LocalKeyId> HashBucket;

    /**
     * The HashBucket for keys, converting char* keys to their LocalKeyId.
     */
    HashBucket keysBucket;
        
    /**
     * The LocalKeyIdToKey for keys, conversion LocalKeyId to char*.
     */
    LocalKeyIdToKeyMap localKeyMap;
    
    /**
     * The QName (PrefixId+LocalKeyId) conversion to char*.
     */
    QNameMap qnameMap;
    
    /**
     * The HashBucket for namespaces, converting char* URLs to NamespaceId
     */
    HashBucket namespaceBucket;
    
    /**
     * The LocalKeyIdToKey for namespaces, converting NamespaceIds to URLs.
     */
    LocalKeyIdToKeyMap namespaceMap;

    /**
     * Fills the builtinKeys members, by parsing keys from the <Xemeiah/kern/builtin_keys.h>
     */
    bool buildBuiltinKeys ();
    
    /**
     * Frees all the in-memory mappings, keys, and stuff.
     */
    void unloadKeys ();

  public:
    /**
     * Parses a built-in key and returns a fully-qualified KeyId.
     * This function may only be a direct alias for getKeyId().
     * @param nsName the namespace part of the key.
     * @param keyName the local part of the key.
     */
    void processBuiltinKeyId ( KeyId& keyId, NamespaceId nsId, const char* keyName );
    
    /**
     * Parses a builtin-key and returns its namespaceId.
     * This also sets the default namespace alias binding.
     * @param prefix the prefix of the namespace
     * @param url the corresponding url of the namespace.
     */
    void processBuiltinNamespaceId ( KeyId& nsKeyId, NamespaceId& nsId, const char* prefix, const char* url );

  public:
    /**
     * Gets a reference to the store we are linked to.
     */
    Store& getStore() const { return store; }
    
    /**
     * Extracts the NamespaceId from a KeyId.
     * @param keyId the keyId
     * @return the NamespaceId part of the KeyId.
     */
    static NamespaceId getNamespaceId ( KeyId keyId )
      { return ( keyId >> KeyId_LocalBits ); }
      
    /**
     * Extracts the LocalKeyId from a KeyId.
     * @param keyId the keyId
     * @return the LocalKeyId part of the KeyId.
     */
    static LocalKeyId getLocalKeyId ( KeyId keyId )
      { return ( keyId & KeyId_LocalMask ); }
      
    /**
     * Builds a KeyId from its NamespaceId part and LocalKeyId part.
     * @param nsId the NamespaceId part
     * @param localKeyId the localKeyId part
     * @return The corresponding KeyId.
     */
    static KeyId getKeyId ( NamespaceId nsId, LocalKeyId localKeyId )
      { return ( nsId << KeyId_LocalBits ) + localKeyId; }

    /**
     * Retrieves a LocalKeyId for a non-prefixed keyName.
     * If the keyName does not exist in cache, then it will be created, and a unique LocalKeyId will be assigned.
     * @param keyName the non-prefixed key.
     * @return the LocalKeyId corresponding to this keyName.
     */
    INLINE LocalKeyId getFromCache ( const char* keyName );
    
    /**
     * Builds a KeyId from an NamespaceId and a local key.
     * The local key will be parsed using getFromCache(), and a KeyId will be built.
     * @param nsId the NamespaceId of the key to build.
     * @param keyName the local part of the key.
     * @param create if true, the keyName will be created if it does not exist. 
     * @return the corresponding KeyId. If keyName does not exist and create is false, a zero KeyId will be returned.
     */
    KeyId getKeyId ( NamespaceId nsId, const char* keyName, bool create );

    /**
     * Builds a fully-qualified KeyId from a char*, using a NamespaceAlias
     * @param nsAlias the NamespaceAlias binding to use for parsing the prefix part.
     * @param keyName the fully-qualified (or non-prefixed) keyName to parse
     * @param create creates the key if it does not exist.
     * @return The corresponding KeyId, or 0 if the keyName does not exist and create is false.
     */
    INLINE KeyId getKeyId ( NamespaceAlias& nsAlias, const char* keyName, bool create );
    
    /**
     * Convenience function to parse a String keyName.
     * @param nsAlias the NamespaceAlias binding to use for parsing the prefix part.
     * @param keyName the fully-qualified (or non-prefixed) keyName to parse
     * @param create creates the key if it does not exist.
     * @return The corresponding KeyId, or 0 if the keyName does not exist and create is false.
     */
    INLINE KeyId getKeyId ( NamespaceAlias& nsAlias, const String& keyName, bool create );

    /**
     * Parse a keyName using an element hierarchy for namespace prefix resolution.
     * @param parsingFromElementRef the element hierarchy to find namespace aliases.
     * @param keyName the key to parse
     * @param create creates the key if it does not exist.
     * @return the KeyId associated, or 0 if it does not exist and create is false.
     */
    KeyId getKeyIdWithElement ( const ElementRef& parsingFromElementRef, const String& keyName, bool useDefaultNamespace = true );

    /**
     * Parse a keyName using an element hierarchy for namespace prefix resolution.
     * @param parsingFromElementRef the element hierarchy to find namespace aliases.
     * @param keyName the key to parse
     * @param create creates the key if it does not exist.
     * @return the KeyId associated, or 0 if it does not exist and create is false.
     */
    INLINE KeyId getKeyIdWithElement ( ElementRef* parsingFromElementRef, const String& keyName ) DEPRECATED;

    /**
     * Returns the textual representation of a key from its prefixId and localKeyId.
     * @param prefixId the LocalKeyId of the namespace prefix.
     * @param localKeyId the local part of the key.
     * @return the corresponding textual representation.
     */
    INLINE const char* getKey ( LocalKeyId prefixId, LocalKeyId localKeyId );
    
    /**
     * Returns the textual representation of a key from its KeyId, using a given NamespaceAlias for prefixing namespaces.
     * @param nsAlias the NamespaceAlias to use.
     * @param keyId the fully-qualified keyId.
     * @return the corresponding textual representation. The persistence and validity of the pointer depends on the NamespaceAlias.
     */
    INLINE const char* getKey ( NamespaceAlias& nsAlias, KeyId keyId );

    /**
     * Debug-only function, returns a textual representation of the keyId provided
     * @param keyId the fully-qualified keyId.
     * @return the corresponding textual representation, prefixed by the namespace URL if the keyId is namespaced.
     */
    String dumpKey ( KeyId keyId );
    
    /**
     * Returns the local part of a key.
     */
    INLINE const char* getLocalKey ( LocalKeyId localKeyId );

    /**
     * Namespace alias getting
     * @param namespaceURL the namespace "http://..." declaration.
     * @return the corresponding NamespaceId alias.
     */
    NamespaceId getNamespaceId ( const char* namespaceURL );

    /**
     * Namespace URL getting
     * @param nsId the namespaceId
     * @return the URL of the namespace.
     */
    const char* getNamespaceURL ( NamespaceId nsId );
    
    /**
     * Parse a key
     * @param name the name to parse, may be splitted in prefix:local
     */
    void parseKey ( const String& name, LocalKeyId& prefixId, LocalKeyId& localKeyId );

    /**
     * Generate a prefixId for a non-aliased NamespaceId
     * @param nsAlias the NamespaceAlias to use
     * @param nsId the namespaceId to provide a prefix for
     * @return the localKeyId of the prefix generated.
     */
    LocalKeyId buildNamespacePrefix ( NamespaceAlias& nsAlias, NamespaceId nsId );

    /**
     * Create a xmlns:namespace key using a LocalKeyId as prefix
     */
    KeyId buildNamespaceDeclaration ( LocalKeyId prefixId );
    
    /**
     * Built-in Keys class
     * Built-in keys are a pre-parsed set of usefull keys defined in the <Xemeiah/kern/builtin_keys.h> file.
     * They are accessible READ-ONLY, using the corresponding functions :
     * - for Namespaces : the builtinKeys.(class).ns() inlined function.
     * - for Keys : the builtinKeys.(class).(key)() inlined function.
     * - the xmlns:(prefix) is accessible using builtinKeys.(class).defaultPrefix() inlined function.
     * Example :
     * The 'xsl' namespace (as defined in 'Xemeiah/kern/builtin_keys.h') is accesible using
     *   builtinKeys.xsl.ns()
     * The 'xmlns:xsl' key is accessible using builtinKeys.xsl.defaultPrefix()
     * The 'xsl:template' key is accesible using :
     *   builtinKeys.xsl.template()
     *
     * Nota : Most qnames in XML use the '-' character to separate words.
     * As '-' is forbidden in C++ values and functions, BuiltinKeys convert them to '_' instead.
     * Example : the 'xsl:value-of' markup is accessible using :
     *   builtinKeys.xsl_value_of()
     */
    class BuiltinKeys : public BuiltinKeyClass
    {
      friend class KeyCache;
      int dummy;
     public:
      /**
       * BuiltinKeys constructor
       */
      BuiltinKeys ( KeyCache& keyCache );
      
      /**
       * BuiltinKeys destructor
       */
      virtual ~BuiltinKeys ();
      
      /**
       * Build the builtin keys using the keyCache
       */
      bool __build ( KeyCache& keyCache );

#define __STARTNAMESPACE(__globalid,__url)
#define __ENDNAMESPACE(__globalid,__prefix) __BUILTIN__##__globalid##_CLASS __prefix;
#define __KEY(__key)
#include <Xemeiah/kern/builtin_keys.h>
#undef __KEY
#undef __ENDNAMESPACE
#undef __STARTNAMESPACE
    };

  protected:
    /**
     * BuiltinKeys instance
     */
    BuiltinKeys* builtinKeys;

  public:
    BuiltinKeys& getBuiltinKeys() const { return *builtinKeys; }
    /**
     * For all defined keys, run the specified job
     * @param func the functor to run for each key
     * @param arg the argument to provide to the functor
     */
    void forAllKeys ( void (*func) ( void* arg, LocalKeyId localKeyId, const char* keyName ), void* arg );

    /**
     * For all defined namespaces, run the specified job
     * @param func the functor to run for each namespace
     * @param arg the argument to provide to the functor
     */    
    void forAllNamespaces ( void (*func) ( void* arg, NamespaceId nsId, const char* url ), void* arg );

    /*
     * HTML Key Identificators
     */
  protected:
    /**
     * Check that a given key is part of a list of qnames, case insensitive
     */
    bool isHTMLKeyIn ( KeyId keyId, const char** qnames );

  public:
    /**
     * Returns true if the provided key is a non-namespaced 'html' qname, case insensitive
     */
    bool isHTMLKeyHTML ( KeyId keyId );

    /**
     * Returns true if the provided key is a non-namespaced 'head' qname, case insensitive
     */
    bool isHTMLKeyHead ( KeyId keyId );

    /**
     *
     */
    bool isHTMLKeyDisableTextProtect ( KeyId keyId );

    /**
     *
     */
    bool isHTMLKeyDisableAttributeValue ( KeyId keyId );

    /**
     *
     */
    bool isHTMLKeySkipIndent ( KeyId keyId );

    /**
     *
     */
    bool isHTMLKeySkipMarkupClose ( KeyId keyId );

    /**
     *
     */
    bool isHTMLKeyURIAttribute ( KeyId eltKeyId, KeyId attrKeyId );
  };

  /**
   * Simple list of keyids
   */
  typedef std::list<KeyId> KeyIdList;
};

#endif // __XEM_KERN_KEYCACHE_H

