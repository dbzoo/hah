/* $Id$
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
#include <stdarg.h>
#include "xap.h"
#define INFO_INTERVAL 120

char *interfaceName = "eth0";
const char inifile[] = "/etc/xap-livebox.ini";

struct serialPort {
	char *device;  //  ie. /dev/ttyUSB0
	speed_t speed;
	struct termios tios;
	int fd;
	struct serialPort *next;
} *serialList = NULL;

void serialError(const char *fmt, ...)
{
        char buff[XAP_DATA_LEN];
	va_list ap;

	va_start(ap, fmt);
	err(fmt, ap); // Log it

        int len = sprintf(buff, "xap-header\n"
			   "{\n"
			   "v=12\n"
			   "hop=1\n"
			   "uid=%s\n"
			   "class=Serial.Comms\n"
			   "source=%s\n"
			   "}\n"
			   "Serial.Error\n"
			   "{\n"
                          "text=", xapGetUID(), xapGetSource());
	len += vsnprintf(&buff[len], XAP_DATA_LEN-len, fmt, ap);
	va_end(ap);
	len += snprintf(&buff[len], XAP_DATA_LEN-len, "\n}\n");

	if(len < XAP_DATA_LEN) {
	xapSend(buff);
	} else {
		err("Buffer overflow %d/%d", len, XAP_DATA_LEN);
	}
}

void processSerialCommand(void *data, struct serialPort *p)
{	
        char buff[XAP_DATA_LEN];  // nominally 1500 bytes
        int len = snprintf(buff, sizeof(buff), "xap-header\n"
			   "{\n"
			   "v=12\n"
			   "hop=1\n"
			   "uid=%s\n"
			   "class=Serial.Comms\n"
			   "source=%s\n"
			   "}\n"
			   "Serial.Received\n"
			   "{\n"
			   "port=%s\n"
			   "data=%s\n"
                           "}\n", xapGetUID(), xapGetSource(), p->device, data);
	if(len > sizeof(buff)) {
	  serialError("Buffer overflow");
	  return;
	}
	xapSend(buff);
}

/** Read a buffer of serial text and process a line at a time.
* if a CR/LF isn't seen continue to accumulate cmd data.
*/
void xapSerialRx(int fd, void *userData)
{
	struct serialPort *p = (struct serialPort *)userData;
        char serial_buff[512];
        int i, len;

        static char cmd[128];
        static int pos = 0;

        len = read(fd, serial_buff, sizeof(serial_buff));
        for(i=0; i < len; i++) {
                cmd[pos] = serial_buff[i];
                if (cmd[pos] == '\r' || cmd[pos] == '\n') {
                        cmd[pos] = '\0';
                        if(pos)
                                processSerialCommand(cmd, p);
                        pos = 0;
                } else if(pos < sizeof(cmd)) {
                        pos++;
                }
        }
}

struct serialPort *findDevice(char *idevice) {
	struct serialPort *e = NULL;

	LL_FOREACH(serialList, e) {
		if(strcmp(e->device, idevice) == 0)
			break;
	}	
	return e;
}

void closeSerialPort(char *port) {
	if(port == NULL) return;

	struct serialPort *p = findDevice(port);
	if(p) {
		info("Closing port %s", port);
		close(p->fd);
		xapDelSocketListener(xapFindSocketListenerByFD(p->fd));
		LL_DELETE(serialList, p);
		free(p->device);
		free(p);
	}
}

void xapSerialClose(void *userData) {
	closeSerialPort(xapGetValue("serial.close","port"));
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
		log_write(LOG_ALERT, strerror(errno));
		serialError("Error opening Device %s", s_port);
		return;
	}
	else
	{
		int mflgs;
		if (((mflgs = fcntl(fd, F_GETFL, 0)) == -1) ||
		    (fcntl(fd, F_SETFL, mflgs | O_NONBLOCK ) == -1))
		{
			log_write(LOG_ALERT, strerror(errno));
			serialError("Error Setting O_NONBLOCK on device %s", s_port);
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
	bzero(&tios, sizeof(tios));
	tios.c_cflag = B38400 | CRTSCTS | CS8 | CREAD | CLOCAL ;
	tios.c_iflag |= IGNBRK;
	tcflush(p->fd, TCIOFLUSH);
	tcsetattr(p->fd, TCSANOW, &tios);

	bzero(&p->tios, sizeof(tios));
	p->tios.c_cflag = CREAD | CLOCAL;
	
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
		serialError("Invalid speed %s", s_baud);
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
		serialError("Invalid value flow=%s", s_flow);
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
		serialError("Invalid value stop=%s", s_stop);
		return;
	}

	//DATA BITS
	switch(atoi(s_databits)) {
	case 5: p->tios.c_cflag |= CS5; break;
	case 6: p->tios.c_cflag |= CS6; break;
	case 7: p->tios.c_cflag |= CS7; break;
	case 8: p->tios.c_cflag |= CS8; break;
	default:
		serialError("Invalid value databit=%s", s_databits);
		return;
	}

	// Set it as per the xAP message.
	cfsetispeed(&p->tios, p->speed);
	cfsetospeed(&p->tios, p->speed);
	tcsetattr(p->fd, TCSANOW, &p->tios);

	xapAddSocketListener(p->fd, &xapSerialRx, p);
	info("Setup port %s", p->device);
}


/* Send a packet down the serial port.  As the port is NON-BLOCKING a
 * write() failure will return EAGAIN; we increasing back-off until
 * the retry count is expired.
 */
int sendSerialMsg(struct serialPort* p, char *msg, size_t size) {
     const int retries = 6;  // sum(1..5)*300ms = 4.5sec
     int i, rv;
     info("send to %s '%s'", p->device, msg);
     for(i=1; i<retries; i++) {
          rv = write(p->fd, msg, size);
          if(rv == -1) {
	       notice_strerror("Failed serial write %s: retry %d", p->device, i);
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

size_t unescape(const char *in, const char *data) {
  size_t len = 0;
  char *result = (char *)in;
#define UCHAR(cp) ((const unsigned char *)(cp))
#define ISOCTAL(ch) (isdigit(ch) && (ch) != '8' && (ch) != '9')

  int ch, oval, i;

  while((ch = *UCHAR(data++)) != 0) {
    if(ch == '\\') {
      if((ch = *UCHAR(data++)) == 0)
	break;
      switch(ch) {
      case 'a': ch = '\a'; break;
      case 'b': ch = '\b'; break;
      case 'f': ch = '\f'; break;
      case 'n': ch = '\n'; break;
      case 'r': ch = '\r'; break;
      case 't': ch = '\t'; break;
      case 'v': ch = '\v'; break;
      case '\\': ch = '\\'; break;
      case 'x' : // hex
	for(oval=0, i=0; i<2 && (ch = *UCHAR(data))!=0 && isxdigit(ch); i++, data++) {
	  oval = (oval << 4);
	  if(ch >= 'A') {
	      oval = oval | (toupper(ch) - 'A' + 10);
	  } else {
	      oval = oval | (ch - '0'); 
	  }
	}
	ch = oval;
	break;
      case '0':  // octal
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
	for(oval = ch - '0', i=0;
	    i < 2 && (ch = *UCHAR(data)) != 0 && ISOCTAL(ch);
	    i++, data++) {
	  oval = (oval << 3) | (ch - '0');
	}
	ch = oval;
	break;
      default:
	break;
      }
    }
    *result++ = ch;
    len++;
  }
  *result = '\0';
  return len;
}

void xapSerialTx(void *userData) {
	char *port = xapGetValue("serial.send","port");
	char *data = xapGetValue("serial.send","data");

	struct serialPort *p = findDevice(port);
	if(p == NULL) {
		serialError("Device %s not configured", port);
		return;
	}
	// Unescaped data must be smaller...
	char *unescaped = (char *)malloc(strlen(data)+1);
	size_t len = unescape(unescaped, data);
	sendSerialMsg(p, unescaped, len);
	free(unescaped);
}

/// MAIN
int main(int argc, char *argv[])
{
	printf("Serial Connector for xAP\n");
	printf("Copyright (C) DBzoo, 2008-2010\n\n");
	
	simpleCommandLine(argc, argv, &interfaceName);
	xapInitFromINI("serial","dbzoo.livebox","Serial","00D5",interfaceName,inifile);

	xAPFilter *f = NULL;

	// Mandatory elements are part of the filter
	xapAddFilter(&f, "xap-header", "target", xapGetSource());
	xapAddFilter(&f, "xap-header", "class", "Serial.Comms");
	xapAddFilter(&f, "Serial.Setup", "port", XAP_FILTER_ANY);
	xapAddFilter(&f, "Serial.Setup", "baud", XAP_FILTER_ANY);
	xapAddFilter(&f, "Serial.Setup", "stop", XAP_FILTER_ANY);
	xapAddFilter(&f, "Serial.Setup", "databits", XAP_FILTER_ANY);
	xapAddFilter(&f, "Serial.Setup", "parity", XAP_FILTER_ANY);
	xapAddFilter(&f, "Serial.Setup", "flow", XAP_FILTER_ANY);
	xapAddFilterAction(&xapSerialSetup, f, NULL);

	f = NULL;
	xapAddFilter(&f, "xap-header", "target", xapGetSource());
	xapAddFilter(&f, "xap-header", "class", "Serial.Comms");
	xapAddFilter(&f, "Serial.Send", "port", XAP_FILTER_ANY);
	xapAddFilter(&f, "Serial.Send", "data", XAP_FILTER_ANY);
	xapAddFilterAction(&xapSerialTx, f, NULL);

	f = NULL;
	xapAddFilter(&f, "xap-header", "target", xapGetSource());
	xapAddFilter(&f, "xap-header", "class", "Serial.Comms");
	xapAddFilter(&f, "Serial.Close", "port", XAP_FILTER_ANY);
	xapAddFilterAction(&xapSerialClose, f, NULL);

        xapProcess();
	return 0;
}
