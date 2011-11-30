/* $Id$
 */
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h> 
#include <signal.h>
#include "const.h"
#include "utils.h"

void upperstr(char *s) {
    while(*s) {
	*s = toupper(*s);
	s++;
    }
}

// Carve up a string delimited by a comma
// ex:   hello,world,foobar
//xo arg[0] = hello
// arg[1] = world
// arg[2] = foobar
// return 3
int commaTok(char *arg[], int arglen, char *str)
{
	 char *p = str;
	 int j=0;
	 arg[j++] = str;
	 do {
		  if(*p == ',') {
			   arg[j++] = p+1;
			   *p = 0;
			   if (j == arglen) return j;
		  }
		  p++;
	 } while (*p);
	 return j;
}

void submit_statickey(request_t *request, char *section, char *key)
{
	char formKey[20];
	snprintf(formKey, sizeof(formKey), "%s.%s", section, key);
	const char *value = request_get_arg(request, formKey);
	if(value) {
		ini_puts(section, key, value, con.inifile);
	}
}

void submit_dynkey(request_t *request, char *section, char *keyPattern, int id)
{
	char iniKey[20];
	snprintf(iniKey, sizeof(iniKey), keyPattern, id);
	const char *value = request_get_arg(request, iniKey);
	if(value) {
		ini_puts(section, iniKey, value, con.inifile);
	}
}

void submit_dynkeys(request_t *request, char *section, char *keyPattern, int cnt) {
	int i;
	for(i=1; i<=cnt; i++) {
		submit_dynkey(request, section, keyPattern, i);
	}
}

static char buf[128];

char *iniGetDynKeyWithDefault(char *section, char *keyPattern, int id, char *dfltPattern) {
	char key[20];
	snprintf(key, sizeof(key), keyPattern, id);
	if(ini_gets(section, key, "", buf, sizeof(buf), con.inifile) == 0) {
		snprintf(buf, sizeof(buf), dfltPattern, id);
	}
	return buf;
}

char *iniGetDynKey(char *section, char *keyPattern, int id) {
	char key[20];
	snprintf(key, sizeof(key), keyPattern, id);
	ini_gets(section, key, "", buf, sizeof(buf), con.inifile);
	return buf;
}

// Given a type and key return its label.
char *iniGetDynLabel(char *name, int id) {
	if(strcmp(name,"sensor") == 0) 
		return iniGetDynKeyWithDefault("1wire","sensor%d.label",id,"Sensor %d");
	if(strcmp(name,"rf") == 0) 
		return iniGetDynKeyWithDefault("rf","rf%d.label",id,"RF %d");
	if(strcmp(name,"relay") == 0) 
		return iniGetDynKeyWithDefault("relay","relay%d.label",id,"Relay %d");
	if(strcmp(name,"input") == 0) 
		return iniGetDynKeyWithDefault("input","input%d.label",id,"Input %d");
}
