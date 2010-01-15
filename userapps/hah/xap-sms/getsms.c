/* $Id$ 
*/
#ifdef IDENT
#ident "@(#) $Id$"
#endif

#include "modem.h"
#include "debug.h"
#include "xapdef.h"
#include <stdio.h>
#include <string.h>

/*reads a SMS from the SIM-memory 1-10 */
/* returns number of SIM memory if successful */
static int fetchsms(int sim, char* pdu) {
	 char command[16];
	 char answer[512];
	 char* position;
	 char* beginning;
	 char* end;
	 int  clen;

	 if (g_debuglevel>1) printf("trying to get stored message %i\n",sim);
	 clen=sprintf(command,"AT+CMGR=%i\r",sim);
	 put_command(command,clen,answer,sizeof(answer),50,0);
	 /* search for beginning of the answer */
	 position=strstr(answer,"+CMGR:");

	 if (position==0)
		  return 0;
	 beginning=position+7;

	 if (strstr(answer,",,0\r"))
		  return 0;

	 /* After that we have the ASCII string */
	 for( end=beginning ; *end && *end!='\r' ; end++ );
	 if ( !*end || end-beginning<4)
		  return 0;
	 for( end=end+1 ; *end && *end!='\r' ; end++ );
	 if ( !*end || end-beginning<4)
		  return 0;
	 /* Now we have the end of the ASCII string */
	 *end=0;
	 strcpy(pdu, beginning);

	 return sim;
}

/* splits an ASCII string into the parts */
/* returns length of ascii */
static int splitascii(char *source, struct inbound_sms *sms) {
	 char* start;
	 char* end;

	 /* the text is after the \n */
	 for( start=source ; *start && *start!='\n' ; start++ );
	 if (!*start)
		  return 1;
	 start++;

	 strcpy(sms->ascii, start);
//	 printf("0: %s\n", start);

	 /* get the senders MSISDN */
	 start=strstr(source,"\",\"");
	 if (start==0) {
		  sms->userdatalength=strlen(sms->ascii);
		  return 1;
	 }
	 start+=3;
	 end=strstr(start,"\",");
	 if (end==0) {
		  sms->userdatalength=strlen(sms->ascii);
		  return 1;
	 }
	 *end=0;
	 strcpy(sms->sender,start);

	 start=end+3;
//	 printf("1: %s\n", start);
	 
	 if (start[0]=='\"')
		  start++;
	 if (start[2]!='/')  { // if next is not a date is must be the name
		  end=strstr(start,"\",");
		  if (end==0) {
			   sms->userdatalength=strlen(sms->ascii);
			   return 1;
		  }
		  *end=0;
		  strcpy(sms->name,start);
		  start=end+3;
	 }

	 /* Get the date */
//	 printf("2: %s\n", start);
	 sprintf(sms->date,"%c%c-%c%c-%c%c",start[0],start[1],start[3],start[4],
			 start[6],start[7]);
	 /* Get the time */
	 start+=9;
//	 printf("3: %s\n", start);
	 sprintf(sms->time,"%c%c:%c%c:%c%c",start[0],start[1],start[3],start[4],
			 start[6],start[7]);

	 sms->userdatalength=strlen(sms->ascii);
//	 printf("4: %d\n", sms->userdatalength);
	 return 1;
}


static inline int decode_pdu( char *pdu, struct inbound_sms *sms)
{
	 int ret;

	 memset( sms, 0, sizeof(struct inbound_sms) );
	 /* Ok, now we split the PDU string into parts and show it */
	 if(g_debuglevel) {
		  fprintf(stderr,"decoding: %s\n", pdu);
		  fprintf(stderr,"Saved to /tmp/pdu.txt\n");
		  FILE *f = fopen("/tmp/pdu.txt","w");
		  fputs(pdu, f);
		  fclose(f);
	 }

	 ret = splitascii(pdu, sms);
		  
	 if (ret==-1) {
		  if (g_debuglevel) fprintf(stderr,"failed to split ascii!\n");
		  return -1;
	 }
	 return 1;
}

static void deletesms(int sim) {
	 char command[32];
	 char answer[32];
	 int clen;

	 clen = sprintf(command, "at+cmgd=%d\r", sim);
	 put_command(command, clen, answer, sizeof(answer), 50, 0);
}

int getsms( struct inbound_sms *sms, int sim) {
	 char   pdu[512];
	 int    found;
	 int    ret;

	 found = fetchsms(sim, pdu);
	 if ( !found ) {
		  if (g_debuglevel) fprintf(stderr,"failed to fetch sms %d!\n",sim);
		  return -1;
	 }

	 ret = decode_pdu(pdu, sms);
	 deletesms(found);

	 return ret;
}
