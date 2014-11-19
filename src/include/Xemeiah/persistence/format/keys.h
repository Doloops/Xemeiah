#ifndef __XEM_KERN_FORMAT_CORE_TYPES_H
#error Shall include <Xemeiah/kern/format/core_types.h> first !
#endif

#ifndef __XEM_STORE_FORMAT_KEYS_H
#define __XEM_STORE_FORMAT_KEYS_H

namespace Xem
{
  /**
   * On-disk Key structure.
   * Each KeySegment records a LocalKeyId, and a string corresponding to the key.
   * The 64 byte key limitation shall not be problematic here.
   * We could optionnaly think of a dynamic-sized KeySegment, with hole management...
   */
  struct KeySegment
  {
    LocalKeyId id;
    static const __ui32 nameLength = 64 - sizeof(LocalKeyId);
    char name[nameLength];
  };

  /**
   * The number of keys per page.
   * sizeof(AbsolutePagePtr) represents the next key page.
   */
  static const __ui64 KeyPage_keyNumber = 
    ( PageSize - sizeof(AbsolutePagePtr) ) / sizeof ( KeySegment );

  /**
   * Key Pages : linear, entropic (no deletion) list of keys.
   * 
   * As the in-mem store must load all keys at load() time, the on-disk
   * format can be inefficient. We only have to optimize on-disk addition
   * time and a strong crash-proof mechanism (loosing all keys could be
   * really damagefull).
   *
   * Each key is splitted in the nsId part (NamespaceId) and localId part (LocalKeyId).
   * Key pages make the corresponding work between ns/local string key
   * and the corresponding nsId/localId.
   * 
   * The keyId segment pages associate (nsId,localId) -> full defined id.
   *
   * In-mem implementation may store both ns-prefixed and ns-less key names.
   */
  struct KeyPage
  {
    AbsolutePagePtr nextPage;
    KeySegment keys[KeyPage_keyNumber];
  };
  
  /**
   * On-disk Namespace segment declaration.
   * The namespaceId defines a unique Id to the namespace (defined by its url).
   * The defaultAliasId refers to a standard LocalKeyId, used in serialization when no
   * explicit LocalKeyId binding is defined for this namespace.
   */
  struct NamespaceSegment
  {
    NamespaceId namespaceId;
    static const __ui32 urlLength = 128 - sizeof(NamespaceId);
    char url[urlLength];
  };

  /**
   * Number of namespace segments per page.
   */
  static const __ui64 NamespacePage_namespaceNumber = 
    ( PageSize - sizeof(AbsolutePagePtr) ) / sizeof ( NamespaceSegment );

  /**
   * A namespace page.
   * Refer to KeyPage for further details.
   */
  struct NamespacePage
  {
    AbsolutePagePtr nextPage;
    NamespaceSegment namespaces[NamespacePage_namespaceNumber];
  };

};

#endif // __XEM_STORE_FORMAT_KEYS_H
