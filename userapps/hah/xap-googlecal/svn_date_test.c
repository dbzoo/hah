#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "svn_date.h"

/* Quick-and-dirty test harness for svn_date.c.
 * Usage:
 *  ./svn_date "yesterday"
 *  ./svn_date "next year"
 *  ./svn_date "Fri, 22 Aug 2003 21:25:11 -0400"
 *  ./svn_date "2001-04-03T12:43:55-04:00"
 * etc.
 */
int
main (int argc, char **argv)
{
  char buf[80];
  time_t now, then;
  struct tm *tm;
  if (argc != 2)
    {
      printf ("Usage:\n\t%s \"date specification\"\n", argv[0]);
      exit (1);
    }
  now = time (NULL);
  if (svn_parse_date (argv[1], now, &then) != 0)
    {
      printf ("Invalid date.\n");
      exit (2);
    }
  tm = localtime (&then);
  /* print out parsed date RFC822-style. */
  strftime (buf, sizeof (buf), "%a, %d %b %Y %H:%M:%S %z", tm);
  printf ("%s\n", buf);
  return 0;
}
