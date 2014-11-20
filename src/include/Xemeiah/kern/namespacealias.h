#ifndef __XEM_CORE_NAMESPACEALIAS_H
#define __XEM_CORE_NAMESPACEALIAS_H

#include <Xemeiah/dom/string.h>
#include <Xemeiah/kern/exception.h>
#include <Xemeiah/kern/format/core_types.h>

#include <map>


namespace Xem
{
  class KeyCache;
  
  /**
   * The NamespaceAlias class is in charge of mapping a KeyId prefix to a given namespace.
   * This mapping is very volatile, i.e. it depends on the evaluation context,
   * and the namespace aliases defined before.
   * For this reason, it is outside of the KeyCache class.
   * 
   * For each stack, we have two mappings to store : the aliasId to namespaceId mapping for prefix resolution
   * and the namespaceId to aliasId for prefix creation (at serialization, for example).
   *
   * When calling the pop() function, we have to make sure that all mappings are restored to their original values
   * for both the two kinds of mappings.
   */
  class NamespaceAlias
  {
    friend class KeyCache;
  protected:
    typedef __ui32 StackLevel;
  protected:
    /**
     * Reference to the KeyCache
     */
    KeyCache& keyCache;
    
    /*
     * Current mapping.
     */

    typedef std::map<NamespaceId,LocalKeyId> NamespaceIdToLocalKeyIdMap;
    /**
     * Defines the current mapping from a NamespaceId to a LocalKeyId.
     */
    NamespaceIdToLocalKeyIdMap namespaceIdToLocalKeyIdMap;
    
    typedef std::map<LocalKeyId,NamespaceId> LocalKeyIdToNamespaceIdMap;
    /**
     * Defines the current mapping from a LocalKeyId to a NamespaceId;
     * The zero LocalKeyId is used for default namespace.
     */
    LocalKeyIdToNamespaceIdMap localKeyIdToNamespaceIdMap;

    /**
     * Current stack level.
     */
    StackLevel currentStackLevel;

    /**
     * The AliasDeclaration defines an alias setting at one level of stack
     * and the way to restore previous values easily.
     */
    struct AliasDeclaration
    {
      LocalKeyId localKeyId;
      NamespaceId namespaceId;
      
      LocalKeyId previousLocalKeyId;
      NamespaceId previousNamespaceId;
            
      AliasDeclaration* nextDeclaration;
    };
    
    /**
     * Little structure to store each list of Declarations per level.
     */
    struct StackHeader
    {
      StackLevel level;
      AliasDeclaration* firstDeclaration;

      StackHeader* previousHeader;
    };
    
    /**
     * Head of the StackHeader list of declarations.
     */
    StackHeader* stackHead;
    
    /**
     * Special constructor for the KeyCache::defaultNamespaceAlias()
     * This class will *not* instanciate default namespaces.
     */
    NamespaceAlias ( KeyCache& keyCache, bool noDefaultInstanciation );
    
    /**
     * Returns a StackHeader for the current StackLevel.
     * If there is no StackHeader defined for this level, it will create one.
     * @return a StackHeader for *this* level.
     */
    StackHeader* createStackHeader ();
    
  public:
    /**
     * NamespaceAlias constructor.
     * The keyCache is used to build the default namespace aliases (xmlns, xmlns:xmlns, xmlns:xml, xml:test).
     */
    NamespaceAlias ( KeyCache& keyCache );
    
    /**
     * NamespaceAlias destructor.
     */
    virtual ~NamespaceAlias ();

    /**
     * Sets a new namespace prefix aliasing.
     * The provided aliasId may be a xmlns:(namespace) keyId, or a default namespace declaration with (nons_) xmlns keyId.
     * @param aliasId the prefixId keyId, a xmlns:(namespace) or a xmlns keyId.
     * @param namespaceId the corresponding namespaceId to map.
     * @param overwriteAlias set to true to allow overwriting of an alias already defined in the CURRENT stack level.
     */
    void setNamespacePrefix ( KeyId aliasId, NamespaceId namespaceId, bool overwriteAlias );    

    /**     * Sets a new namespace prefix aliasing.
     * The provided aliasId may be a xmlns:(namespace) keyId, or a default namespace declaration with (nons_) xmlns keyId.
     * @param aliasId the prefixId keyId, a xmlns:(namespace) or a xmlns keyId.
     * @param namespaceId the corresponding namespaceId to map.
     * \note default : overwriteAlias is set to false.
     */
    inline void setNamespacePrefix ( KeyId aliasId, NamespaceId namespaceId )
    { setNamespacePrefix ( aliasId, namespaceId, false ); }
        
    /**
     * Gets a NamespaceId association from a xmlns:(namespace) or a xmlns KeyId.
     * @param aliasId the KeyId of the namespace aliasing declaration, a xmlns:(namespace) or a xmlns keyId.
     * @return the corresponding NamespaceId, or 0 if no aliasing is defined.
     */
    NamespaceId getNamespaceIdFromAlias ( KeyId aliasId );

    /**
     * Gets a NamespaceId association from a LocalKeyId prefix.
     * @param prefixId the prefixId, i.e. the (namespace):* part of the key.
     * @return the corresponding NamespaceId, or 0 if no aliasing is defined.
     */
    virtual NamespaceId getNamespaceIdFromPrefix ( LocalKeyId prefixId );

    /**
     * Provides the current prefix defined for a namespaceId.
     * @param namespaceId the namespaceId to provide a prefix for.
     * @return the corresponding LocalKeyId of the prefix.
     */
    LocalKeyId getNamespacePrefix ( NamespaceId namespaceId );

    /**
     * Get default namespaceId for the current context
     * @return the default NamespaceId
     */
    NamespaceId getDefaultNamespaceId ();

    /**
     * Set default namespaceId for the current context
     * This is just a helper function, calling setNamespacePrefix() with nons_xmlns as KeyId.     * @return the default NamespaceId
     */
    void setDefaultNamespaceId ( NamespaceId nsId );
    
    /**
     * Push the NamespaceAlias declaration stack.
     * The next call to pop() will automagically remove all the prefixes set by setNamespacePrefix().
     */
    void push ();
    
    /**
     * Pop the NamespaceAlias declaration stack.
     * All the prefixes set by setNamespacePrefix() after the last call to push() are removed.
     * Original values are restored as well.
     */
    void pop ();
    
    /**
     *  Check whether we can pop or not
     */
    bool canPop ();

    /**
     * Debuggig only : access to current StackLevel
     */
    inline StackLevel getCurrentStackLevel() const { return currentStackLevel; }
  };



};

#endif // __XEM_CORE_NAMESPACEALIAS_H
