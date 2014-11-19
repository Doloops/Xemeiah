#include <Xemeiah/parser/parser.h>

#include <Xemeiah/auto-inline.hpp>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#define Log_PDT Debug

namespace Xem
{
  void Parser::setDocTypeEntity ( const String& name_, const String& value_, bool system )
  {
    String name = stringFromAllocedStr(strdup(name_.c_str()));
    if ( system )
      {
        String reference = initialReader.getBaseURI ();
        reference += value_; 
        parsing.userDefinedSystemEntities[name] = reference;
      }
    else
      {
        String value = stringFromAllocedStr(strdup(value_.c_str()));
        parsing.userDefinedEntities[name] = value;
      }
  }

  String Parser::getDocTypeFileContents ( const String& name )
  {
    String reference = initialReader.getBaseURI ();
    reference += name; 
    Log_PDT ( "Parsing docfile : '%s'\n", reference.c_str() );

    int fd = ::open ( reference.c_str(), O_RDONLY );
    if ( fd == -1 )
      {
        /**
         * not found URLs are non-fatal
         */
        Warn ( "Could not read file '%s'\n", reference.c_str() );
        return "";
        throwException ( Exception, "Could not open file '%s' : err=%d:%s\n", reference.c_str(), errno, strerror(errno) );
      }
      
    struct stat st;
    if ( fstat ( fd, &st ) )
      {
        ::close ( fd ); 
        throwException ( Exception, "Could not stat() file '%s' : err=%d:%s\n", reference.c_str(), errno, strerror(errno) );
      }
    if ( S_ISREG(st.st_mode) || S_ISLNK(st.st_mode) )
      {
        char* buffer = NULL;
        buffer = (char*) malloc ( st.st_size + 1 );
        ssize_t res = read ( fd, buffer, st.st_size );
        buffer[st.st_size] = '\0';
        ::close ( fd );
        if ( res != st.st_size )
          {
            free ( buffer );
            throwException ( Exception, "Could not read() file '%s' : err=%d:%s\n", reference.c_str(), errno, strerror(errno) );
          }

        return stringFromAllocedStr ( buffer );
      }
    else
      {
        ::close ( fd );
        throwException ( Exception, "Invalid file type for '%s'\n", reference.c_str() );
      }
  }

};

