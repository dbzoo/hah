/* $Id: livebox.c 33 2009-11-10 09:56:39Z brett $

   Compliant Interface for Livebox AVR board

   Usage: livebox [<serial device>] [<interface>] [<port>]
   e.g. livebox /dev/ttyS0 eth0 2222

  Copyright (c) Brett England, 2009
   
  No commercial use.
  No redistribution at profit.
  All derivative work must retain this message and
  acknowledge the work of the original author.  
*/

#ifdef IDENT
#ident "@(#) $Id: livebox.c 33 2009-11-10 09:56:39Z brett $"
#endif

#include "xapdef.h"
#include "xapGlobals.h"
#include "appdef.h"
#include "server.h"
#include "reply.h"
#include "debug.h"
#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
//#include <strings.h>

// Globals
static int g_serial_fd;
char g_lcd_text[17];

const char* XAP_ME = "dbzoo";
const char* XAP_SOURCE = "livebox"; 
const char* XAP_GUID;
const char* XAP_DEFAULT_INSTANCE;

#define SEND_INTERVAL 60  // in seconds

// Incoming serial messages

typedef struct _cmd {
	 char *name;
	 void (* func)(struct _cmd *s, char **args);
} cmd_t;

static void serin_input(cmd_t *s, char **argv);
static void serin_1wire(cmd_t *s, char **argv);
static void serin_ppe(cmd_t *s, char **argv);

cmd_t cmd[] = {
	 {"input", &serin_input},
	 {"1wire", &serin_1wire},
	 {"i2c-ppe", &serin_ppe},
	 { NULL, NULL },
};

static void serin_binary_helper(char *endpoint_name, int state) {
	 endpoint_t *endpoint = find_endpoint(endpoint_name);
	 if(endpoint) {
		  strcpy(endpoint->state, state ? "on" : "off");
		  event_binary_input(endpoint);	 	 // generate an XAP event.
	 }
}

// Process inbound SERIAL command
// input o n
//
// input: will be followed by two decimal numbers
//        these represent the before and after state of
//        the input lines.  There are 4 input lines
//        Each will be bit mapped into a decimal number.
//        0110 = 6
static void serin_input(cmd_t *s, char *argv[]) {
	 int old, new;
	 int bit, i;
	 char buff[20];
	 in("serin_input");

	 if(argv[0] == NULL || argv[1] == NULL) // Bad input shouldn't happen?!
		  return;

	 old = atoi(argv[0]);
	 new = atoi(argv[1]);

	 // Find out which bit(s) changed
     // The 4 bits that can be set are: 00111100
	 for(i=0; i<4; i++) {
          bit = i + 2;
		  if ((old ^ new) & (1 << bit)) {
			   snprintf(buff,sizeof buff,"input.%d", i+1);   // 1 base index - input1, input2 etc..
			   serin_binary_helper(buff, new & (1<<bit));
		  }
	 }
	 out("serin_input");
}

// 1wire N L
//
// N - is the number of the i2c/1-wire device on the BUS we support a larger number
//     by amending the endpoint struct.
// L - a numeric value.  Temp, Pressure etc..
static void serin_1wire(cmd_t *s, char *argv[]) {
	 in("serin_1wire");
     // Expect the command from the PCB begin with "s->name" too.
	 char buff[20];
	 if(argv[0] == NULL) return;  // Bad input shouldn't happen?!

	 strcpy(buff, "1wire.");
	 strlcat(buff, argv[0], sizeof buff);
	 
	 endpoint_t *endpoint = find_endpoint(buff);
	 if(endpoint) {
		  strlcpy(endpoint->state, argv[1], EP_STATE_SIZE);
		  event_level_input(endpoint);
	 }
	 out("serin_1wire");
}

// i2c-ppe A O N
//
// A - Address of PPE chip (in hex 2 digits only)
// O - Original value before state change
// N - Value after state change
static void serin_ppe(cmd_t *s, char *argv[]) {
	 // We need to do a bit more work to find the endpoint.
	 // As the user may have configure a single ENDPOINT or ONE per PIN
	 char buff[30];
	 char *addr = argv[0];  // PPE address
	 char *old = argv[1];
	 char *new = argv[2];

	 in("serin_ppe");
	 if(new == NULL || old == NULL || addr == NULL) // Bad input shouldn't happen?!
		  return;

	 snprintf(buff, sizeof buff, "i2c.%s", addr);

	 endpoint_t *endpoint = find_endpoint(buff);
	 if(endpoint) { // We are in BYTE mode
		  strlcpy(endpoint->state, new, EP_STATE_SIZE);
		  event_level_input(endpoint);
	 } else {
		  int pin;
		  int newi = atoi(new);
		  int oldi = atoi(old);
		  for(pin=0; pin<8; pin++) {
			   // Figure out what changed in the PPE
			   if ((oldi ^ newi) & (1 << pin)) {
					// Compute an ENDPOINT name
					snprintf(buff,sizeof buff,"i2c.%s.%d", addr, pin);
					serin_binary_helper(buff, newi & (1<<pin));
			   }
		  }
	 }
	 out("serin_ppe");
}

static void serial_handler(char* a_cmd, int len) {
	 cmd_t *s = &cmd[0];
	 char *command = NULL;

	 in("serial_handler");
	 // Serial input terminates with \r\n  (17\r\n = length 4)
	 // Replace \r with 0
	 *(a_cmd+len-1) = 0;

	 if(g_debuglevel>4) printf("Serial RX: %s\n",a_cmd);
	 command = strtok(a_cmd," ");
	 while(s->name) {
		  if(strcmp(command, s->name) == 0) {
			   int i;
			   char *argv[4];
			   char *arg;
			   for(i=0; i < sizeof(argv); i++) {
					arg = strtok(NULL," ,");		  
					if(arg == NULL)
						 break;
					argv[i] = arg;
			   }
			   (*s->func)(s, argv);
			   break;
		  }
		  s++;
	 }
	 out("serial_handler");
}

int serial_cmd_msg(const char *cmd, const char *arg) {
	 char buf[30];  // WARN: If you adjust this you must also adjust FIRMWARE in AVR.

	 // Print \n a the beginning.
	 // The AVR will reset its command interpreter (flush its input buffer)
	 if(arg == NULL) {
		  snprintf(buf, sizeof(buf), "\n%s\n",cmd); 
	 } else {
		  snprintf(buf, sizeof(buf), "\n%s %s\n",cmd, arg); 
	 }
	 if(g_debuglevel>4) printf("serial TX: %s", buf);
	 return write(g_serial_fd, buf, strlen(buf));
}

static int serial_lcd_msg(char *msg) {
	 return serial_cmd_msg("lcd", msg);
}

int decode_state(const char *msg) {
	static const char *value[] = {"on","off","true","yes","false","no","1","0"};
	static const int state[] = {1,0,1,1,0,0,1,0};
	int i;

	for(i=0; i < sizeof(value); i++) {
		if(strcasecmp(msg, value[i]) == 0)
			return state[i];
	}
	return -1;
}

/* An XAP BSC level message may be of the form.
      level = <value>
      level = <value>/<range>  i.e: 256/512 = 50
 */
int parse_level(char *str) {
	 int level;
	 int range;
	 in("parse_level");
	 if(index(str,'/') == 0) {
		  sscanf(str,"%d", &level);
	 } else {
		  sscanf(str,"%d/%d", &level, &range);
		  level = range / level;
	 }
	 out("parse_level");
	 return level;
}

// The relay endpoint being controlled and state
// will act upon this relay
int cmd_relay(endpoint_t *self, char *stateStr) {
	 int state = decode_state(stateStr);
	 if(state >= 0) {
		  char arg[10];
		  strcpy(self->state, state ? "on" : "off");
		  snprintf(arg, sizeof(arg), "%d", self->subid);
		  // The state is also the AVR command.
		  event_binary_output(self);
		  return serial_cmd_msg(self->state, arg);
	 }
	 return -1;
}

void xap_cmd_relay(endpoint_t *self, char *section) {
	 char key[XAP_MAX_KEYVALUE_LEN];
	 char i_msg[XAP_MAX_KEYVALUE_LEN];
	 snprintf(key, sizeof(key), "%s:state", section);
	 if(xapmsg_getvalue(key, i_msg)) {
		  cmd_relay(self, i_msg);
	 }
}


// Send a message to the LCD
int cmd_lcd(char *msg) {
	 strlcpy(g_lcd_text, msg, sizeof(g_lcd_text));
	 return serial_lcd_msg(msg);
}

// Decode an XAP message for the LCD
void xap_cmd_lcd(endpoint_t *self, char *section) {
	 char key[XAP_MAX_KEYVALUE_LEN];
	 char i_msg[XAP_MAX_KEYVALUE_LEN];

	 snprintf(key, sizeof(key), "%s:text",section);
	 if(xapmsg_getvalue(key, i_msg)) {
		  cmd_lcd(i_msg);
	 }
}

/* This is called in PPE pin mode
*/
void xap_cmd_ppe_pin(endpoint_t *self, char *section) {
	 char key[XAP_MAX_KEYVALUE_LEN];
	 char i_msg[XAP_MAX_KEYVALUE_LEN];

	 in("xap_cmd_ppe_pin");
	 snprintf(key, sizeof(key), "%s:state", section);
	 if(xapmsg_getvalue(key, i_msg)) {
		  int state = decode_state(i_msg);
		  if(state >= 0) {
			   char arg[10];
			   // The pin being modified corresponds to the last digit of the ENDPOINT name
			   // Endpoint name is of the form: i2c.<addr>.<pin>
			   char pin = *(self->name + strlen(self->name) - 1);
			   // subid = PPE address
			   // We invert the STATE.  Logical ON is a LOW PPE state 0.
			   snprintf(arg, sizeof(arg),"P%02X%c%d",self->subid, pin, 
						state ? 0 : 1);
			   serial_cmd_msg("i2c", arg);
		  }
	 }
	 out("xap_cmd_ppe_pin");
}

void xap_cmd_ppe_byte(endpoint_t *self, char *section) {
	 char key[XAP_MAX_KEYVALUE_LEN];
	 char i_msg[XAP_MAX_KEYVALUE_LEN];

	 in("xap_cmd_ppe_byte");
	 snprintf(key, sizeof(key), "%s:level", section);
	 if(xapmsg_getvalue(key, i_msg)) {
		  char arg[10];
		  char byte_s[3];
		  unsigned char byte = (unsigned char)parse_level(i_msg);
		  // subid = PPE address
		  snprintf(arg, sizeof(arg),"B%02X%02X",self->subid, byte);
		  strlcpy(self->state, i_msg, EP_STATE_SIZE);
		  serial_cmd_msg("i2c", arg);
	 }
	 out("xap_cmd_ppe_byte");
}

// Receive a xap (broadcast) packet and process
void xap_handler(const char* a_buf) {
	 char i_identity[XAP_MAX_KEYVALUE_LEN];
	 char i_target[XAP_MAX_KEYVALUE_LEN];
	 char i_class[XAP_MAX_KEYVALUE_LEN];
	 char i_source[XAP_MAX_KEYVALUE_LEN];
	 char i_msg[XAP_MAX_KEYVALUE_LEN];
	 char i_body[XAP_MAX_KEYVALUE_LEN];
	 int i, j, msg_type;

	 xapmsg_parse(a_buf);

	 if (xapmsg_getvalue("xap-header:target", i_target)==0) {
		  if (g_debuglevel>3) printf("No target in header, not for us\n");

		  // check source addresses instead
		  if (xapmsg_getvalue("xap-header:source", i_source)==0) {
			   if (g_debuglevel>3) printf("Error: no source in header\n");
			   return;
		  }

		  // Pick up clock broadcasts and display them
		  // Might want a { display:clock=on } to enable/disable this functionality
		  // otherwise it will overwrite the display which might be annoying.
		  if (xap_compare(i_source, "*.Clock.Bigben")) {
			   xapmsg_getvalue("prettylocal:short", i_msg);
			   cmd_lcd(i_msg);
		  }
		  return;
	 }

	 if (! xapmsg_getvalue("xap-header:class", i_class)) {
		  if (g_debuglevel>3)
			   printf("Error: Missing class in header\n");
	 }
#define CMD_MSG 1
#define QUERY_MSG 2
	 if(strcmp("xAPBSC.cmd", i_class) == 0) {
		  msg_type = CMD_MSG;
	 } else if(strcmp("xAPBSC.query", i_class) == 0) {
		  msg_type = QUERY_MSG;
	 } else {
		  if (g_debuglevel>3)
			   printf("Error: Invalid class in header\n");
		  return;
	 }

	 for(i=0; i<g_xap_index; i++) {
		  FOREACH_ENDPOINT(endpoint) {
			   // Check if we are targeting one of our named endpoints
			   snprintf(i_identity, sizeof(i_identity), "%s.%s.%s:%s", XAP_ME, XAP_SOURCE, g_instance, endpoint->name);
			   if (! xap_compare(i_target, i_identity) ) {
					if (g_debuglevel>3) printf("Target didn't match %s to %s\n", i_target, i_identity);
					continue;
			   }

			   struct tg_xap_msg *map = &g_xap_msg[i];

			   if(msg_type == CMD_MSG) {
					// Did the message contain ...
					if(! xap_compare(map->section, "output.state.*")) {
						 continue;
					}
					// As the g_xap_map is flattened to avoid doing this for each
					// section member we only trigger on ID
					if(! strcasecmp(map->name,"id"))
						 continue;

					// pull the messages ID attribute
					snprintf(i_body, sizeof(i_body), "%s:id", map->section);
					if(! xapmsg_getvalue(i_body, i_msg)) {
						 continue;
					}
					// is this message for our ID ?  Match the wildcard.
					if(i_msg[0] == '*' || strcmp(i_msg, endpoint->id) == 0) {
						 // Pass the message section that matched and our ID.
						 // to the appropriate handler.
						 if (endpoint->cmd) {
							  (* endpoint->cmd)( endpoint, map->section );
						 }
					}
			   }
			   else if(msg_type == QUERY_MSG) {
					// Only do this ONCE for each endpoint (uid per message will be unique)
					if(! xap_compare(map->name,"uid"))
						 continue;
					(* endpoint->info)( endpoint );
			   } else {
					printf("Unknown message type\n"); // we should never get here.
			   }
		  }
	 }
}

void send_all_info() {
	 in("send_all_info");
	 FOREACH_ENDPOINT(endpoint) {
		  (*endpoint->info)( endpoint );
	 }
	 out("send_all_info");
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
	 int server_sockfd;	 

	 int i;
	 struct timeval i_tv;       // Unblock serial READ

	 in("process_event");
	 server_sockfd = svr_bind();

	 // Setup the select() sets.
	 FD_ZERO(&master);
	 FD_SET(g_serial_fd, &master);
	 FD_SET(server_sockfd, &master);
	 FD_SET(g_xap_receiver_sockfd, &master);

	 i_highest_fd = g_serial_fd;
	 if (i_highest_fd < g_xap_receiver_sockfd) i_highest_fd=g_xap_receiver_sockfd;
	 if (i_highest_fd < server_sockfd) i_highest_fd=server_sockfd;

	 while (1)
	 {
		  i_tv.tv_sec=HBEAT_INTERVAL;
		  i_tv.tv_usec=0;
	 
		  i_rdfs = master; // copy it - select() alters this each call.
		  i_retval = select(i_highest_fd+1, &i_rdfs, NULL, NULL, &i_tv);

		  // If incoming serial data...
		  // The AVR will push data at us when an event occurs that changes its internal state.
		  // We must read this and relay this state change as an XAP message
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

		  // If incoming SERVER message...
		  if (FD_ISSET(server_sockfd, &i_rdfs)) {
			   int inbound_sockfd = svr_accept( server_sockfd );
			   if ( inbound_sockfd == -1) {
					continue;
			   }
/*
			   if(!fork())  { // this is the child process
					close(server_sockfd); // child doesn't need the listener
					svr_process( inbound_sockfd );
					close(inbound_sockfd);
					exit(0);
			   }
			   close( inbound_sockfd ); //parent doesn't need this
*/
			   svr_process(inbound_sockfd);
			   close(inbound_sockfd);
		  }
		  
		  // As external input can change on the AVR we send a TICK to display their state
		  if (xap_send_tick(SEND_INTERVAL)) {
			   send_all_info();
		  }

		  // Send heartbeat periodically
		  xap_heartbeat_tick(HBEAT_INTERVAL);
	 } // while(1)
}

void setup_serial_port() {
	 struct termios newtio;

	 /* Open the serial port
		O_NDELAY: Don't wait for DCD signal line when opening the port.
		O_RDWR: Open for read and write
	 */
	 in("setup_serial_port");
	 printf("\nUsing serial device %s\n\n",g_serialport);
	 g_serial_fd = open(g_serialport, O_RDWR | O_NDELAY);
	 if (g_serial_fd < 0)
	 {
		  printf("Unable to open the serial port %s\n",g_serialport);
		  exit(4);
	 }
	 /* port settings: 9600, 8bit
		CREAD enables port to read data
		CLOCAL indicates device is not a modem
	 */
	 cfmakeraw(&newtio);
	 newtio.c_cflag = B115200 | CS8 | CLOCAL | CREAD ;
	 newtio.c_iflag = IGNPAR;
	 newtio.c_lflag = ~ICANON;
	 newtio.c_cc[VTIME] = 0; /* ignore timer */
	 newtio.c_cc[VMIN] = 0; /* no blocking read */
	 tcflush(g_serial_fd, TCIFLUSH);
	 tcsetattr(g_serial_fd, TCSANOW, &newtio);
	 out("setup_serial_port");
}

int main(int argc, char* argv[]) {
	 int i;
	 char buff[22];

	 printf("\nLivebox Connector for xAP v12\n");
	 printf("Copyright (C) DBzoo, 2008,2009\n\n");
	 setupXAPini();
	 xap_init(argc, argv, 0);

	 setup_serial_port();
	 load_endpoints();  // Order: May send serial commands to setup PPE chips
	 parseini();        // Load Dynamic endpoints
	 
	 // Show my IP address on the LCD
	 cmd_lcd(g_ip);

	 send_all_info(); // Announce our capabilities

	 // Ask devices to report their current status
	 serial_cmd_msg("report",NULL);
	 process_event();

	 return 0; // not reached
}
