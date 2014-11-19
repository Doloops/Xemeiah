#define __STARTNAMESPACE(__globalid,__url) \
  bool __BUILTIN__##__globalid##_CLASS::__build ( KeyCache& keyCache ) \
  { \
    keyCache.processBuiltinNamespaceId ( xmlns_PREFIX_VALUE, ns_VALUE, STRINGIFY(__globalid), __url ); \
    Debug ( "Building class '%s', nsId='%x' (namespace='%s')\n", STRINGIFY(__globalid), ns_VALUE,__url );

#define __KEY(__key) keyCache.processBuiltinKeyId( __key##_VALUE, ns_VALUE, STRINGIFY(__key) );
#define __ENDNAMESPACE(__globalid,__prefixid) \
    Debug ( "Built class '%s' '%s' (ns=%x)\n", STRINGIFY(__globalid), STRINGIFY(__prefixid), ns_VALUE ); \
    return true; \
 }
