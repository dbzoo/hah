/* $Id$
  Copyright (c) Brett England, 2009
   
  No commercial use.
  No redistribution at profit.
  All derivative work must retain this message and
  acknowledge the work of the original author.  
 */
#ifdef IDENT
#ident "@(#) $Id$"
#endif

#include "xapdef.h"
#include "xapGlobals.h"
#include "modem.h"
#include "putsms.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define XAP_CLASS "sms.message"


const char* XAP_ME = "dbzoo";
const char* XAP_SOURCE = "livebox"; 
const char* XAP_GUID;
const char* XAP_DEFAULT_INSTANCE;

#define SEND_INTERVAL 60  // in seconds
const char inifile[] = "/etc/xap-livebox.ini";

// When we receive an "outbound" sms.message report on whether is could be sent or not.
int xap_receipt(char* msg, char* sender, char *sent, char *err) {
	 char buff[1500];
	 int len;

	 len = snprintf(buff, sizeof(buff), 
					"xap-header\n{\nv=12\nhop=1\nuid=%s\n"
					"class=SMS.Receipt\nsource=%s.%s.%s\n}\n"
					"Receipt\n{\nmsg=%s\nnum=%s\nsent=%s\nerror=%s\n}\n", 
					g_uid, XAP_ME, XAP_SOURCE, g_instance, msg, sender, sent, err);

	 if(len > sizeof(buff)) // Buffer overflow!
		  return 0;
	 else
		  return xap_send_message(buff);
}


int xap_relay_sms(char* msg, char* sender, char* date, char *time) {
	 char buff[1500];
	 int len;

	 len = snprintf(buff, sizeof(buff), 
					"xap-header\n{\nv=12\nhop=1\nuid=%s\n"
					"class=%s\nsource=%s.%s.%s\n}\n"
					"inbound\n{\nmsg=%s\nnum=%s\ntimestamp=%s %s\n}\n", 
					g_uid, XAP_CLASS, XAP_ME, XAP_SOURCE, g_instance, msg, sender, date, time);

	 if(len > sizeof(buff)) // Buffer overflow!
		  return 0;
	 else
		  return xap_send_message(buff);
}

void serial_handler(char* a_cmd, int len) {
  
	 // Serial input terminates with \r\n  (17\r\n = length 4)
	 // Replace \r with 0
	 *(a_cmd+len-1) = 0;

	 if(g_debuglevel>4) printf("Serial Rx: %s\n",a_cmd);

	 if(strncmp(a_cmd,"+CMTI:",6) == 0) {
		  int message_number;
		  // Recieved an SMS notification event: +CMTI: "MT",3
		  char *p = strchr(a_cmd,',');
		  if (p) {
			   message_number = atoi(p + 1);
		  } else {
			   // The message begins as expected but has no comma!
			   return;
		  }

		  struct inbound_sms msg;
		  getsms( &msg, message_number );
		  xap_relay_sms(msg.ascii, msg.sender, msg.date, msg.time);		  
	 }

}

// Receive a xap (broadcast) packet and process
void xap_handler(const char* a_buf) {
	 char i_identity[XAP_MAX_KEYVALUE_LEN];
	 char i_target[XAP_MAX_KEYVALUE_LEN];
	 char i_class[XAP_MAX_KEYVALUE_LEN];
	 char i_body[XAP_MAX_KEYVALUE_LEN];
	 char i_dest[XAP_MAX_KEYVALUE_LEN];
	 char i_msg[XAP_MAX_KEYVALUE_LEN];

	 xapmsg_parse(a_buf);

	 if (xapmsg_getvalue("xap-header:target", i_target)==0) {
		  if (g_debuglevel>7) printf("No target in header, not for us\n");
		  return;
	 }
	 
	 if (! xapmsg_getvalue("xap-header:class", i_class)) {
		  if (g_debuglevel>7) printf("Error: Missing class in header\n");
		  return;
	 }
	 
	 if (strcasecmp(i_class, XAP_CLASS)!=0) {
		  if (g_debuglevel>7) printf("Class didn't match %s to %s\n", i_class, XAP_CLASS);
		  return;
	 }
	 
	 snprintf(i_identity, sizeof(i_identity), "%s.%s.%s", XAP_ME, XAP_SOURCE, g_instance);
	 if (xap_compare(i_target, i_identity)!=1) {
		  if (g_debuglevel>7) printf("Target didn't match %s to %s\n", i_target, i_identity);
		  return;
	 }
	 
	 if (xapmsg_getvalue("outbound:num", i_dest)==0) {
		  if (g_debuglevel>5) printf("Error: No number specified in message body\n");
		  return;
	 }
	 if (xapmsg_getvalue("outbound:msg", i_msg)==0) {
		  if (g_debuglevel>5) printf("Error: No msg specified in message body\n");
		  return;
	 }
	
	 switch(putsms(i_msg, i_dest)) {
	 case 0: xap_receipt(i_msg, i_dest, "no", "Modem not ready"); break;
	 case 1: xap_receipt(i_msg, i_dest, "no", "Possible corrupt sms"); break;
	 case 2: xap_receipt(i_msg, i_dest, "yes", ""); break;
	 default: xap_receipt(i_msg, i_dest, "no", "No idea!"); break;
	 }
}



void process_event() {
	 fd_set master, i_rdfs;
	 int i_retval;
	 int i_highest_fd;
	 char i_xap_buff[1500+1];   // incoming XAP message buffer

#define BUFFER_LEN 250
#define CMD_LEN 1024
	 int i_serial_cursor=0;
	 int i_serial_data_len;
	 char i_serial_buff[BUFFER_LEN+1];
	 char i_serial_cmd[CMD_LEN+1];
	 int i_flag=0;

	 int i;
	 struct timeval i_tv;       // Unblock serial READ

	 // Setup the select() sets.
	 FD_ZERO(&master);
	 FD_SET(g_serial_fd, &master);
	 FD_SET(g_xap_receiver_sockfd, &master);

	 i_highest_fd = g_serial_fd;
	 if (i_highest_fd < g_xap_receiver_sockfd) i_highest_fd=g_xap_receiver_sockfd;
	 
	 // Initialize the modem
	 if(initmodem()) {
		  exit(2);
	 }

	 while (1)
	 {
		  i_tv.tv_sec=HBEAT_INTERVAL;
		  i_tv.tv_usec=0;
	 
		  i_rdfs = master; // copy it - select() alters this each call.
		  i_retval = select(i_highest_fd+1, &i_rdfs, NULL, NULL, &i_tv);

		  // If incoming serial data...
		  if (FD_ISSET(g_serial_fd, &i_rdfs)) {
			   i_serial_data_len = read(g_serial_fd, i_serial_buff, BUFFER_LEN);
			   if (i_serial_data_len) {
					for(i=0; i < i_serial_data_len; i++) {
						 //putchar(i_serial_buff[i]);
						 i_serial_cmd[i_serial_cursor++]=i_serial_buff[i];

						 if (i_serial_buff[i]=='\r') {
							  i_flag=1;
						 }
						 else {
							  if ((i_flag==1) && (i_serial_buff[i]=='\n')) {
								   serial_handler(i_serial_cmd, i_serial_cursor);
								   i_serial_cursor=0;
							  }
							  else
								   i_flag=0;
                         }
					}
			   }
		  }

		  // if incoming XAP message...
		  // Check for new incoming XAP messages.  They are handled in xap_handler.
		  if (FD_ISSET(g_xap_receiver_sockfd, &i_rdfs)) {
			   if (xap_poll_incoming(g_xap_receiver_sockfd, i_xap_buff, sizeof(i_xap_buff))>0) {
					xap_handler(i_xap_buff);
			   }
		  }

		  // Send heartbeat periodically
		  xap_heartbeat_tick(HBEAT_INTERVAL);
	 }
}


void setupXAPini() {
	 char uid[5];
	 char guid[9];
	 long n;

	 // default to 00DC if not present
	 n = ini_gets("sms","uid","00DD",uid,sizeof(uid),inifile);

	 // validate that the UID can be read as HEX
	 if(n == 0 || !(isxdigit(uid[0]) && isxdigit(uid[1]) && 
					isxdigit(uid[2]) && isxdigit(uid[3]))) 
	 {
		  strlcpy(uid,"00DD",sizeof uid);
	 }
	 snprintf(guid,sizeof guid,"FF%s00", uid);
	 XAP_GUID = strdup(guid);

	 char control[30];
	 n = ini_gets("sms","instance","sms",control,sizeof(control),inifile);
	 if(n == 0 || strlen(control) == 0) 
	 {
		  strlcpy(control,"sms",sizeof control);
	 }
	 XAP_DEFAULT_INSTANCE = strdup(control);

	 ini_gets("sms","usbserial","/dev/ttyUSB0", g_serialport, sizeof(g_serialport), inifile);
}

int main(int argc, char *argv[]) {
	 FILE *fp;

	 printf("\nSMS Connector for xAP v12\n");
	 printf("Copyright (C) DBzoo, 2009\n\n");
	 setupXAPini();
	 xap_init(argc, argv, 0);
	 setup_serial_port();
	 process_event();
}
