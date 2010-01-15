/* $Id$
 */

#ifndef ALIAS_H
#define ALIAS_H

#include <sys/types.h>
#include <regex.h>

typedef struct _alias {
  char *select;
  regex_t *selectp; // compiled regex.
  char *xap;
  struct _alias *next;
} alias_t;

alias_t *parseAliasDoc(char *docname);
void freeAliases(alias_t *alias);
void dumpAliases(alias_t *alias);
char *matchAlias(alias_t *alias, char *seek);

#endif
