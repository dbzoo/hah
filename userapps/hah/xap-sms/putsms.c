/* $Id$ 
*/
#ifdef IDENT
#ident "@(#) $Id$"
#endif

#include <stdio.h>
#include "modem.h"
#include "xapdef.h"
#include "debug.h"

int putsms(char *msg, char *num) {
	 char command[50];
	 char command2[500];
	 int clen, clen2;
	 int err_code;
	 int retries;
	 char answer[100];

	 in("putsms");

	 if (g_debuglevel>3) { 
		  printf("Sending message\n");
		  printf("Dest <%s>\n", num);
		  printf("Msg <%s>\n", msg);
	 }

	 clen = sprintf(command,"AT+CMGS=\"%s\"\r", num);
	 clen2 = sprintf(command2, "%s\x1A", msg);
	 
	 for(err_code=0, retries=0; err_code<2 && retries<3; retries++) {
		  if( put_command(command, clen, answer,sizeof(answer), 50,"\r\n> ")
			  && put_command(command2, clen2, answer, sizeof(answer), 1000, 0)
			  && strstr(answer,"OK") )
		  {
			   err_code = 2;
		  } else {
			   if(checkmodem() == -1) {
					err_code = 0;
					if(g_debuglevel) printf("Resending last SMS\n");
			   } else if (err_code == 0) {
					if(g_debuglevel) printf("Possible corrupted sms. Trying again.\n");
					err_code = 1;
			   } else {
					if(g_debuglevel) printf("No idea?!  Dropping sms\n");
					err_code = 3;
			   }
		  }
	 }
	 if(err_code == 0) {
		  if(g_debuglevel) 
			   printf("re-inited and re-tried %d times without success\n", retries);
	 }
	 out("putsms");
	 return err_code;
}
