/* 
 Date manipulation routines - taken from libneon27
 Copyright (C) 1999-2006, Joe Orton <joe@manyfish.co.uk>
 Copyright (C) 2004 Jiang Lei <tristone@deluxe.ocn.ne.jp>

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License as published by the Free Software Foundation; either
 version 2 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.

 You should have received a copy of the GNU Library General Public
 License along with this library; if not, write to the Free
 Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 MA 02111-1307, USA

 */

#include <Xemeiah/xemfs/xemfsmodule.h>

#include <Xemeiah/auto-inline.hpp>

#include <time.h>

namespace Xem
{
  /* Generic date manipulation routines. */

  /* ISO8601: 2001-01-01T12:30:00Z */
#define ISO8601_FORMAT_Z "%04d-%02d-%02dT%02d:%02d:%lfZ"
#define ISO8601_FORMAT_M "%04d-%02d-%02dT%02d:%02d:%lf-%02d:%02d"
#define ISO8601_FORMAT_P "%04d-%02d-%02dT%02d:%02d:%lf+%02d:%02d"

  /* RFC1123: Sun, 06 Nov 1994 08:49:37 GMT */
#define RFC1123_FORMAT "%3s, %02d %3s %4d %02d:%02d:%02d GMT"
  /* RFC850:  Sunday, 06-Nov-94 08:49:37 GMT */
#define RFC1036_FORMAT "%10s %2d-%3s-%2d %2d:%2d:%2d GMT"
  /* asctime: Wed Jun 30 21:49:08 1993 */
#define ASCTIME_FORMAT "%3s %3s %2d %2d:%2d:%2d %4d"

  static const char rfc1123_weekdays[7][4] =
    { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
  static const char short_months[12][4] =
    { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct",
        "Nov", "Dec" };

  /* Returns the time/date GMT, in RFC1123-type format: eg
   *  Sun, 06 Nov 1994 08:49:37 GMT. */
  char *
  ne_rfc1123_date(time_t anytime)
  {
    struct tm *gmt;
    char *ret;
    gmt = gmtime(&anytime);
    if (gmt == NULL)
      return NULL;
    // ret = ne_malloc(29 + 1); /* dates are 29 chars long */
    ret = (char*) malloc(29 + 1);
    /*  it goes: Sun, 06 Nov 1994 08:49:37 GMT */
    snprintf(ret, 30, RFC1123_FORMAT, rfc1123_weekdays[gmt->tm_wday],
        gmt->tm_mday, short_months[gmt->tm_mon], 1900 + gmt->tm_year,
        gmt->tm_hour, gmt->tm_min, gmt->tm_sec);

    return ret;
  }

  String
  XemFSModule::timeToRFC1123Time(time_t anytime)
  {
    char* st = ne_rfc1123_date(anytime);
    if (st == NULL)
      throwException ( Exception, "Could not write time !\n" );
    return stringFromAllocedStr(st);
  }

#define GMTOFF(t) (0)
  /* Takes an RFC1123-formatted date string and returns the time_t.
   * Returns (time_t)-1 if the parse fails. */
  time_t
  ne_rfc1123_parse(const char *date)
  {
    struct tm gmt =
      { 0 };
    char wkday[4], mon[4];
    int n;
    time_t result;

    /*  it goes: Sun, 06 Nov 1994 08:49:37 GMT */
    n = sscanf(date, RFC1123_FORMAT, wkday, &gmt.tm_mday, mon, &gmt.tm_year,
        &gmt.tm_hour, &gmt.tm_min, &gmt.tm_sec);
    /* Is it portable to check n==7 here? */
    gmt.tm_year -= 1900;
    for (n = 0; n < 12; n++)
      if (strcmp(mon, short_months[n]) == 0)
        break;
    /* tm_mon comes out as 12 if the month is corrupt, which is desired,
     * since the mktime will then fail */
    gmt.tm_mon = n;
    gmt.tm_isdst = -1;
    result = mktime(&gmt);
    return result + GMTOFF(gmt);
  }

  time_t XemFSModule::rfc1123TimeToTime ( const String& time )
  {
    return ne_rfc1123_parse(time.c_str());
  }
};

