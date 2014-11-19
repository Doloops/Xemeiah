#define __STARTNAMESPACE(__globalid,__url) \
    class __BUILTIN__##__globalid##_CLASS : public BuiltinKeyClass \
    { \
      public: \
        bool __build ( KeyCache& keyCache ); \
        __BUILTIN__##__globalid##_CLASS(KeyCache& keyCache) { __build (keyCache); } \
        ~__BUILTIN__##__globalid##_CLASS() {} \
        NamespaceId ns_VALUE; KeyId xmlns_PREFIX_VALUE; \
        inline NamespaceId getNamespaceId() const { return ns_VALUE; } \
        inline NamespaceId ns() const { return getNamespaceId(); } \
        inline KeyId defaultPrefix() const { return xmlns_PREFIX_VALUE; }
#define __ENDNAMESPACE(__globalid,__prefix) \
    };
#define __KEY(__key) KeyId __key##_VALUE; inline KeyId __key() const { return __key##_VALUE; }

