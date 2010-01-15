/* replace using posix regular expressions */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <regex.h>

static void regex_error(int err, regex_t *compiled, const char *re) {
      int length = regerror(err, compiled, NULL, 0);
      char *error = malloc(length + 1);
      regerror(err, compiled, error, length);
      fprintf(stderr, "%s: %s\n", error, re);
      free(error);
}

int rreplace (char *buf, int size, regex_t *re, char *rp)
{
  char *pos;
  int sub, so, n;
  regmatch_t pmatch [10]; /* regoff_t is int so size is int */

  int ret = regexec (re, buf, 10, pmatch, 0);
  if (ret) return ret;
  for (pos = rp; *pos; pos++)
    if (*pos == '\\' && *(pos + 1) > '0' && *(pos + 1) <= '9') {
      so = pmatch [*(pos + 1) - 48].rm_so;
      n = pmatch [*(pos + 1) - 48].rm_eo - so;
      if (so < 0 || strlen (rp) + n - 1 > size) return REG_ESPACE;
      memmove (pos + n, pos + 2, strlen (pos) - 1);
      memmove (pos, buf + so, n);
      pos = pos + n - 2;
    }
  sub = pmatch [1].rm_so; /* no repeated replace when sub >= 0 */
  for (pos = buf; !regexec (re, pos, 1, pmatch, 0); ) {
    n = pmatch [0].rm_eo - pmatch [0].rm_so;
    pos += pmatch [0].rm_so;
    if (strlen (buf) - n + strlen (rp) > size) return REG_ESPACE;
    memmove (pos + strlen (rp), pos + n, strlen (pos) - n + 1);
    memmove (pos, rp, strlen (rp));
    pos += strlen (rp);
    if (sub >= 0) break;
  }
  return 0;
}

int main (int argc, char **argv)
{
  char buf [FILENAME_MAX], rp [FILENAME_MAX], regex[FILENAME_MAX];
  regex_t re;
  
  strcpy(regex,"rf \\([0-9]\\) \\(off\\|on\\)");
  if (regcomp (&re, regex, REG_ICASE)) goto err;
  strcpy(rp,"XAP: rf \\1 is \\2");
  strcpy(buf,"rf 1 on");

  int ret = rreplace(buf, FILENAME_MAX, &re, rp);
  if(ret) {
    regex_error(ret, &re, buf);
  }
  printf("ret %d\n", ret);
  printf("buf: %s\n",buf);

  /*
  if (argc < 2) return 1;
  if (regcomp (&re, argv [1], REG_ICASE)) goto err;
  for (; fgets (buf, FILENAME_MAX, stdin); printf ("%s", buf))
    if (rreplace (buf, FILENAME_MAX, &re, strcpy (rp, argv [2])))
      goto err;
  */
  regfree (&re);
  return 0;
 err:   
  printf("ERROR\n");
  regfree (&re);
  return 1;
}

/*
    ./rreplace 'rf \([0-9]\) \(off\|on\)' 'MATCH \1'
*/
