/* $Id$
   Copyright (c) Brett England, 2010

   Demonstration usage for the xAP and BSC library calls.
   Based around a stripped down xap-livebox daemon.

   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.
*/
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <ctype.h>
#include "minIni.h"
#include "xap.h"
#include "bsc.h"
#include "serial.h"
#include "server.h"
#include "ini.h"

#define INFO_INTERVAL 120
#define WEB_PORT 79

xAP *gXAP;
bscEndpoint *endpointList;
static char *serialPort = "/dev/ttyS0";
static char *interfaceName = "br0";

/// Handle xapBSC.cmd for the RELAY endpoints.
static void cmdRelay (bscEndpoint *e)
{
        char buf[30];
	snprintf(buf, sizeof(buf), "%s %s", bscStateToString(e), e->subaddr);
        serialSend(buf);
}

/// Handle xapBSC.cmd for the LCD endpoint.
static void cmdLCD(bscEndpoint *e)
{
        char buf[30];
        snprintf(buf, sizeof(buf), "lcd %s", e->text);
        serialSend(buf);
}

/// Display usage information and exit.
static void usage(char *prog) {
	 printf("%s: [options]\n",prog);
	 printf("  -i, --interface IF     Default %s\n", interfaceName);
	 printf("  -s, --serial DEV       Default %s\n", serialPort);
	 printf("  -d, --debug            0-7\n");
	 printf("  -h, --help\n");
	 exit(1);
}

/// Process the INI file for xAP control data and setup the global gXAP object.
void setupXap() {
	long n;
	char i_uid[5];
	char s_uid[10];

	n = ini_gets("xap","uid","00DB",i_uid, sizeof(i_uid),inifile);

	// validate that the UID can be read as HEX
	if(! (n > 0 
	      && (isxdigit(i_uid[0]) && isxdigit(i_uid[1]) && 
		  isxdigit(i_uid[2]) && isxdigit(i_uid[3]))
	      && strlen(i_uid) == 4))
	{
		err("invalid uid %s", i_uid);
		strcpy(i_uid,"00DB"); // not valid put back default.
	}
	snprintf(s_uid, sizeof(s_uid), "FF%s00", i_uid);

	char i_control[64];
	char s_control[128];
	n = ini_gets("xap","instance","Controller",i_control,sizeof(i_control),inifile);
	snprintf(s_control, sizeof(s_control), "dbzoo.livebox.%s", i_control);

	xapInit(s_control, s_uid, interfaceName);
	die_if(gXAP == NULL,"Failed to init xAP");
}

/// Setup Endpoints, the Serial port, a callback for the serial port and process xAP messages.
int main(int argc, char *argv[])
{
	bscEndpoint *lcd;
	int i;
	printf("Livebox Connector for xAP\n");
	printf("Copyright (C) DBzoo, 2008-2010\n\n");

	for(i=0; i<argc; i++) {
		if(strcmp("-i", argv[i]) == 0 || strcmp("--interface",argv[i]) == 0) {
			interfaceName = argv[++i];
		} else if(strcmp("-s", argv[i]) == 0 || strcmp("--serial", argv[i]) == 0) {
			serialPort = argv[++i];
		} else if(strcmp("-d", argv[i]) == 0 || strcmp("--debug", argv[i]) == 0) {
			setLoglevel(atoi(argv[++i]));			
		} else if(strcmp("-h", argv[i]) == 0 || strcmp("--help", argv[i]) == 0) {
			usage(argv[0]);
		}
	}	

	setupXap();
	setupSerialPort(serialPort, B115200);

	bscAddEndpoint(&endpointList, "input", "1", BSC_INPUT, BSC_BINARY, NULL, &infoEventBinary);
	bscAddEndpoint(&endpointList, "input", "2", BSC_INPUT, BSC_BINARY, NULL, &infoEventBinary);
	bscAddEndpoint(&endpointList, "input", "3", BSC_INPUT, BSC_BINARY, NULL, &infoEventBinary);
	bscAddEndpoint(&endpointList, "input", "4", BSC_INPUT, BSC_BINARY, NULL, &infoEventBinary);
	bscAddEndpoint(&endpointList, "relay", "1", BSC_OUTPUT, BSC_BINARY, &cmdRelay, &infoEventBinary);
	bscAddEndpoint(&endpointList, "relay", "2", BSC_OUTPUT, BSC_BINARY, &cmdRelay, &infoEventBinary);
	bscAddEndpoint(&endpointList, "relay", "3", BSC_OUTPUT, BSC_BINARY, &cmdRelay, &infoEventBinary);
	bscAddEndpoint(&endpointList, "relay", "4", BSC_OUTPUT, BSC_BINARY, &cmdRelay, &infoEventBinary);
	lcd = bscAddEndpoint(&endpointList, "lcd",  NULL, BSC_OUTPUT, BSC_STREAM, &cmdLCD, NULL);
	addIniEndpoints();

	// Register the endpoints
        xapAddBscEndpointFilterList(endpointList, INFO_INTERVAL);

	// If the serial port is setup register a listener
	if(gSerialfd > 0)
		xapAddSocketListener(gSerialfd, &serialInputHandler, endpointList);

	// Handle WEB server requests
	xapAddSocketListener(svr_bind(WEB_PORT), &webHandler, endpointList);
	
	setbscTextNow(lcd, gXAP->ip);  // Display our IP address on the LCD

	serialSend("report"); // Ask AVR firmware to report current endpoints states
        xapProcess();  // Loop and run the program.
	return 0;  // not reached
}
