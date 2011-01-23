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
        void (* func)(struct _cmd *, bscEndpoint *, char **);
}
cmd_t;

static void serin_input(cmd_t *, bscEndpoint *, char **);
static void serin_1wire(cmd_t *, bscEndpoint *, char **);
static void serin_ppe(cmd_t *, bscEndpoint *, char **);
static void serin_firmware_rev(cmd_t *, bscEndpoint *, char **);

// incoming serial command dispatch table.
static cmd_t cmd[] = {
                             {"input", &serin_input},
                             {"1wire", &serin_1wire},
                             {"i2c-ppe", &serin_ppe},
                             {"rev", &serin_firmware_rev},
                             { NULL, NULL }
                     };

int firmwareMajor() {
	return major_firmware;
}

int firmwareMinor() {
	return minor_firmware;
}

static void serin_firmware_rev(cmd_t *s, bscEndpoint *unused, char *argv[])
{
	// string returned MAJOR.MINOR or MAJOR
	char *dot = strchr(argv[0],'.');
	if(dot == NULL) {
		major_firmware = atoi(argv[0]);
		minor_firmware = 0;
	} else {
		*dot = '\0';
		major_firmware = atoi(argv[0]);
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
static void serin_input(cmd_t *s, bscEndpoint *head, char *argv[])
{
        int old, new;
        int bit, i;
        char buff[2];

        if(argv[0] == NULL || argv[1] == NULL) // Bad input shouldn't happen?!
                return;

        bzero(buff, sizeof(buff));
        old = atoi(argv[0]);
        new = atoi(argv[1]);

        // Find out which bit(s) changed
        // The 4 bits that can be set are: 00111100
        for(i=0; i<4; i++) {
                bit = i + 2;
                if ((old ^ new) & (1 << bit)) {
                        buff[0] = '1' + i;
                        bscEndpoint *e = bscFindEndpoint(head, "input", buff);
                        if(e) {
                                bscSetState(e, new & (1<<bit) ? BSC_STATE_ON : BSC_STATE_OFF);
	                        bscSendCmdEvent(e);
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

static bscEndpoint *findROMID(bscEndpoint *head, char *name, char *romid)
{
        debug("name %s romid %s", name, romid);
        bscEndpoint *e;
	LL_FOREACH(head, e) {
                if(strcmp(e->name, name) == 0 && 
		   strcmp(((struct tempSensor *)e->userData)->romid, romid) == 0)
		{
	                info("Found ", e->source);
                        return e;
                }
        }
        return NULL;
}

static void serin_1wire(cmd_t *s, bscEndpoint *head, char *argv[])
{
        if(argv[0] == NULL)
                return;  // Bad input shouldn't happen?!
	char *temp = argv[1];

	bscEndpoint *e;
	if(firmwareMajor() > 1) {
		char *romid = argv[0]; // 1wire ROMID L
		e = findROMID(head, "1wire", romid);		
		if(e == NULL) { 
			notice("ROMID %s %s not assigned", romid, temp);

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
			strlcpy(u->temperature, temp, sizeof(u->temperature));
			return;
		}
	} else {
		char *id = argv[0]; // 1wire N L
		e = bscFindEndpoint(head, "1wire", id);
		if(e == NULL) { 
			notice("Endpoint 1wire.%s not found", id);
			return;
		}
	}

	bscSetState(e, BSC_STATE_ON);  // got a serial event we know its alive.
	bscSetText(e, temp);
	bscSendCmdEvent(e);
	// record the last time this 1wire device reported in.
	((struct tempSensor *)e->userData)->lastSerialEvent = time(NULL);

}

/** Processing inbound SERIAL command from the I2C PPE chip.
*
* i2c-ppe A O N
*
* A - Address of PPE chip (in hex 2 digits only)
* O - Original value before state change
* N - Value after state change
*/
static void serin_ppe(cmd_t *s, bscEndpoint *head, char *argv[])
{
        // We need to do a bit more work to find the endpoint.
        // As the user may have configure a single ENDPOINT or ONE per PIN
        char buff[30];
        char *addr = argv[0];  // PPE address
        char *old = argv[1];
        char *new = argv[2];

        info("addr %s new %s old %s", addr, new, old);
        if(new == NULL || old == NULL || addr == NULL) // Bad input shouldn't happen?!
                return;

        bscEndpoint *e = bscFindEndpoint(head, "i2c", addr);
        if(e) {
                bscSetText(e, new);
                (*e->infoEvent)(e, BSC_EVENT_CLASS);
        } else {
                int pin;
                int newi = atoi(new);
                int oldi = atoi(old);
                for(pin=0; pin<8; pin++) {
                        // Figure out what changed in the PPE
                        if ((oldi ^ newi) & (1 << pin)) {
                                // Compute an ENDPOINT name
                                snprintf(buff,sizeof buff,"%s.%d", addr, pin);
                                e = bscFindEndpoint(head, "i2c", buff);
                                bscSetState(e, newi & (1<<pin) ? BSC_STATE_ON : BSC_STATE_OFF);
				(*e->infoEvent)(e, BSC_EVENT_CLASS);

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
static void processSerialCommand(bscEndpoint *head, char *a_cmd)
{
        cmd_t *s = &cmd[0];
        char *command = NULL;
	int i = 0;
	char *argv[8];
	
	command = a_cmd;
	char *space = strchr(a_cmd,' ');
	if(space) {
		*space = '\0';
		a_cmd = space + 1;
	}
	
        while(s->name) {
                if(strcmp(command, s->name) == 0) {
			while(*a_cmd) {
				argv[i++] = a_cmd;
				if(i > 7) break;
				while(*a_cmd && !isspace(*a_cmd ) && *a_cmd != ',')
					a_cmd++;
				if(*a_cmd) {
					*a_cmd = '\0';
					a_cmd++;
				}
			}
                        (*s->func)(s, head, argv);
                        break;
                }
                s++;
        }
}

/** Read a buffer of serial text and process a line at a time.
* if a CR/LF isn't seen continue to accumulate cmd data.
*/
void serialInputHandler(int fd, void *data)
{
        char serial_buff[128];
        int i, len;

        static char cmd[128];
        static int pos = 0;

        len = read(gSerialfd, serial_buff, 128);
        for(i=0; i < len; i++) {
                cmd[pos] = serial_buff[i];
                if (cmd[pos] == '\r' || cmd[pos] == '\n') {
                        cmd[pos] = '\0';
                        if(pos)
                                processSerialCommand(endpointList, cmd);
                        pos = 0;
                } else if(pos < sizeof(cmd)) {
                        pos++;
                }
        }
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
	usleep(150 * 1000);    // wait for response - 150ms
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
        newtio.c_lflag = ~ICANON;
        newtio.c_cc[VTIME] = 0; // ignore timer
        newtio.c_cc[VMIN] = 0; // no blocking read
        tcflush(fd, TCIFLUSH);
        tcsetattr(fd, TCSANOW, &newtio);
        gSerialfd = fd;

	getFirmwareVersion();

        return fd;
}
