/* $Id$
 */
#include <klone/request.h>

void upperstr(char *s);
int commaTok(char *arg[], int arglen, char *str);

void submit_statickey(request_t *, char *, char *);
void submit_dynkey(request_t *,char *, char *, int);
void submit_dynkeys(request_t *,char *, char *, int);
char *iniGetDynLabel(char *name, int id);
char *iniGetDynKeyWithDefault(char *section, char *keyPattern, int id, char *dfltPattern);
char *iniGetDynKey(char *section, char *keyPattern, int id);
