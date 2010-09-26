/* $Id
   Copyright (c) Brett England, 2010

   Serial communication using the Serial.Comm Schema
   As this is for UNIX the PORT is the path to the /dev/ttyxxx entry.

   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include "xap.h"
#define INFO_INTERVAL 120

xAP *gXAP;
char *interfaceName = "eth0";

struct serialPort {
	char *device;  //  ie. /dev/ttyUSB0
	speed_t speed;
	struct termios tios;
	int fd;
	struct serialPort *next;
} *serialList = NULL;


void xapSerialRx(int fd, void *userData)
{
	struct serialPort *p = (struct serialPort *)userData;
	
	char sdata[1024];
	int slen = read(fd, sdata, sizeof(sdata));

        char buff[XAP_DATA_LEN];
        int len = snprintf(buff, XAP_DATA_LEN, "xap-header\n"
			   "{\n"
			   "v=12\n"
			   "hop=1\n"
			   "uid=FF00DF01\n"
			   "class=Serial.Comms\n"
			   "source=%s\n"
			   "}\n"
			   "Serial.Received\n"
			   "{\n"
			   "port=%s\n"
			   "data=", gXAP->source, p->device);
	strncpy(&buff[len], sdata, slen);
	len += slen;
	len += snprintf(&buff[len], XAP_DATA_LEN-len, "}\n");

	if(len > XAP_DATA_LEN)
                err("xAP message buffer truncated - not sending\n");
        else
                xapSend(buff);
}

struct serialPort *findDevice(char *idevice) {
	struct serialPort *e = NULL;
	LL_SEARCH_SCALAR(serialList, e, device, idevice);
	return e;
}

void closeSerialPort(char *port) {
	if(port == NULL) return;

	struct serialPort *p = findDevice(port);
	if(p) {
		xAPSocketConnection *cb = xapFindSocketListenerByFD(p->fd);
		xapDelSocketListener(&cb);
		close(p->fd);
		LL_DELETE(serialList, p);
	}	
}

void xapSerialClose(void *userData) {
	char *s_port = xapGetValue("serial.close","port");
	closeSerialPort(s_port);
}

void xapSerialSetup(void *userData) {
	char *s_port = xapGetValue("serial.setup","port");
	char *s_baud = xapGetValue("serial.setup","baud");
	char *s_stop = xapGetValue("serial.setup","stop");
	char *s_databits = xapGetValue("serial.setup","databits");
	char *s_parity = xapGetValue("serial.setup","parity");
	char *s_flow = xapGetValue("serial.setup","flow");

	// Is the DEVICE being reinitialized?
	closeSerialPort(s_port);

	int fd;
	if ((fd = open(s_port, O_RDWR | O_NONBLOCK )) < 0)
	{
		err_strerror("Error opening Device %s", s_port);
		return;
	}
	else
	{
		int mflgs;
		if (((mflgs = fcntl(fd, F_GETFL, 0)) == -1) ||
		    (fcntl(fd, F_SETFL, mflgs | O_NONBLOCK ) == -1))
		{
			err_strerror("Error Setting O_NONBLOCK on device %s", s_port);
			close(fd);
		}
	}

	struct serialPort *p = (struct serialPort *)malloc(sizeof(struct serialPort));
	p->fd = fd;
	p->device = strdup(s_port);
	LL_APPEND(serialList, p);

	/*
	** Get the current termios structure for the device
	** and set up the initial defaults. Then configure
	** it as was specified in the xAP message.
	*/
	struct termios tios;
	tcgetattr(p->fd, &tios);
	tios.c_cflag = B38400 | CRTSCTS | CS8 | CREAD | CLOCAL ;
	tios.c_lflag = 0;
	tios.c_oflag = 0;
	tios.c_iflag |= IGNBRK;
	tios.c_cc[VMIN] = 0;
	tios.c_cc[VTIME] = 0;
	tcsetattr(p->fd, TCSANOW, &tios);
	tcflush(p->fd, TCIFLUSH);

	// BAUD
	switch (atoi(s_baud))
	{
	case 50: p->speed = B50; break;
	case 75: p->speed = B75; break;
	case 110: p->speed = B110; break;
	case 134: p->speed = B134; break;
	case 150: p->speed = B150; break;
	case 200: p->speed = B200; break;
	case 300: p->speed = B300; break;
	case 600: p->speed = B600; break;
	case 1200: p->speed = B1200; break;
	case 1800: p->speed = B1800; break;
	case 2400: p->speed = B2400; break;
	case 4800: p->speed = B4800; break;
	case 9600: p->speed = B9600; break;
	case 19200: p->speed = B19200; break;
	case 38400: p->speed = B38400; break;
	case 57600: p->speed = B57600; break;
	case 115200: p->speed = B115200; break;
	default:
		err("Invalid speed %s", s_baud);
		return;
	}  
	
	// FLOW CONTROL
	if(strcasecmp("none", s_flow) == 0) {
		p->tios.c_iflag &= ~(IXOFF | IXON);
	} else if(strcasecmp("xonxoff", s_flow) == 0) {
		p->tios.c_iflag |= IXOFF | IXON;
	} 
         /* else if(strcasecmp("hardware", s_flow) == 0) {
		p->tios.c_cflag |= CNEW_RTSCTS;
	} */ 
	else {
		err("Invalid flow control '%s'", s_flow);
		return;
	}

	// PARITY
	if(strcasecmp("none", s_parity) == 0) {
		p->tios.c_iflag |= IGNPAR;
		p->tios.c_cflag &= ~(PARENB | PARODD);
	} else if(strcasecmp("even", s_parity) == 0) {
		p->tios.c_iflag &= ~(IGNPAR | PARMRK);
		p->tios.c_iflag |= INPCK;
		p->tios.c_cflag |= PARENB;
		p->tios.c_cflag &= ~PARODD;
	} else if(strcasecmp("odd", s_parity) == 0) {
		p->tios.c_iflag &= ~(IGNPAR | PARMRK);
		p->tios.c_iflag |= INPCK;
		p->tios.c_cflag |= (PARENB | PARODD);
	} else {
		err("Invalid parity '%s'", s_parity);
		return;
	}

	// FLOW CONTROL
	switch (atoi(s_stop)) {
	case 1:
		p->tios.c_cflag &= ~CSTOPB;
		break;
	case 2:
		p->tios.c_cflag |= CSTOPB;
		break;
	default:
		err("Invalid stop '%s'", s_stop);
		return;
	}

	//DATA BITS
	switch(atoi(s_databits)) {
	case 5: p->tios.c_cflag |= CS5; break;
	case 6: p->tios.c_cflag |= CS6; break;
	case 7: p->tios.c_cflag |= CS7; break;
	case 8: p->tios.c_cflag |= CS8; break;
	default:
		err("Invalid databits '%s'", s_databits);
		return;
	}

	// Set it as per the xAP message.
	cfsetispeed(&p->tios, p->speed);
	cfsetospeed(&p->tios, p->speed);
	tcsetattr(p->fd, TCSANOW, &p->tios);

	xapAddSocketListener(p->fd, &xapSerialRx, p);
}


/* Send a packet down the serial port.  As the port is NON-BLOCKING a
 * write() failure will return EAGAIN; we increasing back-off until
 * the retry count is expired then port tranmissions are disabled.
 */
int sendSerialMsg(struct serialPort* p, char *msg) {
     const int retries = 6;  // sum(1..5)*300ms = 4.5sec
     int i, rv, size;
     for(i=1; i<retries; i++) {
          size = strlen(msg);
          rv = write(p->fd, msg, size);
          if(rv == -1) {
		  notice_strerror("Failed serial write %s: retry %d", p->device, retries);
               if (errno == EAGAIN) { // would block..
                    usleep(300*1000*i);  // 300ms * i
                    continue;
               }
          }
          tcdrain(p->fd);
          return rv;
     }

     warning("Too many retries on %s", p->device);
     return -1;
}

void xapSerialTx(void *userData) {
	char *port = xapGetValue("serial.send","port");
	char *data = xapGetValue("serial.send","data");

	struct serialPort *p = findDevice(port);
	if(p == NULL) {
		warning("Device %s not configured", port);
		return;
	}		
	sendSerialMsg(p, data);
}

/// Display usage information and exit.
static void usage(char *prog) {
	 printf("%s: [options]\n",prog);
	 printf("  -i, --interface IF     Default %s\n", interfaceName);
	 printf("  -d, --debug            0-7\n");
	 printf("  -h, --help\n");
	 exit(1);
}

/// MAIN
int main(int argc, char *argv[])
{
	int i;
	printf("Serial Connector for xAP\n");
	printf("Copyright (C) DBzoo, 2008-2010\n\n");

	for(i=0; i<argc; i++) {
		if(strcmp("-i", argv[i]) == 0 || strcmp("--interface",argv[i]) == 0) {
			interfaceName = argv[++i];
		} else if(strcmp("-d", argv[i]) == 0 || strcmp("--debug", argv[i]) == 0) {
			setLoglevel(atoi(argv[++i]));			
		} else if(strcmp("-h", argv[i]) == 0 || strcmp("--help", argv[i]) == 0) {
			usage(argv[0]);
		}
	}	


	xapInit("dbzoo.livebox.Serial", "FF00DF00", interfaceName);
	die_if(gXAP == NULL,"Failed to init xAP");

	xAPFilter *f = NULL;

	// Mandatory elements are part of the filter
	xapAddFilter(&f, "xap-header", "class", "Serial.Comms");
	xapAddFilter(&f, "Serial.Setup", "port", NULL);
	xapAddFilter(&f, "Serial.Setup", "baud", NULL);
	xapAddFilter(&f, "Serial.Setup", "stop", NULL);
	xapAddFilter(&f, "Serial.Setup", "databits", NULL);
	xapAddFilter(&f, "Serial.Setup", "parity", NULL);
	xapAddFilter(&f, "Serial.Setup", "flow", NULL);
	xapAddFilterAction(&xapSerialSetup, f, NULL);

	f = NULL;
	xapAddFilter(&f, "xap-header", "class", "Serial.Comms");
	xapAddFilter(&f, "Serial.Send", "port", NULL);
	xapAddFilter(&f, "Serial.Send", "data", NULL);
	xapAddFilterAction(&xapSerialTx, f, NULL);

	f = NULL;
	xapAddFilter(&f, "xap-header", "class", "Serial.Comms");
	xapAddFilter(&f, "Serial.Close", "port", NULL);
	xapAddFilterAction(&xapSerialClose, f, NULL);

        xapProcess();
	return 0;
}
