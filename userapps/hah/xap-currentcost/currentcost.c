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
#include <libxml/parser.h>

const char* XAP_ME = "dbzoo";
const char* XAP_SOURCE = "livebox"; 
const char* XAP_GUID;
const char* XAP_DEFAULT_INSTANCE;

#define SEND_INTERVAL 60  // in seconds
const char inifile[] = "/etc/xap-livebox.ini";

// Globals
static int g_serial_fd;

struct {
	 int channel[3];
	 float temp;
	 float hrs[13];
	 int days[31];
	 int months[12];
	 int years[4];
	 int sensor;
} cc, prevcc;

enum {NONE,HRS,DAYS,MTHS,YRS,CHANNEL1,CHANNEL2,CHANNEL3,TEMP,SENSOR};
int idx;
int state;
int hysteresis;

enum {CC128,CLASSIC};
int model;

/* SAX element (TAG) callback
 */
static void startElementCB(void *ctx, const xmlChar *name, const xmlChar **atts)
{
     if(strcmp(name,"hrs") == 0) {
	  state = HRS;
	  idx = 0;
     } else if(strcmp(name,"days") == 0) {
	  state = DAYS;
	  idx = 0;
     } else if(strcmp(name,"mths") == 0) {
	  state = MTHS;
	  idx = 0;
     } else if(strcmp(name,"yrs") == 0) {
	  state = YRS;
	  idx = 0;
     } else if (strcmp(name,"ch1") == 0) {
	  state = CHANNEL1;
     } else if (strcmp(name,"ch2") == 0) {
	  state = CHANNEL2;
     } else if (strcmp(name,"ch3") == 0) {
	  state = CHANNEL3;
     } else if (strcmp(name,"tmpr") == 0) {
	  state = TEMP;
     } else if (strcmp(name,"tmprF") == 0) {  // For the US model
	  state = TEMP;
     } else if (strcmp(name,"sensor") == 0) { // cc128 model
	  state = SENSOR;
     }
}

/* SAX cdata callback
 */
static void cdataBlockCB(void *ctx, const xmlChar *value, int len)
{
     switch(state) {
     case SENSOR:
	  cc.sensor = atoi(value);
	  break;
     case CHANNEL1:
	  cc.channel[0] = atoi(value);
	  break;
     case CHANNEL2:
	  cc.channel[1] = atoi(value);
	  break;
     case CHANNEL3:
	  cc.channel[2] = atoi(value);
	  break;
     case TEMP:
	  cc.temp = atof(value);
	  break;
     case HRS:
	  sscanf(value,"%f", &(cc.hrs[idx]));
	  break;
     case DAYS:
	  cc.days[idx] = atoi(value);
	  break;
     case MTHS:
	  cc.months[idx] = atoi(value);
	  break;
     case YRS:
	  cc.years[idx] = atoi(value);
	  break;
     }
}

/* SAX element (TAG) callback
 */
static void endElementCB(void *ctx, const xmlChar *name) 
{
     switch(state) {
     case SENSOR:
     case CHANNEL1:
     case CHANNEL2:
     case CHANNEL3:
     case TEMP:
	  state = NONE;
	  break;
     case HRS: 
	  if(idx < 13) idx++; break;
     case DAYS:
	  if(idx < 31) idx++; break;
     case MTHS:
	  if(idx < 12) idx++; break;
     case YRS: 
	  if(idx < 4) idx++; break;
     }
}

static void xap_message(char *body, char *subtype, int id) {
	 char i_xapmsg[1500];
	 char uid[8];
	 int len;

	 strlcpy(uid, g_uid, sizeof uid);
	 sprintf(&uid[6], "%02x", id);  // create sub address

	 if(model == CC128) {
		  // For the CC128 each sensor is a sub-subtype
		  len = snprintf(i_xapmsg, sizeof(i_xapmsg),
						 "xAP-header\n{\nv=12\nhop=1\nuid=%s\n"	\
						 "class=xAPBSC.event\nsource=%s.%s.%s:%s.%d\n}"	\
						 "\ninput.state\n{\n%s\n}\n",
						 uid, XAP_ME, XAP_SOURCE, g_instance, subtype, cc.sensor, body);
	 } else {
		  len = snprintf(i_xapmsg, sizeof(i_xapmsg),
						 "xAP-header\n{\nv=12\nhop=1\nuid=%s\n"	\
						 "class=xAPBSC.event\nsource=%s.%s.%s:%s\n}"	\
						 "\ninput.state\n{\n%s\n}\n",
						 uid, XAP_ME, XAP_SOURCE, g_instance, subtype, body);
	 }
	 // If truncation of the buffer occurs don't send the message treat as malformed.
	 if( len < sizeof(i_xapmsg)) {
		  xap_send_message(i_xapmsg);
	 }
}

static void xap_watt_info(int ch) {
	 char body[16] = "level=";
	 sprintf(body,"level=%d", cc.channel[ch]);
	 char subtype[10];
	 sprintf(subtype, "ch%d", ch+1);  // Convert ZERO index to base 1.
	 xap_message(body, subtype, 1);
}

static void xap_temp_info() {
	 char body[16];
	 sprintf(body,"level=%3.1f", cc.temp);
	 xap_message(body, "temp", 2);
}

// Send and xAP event when the data values change
void send_all_info() {
	 int i;
	 for(i=0; i<3; i++) {
		  // Watt reporting hysteresis
		  if(cc.channel[i] > prevcc.channel[i]+hysteresis || cc.channel[i] < prevcc.channel[i]-hysteresis)
			   xap_watt_info(i);
	 }
	 if(cc.temp != prevcc.temp)
		  xap_temp_info();
	 memcpy(&prevcc, &cc, sizeof(cc));
}

void parseXml(char *data, int size) {
     xmlSAXHandler handler;

     if (g_debuglevel >=5 ) printf("Parsing... %s\n", data);

     memset(&handler, 0, sizeof(handler));
     handler.initialized = XML_SAX2_MAGIC;
     handler.startElement = startElementCB;
     handler.endElement = endElementCB;
     handler.characters = cdataBlockCB;
     
     if(xmlSAXUserParseMemory(&handler, NULL, data, size) < 0) {
	  if(g_debuglevel) printf("Document not parsed successfully.\n");
	  return;
     }
}

void process_event() {
	 struct timeval i_tv;       // Unblock serial READ
	 fd_set master, i_rdfs;
	 int i_retval;
#define BUFFER_LEN 250
#define SERIAL_XML 4096
	 char serial_buff[BUFFER_LEN+1];
	 char serial_xml[SERIAL_XML+1];
	 int serial_cursor = 0;
	 int i;

	 // Setup the select() sets.
	 FD_ZERO(&master);
	 FD_SET(g_serial_fd, &master);
	 
	 while (1)
	 {
		  i_tv.tv_sec=HBEAT_INTERVAL;
		  i_tv.tv_usec=0;
	 
		  i_rdfs = master; // copy it - select() alters this each call.
		  i_retval = select(g_serial_fd+1, &i_rdfs, NULL, NULL, &i_tv);

		  // If incoming serial data...
		  if (FD_ISSET(g_serial_fd, &i_rdfs)) {
			   int serial_data_len = read(g_serial_fd, serial_buff, BUFFER_LEN);
			   if(serial_data_len) {
					for(i=0; i < serial_data_len; i++) {
						 if(serial_buff[i] == '\r' || serial_buff[i] == '\n') continue;
						 serial_xml[serial_cursor++] = serial_buff[i];
						 serial_xml[serial_cursor] = 0;

						 if(strstr(serial_xml,"</msg>")) {
						      int xmlsize = serial_cursor;
						      serial_cursor = 0;
						      if(strncmp(serial_xml,"<msg>",5) == 0) {
							   parseXml(serial_xml, xmlsize);
						      }
						 }
					}
			   }
		  }

		  // Send EVENT every TICK
		  if (xap_send_tick(SEND_INTERVAL)) {
			   send_all_info();
		  }

		  // Send heartbeat periodically
		  xap_heartbeat_tick(HBEAT_INTERVAL);
	 }
}

void setup_serial_port() {
	 struct termios newtio;
	 /* Open the serial port
		O_NDELAY: Don't wait for DCD signal line when opening the port.
		O_RDWR: Open for read and write
	 */
	 printf("\nUsing serial device %s\n\n", g_serialport);
	 g_serial_fd = open(g_serialport, O_RDONLY | O_NDELAY);
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
	 if(model == CC128) {
		  newtio.c_cflag = B57600 | CS8 | CLOCAL | CREAD ;
	 } else { // classic and default
		  newtio.c_cflag = B9600 | CS8 | CLOCAL | CREAD ;
	 }
	 newtio.c_iflag = IGNPAR;
	 newtio.c_lflag = ~ICANON;
	 newtio.c_cc[VTIME] = 0; /* ignore timer */
	 newtio.c_cc[VMIN] = 0; /* no blocking read */
	 tcflush(g_serial_fd, TCIFLUSH);
	 tcsetattr(g_serial_fd, TCSANOW, &newtio);
}

void setupXAPini() {
	 char uid[5];
	 char guid[9];
	 long n;

	 // default to 00DC if not present
	 n = ini_gets("currentcost","uid","00DC",uid,sizeof(uid),inifile);

	 // validate that the UID can be read as HEX
	 if(n == 0 || !(isxdigit(uid[0]) && isxdigit(uid[1]) && 
					isxdigit(uid[2]) && isxdigit(uid[3]))) 
	 {
		  strlcpy(uid,"00DC",sizeof uid);
	 }
	 snprintf(guid,sizeof guid,"FF%s00", uid);
	 XAP_GUID = strdup(guid);

	 char control[30];
	 n = ini_gets("currentcost","instance","CurrentCost",control,sizeof(control),inifile);
	 if(n == 0 || strlen(control) == 0) 
	 {
		  strlcpy(control,"CurrentCost",sizeof control);
	 }
	 XAP_DEFAULT_INSTANCE = strdup(control);

	 hysteresis = ini_getl("currentcost", "hysteresis", 10, inifile) / 2;

	 ini_gets("currentcost","usbserial","/dev/ttyUSB0", g_serialport, sizeof(g_serialport), inifile);

	 char model_s[20];
	 model = CLASSIC;
	 ini_gets("currentcost","model","classic", model_s, sizeof(model_s), inifile);
	 if(strcasecmp(model_s,"CC128") == 0) {
		  model = CC128;
	 }
}

int main(int argc, char *argv[]) {
	 FILE *fp;

	 printf("\nCurrent Cost Connector for xAP v12\n");
	 printf("Copyright (C) DBzoo, 2009\n\n");
	 setupXAPini();
	 xap_init(argc, argv, 0);
	 setup_serial_port();
	 process_event();
}
