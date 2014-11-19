/*
 * xemfsmodule-id3.cpp
 *
 *  Created on: 6 nov. 2009
 *      Author: francois
 *
 *  Based on the idea provided by id3 - an ID3 tag editor
 * Copyright (c) 1998,1999,2000 Robert Woodcock <rcw@debian.org>
 *
 * This code is hereby licensed for public consumption under either the
 * GNU GPL v2 or greater, or Larry Wall's Artistic license - your choice.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * genre.h is (c) 1998 Robert Alto <badcrc@tscnet.com> and licensed only
 * under the GPL.
 *
 */

#include <Xemeiah/xemfs/xemfsmodule.h>
#include <Xemeiah/kern/utf8.h>

#include <Xemeiah/auto-inline.hpp>

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// #define __XEM_XEMFSMODULE_HAS_GETID3 //< This option is deprecated, superseeded by xem-valhalla lib

namespace Xem
{
#include <Xemeiah/kern/builtin_keys_prolog_inst.h>
#include <Xemeiah/xemfs/builtin-keys/xem_id3>
#include <Xemeiah/kern/builtin_keys_postlog.h>

#ifdef __XEM_XEMFSMODULE_HAS_GETID3
  struct ID3Info
  {
    char tag[3];
    char title[30];
    char artist[30];
    char album[30];
    char year[4];
    /* With ID3 v1.0, the comment is 30 chars long */
    /* With ID3 v1.1, if comment[28] == 0 then comment[29] == tracknum */
    char comment[30];
    unsigned char genre;
  };

  void
  XemFSModule::setId3Property ( ElementRef& documentElement, KeyId keyId, const char* pty, int length )
  {
    String value = iso8859ToUtf8((const unsigned char*)pty,length).normalizeSpace();
    // String value = stringFromAllocedStr(strndup(pty, length)).normalizeSpace();
    if ( ! value.isSpace() )
      documentElement.addAttr(getXProcessor(), keyId, value);

  }

  void
  XemFSModule::getId3 ( ElementRef& documentElement )
  {
    AssertBug ( documentElement, "No Element !\n" );

    AssertBug ( documentElement.getKeyId() == xem_fs.document(),
        "The key '%s' is not implemented, at documentElement %s.\n",
        documentElement.getKey().c_str(), documentElement.generateVersatileXPath().c_str() );

    String href = documentElement.getAttr(xem_fs.href());

    // FILE* fp = fopen(href.c_str(), "r");
    int fd = open(href.c_str(),O_RDONLY);
    if ( fd == -1 )
      {
        Error ( "Could not open '%s' err=%d:%s\n", href.c_str(), errno, strerror(errno));
        return;
      }
    struct stat finfo;

    int res = fstat(fd, &finfo);
    if ( res == -1 )
      {
        Error ( "Could not stat '%s' err=%d:%s\n", href.c_str(), errno, strerror(errno));
        return;
      }

    if (!S_ISREG ( finfo.st_mode ))
      {
        NotImplemented("Not a file !\n" );
      }

    ID3Info id3info;
    off_t start = finfo.st_size;
    if ( start < 128 )
      {
        Error ( "Too small a file.\n" );
        return;
      }
    start -= 128;

    int rd = pread ( fd, &id3info, 128, start );
    if ( rd == -1 )
      {
        Error ( "Could not read '%s' err=%d:%s\n", href.c_str(), errno, strerror(errno));
        ::close(fd);
        return;
      }
    else if ( rd != 128 )
      {
        Error ( "Could not read all '%s' read=%d/128\n", href.c_str(), rd);
        ::close(fd);
        return;
      }
    ::close(fd);
#if 0
    memset(&id3info, 0, sizeof(ID3Info));

    if (lseek(fd, -128, SEEK_END) < 0)
      {
        /* problem rewinding */
        ::close(fd);
        throwException ( Exception, "Could not seek to the end : err=%d:%s\n", errno,strerror(errno));
      }
    else
      { /* we rewound successfully */
        if (read(fd, &id3info, 128) < 0)
          {
            /* read error */
            ::close(fd);
            throwException(Exception,"Could not read : err=%d:%s\n", errno,strerror(errno));

          }
      }
#endif
    ::close(fd);
    if (strncmp(id3info.tag, "TAG", 3))
      {
        Error ( "File has no ID3 tag !\n" );
        return;
        throwException ( Exception, "File has no ID3 tag !\n" );
      }

    setId3Property(documentElement,xem_id3.artist(),id3info.artist,30);
    setId3Property(documentElement,xem_id3.album(),id3info.album,30);
    setId3Property(documentElement,xem_id3.year(),id3info.year,4);
    setId3Property(documentElement,xem_id3.title(),id3info.title,30);

    if (id3info.comment[28] == 0)
      {
        setId3Property(documentElement,xem_id3.comment(),id3info.comment,28);

        int tracknumInt = id3info.comment[29];
        String tracknum;
        stringPrintf(tracknum, "%d", tracknumInt);
        documentElement.addAttr(getXProcessor(), xem_id3.tracknum(), tracknum);

      }
    else
      {
        setId3Property(documentElement,xem_id3.comment(),id3info.comment,30);
      }
  }


  void
  XemFSModule::instructionGetId3(__XProcHandlerArgs__)
  {
    XPath select(getXProcessor(), item, xem_fs.select());
    ElementRef elt = select.evalElement();

    getId3(elt);
  }
#else // __XEM_XEMFSMODULE_HAS_GETID3
  void
  XemFSModule::instructionGetId3(__XProcHandlerArgs__)
  {
    throwException(Exception,"Deprecated instruction : xem-fs:id3 !");
  }
#endif
};
