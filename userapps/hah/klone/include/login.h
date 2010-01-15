/*
 * $Id:$
 * Declarations to the login.kl1 page.
 */
#ifndef _LOGIN_H_
#define _LOGIN_H_
#include <klone/session.h>

#define MENU_ITEM(script, label) menu_item(out, session, response, script, label)

#define REQUIRE_AUTH(self)  login_force(session, request, response, self)
#define PAGE_ALLOWED(page)  isPageAllowed(session, page)

int login_force(session_t *ss, request_t *rq, response_t *rp, const char *script);
int isPageAllowed(session_t *ss, char *page);
void menu_item(FILE *out, session_t *ss, response_t *rp, char *script, char *label);

#endif
