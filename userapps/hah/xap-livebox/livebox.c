/* $Id$
   Copyright (c) Brett England, 2010

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

	xapInitFromINI("xap","dbzoo.livebox","Controller","00DB",interfaceName,inifile);

	setupSerialPort(serialPort, B115200);

	/* Endpoint UID mapping - Identifies a particular hardware device for life.
	   As endpoints can be dynamically added we define ranges so this remains true.
	  LCD / Inputs  (0-63)     - 1 LCD + 4 INPUT on current hardware.
	  I2C           (64-95)    - PPE chips map at 64-71 natively on the i2c bus.
	  Relays        (96-127)   - 32 devices (firmware can only handle 4)
	  1-Wire        (128-159)  - 32 devices (firmware can only handle 16)
	  RF            (160+)     - 96 devices (firmware can only handle 12)
	*/
	lcd = bscAddEndpoint(&endpointList, "lcd",  NULL, BSC_OUTPUT, BSC_STREAM, &cmdLCD, NULL);
	bscAddEndpoint(&endpointList, "input", "1", BSC_INPUT, BSC_BINARY, NULL, &infoEventBinary);
	bscAddEndpoint(&endpointList, "input", "2", BSC_INPUT, BSC_BINARY, NULL, &infoEventBinary);
	bscAddEndpoint(&endpointList, "input", "3", BSC_INPUT, BSC_BINARY, NULL, &infoEventBinary);
	bscAddEndpoint(&endpointList, "input", "4", BSC_INPUT, BSC_BINARY, NULL, &infoEventBinary);
	bscSetEndpointUID(96);
	bscAddEndpoint(&endpointList, "relay", "1", BSC_OUTPUT, BSC_BINARY, &cmdRelay, &infoEventBinary);
	bscAddEndpoint(&endpointList, "relay", "2", BSC_OUTPUT, BSC_BINARY, &cmdRelay, &infoEventBinary);
	bscAddEndpoint(&endpointList, "relay", "3", BSC_OUTPUT, BSC_BINARY, &cmdRelay, &infoEventBinary);
	bscAddEndpoint(&endpointList, "relay", "4", BSC_OUTPUT, BSC_BINARY, &cmdRelay, &infoEventBinary);
	addIniEndpoints();

	// Register the endpoints
        bscAddEndpointFilterList(endpointList, INFO_INTERVAL);

	// If the serial port is setup register a listener
	if(gSerialfd > 0)
		xapAddSocketListener(gSerialfd, &serialInputHandler, endpointList);

	// Handle WEB server requests
	xapAddSocketListener(svr_bind(WEB_PORT), &webHandler, endpointList);
	
	bscSetText(lcd, gXAP->ip);  // Display our IP address on the LCD
	bscSendCmdEvent(lcd);

	serialSend("report"); // Ask AVR firmware to report current endpoints states
        xapProcess();  // Loop and run the program.
	return 0;  // not reached
}
