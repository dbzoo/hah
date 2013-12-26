/* $Id$
 */
#include <klone/request.h>

void upperstr(char *s);
int commaTok(char *arg[], int arglen, char *str);

void submit_statickey(request_t *, char *, char *, const char *);
void submit_dynkey(request_t *,char *, char *, int, const char *);
void submit_dynkeys(request_t *,char *, char *, int, const char *);
char *iniGetDynLabel(char *name, int id, const char *);
char *iniGetDynKeyWithDefault(char *section, char *keyPattern, int id, char *dfltPattern, const char *);
char *iniGetDynKey(char *section, char *keyPattern, int id, const char *);
