/* $Id$
*  Configuration File and Structure Functions
*/
#ifndef _CONF_H
#define _CONF_H

#include <netinet/in.h>
#include <termios.h>
#include <stdio.h>

#define PC_MAXFIELD  16
#define PC_MAXDEVC   80

struct _portConf
{
   int                  enabled;
   struct termios       tios;
   char                 devc[PC_MAXDEVC];
   char                 baud[PC_MAXFIELD];
   char                 port[PC_MAXFIELD];
   speed_t              speed;
   int fd;
   struct _portConf     *pNext;
};

typedef struct _portConf portConf;

void showConfig();
int loadConfig();
portConf *parseCfgEntry(const char *str, int *err);
void setBaudStr(portConf *pDevice, const char *pBaudStr);
void setPortStr(portConf *pDevice, const char *pPortStr);

#endif
