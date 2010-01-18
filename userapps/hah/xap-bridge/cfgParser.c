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
#include <termios.h>
#include "cfgParse.h"
#include "bridge.h"


extern FILE *yyin;
extern char *yytext;

int yylex();

YYSTYPE yylval;

int xlate_iflag[] =
{
     IGNBRK, BRKINT, IGNPAR, PARMRK, INPCK, ISTRIP,
     INLCR, IGNCR, ICRNL,
     IUCLC, IXON, IXANY, IXOFF, IMAXBEL
};

int xlate_oflag[] =
{
     OPOST, OLCUC, ONLCR,
     OCRNL, ONOCR, ONLRET, OFILL, OFDEL,
     NL0, NL1, CR0, CR1, CR2, CR3, TAB0, TAB1, TAB2, TAB3,
     XTABS, BS0, BS1, VT0, VT1, FF0, FF1
};

int xlate_cflag[] =
{
     CS5, CS6, CS7, CS8, CSTOPB, PARENB, PARODD, HUPCL, CLOCAL,
     CIBAUD, CRTSCTS, CREAD
};

int xlate_lflag[] =
{
     ISIG, ICANON, XCASE,
     ECHO, ECHOE, ECHOK, ECHONL, ECHOCTL, ECHOPRT,
     ECHOKE, FLUSHO, NOFLSH, TOSTOP, PENDIN, IEXTEN
};

int xlate_speed[] =
{
     B50, B75, B110, B134, B150, B200, B300, B600, B1200, B1800, B2400,
     B4800, B9600, B19200, B38400,
     B57600, B115200, B230400
};

portConf *parseCfgEntry(const char *str, int *err)
{
     int token, state = 0;
     portConf *rv;

     syslog(LOG_DEBUG, "parseCfgEntry():Enter");
     
     yy_scan_string(str);

     *err = 0;
     if ((rv = (portConf *)malloc(sizeof(portConf))) != NULL)
     {
	  debug(LOG_DEBUG, "parseCfgEntry():Starting to Parse");
	  memset(rv, 0, sizeof(portConf));
	  while (rv != NULL && state >= 0)
	  {
	       if ((token = yylex()) == ';' || token == 0)
	       {
		    debug(LOG_DEBUG, "parseCfgEntry():Found end of Entry");
		    if (state == 0)
		    {
			 *err = 0;
			 free (rv);
			 rv = NULL;
		    }
		    state = -1;
		    continue;
	       }
	       else if (token == ':')
	       {
		    debug(LOG_DEBUG, "parseCfgEntry():Found end of state %d", 
			  state);
		    if (++state > 1)
		    {
			 *err = ECFG_SYNTAX;
			 free (rv);
			 rv = NULL;
		    }
		    continue;
	       }
	       debug(LOG_DEBUG, "parseCfgEntry():Found token  in state %d- %d:|%s|",
		     state, token, yytext);
	       switch (state)
	       {
	       case 0:
		    if (token == YY_DEVICE)
			 strcpy(rv->devc, yylval.s);
		    else
		    {
			 debug(LOG_DEBUG, "parseCfgEntry():Syntax Error");
			 *err = ECFG_SYNTAX;
			 free (rv);
			 rv = NULL;
			 continue;
		    }
		    break;

	       case 1:
		    if (token < 100 || token >= YY_INT)
		    {
			 debug(LOG_DEBUG, "parseCfgEntry():Syntax Error");
			 *err = ECFG_SYNTAX;
			 free (rv);
			 rv = NULL;
			 continue;
		    }
		    else if (token >= YY_SPEED)
			 rv->speed = xlate_speed[token - YY_SPEED];
		    else if (token >= YY_LFLAG) 
			 rv->tios.c_lflag |= xlate_lflag[token - YY_LFLAG]; 
		    else if (token >= YY_CFLAG) 
			 rv->tios.c_cflag |= xlate_cflag[token - YY_CFLAG]; 
		    else if (token >= YY_OFLAG) 
			 rv->tios.c_oflag |= xlate_oflag[token - YY_OFLAG]; 
		    else if (token >= YY_IFLAG) 
			 rv->tios.c_iflag |= xlate_iflag[token - YY_IFLAG]; 
		    else
		    {
			 debug(LOG_DEBUG, "parseCfgEntry():Syntax Error");
			 *err = ECFG_SYNTAX;
			 free (rv);
			 rv = NULL;
			 continue;
		    }
		    break;
	       }
	  }
     }
     else
	  *err = ECFG_MEMORY;

     return(rv);
}
