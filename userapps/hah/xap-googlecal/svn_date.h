/*
 * svn_date.h:  (natural language) date parsing routine.
 *
 * ====================================================================
 * Copyright (c) 2000-2001 C. Scott Ananian.  All rights reserved.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution.  The terms
 * are also available at http://subversion.tigris.org/license-1.html.
 * If newer versions of this license are posted there, you may use a
 * newer version instead, at your option.
 * ====================================================================
 */


#ifndef SVN_DATE_H
#define SVN_DATE_H

#include <time.h>

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */

/* The one public interface of the date parser:  convert human-readable
   date TEXT into a standard C time_t.  Note that 'now' is passed as
   a parameter so that you can use this routine to find out how SVN
   *would have* parsed some string at some *arbitrary* time: relative
   times should always parse the same even if svn_parse_date is called
   multiple times during a computation of finite length.  For this reason,
   the 'now' parameter is *mandatory*. Returns 0 on success. */
  int svn_parse_date (const char *text, const time_t now, time_t * result);

#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif                          /* SVN_DATE_H */
