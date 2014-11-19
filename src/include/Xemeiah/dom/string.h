#ifndef __XEM_DOM_STRING_H
#define __XEM_DOM_STRING_H

#include <string.h>
#include <string>
#include <Xemeiah/trace.h>
#include <Xemeiah/kern/utf8.h>
#include <Xemeiah/kern/format/core_types.h>
#include <list>

#include <Xemeiah/dom/item.h>

#include <stdlib.h>
#define String_NotImplemented(...) Bug ( "NOT IMPLEMENTED : " __VA_ARGS__ )

namespace Xem
{
  int stringComparator ( const char* left, const char* right, bool caseOrderUpperFirst );

#if 1 // Optimized Xem::String container, set to 0 to have the std::string

#define Log_String Debug

  typedef unsigned long StringSize;
  typedef unsigned long StringStats_Nb;

#ifdef __XEM_STRING_STATS  
  extern StringStats_Nb String_numberOfMalloc;
  extern StringStats_Nb String_totalMalloc;
  extern StringStats_Nb String_numberOfStrdup;
  extern StringStats_Nb String_numberOfFree;
  extern StringStats_Nb String_numberOfRealloc;

  void String_ShowStringStats();
  void __String_CheckStrdup ( const char* s );
#define __String_IncrementStat(__s) String_numberOf##__s ++;

#else // __XEM_STRING_STATS
#define __String_IncrementStat(...)
#define __String_CheckStrdup(...)
#define String_ShowStringStats(...)
#endif // __XEM_STRING_STATS

  class String;

  inline String stringFromAllocedStr(char* c);

  /**
   * Xemeiah's String implementation ; supposed to be UTF-8-compliant, and reduce number of strdup() (passing contents by reference).
   * \todo Make Xem::String comply with UTF-8
   */
  class String : public Item
  {
  protected:
    StringSize mallocSz;

    char* buff;
    void
    _malloc(StringSize size) __FORCE_INLINE
    {
      if (mallocSz)
        {
          Bug ( "Already malloced !\n" );
        }
      if (size < 32)
        size = 32;
      buff = (char*) malloc(size);
      Log_String ( "[STR] Alloc this=%p, buff=%p\n", this, buff );
      mallocSz = size;
      __String_IncrementStat ( Malloc );
#ifdef __XEM_STRING_STATS  
      String_totalMalloc += size;
#endif
    }

    void
    _strdup(const char* s) __FORCE_INLINE
    {
      StringSize strsz = strlen(s) + 1;
      _strdup(s, strsz);
    }

    void
    _strdup(const char* s, StringSize strsz) __FORCE_INLINE
    {
      _malloc(strsz);
      memcpy(buff, s, strsz);
      Log_String ( "[STR] Strdup this=%p, s=%p dup-> buff=%p\n", this, s, buff ); __String_CheckStrdup ( s ); __String_IncrementStat ( Strdup );
    }

    void
    _free() __FORCE_INLINE
    {
      if (mallocSz)
        {
          Log_String ( "[STR] Destroy string this=%p, buff=%p\n", this, buff ); __String_IncrementStat ( Free );
          free(buff);
          buff = NULL;
          mallocSz = 0;
        }
      else
        {
          //	  Log_String ( "[STR] NO-Destroy string this=%p, buff=%p\n", this, buff );
          buff = NULL;
        }
    }

    void
    _realloc(StringSize newSize) __FORCE_INLINE
    {
      __String_IncrementStat ( Realloc );
      if (!mallocSz)
        {
          Bug ( "Not implemented !!!\n" );
        }
      if (mallocSz < newSize)
        {
          if (newSize < mallocSz * 2)
            newSize = mallocSz * 2;
          buff = (char*) realloc(buff, newSize);
          AssertBug ( buff, "Could not realloc !!\n" );
          mallocSz = newSize;
        }
    }
  public:
    static const StringSize npos = 2 << 15;
    String() __FORCE_INLINE
    {
      mallocSz = 0;
      buff = NULL;
    }
    ~String() __FORCE_INLINE
    {
      _free();
    }

    String
    toString()
    {
      return *this;
    }

    ItemType
    getItemType() const
    {
      return Type_String;
    }

    void
    clearContents()
    {
      _free();
    }

    bool
    __isMalloced() const
    {
      return (mallocSz > 0);
    }

    String(const char* _buff, bool _isMalloced = false)
    {
      if (_isMalloced)
        {
          mallocSz = strlen(_buff) + 1;
        }
      else
        {
          mallocSz = 0;
        }
      buff = (char*) _buff;
    }

    String(const String& s)
    {
      Log_String ( "[STR] String constructor at %p from String&, mallocSz=%lu, buff=%p\n",
          this, (unsigned long) s.mallocSz, s.buff );
      mallocSz = 0;
      if (s.mallocSz)
        _strdup(s.buff);
      else
        buff = s.buff;
    }

    const char*
    c_str() const
    {
      if (!buff)
        {
          AssertBug ( !mallocSz, "Null buff, but string is supposed to be malloced !\n" );
          return "";
        }
      return buff;
    }

    String
    copy() const
    {
      /**
       * This is wrong, we have to make it alloced
       */
      if (!buff)
        return String();
      String s = stringFromAllocedStr(strdup(buff));
      return s;
    }

    StringSize
    size() const
    {
      if (!buff)
        return 0;
      return strlen(buff);
    }

    String
    operator+(const char* g)
    {
      String l(buff);
      l += g;
      return l;
    }

    String
    operator+(const String& g)
    {
      String l(buff);
      l += g.c_str();

      return l;
    }

    String&
    operator+=(const char* _buff) __FORCE_INLINE
    {
      if (!_buff)
        return *this;

      StringSize size = buff ? strlen(buff) : 0;
      size += strlen(_buff);

      if (!buff)
        {
          _strdup(_buff);
          return *this;
        }

      if (!mallocSz)
        {
          const char* tmp = buff;
          _malloc(size + 1);
          strcpy(buff, tmp);
        }
      else
        {
          _realloc(size + 1);
        }
      strcat(buff, _buff);
      return *this;
    }

    String&
    operator+=(const char _c)
    {
#if 1
      if ( !mallocSz )
        {
          _malloc ( 2 );
          buff[0] = _c;
          buff[1] = '\0';
        }
      else
        {
          StringSize size = strlen ( buff );
          _realloc(size+2);
          buff[size] = _c;
          buff[size+1] = '\0';
        }
#else
      char buff[2];
      buff[0] = _c;
      buff[1] = '\0';
      *this += buff;
#endif
      return *this;
    }

    String&
    operator+=(const String& s)
    {
      return (*this) += s.buff;
    }

    void
    appendUtf8(int code)
    {
      if (code < 0x80)
        {
          (*this) += (char) code;
          return;
        }
      unsigned char utf[8];
      int bytes;
      bytes = utf8CodeToChar(code, utf, 8);
      if (bytes == -1)
        Bug ( "Could not convert back to char* !\n" );
      (*this) += (char*) utf;
    }

    String&
    operator=(const String& s)
    {
      _free();
      if (mallocSz || s.mallocSz)
        {
          _strdup(s.buff, s.mallocSz);
        }
      else
        {
          buff = s.buff;
        }
      return *this;
    }

    void
    assignConstCharPtr(const char*s, bool _isMalloced)
    {
      _free();
      if (_isMalloced)
        {
          _strdup(s);
        }
      else
        buff = (char*) s;
    }

    String&
    operator=(const char* s)
    {
      _free();
      Log_String ( "[STR] Copy from string, this=%p, s=%p\n", this, s );
      assignConstCharPtr(s, false);
      return *this;
    }

    String&
    operator=(const char c)
    {
      _free();
      _malloc(2);

      buff[0] = c;
      buff[1] = '\0';
      return *this;
    }

    bool
    operator==(const String& s) const
    {
      Log_String ( "[left=%s,right=%s\n", buff, s.buff );
      if (!buff || !*buff)
        {
          if (!s.buff || !*(s.buff))
            return true;
          return false;
        }
      if (!s.buff || !*(s.buff))
        return false;
      return (strcmp(buff, s.buff) == 0);
    }

    bool
    operator!=(const String& s) const
    {
      return (!(*this == s));
    }

    bool
    operator<(const String& s) const
    {
      if (!buff || !s.buff)
        return false;
      return strcmp(buff, s.buff) < 0;
    }

    bool
    operator<=(const String& s) const
    {
      if (!buff || !s.buff)
        return false;
      return strcmp(buff, s.buff) <= 0;
    }

    bool
    operator>(const String& s) const
    {
      if (!buff || !s.buff)
        return false;
      return strcmp(buff, s.buff) > 0;
    }

    bool
    operator>=(const String& s) const
    {
      if (!buff || !s.buff)
        return false;
      return strcmp(buff, s.buff) >= 0;
    }

    bool
    operator==(const char* s) const
    {
      if ( (!buff || !*buff) && (!s || !*s) )
        return true;
      if (!buff || !s)
        return false;
      return (strcmp(buff, s) == 0);
    }

    int
    compare(const String& s) const
    {
      if (!buff || !s.buff)
        return -1;
      return strcmp(buff, s.buff);
    }

    int
    compare(StringSize begin, StringSize count, const String&s) const
    {
      if (!buff && !s.buff)
        return -1;
      String sub = substr(begin, count);
      Log_String ( "[STRING] compare %s (%lu, %lu) -> %s with %s\n", buff, begin, count, sub.buff, s.buff );

      return sub.compare(s);
    }

    StringSize
    find(const String& s, StringSize _begin) const
    {
      if (!buff || !s.buff)
        return npos;

      StringSize begin;
      for (begin = 0; begin < _begin; begin++)
        {
          if (buff[begin] == '\0')
            Bug ( "Index out of range !\n" );
        }

      const char* res = strstr(&(buff[begin]), s.buff);
      if (!res)
        return npos;
      return ((StringSize) res - (StringSize) &(buff[begin]));
    }

    StringSize
    find(const char c, StringSize begin) const
    {
      if (!buff)
        {
          Bug ( "String::find() on an empty string !\n" );
        }
      StringSize s;
#if 1
      for (s = 0; s < begin; s++)
        {
          if (buff[s] == '\0')
            Bug ( "Index out of range !\n" );
        }
#else
      s = begin;
#endif
      for (; buff[s]; s++)
        {
          if (buff[s] == c)
            return s;
        }
      return npos;
    }

    String
    substr(StringSize _begin, StringSize _count) const
    {
      if (!buff)
        return String();
      StringSize begin;
#if 1
      for (begin = 0; begin < _begin; begin++)
        if (buff[begin] == '\0')
          return String();
#else
      begin = _begin;
#endif
      StringSize count;
#if 1
      for (count = 0; count < _count; count++)
        if (buff[begin + count] == '\0')
          break;
#else
      count = _count;
#endif
      //      char* target = (char*) malloc ( count + 1 );
      String target;
      target._malloc(count + 1);
      strncpy(target.buff, &(buff[begin]), count);
      target.buff[count] = '\0';
      return target;
#if 0      
      char* target = _malloc ( count + 1);
      strncpy ( target, &(buff[begin]), count );
      target[count] = '\0';
      return String(target, true);
#endif
    }

    char
    at(StringSize begin) const
    {
      if (!buff)
        {
          Bug ( "String::at() on an empty string !\n" );
        }
      for (StringSize s = 0;; s++)
        {
          if (buff[s] == '\0')
            Bug ( "Index out of range !\n" );
          if (s == begin)
            return buff[s];
        }
#if 1
      Bug ( "Shall not be here !\n" );
#endif
      return 0;
    }

    void
    tokenize(std::list<String>& list, char separator = '\0') const;

    inline std::list<String>*
    tokenize(char separator = '\0') const DEPRECATED
    {
      std::list<String>* list = new std::list<String>();
      if (!c_str())
        return list;
      tokenize(*list, separator);
      return list;
    }

    inline bool
    isSpace () const
    {
      if ( ! c_str() ) return true;
      for ( const char* c = c_str() ; *c ; c++ )
        {
          if ( ! isspace(*c) ) return false;
        }
      return true;
    }

    inline
    __ui64 toUI64 () const
    {
      if ( ! c_str() || ! *c_str() )
        {
          return (__ui64)0;
        }
      __ui64 res = strtoull(c_str(), NULL,10);
      return res;
    }

    inline
    String normalizeSpace () const
    {
      if ( ! c_str() || c_str()[0] == '\0' )
        return String();
      char* res = (char*) malloc ( strlen(c_str()) + 1 );
      char* d = res;
      bool inSep = true;
      for ( const char* c = c_str(); *c ; c++ )
        {
          switch ( *c )
            {
            case ' ':
            case '\n':
            case '\r':
            case '\t':
              if ( inSep ) continue;
              inSep = true;
              *d = ' ';
              d++;
              break;
            default:
              inSep = false;
              *d = *c;
              d++;
            }
        }
      if ( d == res )
        {
          res[0] = '\0';
        }
      else
        {
          *d = '\0';
          d--;
          if ( *d == ' ' )
            *d = '\0';
        }
      return stringFromAllocedStr(res);
    }

    inline
    String trim() const
    {
      return normalizeSpace();
    }

    inline
    int compareTo(const String& right, bool caseOrderUpperFirst = false )
    {
      if ( ! c_str() ) return -1;
      if ( ! right.c_str() ) return 1;
      return stringComparator(c_str(), right.c_str(), caseOrderUpperFirst );
    }
  };

  inline String
  stringFromAllocedStr(char* c)
  {
    return String(c, true);
  }

  inline void
  setStringFromAllocedStr(String& str, char* c)
  {
    str.assignConstCharPtr(c, true);
  }

  inline String
  stringToUpperCase(const String& srcStr)
  {
    const char* src = srcStr.c_str();
    if (!src)
      return String();
    char* dest = strdup(src);
    for (char* c = dest; *c; c++)
      {
        if ('a' <= *c && *c <= 'z')
          {
            *c = ((*c - 'a') + 'A');
          }
      }
    return stringFromAllocedStr(dest);
  }

  inline String
  stringToLowerCase(const String& srcStr)
  {
    const char* src = srcStr.c_str();
    if (!src)
      return String();
    char* dest = strdup(src);
    for (char* c = dest; *c; c++)
      {
        if ('A' <= *c && *c <= 'Z')
          {
            *c = ((*c - 'A') + 'a');
          }
      }
    return stringFromAllocedStr(dest);
  }

#else // String is std::String
  typedef std::string String;
  typedef std::string::size_type StringSize;

  inline String stringFromAllocedStr ( char* c )
    {
      String s ( c );
      free ( c );
      return s;
    }

  inline void setStringFromAllocedStr ( String& str, char* c )
    {
      str = c;
      free ( c );
    }
#endif

#define _sPf_alloc_buffer_max (4 * 1024 * 1024)
#define stringPrintf(__s,...) \
  do { \
    char* _sPf_buffer = NULL; int _sPf_alloc_buffer = 256; \
    while ( true ) { \
	if ( _sPf_alloc_buffer >= _sPf_alloc_buffer_max ) { Bug ( "Maximum limit reached for stringPrintf() !\n" ); } \
	_sPf_buffer = (char*) malloc ( _sPf_alloc_buffer ); \
	int _sPf_bytes = snprintf ( _sPf_buffer, _sPf_alloc_buffer, __VA_ARGS__ ); \
	if ( _sPf_bytes >= _sPf_alloc_buffer ) \
	{ free ( _sPf_buffer ); _sPf_alloc_buffer *= 2; continue; } \
	break; } \
    setStringFromAllocedStr ( __s, _sPf_buffer); } while(0)	

  /**
   * String manipulating utilises
   */
  inline bool
  stringStartsWith(const String& res0, const String& res1)
  {
    if (!res0.size() && !res1.size())
      return true;
    else if (!res0.size())
      return false;
    else if (!res1.size())
      return true;
    return (res0.compare(0, res1.size(), res1) == 0);
  }

  inline bool
  stringEndsWith(const String& res0, const String& res1)
  {
    if (!res0.size() && !res1.size())
      return true;
    else if (!res0.size())
      return false;
    else if (!res1.size())
      return true;
    else if (res0.size() < res1.size())
      return false;
    return (res0.compare(res0.size() - res1.size(), res1.size(), res1) == 0);
  }

  inline bool
  stringContains(const String& res0, const String& res1)
  {
    return (res0.find(res1, 0) != String::npos);
  }

  inline String
  substringBefore(const String& res0, const String& res1)
  {
    StringSize found = res0.find(res1, 0);
    if (found == String::npos)
      {
        return String();
      }
    return res0.substr(0, found);
  }

  inline String
  substringAfter(const String& res0, const String& res1)
  {
    StringSize found = res0.find(res1, 0);
    if (found == String::npos)
      {

        return String();
      }
    return res0.substr(found + res1.size(), String::npos);
  }
}
;

#endif // __XEM_DOM_STRING_H
