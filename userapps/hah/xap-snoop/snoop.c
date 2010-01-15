/* $Id: snoop.c 33 2009-11-10 09:56:39Z brett $
   xAP snoop

  Copyright (c) Brett England, 2009
   
  No commercial use.
  No redistribution at profit.
  All derivative work must retain this message and
  acknowledge the work of the original author.  
*/
#ifdef IDENT
#ident "@(#) $Id: snoop.c 33 2009-11-10 09:56:39Z brett $"
#endif

#include "xapdef.h"
#include "xapGlobals.h"
#include <stdlib.h>
#include <string.h>

const char* XAP_ME = "dbzoo";
const char* XAP_SOURCE = "livebox"; 
const char* XAP_GUID = "FF000F00";
const char* XAP_DEFAULT_INSTANCE = "Snoop";

int main(int argc, char* argv[]) {

	 char i_xap_buffer[1500+1];
	 fd_set i_rdfs;
	 struct timeval i_tv;
	 int i_retval;
	 int i;

	 printf("\nxAP Snoop - for xAP v1.2\n");
	 printf("Copyright (C) DBzoo 2009\n\n");

	 xap_init(argc, argv, 0);
  
	 while (1) 
	 { 
		  xap_heartbeat_tick(HBEAT_INTERVAL);
		  
		  FD_ZERO(&i_rdfs);
		  FD_SET(g_xap_receiver_sockfd, &i_rdfs);
		  
		  i_tv.tv_sec=HBEAT_INTERVAL;
		  i_tv.tv_usec=0;
		  
		  i_retval=select(g_xap_receiver_sockfd+1, &i_rdfs, NULL, NULL, &i_tv);		  
		  if (FD_ISSET(g_xap_receiver_sockfd, &i_rdfs)) {
			   // Select either timed out, or there was data - go look for it.
			   i = xap_poll_incoming(g_xap_receiver_sockfd, i_xap_buffer, sizeof(i_xap_buffer));
			   if (i) {
					printf("-------------------------\n%s\n",i_xap_buffer);
			   }
		  }		  
	 }
}


