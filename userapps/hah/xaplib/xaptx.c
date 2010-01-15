/* $Id: xaptx.c 59 2009-12-13 11:05:21Z brett $
  Copyright (c) Patrick Lidstone, 2002.
   
  No commercial use.
  No redistribution at profit.
  All derivative work must retain this message and
  acknowledge the work of the original author.  
*/
#include "xapdef.h"

int xap_send_tick(int a_interval) {

	 
	 time_t i_timenow;
	 static time_t i_sendtick=0;
	 
	 i_timenow = time((time_t*)0);

		// Check timer for heartbeat send

		if ((i_timenow-i_sendtick>=a_interval)||(i_sendtick==0)) {												
 			i_sendtick=i_timenow;
			return 1;
		} // send tick
	return 0;
}



int xap_heartbeat_tick(int a_interval) {

	 // Call this function periodically.
	 // Every HBEAT_INTERVAL, it will send a heartbeat

	 time_t i_timenow;
	 static time_t i_heartbeattick=0;
	 
	 i_timenow = time((time_t*)0);

		// Check timer for heartbeat send

		if ((i_timenow-i_heartbeattick>=a_interval)||(i_heartbeattick==0)) {												
 			i_heartbeattick=i_timenow;
			// Send heartbeat to all external listeners
			xap_broadcast_heartbeat(g_xap_sender_sockfd, &g_xap_sender_address); 
			return 1;
		} // heartbeat tick
	return 0;
}


// Send a heartbeat to the real world

int xap_broadcast_heartbeat(int a_sock,  struct sockaddr_in* a_addr) {

	int i;
	char i_buff[1500];

	// Construct the heartbeat message (to say we are alive)
	// If using a hub, this must be sent on startup, because
	// that is how the hub knows who we are

	sprintf(i_buff, "xap-hbeat\n{\nv=12\nhop=1\nuid=%s\nclass=xap-hbeat.alive\nsource=%s.%s.%s\ninterval=%d\nport=%d\n}\n", g_uid, XAP_ME, XAP_SOURCE,  g_instance, HBEAT_INTERVAL, ntohs(g_xap_receiver_address.sin_port));
	
#ifdef DEBUG
	printf("Heartbeat source=%s, instance=%s, interval=%d, port=%d\n",XAP_SOURCE, g_instance, HBEAT_INTERVAL, 	ntohs(g_xap_receiver_address.sin_port) );
#endif

	// Send it...
    i= sendto(a_sock, i_buff, strlen(i_buff), 0, (struct sockaddr *)a_addr, sizeof(struct sockaddr));	
#ifdef DEBUG
	printf("Broadcasting heartbeat\n");
#endif
	
	return i;
}


// Build an outgoing xap (broadcast) message 

int xap_send_message(const char* a_buff) {

	// Send it...
    sendto(g_xap_sender_sockfd, a_buff, strlen(a_buff), 0, (struct sockaddr *)&g_xap_sender_address, sizeof(struct sockaddr));	
#ifdef DEBUG
	printf("Outgoing xAP message sent\n");	
#endif
	return 1;
}

// Fixup missing fields / Override present fields.
char *normalize_xap(char *body) 
{
     char a_raw_msg[1500];

     if(body == NULL) return NULL;
     int result = xapmsg_parse(body);
     if(result == 0) return NULL; // no body xap message?

     // We only allow the class and target to be specified in the header section
     // everything else will be ignored and automatically defaulted when the xap
     // message is built.  WHY?  We don't want the calendar xap payload spoofing
     // the "source" for example.
     char class[XAP_MAX_KEYVALUE_LEN];
     char target[XAP_MAX_KEYVALUE_LEN];
     if(xapmsg_getvalue("xap-header:class", class) == 0) return NULL;
     if(xapmsg_getvalue("xap-header:target", target) == 0) return NULL;
  
     snprintf(a_raw_msg, sizeof(a_raw_msg),
	      "xAP-header\n{\nv=12\nhop=1\nuid=%s\n"		    \
	      "class=%s\nsource=%s.%s.%s\ntarget=%s\n}\n",		\
	      g_uid, class, XAP_ME, XAP_SOURCE, g_instance, target);
  
     // Produce an XAP message from the parsed message minus the xap-header.
     // If we run out of a_raw_msg buffer the message will be silently truncated.
     // Based on xapmsg_toraw() from xaplib.
     int i;
     char i_active_section[XAP_MAX_SECTION_LEN+1];
     const char* XAPMSG_TORAW_ENDBLOCK="}\n";
     const char* XAPMSG_TORAW_STARTBLOCK="\n{\n";

     i_active_section[0] = '\0';
     for(i=0; i< g_xap_index; i++) {
	  // skip all parsed header bits - we've already done this.
	  if(strcasecmp("xap-header",g_xap_msg[i].section) == 0) continue;

	  if (strcmp(g_xap_msg[i].section, i_active_section)) {
	       if (i_active_section[0]) { //this isn't the first section
		    strlcat(a_raw_msg, XAPMSG_TORAW_ENDBLOCK, sizeof(a_raw_msg));
	       }
	       // append new section header
	       strlcat(a_raw_msg, g_xap_msg[i].section, sizeof(a_raw_msg));
	       strlcat(a_raw_msg, XAPMSG_TORAW_STARTBLOCK, sizeof(a_raw_msg));
      
	       strlcpy(i_active_section, g_xap_msg[i].section, sizeof(i_active_section));
	  }
	  strlcat(a_raw_msg, g_xap_msg[i].name, sizeof(a_raw_msg));
	  strlcat(a_raw_msg, "=", sizeof(a_raw_msg));
	  strlcat(a_raw_msg, g_xap_msg[i].value, sizeof(a_raw_msg));
	  strlcat(a_raw_msg, "\n", sizeof(a_raw_msg));
     } // for i
     // arguable whether last "\n" is required. Spec is vague!
     strlcat(a_raw_msg, "}\n", sizeof(a_raw_msg));

     return strdup(a_raw_msg);
}
