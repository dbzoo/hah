/* Copyright (c) Brett England, 2010
 
   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.
*/
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "minIni.h"

char *xapLowerString(char *str)
{
        char *s = str;
        if(s)
                for(s=str; *s; s++)
                        *s = tolower(*s);
        return str;
}

char *rot47(char *s)
{
  char *p = strdup(s);
  char *ret = p;
  while(*p) {
    if(*p >= '!' && *p <= 'O')
      *p = ((*p + 47) % 127);
    else if(*p >= 'P' && *p <= '~')
      *p = ((*p - 47) % 127);
    p++;
  }
  return ret;
}

/** Convert a string into a series of hex characters representing the string
*/
char *str2hex(char *str)
{
        const char *hex="0123456789ABCDEF";
        char *out = (char *)malloc(strlen(str)*2+1);
        char *p;
        int i=0;
        for(p=str; *p; p++) {
                out[i++] = hex[*p & 0x0f];
                out[i++] = hex[*p>>4 & 0x0f];
        }
        out[i] = '\0';
        return out;
}

char *getINIPassword(char *section, char *key, char *inifile)
{
        long n;
        char inipasswd[64];

	n = ini_gets(section, key,"",inipasswd, sizeof(inipasswd), inifile);
	if (n == 0)
	  return NULL;
	
	if(strncmp("{frob}", inipasswd, 6) == 0) {
	  return rot47(&inipasswd[6]);
	}
	
	char passwd[80];
	strcpy(passwd, "{frob}");
	char *p = rot47(inipasswd);
	strcat(passwd, p);
	ini_puts(section, key, passwd, inifile);
	free(p);
	return strdup(inipasswd);	
}
