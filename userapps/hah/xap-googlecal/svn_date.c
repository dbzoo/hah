/*
 * svn_date.c:  (natural language) date parsing routine.
 *              accepts most of the formats accepted by 
 *                 http://www.eyrx.com/DOC/D_GNUTIL/GSU_3.HTM
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
 * 
 * Overhauled for ulibc bugs - Brett England
 */


/* XXX: autoconf should determine whether _XOPEN_SOURCE is necessary
 * below to get strptime from time.h */
#define _XOPEN_SOURCE           /* make sure to get strptime */
#include <time.h>               /* mktime, localtime, time_t, struct tm, etc */
#include <ctype.h>              /* for isdigit, isspace */
#include <string.h>             /* for memset, etc */
#include <stdlib.h>             /* for atoi */
#include <stdio.h>              /* for sscanf */
#include "svn_date.h"


/*** Prototypes. */
/* parsers for possible date formats */
static int
ISO8601ext_parse (const char *text, const time_t now, time_t * resultp);
static int
ISO8601bas_parse (const char *text, const time_t now, time_t * resultp);
static int
locale_parse (const char *text, const time_t now, time_t * resultp);

/* helper functions for the parsers */
static int ISO8601ext_parse_TZD (const char *text, struct tm *tm);
static int ISO8601bas_parse_TZD (const char *text, struct tm *tm);
static const char *locale_parse_time (const char *text, struct tm *tm);
static const char *locale_parse_date (const char *text, struct tm *tm);
static const char *locale_parse_day (const char *text, struct tm *tm);
static const char *locale_parse_zone (const char *text, struct tm *tm);
static const char *locale_parse_relative (const char *text, struct tm *tm);


/* The one public interface of the date parser:  convert human-readable
   date TEXT into a standard C time_t.  Note that 'now' is passed as
   a parameter so that you can use this routine to find out how SVN
   *would have* parsed some string at some *arbitrary* time: relative
   times should always parse the same even if svn_parse_date is called
   multiple times during a computation of finite length.  For this reason,
   the 'now' parameter is *mandatory*. Returns 0 on success. */
int                             /* XXX: svn_error_t * ??? */
svn_parse_date (const char *text, const time_t now, time_t * result)
{
  /* this is a succession of candidate parsers.  the first one that
   * produces a valid parse of the *complete* string wins. */
  if (ISO8601ext_parse (text, now, result))
    return 0;
  if (ISO8601bas_parse (text, now, result))
    return 0;
  if (locale_parse (text, now, result))
    return 0;
  /* error: invalid date */
  return 1;
}


/* Initialize 'struct tm' to all zeros (except for tm_mday field, for which
 * smallest valid value is 1).  */
static void
init_tm (struct tm *tm)
{
  memset (tm, 0, sizeof (*tm));
  tm->tm_mday = 1;              /* tm is not valid with all zeros. */
}

/* Convert 'struct tm' *expressed as UTC* into the number of seconds
 * elapsed since 00:00:00 January 1, 1970, UTC.  Similar to mktime(3),
 * which converts a 'struct tm' *expressed as localtime* into a time_t.  */
static time_t
mktime_UTC (struct tm *tm)
{
  struct tm localepoch_tm;
  time_t localepoch;
  init_tm (&localepoch_tm);
  localepoch_tm.tm_year = 70;   /* the epoch is jan 1, 1970 */
  localepoch = mktime (&localepoch_tm);
  return mktime (tm) - localepoch;
}


/* Parse according to W3C subset of ISO 8601: see
 * http://www.w3.org/TR/NOTE-datetime
 * The complete date must be specified.
 * Any unspecified time components are assumed to be zeros.
 * This is the 'extended' format of ISO8601.
 * As per W3C subset, we don't allow week and day numbers. */
static int
ISO8601ext_parse (const char *text, const time_t now, time_t * resultp)
{
  struct tm tm;
  char *cp;
  time_t result;
  /* GENERAL EXTENSIONS TO W3C SUBSET: leading zeros are optional in
   * year, month, day, hour, minute, and second fields. We also
   * allow the locale's alternative numeric symbols, if any. */

  /* Try: YYYY-MM-DDThh:mm:ssTZD YYYY-MM-DDThh:mm:ss.sTZD */
  /* EXTENSION TO W3C SUBSET: Allow commas as decimal points as per ISO. */
  /* Allow YYYY-MM-DDThh:mm:ss,sTZD too */
  init_tm (&tm);
  cp = strptime (text, " %Y-%m-%dT%H:%M:%S", &tm);
  if (cp != NULL && (cp[0] == '.' || cp[0] == ',') && isdigit (cp[1]))
    {
      /* Discard fractional seconds. */
      for (cp++; isdigit (*cp); cp++)
        ;
    }
  if (cp != NULL && ISO8601ext_parse_TZD (cp, &tm))
    goto success;

  /* Try: YYYY-MM-DDThh:mmTZD */
  init_tm (&tm);
  cp = strptime (text, " %Y-%m-%dT%H:%M", &tm);
  if (cp != NULL && ISO8601ext_parse_TZD (cp, &tm))
    goto success;

  /* Try: YYYY-MM-DD */
  init_tm (&tm);
  cp = strptime (text, " %Y-%m-%d ", &tm);
  if (cp != NULL && *cp == 0)
    goto success;
  /* XXX: sketchy: should this be midnight local or midnight UTC? */
  /* at the moment this is parsed as midnight UTC, even though no
   * time zone designator is present. */

  /* The YYYY and YYYY-MM formats are disallowed as incomplete. */

  /* Give up: can't parse as ISO8601 extended format. */
  return 0;

success:
  /* We succeeded in parsing as ISO8601 extended format! */
  result = mktime_UTC (&tm);
  if (result == (time_t) - 1)
    return 0;                   /* failure: invalid date! */
  *resultp = result;
  return 1;                     /* success */
}

/* Parse an ISO8601 extended Time Zone Designator from 'text'.
 * Update the 'struct tm' according to parsed time zone.  Returns
 * non-zero if we parse successfully and there is nothing but spaces
 * following the time zone designator. */
static int
ISO8601ext_parse_TZD (const char *text, struct tm *tm)
{
  /* Time Zone Designator is 'Z' or '+hh:mm' or '-hh:mm' */
  int isplus = 0;
  switch (*(text++))
    {
    case 'Z':
      break;                    /* done! */
    case '+':
      isplus = 1;
    case '-':
      /* parse hh:mm */
      if (isdigit (text[0]) && isdigit (text[1]) && (text[2] == ':') &&
          isdigit (text[3]) && isdigit (text[4]))
        {
          /* we jump through hoops to use atoi here, in the hopes that
           * the c-library has localized this function (along with isdigit)
           * to deal with possibly non-roman digit sets */
          char buf[3] = { '0', '0', '\0' };
          int hours, minutes;
          buf[0] = text[0];
          buf[1] = text[1];
          hours = atoi (buf);
          buf[0] = text[3];
          buf[1] = text[4];
          minutes = atoi (buf);
          /* okay, add hours and minutes to tz. mktime will fixup if we exceed
           * the legal range for hours/minutes */
          if (isplus)
            {
              /* if time specified is in +X time zone, must subtract X to get UTC */
              tm->tm_hour -= hours;
              tm->tm_min -= minutes;
            }
          else
            {
              /* if time specified is in -X time zone, must add X to get UTC */
              tm->tm_hour += hours;
              tm->tm_min += minutes;
            }
          text += 5;            /* skip parsed hh:mm string */
          break;                /* done! */
        }
      /* EXTENSION TO W3C SUBSET: allow omitting the minutes section. */
      /* parse hh */
      if (isdigit (text[0]) && isdigit (text[1]))
        {
          /* we jump through hoops to use atoi here, in the hopes that
           * the c-library has localized this function (along with isdigit)
           * to deal with possibly non-roman digit sets */
          char buf[3] = { '0', '0', '\0' };
          int hours;
          buf[0] = text[0];
          buf[1] = text[1];
          hours = atoi (buf);
          /* okay, add hours to tz. mktime will fixup if we exceed
           * the legal range for hours */
          if (isplus)    /* if specified in +X time zone, subtract X for UTC */
            {
              tm->tm_hour -= hours;
            }
          else             /* if specified in -X time zone, add X to get UTC */
            {
              tm->tm_hour += hours;
            }
          text += 2;            /* skip parsed hh string */
          break;                /* done! */
        }
    default:
      return 0;                 /* fail */
    }
  /* should be only spaces until the end of string */
  while (isspace (*text))
    text++;                     /* skip spaces */
  return (*text == '\0');       /* success if this is the end of string */
}


/* Parse according to 'basic' format of ISO8601.
 * We don't allow week and day numbers.
 */
static int
ISO8601bas_parse (const char *text, const time_t now, time_t * resultp)
{
  struct tm tm;
  char *cp;
  time_t result;
  /* extensions to ISO8601: leading zeros are optional in
   * year, month, day, hour, minute, and second fields. We also
   * allow the locale's alternative numeric symbols, if any. */

  /* Try: YYYYMMDDThhmmssTZD YYYYMMDDThhmmss.sTZD YYYYMMDDThhmmss,sTZD */
  init_tm (&tm);
  cp = strptime (text, " %Y%m%dT%H%M%S", &tm);
  if (cp != NULL && (cp[0] == '.' || cp[0] == ',') && isdigit (cp[1]))
    {
      /* Discard fractional seconds. */
      for (cp++; isdigit (*cp); cp++)
        ;
    }
  if (cp != NULL && ISO8601bas_parse_TZD (cp, &tm))
    goto success;

  /* Try: YYYYMMDDThhmmTZD */
  init_tm (&tm);
  cp = strptime (text, " %Y%m%dT%H%M", &tm);
  if (cp != NULL && ISO8601bas_parse_TZD (cp, &tm))
    goto success;

  /* Try: YYYYMMDD */
  init_tm (&tm);
  cp = strptime (text, " %Y%m%d ", &tm);
  if (cp != NULL && *cp == 0)
    goto success;
  /* XXX: sketchy: should this be midnight local or midnight UTC? */
  /* at the moment this is parsed as midnight UTC, even though no
   * time zone designator is present. */

  /* The YYYY and YYYYMM formats are disallowed as incomplete. */

  /* Give up: can't parse as ISO8601 basic format. */
  return 0;

success:
  /* We succeeded in parsing as ISO8601 basic format! */
  result = mktime_UTC (&tm);
  if (result == (time_t) - 1)
    return 0;                   /* failure: invalid date! */
  *resultp = result;
  return 1;                     /* success */
}

/* Parse an ISO8601 basic Time Zone Designator from 'text'.
 * Update the 'struct tm' according to parsed time zone.  Returns
 * non-zero if we parse successfully and there is nothing but spaces
 * following the time zone designator. */
static int
ISO8601bas_parse_TZD (const char *text, struct tm *tm)
{
/* Time Zone Designator is: 'Z' or '+hhmm' or '-hhmm' */
  int isplus = 0;
  switch (*(text++))
    {
    case 'Z':
      break;                    /* done! */
      break;
    case '+':
      isplus = 1;
    case '-':
      /* parse hhmm */
      if (isdigit (text[0]) && isdigit (text[1]) &&
          isdigit (text[2]) && isdigit (text[3]))
        {
          /* we jump through hoops to use atoi here, in the hopes that
           * the c-library has localized this function (along with isdigit)
           * to deal with possibly non-roman digit sets */
          char buf[3] = { '0', '0', '\0' };
          int hours, minutes;
          buf[0] = text[0];
          buf[1] = text[1];
          hours = atoi (buf);
          buf[0] = text[2];
          buf[1] = text[3];
          minutes = atoi (buf);
          /* okay, add hours and minutes to tz. mktime will fixup if we exceed
           * the legal range for hours/minutes */
          if (isplus)    /* if specified in +X time zone, subtract X for UTC */
            {
              tm->tm_hour -= hours;
              tm->tm_min -= minutes;
            }
          else             /* if specified in -X time zone, add X to get UTC */
            {
              tm->tm_hour += hours;
              tm->tm_min += minutes;
            }
          text += 4;            /* skip parsed hhmm string */
          break;                /* done! */
        }
      /* parse hh */
      if (isdigit (text[0]) && isdigit (text[1]))
        {
          /* we jump through hoops to use atoi here, in the hopes that
           * the c-library has localized this function (along with isdigit)
           * to deal with possibly non-roman digit sets */
          char buf[3] = { '0', '0', '\0' };
          int hours;
          buf[0] = text[0];
          buf[1] = text[1];
          hours = atoi (buf);
          /* okay, add hours to tz. mktime will fixup if we exceed
           * the legal range for hours */
          if (isplus)    /* if specified in +X time zone, subtract X for UTC */
            {
              tm->tm_hour -= hours;
            }
          else             /* if specified in -X time zone, add X to get UTC */
            {
              tm->tm_hour += hours;
            }
          text += 2;            /* skip parsed hh string */
          break;                /* done! */
        }
    default:
      return 0;                 /* fail */
    }
  /* should be only spaces until the end of string */
  while (isspace (*text))
    text++;                     /* skip spaces */
  return (*text == '\0');       /* success if this is the end of string */
}


/* Parse 'human-readable' date, for the current locale.  Words not
 * numbers. =)  This does absolute and relative time/dates.  All times are
 * *LOCAL* unless time zone is specified.
 */
static int
locale_parse (const char *text, const time_t now, time_t * resultp)
{
  struct tm tm;
  const char *cp;
  time_t result;

  /* initialize tm to 'now' in localtime */
  {
    struct tm *tmp = localtime (&now);
    memcpy (&tm, tmp, sizeof (tm));
  }

  /* parse elements repeatedly until no elements are left (or we find
   * an unparsable string. */
  while (*text != '\0')
    {
      /* skip leading whitespace */
      while (isspace (*text))
        text++;
      if (*text == '\0')
        break;
      /* now attempt parse */
      if (NULL != (cp = locale_parse_time (text, &tm)) ||
          NULL != (cp = locale_parse_date (text, &tm)) ||
          NULL != (cp = locale_parse_day (text, &tm)) ||
          NULL != (cp = locale_parse_zone (text, &tm)) ||
          NULL != (cp = locale_parse_relative (text, &tm)))
        text = cp;
      else /* can't parse as 'human-readable' absolute/relative date */
        return 0;                       
    }
  /*
  printf("%d tm_sec;   seconds \n",tm.tm_sec);
  printf("%d tm_min;   minutes \n",tm.tm_min);
  printf("%d tm_hour;  hours \n",tm.tm_hour);
  printf("%d tm_mday;  day of the month \n",tm.tm_mday);
  printf("%d tm_mon;   month \n",tm.tm_mon);
  printf("%d tm_year;  year \n",tm.tm_year);
  printf("%d tm_wday;  day of the week \n",tm.tm_wday);
  printf("%d tm_yday;  day in the year \n",tm.tm_yday);
  printf("%d tm_isdst; daylight saving time \n",tm.tm_isdst);
  */
  result = mktime (&tm);        /* tm is localtime */

  if (result == (time_t) - 1)
    return 0;                   /* failure: invalid date! */
  *resultp = result;
  return 1;                     /* success */
}

/* Parse a time specification from the 'text' string.  Fill in fields
 * of 'tm' if successful and return a pointer to the end of the time
 * specification; else leave 'tm' untouched and return NULL. */
static const char *
locale_parse_time (const char *text, struct tm *tm)
{
  char *cp;
  struct tm _tm;

  /* Time fields are zero if not explicitly specified. */
  tm->tm_sec = tm->tm_min = tm->tm_hour = 0;
  if (NULL != (cp = strptime (text, " %I:%M:%S %p ", &_tm)))
    goto success;
  if (NULL != (cp = strptime (text, " %H:%M:%S ", &_tm)))
    goto success;
  if (NULL != (cp = strptime (text, " %I:%M %p ", &_tm)))
    goto success_min;
  if (NULL != (cp = strptime (text, " %H:%M ", &_tm)))
    goto success_min;
  if (NULL != (cp = strptime (text, " %I %p ", &_tm)))
    goto success_hour;
  /* Finally, try locale-specific time format. */
  if (NULL != (cp = strptime (text, " %X ", &_tm)))
    goto success;
  /* failure */
  return NULL;
success:
    tm->tm_sec = _tm.tm_sec;
success_min:
    tm->tm_min = _tm.tm_min;
success_hour:
    tm->tm_hour = _tm.tm_hour;
    return cp;
}

/* Parse a date specification from the 'text' string.  Fill in fields
 * of 'tm' if successful and return a pointer to the end of the date
 * specification; else leave 'tm' untouched and return NULL. */
static const char *
locale_parse_date (const char *text, struct tm *tm)
{
  char *cp;
  struct tm _tm;
  
  /* 1976-09-27: ISO 8601 */
  if (NULL != (cp = strptime (text, " %Y-%m-%d ", &_tm)))
    goto success;
  /* 27 September 1976 or 27sep1976, etc */
  if (NULL != (cp = strptime (text, " %d %b %Y ", &_tm)))
    goto success;
  /* 27-September-1976 */
  if (NULL != (cp = strptime (text, " %d-%b-%Y ", &_tm)))
    goto success;
  /* 27 sep */
  if (NULL != (cp = strptime (text, " %d %b ", &_tm)))
    goto success_day;
  /* 27-sep */
  if (NULL != (cp = strptime (text, " %d-%b ", &_tm)))
    goto success_day;
  /* September-27-1976 */
  if (NULL != (cp = strptime (text, " %b-%d-%Y ", &_tm)))
    goto success;
  /* Sep 27, 1976 */
  if (NULL != (cp = strptime (text, " %b %d, %Y ", &_tm)))
    goto success;
  /* Sep 27 */
  if (NULL != (cp = strptime (text, " %b %d ", &_tm)))
    goto success_day;
  /* Sep-27 */
  if (NULL != (cp = strptime (text, " %b-%d ", &_tm)))
    goto success_day;
  /* Finally, try locale-specific date format. */
  if (NULL != (cp = strptime (text, " %x ", &_tm)))
    goto success;
  /* failure */
  return NULL;

 success:
  tm->tm_year = _tm.tm_year;
 success_day:
  tm->tm_mon = _tm.tm_mon;
  tm->tm_mday = _tm.tm_mday;

  return cp;
}

/* Parse a day-of-week specification from the 'text' string.  Advance
 * date of 'tm' to that day-of-week if parse is successful and return
 * a pointer to the end of the day-of-week specification; else leave
 * 'tm' untouched and return NULL. */
static const char *
locale_parse_day (const char *text, struct tm *tm)
{
  char *cp;
  struct tm _tm = *tm, *tmp;
  int daydiff = 0;
  time_t time;
  int i;
  /* Parse phrases like 'first monday', 'last monday', 'next monday'. */
  static const struct
  {
    const char *phrase;
    int weekdiff;
  } phrases[] = {
    /* These phrases should be translated: note that we call gettext below. */
    { " last %a ", -1 },
    { " this %a ", 0 },
    { " next %a ", 1 },
    { " first %a ", 1 },
    { " second %a ", 2 },
    { " third %a ", 3 },
    { " fourth %a ", 4 },
    { " fifth %a ", 5 },
    { " sixth %a ", 6 },
    { " seventh %a ", 7 },
    { " eighth %a ", 8 },
    { " ninth %a ", 9 },
    { 0, 0 }
  };
  /* Go through phrases and find one which matches. */
  for (i = 0; phrases[i].phrase != NULL; i++)
    if (NULL != (cp = strptime (text, phrases[i].phrase, &_tm)))
      {
        daydiff = 7 * phrases[i].weekdiff;
        goto success;
      }
  /* Try parsing simple day of the week. */
  if (NULL != (cp = strptime (text, " %a ", &_tm)))
    goto success;
  return NULL;

success:
  /* _tm.tm_wday has the day of the week.  Push current date ahead if
   * necessary. */
  time = mktime (tm);
  tmp = localtime (&time);      /* *tmp has normalized version of tm */
  if (_tm.tm_wday != tmp->tm_wday) /* If selected day is not current day: */
    /* Compute the number of days we need to push ahead. */
    daydiff += (7 + _tm.tm_wday - tmp->tm_wday) % 7;
  /* Increment day of the month. */
  _tm = *tmp;
  _tm.tm_mday += daydiff;
  /* Renormalize struct tm _tm. */
  time = mktime (&_tm);
  tmp = localtime (&time);  /* *tmp has normalized version of _tm */
  *tm = *tmp;               /* This is the new date. */

  /* A comma following a day of the week item is ignored. */
  while (isspace (*cp))
    cp++;
  if (*cp == ',')
    cp++;
  /* done! */
  return cp;
}

/* Parse a time-zone specification from the 'text' string.  Adjust
 * time of 'tm' to that time zone if parse is successful and return a
 * pointer to the end of the time-zone specification; else leave 'tm'
 * untouched and return NULL. */
static const char *
locale_parse_zone (const char *text, struct tm *tm)
{
  /* this is ugly: we accept '-0100', '+0100', '-01:00', and '+01:00'
   * as well as 'GMT' 'UTC' and 'UT' for UTC.  We also allow a single-letter
   * 'military' time zone designation, so long as it is followed by a space
   * or end-of-string.
   * No other time zone designations are allowed; too many ambiguities.
   */
  int i;
  const char *cp;
  int minutediff;
  const static struct
  {
    const char *name;
    int hourdiff;
  }
  zones[] = {
    { "GMT", 0 },
    { "UTC", 0 },
    { "UT", 0 },
    { "A", 1 },
    { "B", 2 },
    { "C", 3 },
    { "D", 4 },
    { "E", 5 },
    { "F", 6 },
    { "G", 7 },
    { "H", 8 },
    { "I", 9 },
    { "K", 10 },
    { "L", 11 },
    { "M", 12 },
    { "N", -1 },
    { "O", -2 },
    { "P", -3 },
    { "Q", -4 },
    { "R", -5 },
    { "S", -6 },
    { "T", -7 },
    { "U", -8 },
    { "V", -9 },
    { "W", -10 },
    { "X", -11 },
    { "Y", -12 },
    { "Z", 0 },
    { 0, 0 }
  };
  /* Try time zone abbreviations from zones list. */
  for (i = 0; zones[i].name != NULL; i++)
    {
      int zonelen = strlen (zones[i].name);
      if (strncmp (text, zones[i].name, zonelen) == 0 &&
          (zonelen > 1 || isspace (text[zonelen]) || text[zonelen] == '\0'))
        {
          cp = text + zonelen;
          minutediff = 60 * zones[i].hourdiff;
          goto success;
        }
    }
  /* Try +0000 +00:00 formats. */
  if ((text[0] == '-' || text[0] == '+') &&
      isdigit (text[1]) && isdigit (text[2]))
    {
      /* Again, jump through hoops to use atoi in order to accomodate
       * non-roman digit sets. */
      char buf[3] = { '0', '0', '\0' };
      buf[0] = text[1];
      buf[1] = text[2];
      minutediff = 60 * atoi (buf);
      cp = text + 3;
      if (*cp == ':')
        cp++;
      if (isdigit (cp[0]) && isdigit (cp[1]))
        {
          buf[0] = cp[0];
          buf[1] = cp[1];
          minutediff += atoi (buf);
          if (text[0] == '-')
            minutediff = -minutediff;
          cp += 2;
          goto success;
        }
    }
  /* failure */
  return NULL;

success:
  /* Apply GMT correction. */
  {
    struct tm localepoch_tm;
    time_t localepoch;
    init_tm (&localepoch_tm);
    localepoch_tm.tm_year = 70;
    localepoch_tm.tm_isdst = tm->tm_isdst;
    localepoch = mktime (&localepoch_tm);
    tm->tm_sec -= localepoch;
  }
  /* Now apply zone correction. */
  tm->tm_min -= minutediff;
  /* Renormalize tm. */
  {
    time_t time = mktime (tm);
    struct tm *tmp = localtime (&time);
    *tm = *tmp;
  }
  /* Done. */
  return cp;
}

/* Parse 'relative items' in date string, such as '1 year', '2 years
* ago', etc.  The strings in this function should be translated.
* Adjust 'tm' if parse is successful and return a pointer to the end
* of the 'relative item' specification; else leave 'tm' untouched and
* return NULL. */
static const char *
locale_parse_relative (const char *text, struct tm *tm)
{
  int i;
  /* Phrases in this list should all be translated. */
  /* XXX: it would nice to be able to allow the translator to
   * extend/shorten this list, as all languages may not have words for
   * all the time units expressed here ("fortnight"?), and some
   * languages may have words for concepts (such as "the day before
   * the day before yesterday") which are not in this list.  Hopefully
   * some internationalization expert will give me some ideas on how
   * such a thing might be done.  Certainly "unmatchable" strings may
   * be used as "translations" for untranslatable concepts, and we
   * could add "unmatchable" strings to this phrase list as place-holders
   * for those foreign time units with no english equivalent. */
  const static struct
  {
    const char *phrase;
    /* A 'month' and a 'year' may be different numbers of seconds depending
     * on which month/year we're talking about; so we can express the
     * relative time using either units of seconds or of months. */
    int seconds;
    int months;
  }
  phrases[] = {
    /* ALL PHRASES MUST END WITH %n */
    /* times in the past */
    { "%d second ago%n", -1, 0 },
    { "%d seconds ago%n", -1, 0 },
    { "%d minute ago%n", -60, 0 },
    { "%d minutes ago%n", -60, 0 },
    { "%d hour ago%n", -60 * 60, 0 },
    { "%d hours ago%n", -60 * 60, 0 },
    { "%d day ago%n", -60 * 60 * 24, 0 },
    { "%d days ago%n", -60 * 60 * 24, 0 },
    { "%d week ago%n", -60 * 60 * 24 * 7, 0 },
    { "%d weeks ago%n", -60 * 60 * 24 * 7, 0 },
    { "%d fortnight ago%n", -60 * 60 * 24 * 7 * 2, 0 },
    { "%d fortnights ago%n", -60 * 60 * 24 * 7 * 2, 0 },
    { "%d month ago%n", 0, -1 },
    { "%d months ago%n", 0, -1 },
    { "%d year ago%n", 0, -12 },
    { "%d years ago%n", 0,-12 },
      /* times in the future */
    { "%d second%n", 1, 0 },
    { "%d seconds%n", 1, 0 },
    { "%d minute%n", 60, 0 },
    { "%d minutes%n", 60, 0 },
    { "%d hour%n", 60 * 60, 0 },
    { "%d hours%n", 60 * 60, 0 },
    { "%d day%n", 60 * 60 * 24, 0 },
    { "%d days%n", 60 * 60 * 24, 0 },
    { "%d week%n", 60 * 60 * 24 * 7, 0 },
    { "%d weeks%n", 60 * 60 * 24 * 7, 0 },
    { "%d fortnight%n", 60 * 60 * 24 * 7 * 2, 0 },
    { "%d fortnights%n", 60 * 60 * 24 * 7 * 2, 0 },
    { "%d month%n", 0, 1 },
    { "%d months%n", 0, 1 },
    { "%d year%n", 0, 12 },
    { "%d years%n", 0, 12 },
      /* other special times */
      /* NOTE: the %n after first character so that 'multiplier' is 1 */
    { "t%nomorrow%n", 60 * 60 * 24, 0 },
    { "n%next week%n", 60 * 60 * 24 * 7, 0 },
    { "n%next fortnight%n", 60 * 60 * 24 * 7 * 2, 0 },
    { "n%next month%n", 0, 1 },
    { "n%next year%n", 0, 12 },
    { "y%nesterday%n", -60 * 60 * 24, 0 },
    { "l%nast week%n", -60 * 60 * 24 * 7, 0 },
    { "l%nast fortnight%n", -60 * 60 * 24 * 7 * 2, 0 },
    { "l%nast month%n", 0, -1 },
    { "l%nast year%n", 0, -12 },
      /* the current time */
      /* NOTE: the %n at start of string so that 'multiplier' is 0 */
    { "%ntoday%n", 0, 0 },
    { "%nnow%n", 0, 0 },
      /* nonce words */
    { "%nat%n", 0, 0 },
      /* end of list */
    { 0, 0, 0 }
  };
  /* Try matching the phrases in the list.
   * Note the 'clever' use of %n to find out how many letters sscanf
   * has grokked over in its match, as well as to provide a 'multiplier'
   * of 1 for certain phrases like 'next week'. */
  for (i = 0; phrases[i].phrase != NULL; i++)
    {
      int multiplier = 0, chars = 0;
      sscanf (text, phrases[i].phrase, &multiplier, &chars);
      /* if sscanf errors out before matching the final %n pattern, then
       * 'chars' will still be its initial 0 value.  We also want to
       * make sure that there is a space or end-of-string delimiter
       * after our candidate phrase. */
      if (chars != 0 && (text[chars] == '\0' || isspace (text[chars])))
        {
          /* found matching phrase! */
          text += chars;
          tm->tm_sec += multiplier * phrases[i].seconds;
          tm->tm_mon += multiplier * phrases[i].months;
          /* renormalize tm */
          {
            time_t time = mktime (tm);
            struct tm *tmp = localtime (&time);
            *tm = *tmp;
          }
          /* done */
          return text;
        }
    }
  /* failure */
  return NULL;
}
