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
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "xap.h"
#include "bsc.h"

#define INFO_INTERVAL 120

int serialfd;

typedef struct _cmd
{
        char *name;
        void (* func)(struct _cmd *, xAP *, bscEndpoint *, char **);
}
cmd_t;

void serin_input(cmd_t *, xAP *, bscEndpoint *, char **);
void serin_1wire(cmd_t *, xAP *, bscEndpoint *, char **);

// incoming serial command dispatch table.
cmd_t cmd[] = {
                      {"input", &serin_input},
                      {"1wire", &serin_1wire},
                      { NULL, NULL }
              };

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
void serin_input(cmd_t *s, xAP *x, bscEndpoint *head, char *argv[])
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
                        bscEndpoint *e = findbscEndpoint(head, "input", buff);
                        if(e) {
                        	setbscState(e, new & (1<<bit) ? STATE_ON : STATE_OFF);
                                (*e->infoEvent)(x, e, "xapBSC.event");
                        }
                }
        }
}

/** Process inbound SERIAL command for a 1wire endpoint.
*
* 1wire N L
*
* N - is the number of the i2c/1-wire device on the BUS we support a larger number
*     by amending the endpoint struct.
* L - a numeric value.  Temp, Pressure etc..
*/
void serin_1wire(cmd_t *s, xAP *x, bscEndpoint *head, char *argv[])
{
        if(argv[0] == NULL)
                return;  // Bad input shouldn't happen?!
        bscEndpoint *e = findbscEndpoint(head, "1wire", argv[0]);
        if(e) {
                setbscText(e, argv[1]);
                (*e->infoEvent)(x, e, "xapBSC.event");
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
void serial_handler(xAP *x, bscEndpoint *head, char *a_cmd)
{
        cmd_t *s = &cmd[0];
        char *command = NULL;

        command = strtok(a_cmd," ");
        while(s->name) {
                if(strcmp(command, s->name) == 0) {
                        int i;
                        char *argv[8];
                        char *arg;
                        for(i=0; i < sizeof(argv); i++) {
                                arg = strtok(NULL," ,");
                                if(arg == NULL)
                                        break;
                                argv[i] = arg;
                        }
                        (*s->func)(s, x, head, argv);
                        break;
                }
                s++;
        }
}

/** Read a buffer of serial text and process a line at a time.
* if a CR/LF isn't seen continue to accumulate cmd data.
*/
void serialInputHandler(xAP *x, int fd, void *data)
{
        bscEndpoint *head = (bscEndpoint *)data;
        char serial_buff[128];
        int i, len;

        static char cmd[128];
        static int pos = 0;

        len = read(serialfd, serial_buff, 128);
        for(i=0; i < len; i++) {
                cmd[pos] = serial_buff[i];
                if (cmd[pos] == '\r' || cmd[pos] == '\n') {
                        cmd[pos] = '\0';
                        if(pos)
                                serial_handler(x, head, cmd);
                        pos = 0;
                } else if(pos < sizeof(cmd)) {
                        pos++;
                }
        }
}

/// Send a message to the serial port.
void serialSend(char *buf)
{
        write(serialfd, buf, strlen(buf));
}

/// Handle xapBSC.cmd for the RELAY endpoints.
void relay_cmd (xAP *xap, bscEndpoint *e)
{
        char buf[30];
        char *state = e->state == STATE_ON ? "on" : "off";
        snprintf(buf, sizeof(buf), "relay %s %s\n", e->subaddr, state);
        serialSend(buf);
}

/// Handle xapBSC.cmd for the LCD endpoint.
void lcd_cmd (xAP *xap, bscEndpoint *e)
{
        char buf[30];
        snprintf(buf, sizeof(buf), "lcd %s\n", e->text);
        serialSend(buf);
}

/// Augment default xapBSC.info & xapBSC.event handler.
void infoEvent1wire (xAP *xap, bscEndpoint *e, char *clazz)
{
        // Lazy memory allocation of displayText
        if(e->displayText == NULL)
                e->displayText = (char *)malloc(20);
        // Configure the displayText optional argument.
        snprintf(e->displayText, 20, "Temperature %s C", e->text);

        bscInfoEvent(xap, e, clazz); // do the default.
}

/// Setup the serial port.
int setup_serial_port(char *serialport, int baud)
{
        struct termios newtio;
        int fd = open(serialport, O_RDWR | O_NDELAY);
        if (fd < 0)
                return -1;
        cfmakeraw(&newtio);
        newtio.c_cflag = baud | CS8 | CLOCAL | CREAD ;
        newtio.c_iflag = IGNPAR;
        newtio.c_lflag = ~ICANON;
        newtio.c_cc[VTIME] = 0; /* ignore timer */
        newtio.c_cc[VMIN] = 0; /* no blocking read */
        tcflush(fd, TCIFLUSH);
        tcsetattr(fd, TCSANOW, &newtio);
        return fd;
}

/// Setup Endpoints, the Serial port, a callback for the serial port, and process xAP messages.
int main(int argc, char *argv[])
{
        serialfd = setup_serial_port("/dev/ttyS0", B115200);
        if(serialfd < 0) {
                printf("Failed to open serial port");
                exit(1);
        }

        xAP *x = xapNew("dbzoo.livebox.Demo","FF00DB00", "eth0");

        bscEndpoint *ep = NULL;
        // An INPUT can't have control command.
        // A NULL infoEvent function is equiv to supplying bscInfoEvent().
	bscAddEndpoint(&ep, "1wire", "1", "01", BSC_INPUT, BSC_STREAM, NULL, &infoEvent1wire);
        bscAddEndpoint(&ep, "1wire", "2", "02", BSC_INPUT, BSC_STREAM, NULL, &infoEvent1wire);
        bscAddEndpoint(&ep, "input", "1", "03", BSC_INPUT, BSC_BINARY, NULL, NULL);
        bscAddEndpoint(&ep, "input", "2", "04", BSC_INPUT, BSC_BINARY, NULL, NULL);
        bscAddEndpoint(&ep, "input", "3", "05", BSC_INPUT, BSC_BINARY, NULL, NULL);
        bscAddEndpoint(&ep, "relay", "1", "06", BSC_OUTPUT, BSC_BINARY, &relay_cmd, NULL);
        bscAddEndpoint(&ep, "relay", "2", "07", BSC_OUTPUT, BSC_BINARY, &relay_cmd, NULL);
        bscAddEndpoint(&ep, "relay", "3", "08", BSC_OUTPUT, BSC_BINARY, &relay_cmd, NULL);
        bscAddEndpoint(&ep, "relay", "4", "09", BSC_OUTPUT, BSC_BINARY, &relay_cmd, NULL);
        bscAddEndpoint(&ep, "lcd",  NULL, "10", BSC_OUTPUT, BSC_STREAM, &lcd_cmd, NULL);

        xapAddBscEndpointFilters(x, ep, INFO_INTERVAL);

        // Install a handler for SERIAL input.
        xapAddSocketListener(x, serialfd, &serialInputHandler, ep);

        xapProcess(x);
	return 0;  // not reached
}
