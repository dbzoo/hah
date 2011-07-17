/* $Id$
 
Serial interfacing to the external AVR hardware
*/
#ifdef IDENT
#ident "@(#) $Id$"
#endif


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include "xap.h"
#include "bsc.h"
#include "log.h"
#include "ini.h"

extern bscEndpoint *endpointList;

int gSerialfd;
static int major_firmware = 1;
static int minor_firmware = 0;

typedef struct _cmd
{
        char *name;
        void (* func)(int, char **);
	int args;
}
cmd_t;

static void serin_input(int, char **);
static void serin_1wire(int, char **);
static void serin_ppe(int, char **);
static void serin_firmware_rev(int, char **);

// incoming serial command dispatch table.
static cmd_t cmd[] = {
	{"input", &serin_input, 2},
	{"1wire", &serin_1wire, 2},
	{"i2c-ppe", &serin_ppe, 3},
	{"rev", &serin_firmware_rev, 1},
	{ NULL, NULL }
};

int firmwareMajor() {
	return major_firmware;
}

int firmwareMinor() {
	return minor_firmware;
}

static void serin_firmware_rev(int argc, char *argv[])
{
	// string returned MAJOR.MINOR or MAJOR
	char *dot = strchr(argv[1],'.');
	if(dot == NULL) {
		major_firmware = atoi(argv[1]);
		minor_firmware = 0;
	} else {
		*dot = '\0';
		major_firmware = atoi(argv[1]);
		minor_firmware = atoi(dot+1);
	}
        info("AVR firmware version: %d.%d", major_firmware, minor_firmware);
}

/** Process inbound SERIAL command for an INPUT endpoint.
*
* input o n
*
* input: will be followed by two decimal numbers
*        these represent the before and after state of
*        the input lines.  There are 4 input lines
*        Each will be bit mapped into a decimal number.
*        0110 = 6
*/
static void serin_input(int argc, char *argv[])
{
        int old, new;
        int bit, i;
        char buff[2];

        bzero(buff, sizeof(buff));
        old = atoi(argv[1]);
        new = atoi(argv[2]);

        // Find out which bit(s) changed
        // The 4 bits that can be set are: 00111100
        for(i=0; i<4; i++) {
                bit = i + 2;
                if ((old ^ new) & (1 << bit)) {
                        buff[0] = '1' + i;
                        bscEndpoint *e = bscFindEndpoint(endpointList, "input", buff);
                        if(e) {
                                bscSetState(e, new & (1<<bit) ? BSC_STATE_ON : BSC_STATE_OFF);
				bscSendEvent(e);
                        }
                }
        }
}

/** Process inbound SERIAL command for a 1wire endpoint.
*
* V1 - 1wire N L
* V2 - 1wire ROMID L
* 
* N - is the number of the i2c/1-wire device on the BUS we support a larger number
*     by amending the endpoint struct.
* L - a numeric value.  Temp, Pressure etc..
*/

static bscEndpoint *findROMID(char *name, char *romid)
{
        debug("name %s romid %s", name, romid);
        bscEndpoint *e;
	LL_FOREACH(endpointList, e) {
                if(strcmp(e->name, name) == 0 && 
		   strcmp(((struct tempSensor *)e->userData)->romid, romid) == 0)
		{
                        return e;
                }
        }
        return NULL;
}

static void serin_1wire(int argc, char *argv[])
{
	char *temperature = argv[2];

	bscEndpoint *e;
	if(firmwareMajor() > 1) {
		char *romid = argv[1]; // 1wire ROMID L
		if(strlen(romid) > 16) { // Workaround AVR <2.4 firmware bug
		  romid[16] = '\0';
		}
		e = findROMID("1wire", romid);		
		if(e == NULL) { 
			notice("ROMID %s %s not assigned", romid, temperature);

			// Add the ROM ID if not found, always update the current temperature so
			// its reported to the WEB interface.  It may help with assignment.
			struct unassignedROMID *u;
			int toadd = 1;
			LL_FOREACH(unassignedROMIDList, u) {
				if(strcmp(u->romid, romid) == 0) {
					toadd = 0;
					break;
				}
			}
			if(toadd) {
				info("Adding unassigned ROMID to list");
				u = (struct unassignedROMID *)calloc(1,sizeof(struct unassignedROMID));
				strlcpy(u->romid, romid, sizeof(u->romid));
				LL_PREPEND(unassignedROMIDList, u);
			}
			strlcpy(u->temperature, temperature, sizeof(u->temperature));
			return;
		}
	} else {
		char *id = argv[1]; // 1wire N L
		e = bscFindEndpoint(endpointList, "1wire", id);
		if(e == NULL) { 
			notice("Endpoint 1wire.%s not found", id);
			return;
		}
	}

	bscSetState(e, BSC_STATE_ON);  // got a serial event we know its alive.
	bscSetText(e, temperature);
	bscSendEvent(e);
	// record the last time this 1wire device reported in.
	((struct tempSensor *)e->userData)->lastSerialEvent = time(NULL);

}

/** Processing inbound SERIAL command from the I2C PPE chip.
*
* i2c-ppe A O N
*
* A - Address of PPE chip (in decimal)
* O - Original value before state change
* N - Value after state change
*/
static bscEndpoint *findPPEendpoint(int addr, char *subaddr) {
	bscEndpoint *e = NULL;
        info("0x%02X %s", addr, subaddr);
	LL_FOREACH(endpointList, e) {
		if(strcmp(e->name, "ppe") == 0 && ((struct ppeEndpoint *)(e->userData))->i2cAddr == addr) {
			if(subaddr == NULL) return e;
			if(strcmp(e->subaddr, subaddr) == 0) return e;
		}
	}
	return NULL;
}

static void serin_ppe(int argc, char *argv[])
{
        // We need to do a bit more work to find the endpoint.
        // As the user may have configure a single ENDPOINT or ONE per PIN
        info("addr %s old %s new %s", argv[1], argv[2], argv[3]);
        int addr = atoi(argv[1]);  // PPE address

        bscEndpoint *e = findPPEendpoint(addr, NULL);
	if(e == NULL) {
		err("PPE endpoint not found");
		return;
	}

        if(e->type == BSC_STREAM) { // BYTE mode
                bscSetText(e, argv[3]);
		bscSendEvent(e);
        } else { // BSC_BINARY .. PIN mode
		// The PPE endpoint we have located is one of the i2c pins.
		// Each has the same UserData so we use this to locate the RIGHT section/pin.
		int section = ((struct ppeEndpoint *)(e->userData))->section;
		char subaddr[10];
                int oldi = atoi(argv[2]);
                int newi = atoi(argv[3]);
                int pin;

                for(pin=0; pin<8; pin++) {
                        // Figure out what changed in the PPE
                        if ((oldi ^ newi) & (1 << pin)) {

                                // Compute an ENDPOINT name
                                snprintf(subaddr,sizeof subaddr,"%d.%d", section, pin);
                                e = findPPEendpoint(addr, subaddr);
				bscSetState(e, newi & (1<<pin) ? BSC_STATE_ON : BSC_STATE_OFF);
				bscSendEvent(e);
                        }
                }
        }
}

/** Tokenize a serial command and dispatch.
*
* Each serial command will be tokenized into the form
* command [arguments...]
*
* The command will be looked up in our serial command
* dispatch table and it will be passed the remaining arguments.
* Up to 8 arguments are allowed.
*/
static void processSerialCommand(char *a_cmd)
{
	int argc = 0;
	char *argv[8];

	info("%s", a_cmd);

	// Tokenize AVR command into arguments
	while(*a_cmd) {
		argv[argc++] = a_cmd;
		if(argc > sizeof(argv)/sizeof(argv[0])) {
			alert("%s: too many arguments", argv[0]);
			return;
		}
		while(*a_cmd && !isspace(*a_cmd ) && *a_cmd != ',')
			a_cmd++;
		if(*a_cmd) {
			*a_cmd = '\0';
			a_cmd++;
		}
	}
	if(argc == 0) return;

	// Dispatch
        cmd_t *s;
	for(s=&cmd[0]; s->name; s++) {
                if(strcmp(argv[0], s->name) == 0) {
			if(argc - 1 < s->args) {
				crit("%s: minimum arguments %d", argv[0], argc);
			} else {
				(*s->func)(argc, argv);
			}
                        break;
                }
        }
}

/** Read a buffer of serial text and process a line at a time.
* if a CR/LF isn't seen continue to accumulate cmd data.
*/
void serialInputHandler(int fd, void *data)
{
        static char serial_buff[128];
        int i, len;

        static char cmd[sizeof(serial_buff)];
        static int pos = 0;

	info("enter");
        len = read(gSerialfd, serial_buff, sizeof(serial_buff));
        for(i=0; i < len; i++) {
                cmd[pos] = serial_buff[i];
                if (cmd[pos] == '\r' || cmd[pos] == '\n') {
                        cmd[pos] = '\0';
                        if(pos)
                                processSerialCommand(cmd);
                        pos = 0;
                } else if(pos < sizeof(cmd)) {
                        pos++;
                }
        }
	info("exit");
}

/// Send a message to the serial port.
void serialSend(char *buf)
{
        if(gSerialfd > 0) {
                info("Serial Tx: %s", buf);
                char nbuf[128];
                snprintf(nbuf, sizeof(nbuf), "\n%s\n", buf);
                write(gSerialfd, nbuf, strlen(nbuf));
        }
}

static void getFirmwareVersion()
{
	serialSend("version"); // Get AVR version
	usleep(200 * 1000);     // wait for response - 200ms
	serialInputHandler(gSerialfd, NULL);	// non-blocking read
}

/** Setup the serial port.
* @param serialport Path to serial device
* @param baud a TERMIOS baud rate value
*/
int setupSerialPort(char *serialport, int baud)
{
        struct termios newtio;
        int fd = open(serialport, O_RDWR | O_NDELAY);
        if (fd < 0) {
                err_strerror("Failed to open serial port %s", serialport);
                return -1;
        }
        cfmakeraw(&newtio);
        newtio.c_cflag = baud | CS8 | CLOCAL | CREAD ;
        newtio.c_iflag = IGNPAR;
        newtio.c_lflag = 0;
        newtio.c_cc[VTIME] = 0; // ignore timer
        newtio.c_cc[VMIN] = 0; // no blocking read
        tcflush(fd, TCIFLUSH);
        tcsetattr(fd, TCSANOW, &newtio);
        gSerialfd = fd;

	getFirmwareVersion();

        return fd;
}
