/*
 * $Id$
 * Declarations to the login.kl1 page.
 */
#ifndef _LOGIN_H_
#define _LOGIN_H_
#include <klone/session.h>

#define REQUIRE_AUTH(self)  login_force(session, request, response, self)

int login_force(session_t *ss, request_t *rq, response_t *rp, const char *script);
int isScriptAllowed(session_t *ss, response_t *rp, char *script);

#endif
