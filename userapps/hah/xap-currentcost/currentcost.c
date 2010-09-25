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

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <libxml/parser.h>
#include "xap.h"
#include "bsc.h"

// Seconds between xapBSC.info messages.
#define INFO_INTERVAL 120

xAP *gXAP;
const char inifile[] = "/etc/xap-livebox.ini";
char serialPort[20];
int hysteresis;
char *interfaceName = "eth0";

enum {CC128, CLASSIC} model;

bscEndpoint *endpointList = NULL;
bscEndpoint *currentTag = NULL;

/// BSC callback - Only emit info/event for Channels that adjust outside of the hysteresis amount.
static void infoEventChannel(bscEndpoint *e, char *clazz)
{
	int old = 0;
	if(e->userData)
	  old = atoi((char *)e->userData);
	int new = atoi(e->text);
	// Alway report INFO events so we can repond to xAPBSC.query + Timeouts.
	// xapBSC.event are only emitted based on the hystersis
	if(strcmp(clazz,"xapBSC.info") == 0 || new > old + hysteresis || new < old - hysteresis) {
	  if(e->displayText == NULL)
	    e->displayText = (char *)malloc(15);
	  snprintf(e->displayText, 15, "%d watts", new);
	  bscInfoEvent(e, clazz);
	}
}

/// BSC callback - Emit info/event for Temperature endpoints.
static void infoEventTemp(bscEndpoint *e, char *clazz)
{
	if(e->displayText == NULL)
		e->displayText = (char *)malloc(15);
	char unit = strcmp(e->name,"tmpr") == 0 ? 'C' : 'F'; // tmpr/tmprF
	snprintf(e->displayText, 15, "Temp %s %c", e->text, unit);
	bscInfoEvent(e, clazz);
}

/// SAX element (TAG) callback
static char *xmlTag[] = {"ch1","ch2","ch3","tmpr","tmprF",NULL};
static void startElementCB(void *ctx, const xmlChar *name, const xmlChar **atts)
{
        char **p;
        for(p=&xmlTag[0]; *p; p++) {
                if(strcmp(name, *p) == 0) {
                        currentTag = bscFindEndpoint(endpointList, (char *)name, NULL);
                        // Dynamic endpoints are useful for <tmpr> and <tmprF>
                        // We defer creation until we see it in the XML to handle UK/US variants.
                        if(currentTag == NULL) {
                                // Add to the list we want to search and manage
	                        currentTag = bscAddEndpoint(&endpointList, (char *)name, NULL, BSC_INPUT, BSC_STREAM, NULL, &infoEventTemp);
                                bscAddEndpointFilter(currentTag, INFO_INTERVAL);
                        }
                }
        }
}

/// SAX cdata callback
static void cdataBlockCB(void *ctx, const xmlChar *value, int len)
{
        if(currentTag == NULL)
                return;
        if(strncmp(value, currentTag->text, len)) { // If its different report it.
                // Rotate the original data value through the user data field.
                if(currentTag->userData)
                        free(currentTag->userData);
                currentTag->userData = (void *)currentTag->text;
                currentTag->text = NULL;
		bscSetState(currentTag, BSC_STATE_ON);
		if(currentTag->text) free(currentTag->text);
		currentTag->text = (char *)malloc(len+1);
		strncpy(currentTag->text, value, len);
	        bscSendCmdEvent(currentTag);
        }
        currentTag = NULL;
}

/// Parse an Currentcost XML message.
void parseXml(char *data, int size)
{
        xmlSAXHandler handler;

        debug("%s", data);

        memset(&handler, 0, sizeof(handler));
        handler.initialized = XML_SAX2_MAGIC;
        handler.startElement = startElementCB;
        handler.characters = cdataBlockCB;

        if(xmlSAXUserParseMemory(&handler, NULL, data, size) < 0) {
                err("Document not parsed successfully.");
                return;
        }
}

///. Read the XML stream from the currentcost unit.
void serialInputHandler(int fd, void *data)
{
        char serial_buff[128];
        char serial_xml[4096];
        static int serial_cursor = 0;
        int i;
        int len;

        while((len = read(fd, serial_buff, sizeof(serial_buff))) > 0) {
                for(i=0; i < len; i++) {
                        if(serial_buff[i] == '\r' || serial_buff[i] == '\n')
                                continue;
	                // Prevent buffer overruns.
	                if(serial_cursor == sizeof(serial_xml)-1) {
		                serial_cursor = 0;
	                }
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

/// Setup the serial port.
int setupSerialPort()
{
        struct termios newtio;
        int fd = open(serialPort, O_RDONLY | O_NDELAY);
        if (fd < 0) {
                die_strerror("Failed to open serial port %s", serialPort);
        }
        cfmakeraw(&newtio);
        if(model == CC128) {
                newtio.c_cflag = B57600 | CS8 | CLOCAL | CREAD ;
        } else { // classic and default
                newtio.c_cflag = B9600 | CS8 | CLOCAL | CREAD ;
        }
        newtio.c_iflag = IGNPAR;
        newtio.c_lflag = ~ICANON;
        newtio.c_cc[VTIME] = 0; // ignore timer
        newtio.c_cc[VMIN] = 0; // no blocking read
        tcflush(fd, TCIFLUSH);
        tcsetattr(fd, TCSANOW, &newtio);
        return fd;
}

/// Display usage information and exit.
static void usage(char *prog)
{
        printf("%s: [options]\n",prog);
        printf("  -i, --interface IF     Default %s\n", interfaceName);
        printf("  -s, --serial DEV       Default %s\n", serialPort);
        printf("  -m, --model [CLASSIC|CC128]  Default CLASSIC\n");
        printf("  -d, --debug            0-7\n");
        printf("  -h, --help\n");
        exit(1);
}

/// Process the INI file for xAP control data and setup the global gXAP object.
void setupXap()
{
        long n;
        char i_uid[5];
        char s_uid[10];

        n = ini_gets("xap","uid","00DC",i_uid, sizeof(i_uid),inifile);

        // validate that the UID can be read as HEX
        if(! (n > 0
                        && (isxdigit(i_uid[0]) && isxdigit(i_uid[1]) &&
                            isxdigit(i_uid[2]) && isxdigit(i_uid[3]))
                        && strlen(i_uid) == 4)) {
                err("invalid uid %s", i_uid);
                strcpy(i_uid,"00DC"); // not valid put back default.
        }
        snprintf(s_uid, sizeof(s_uid), "FF%s00", i_uid);

        char i_control[64];
        char s_control[128];
        n = ini_gets("xap","instance","CurrentCost",i_control,sizeof(i_control),inifile);
        snprintf(s_control, sizeof(s_control), "dbzoo.livebox.%s", i_control);

        xapInit(s_control, s_uid, interfaceName);
        die_if(gXAP == NULL,"Failed to init xAP");

        hysteresis = ini_getl("currentcost", "hysteresis", 10, inifile);

        ini_gets("currentcost","usbserial","/dev/ttyUSB0", serialPort, sizeof(serialPort), inifile);

        char model_s[20];
        model = CLASSIC;
        ini_gets("currentcost","model","classic", model_s, sizeof(model_s), inifile);
        if(strcasecmp(model_s,"CC128") == 0) {
	        info("Selecting CC128 model");
                model = CC128;
        } else {
	        info("Selecting CLASSIC model");
	}
}

/// Setup Endpoints, the Serial port, a callback for the serial port and process xAP messages.
int main(int argc, char *argv[])
{
        int i;
        printf("\nCurrent Cost Connector for xAP v12\n");
        printf("Copyright (C) DBzoo, 2009-2010\n\n");
	strcpy(serialPort,"/dev/ttyUSB0");

        for(i=0; i<argc; i++) {
                if(strcmp("-i", argv[i]) == 0 || strcmp("--interface",argv[i]) == 0) {
                        interfaceName = argv[++i];
                } else if(strcmp("-s", argv[i]) == 0 || strcmp("--serial", argv[i]) == 0) {
                        strlcpy(serialPort, argv[++i], sizeof(serialPort));
                } else if(strcmp("-d", argv[i]) == 0 || strcmp("--debug", argv[i]) == 0) {
                        setLoglevel(atoi(argv[++i]));
                } else if(strcmp("-h", argv[i]) == 0 || strcmp("--help", argv[i]) == 0) {
                        usage(argv[0]);
                }
        }

        setupXap();

	// The model switch if provided overrides the INI setting.
        for(i=0; i<argc; i++) {
                if(strcmp("-m", argv[i]) == 0 || strcmp("--model",argv[i]) == 0) {
		  if(strcasecmp(argv[++i], "CC128") == 0) {
		     info("Command line override selecting CC128 model");
		     model = CC128;
		  }
		}
        }

        bscAddEndpoint(&endpointList, "ch1", NULL, BSC_INPUT, BSC_STREAM, NULL, &infoEventChannel);
        bscAddEndpoint(&endpointList, "ch2", NULL, BSC_INPUT, BSC_STREAM, NULL, &infoEventChannel);
        bscAddEndpoint(&endpointList, "ch3", NULL, BSC_INPUT, BSC_STREAM, NULL, &infoEventChannel);
        bscAddEndpointFilterList(endpointList, INFO_INTERVAL);

        xapAddSocketListener(setupSerialPort(), &serialInputHandler, endpointList);
        xapProcess();

        return 0;  // not reached
}
