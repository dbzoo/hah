/*
 * $Id$
 */
#ifndef _MENU_H_
#define _MENU_H_
#include <klone/session.h>

typedef struct _menu {
  char *l1; // Level 1 TAB
  char *l2; // Level 2 TAB
  char *page; // Page filename. ie index.kl1
  int (*cond)(); // for conditional page display
} MENU;


// Defined in menu.kl1
void menu_item(FILE *out, session_t *ss, response_t *rp, char *script, char *label, int linked);
extern MENU mymenu[];

#endif
