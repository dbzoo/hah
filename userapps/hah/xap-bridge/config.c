/* $Id$
   Configuration File and Structure Functions

  Copyright (c) Brett England, 2010
  Portions Copyright (C) 1996  Joseph Croft <joe@croftj.net>  
   
  No commercial use.
  No redistribution at profit.
  All derivative work must retain this message and
  acknowledge the work of the original author.  
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include "cfgParse.h"
#include "bridge.h"

static int baudList[] = {
   0, 50, 75, 110, 134, 150, 200, 300, 600, 1200, 1800,
   2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400
};

void showConfig()
{
     if (g_debuglevel < LOG_INFO) return;
     portConf *pEntry;
     int x;
   
     for (x = 1, pEntry = pPortList; pEntry != NULL; pEntry = pEntry->pNext, x++)
     {
	  printf("Entry(0x%lx) %d:\n", (long)pEntry, x);
	  printf("\tEnbl = %d\n", pEntry->enabled);
	  printf("\tDevc = %s\n", pEntry->devc);
	  printf("\tSpeed = %d\n", baudList[pEntry->speed]);
	  printf("\tNext = 0x%lx\n", (long)pEntry->pNext);
     }
}

/*
** int loadConfig()
** 
** Reading .INI file and parse each serial port entry
**
** [bridge]
** s1=/dev/ttyS0:B38400 CLOCAL IGNBRK CRTSCTS CS8
** s2=/dev/ttyUSB1:B38400 CLOCAL IGNBRK CRTSCTS CS8
*/
int loadConfig()
{
   portConf *pEntry;
   int x, err;
   char entry[128];
   char key[4];

   if (pPortList != NULL)
   {
      for (pEntry = pPortList; pEntry; pEntry = pPortList)
      {
         pPortList = pEntry->pNext;
         free(pEntry);
      }
      pPortList = NULL;
   }

   for (x = 1; ; x++)
   {
         debug(LOG_DEBUG, "loadConfig():Parsing next item");

	 sprintf(key,"s%d", x);
	 if(ini_gets("bridge", key, "", entry, sizeof(entry), inifile) == 0)
	      break;

	 debug(LOG_DEBUG, "loadConfig(): Parsing %s", entry);

         if ((pEntry = parseCfgEntry(entry, &err)) != NULL)
         {
            debug(LOG_DEBUG, "loadConfig():Adding entry to list");
            debug(LOG_DEBUG, "loadConfig():Start of list = 0x%x",
                            (unsigned int)pPortList);
            debug(LOG_DEBUG, "loadConfig():This entry = 0x%x",
                            (unsigned int)pEntry);
            pEntry->pNext = pPortList;
            pPortList = pEntry;
         }
         else if (err == ECFG_SYNTAX)
            debug(LOG_WARNING, "Config File syntax error in item %d\n", x);
         else if (err == ECFG_MEMORY)
            debug(LOG_WARNING, "Out of memory!!!\n");
         else
            break;
   }
   debug(LOG_DEBUG, "loadConfig():Parse complete!");

   showConfig();
   return x-1;
}

void setBaudStr(portConf *pDevice, const char *pBaudStr)
{
   debug(LOG_DEBUG, "setBaudStr(%s):Enter", pBaudStr);

   strncpy(pDevice->baud, pBaudStr, PC_MAXFIELD);
   if (strlen(pBaudStr) >= PC_MAXFIELD)
      pDevice->baud[PC_MAXFIELD -1] = '\0';

   debug(LOG_DEBUG, "setBaudStr():Exit");
}

void setPortStr(portConf *pDevice, const char *pPortStr)
{
   debug(LOG_DEBUG, "setPortStr(%s):Enter", pPortStr);

   strncpy(pDevice->port, pPortStr, PC_MAXFIELD);
   if (strlen(pPortStr) >= PC_MAXFIELD)
      pDevice->port[PC_MAXFIELD -1] = '\0';

   debug(LOG_DEBUG, "setPortStr():Exit");
}
