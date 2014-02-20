/* $Id$
   Universal RF receiver

   Copyright (c) Brett England, 2012

   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.
*/
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <termios.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/file.h>
#include "log.h"
#include "rfrx.h"
#include "urfdecoder.h"
#include "xap.h"
#include "minIni.h"

#define INFO_INTERVAL 120
static char *interfaceName = NULL;
static char *serialPort = "/dev/ttyUSB0";
const char *inifile = "/etc/xap.d/xap-urfrx.ini";
static RFdecoder *decoderList;
static bscEndpoint *endpointList;

static unsigned char vSerial = 0;
static char vSource[60]; // source for virtual serial data.

RFdecoder *addDecoder(URFContext *ctx) {
  if(ctx == NULL) return NULL;

  RFdecoder *d = (RFdecoder *)malloc(sizeof(RFdecoder));
  d->ctx = ctx;
  LL_APPEND(decoderList, d);
  return d;
}

/// Setup the serial port.
int setupSerialPort()
{
  int fd;
  if(strncmp(serialPort,"/dev/",5) == 0) {
          struct termios newtio;
	  fd = open(serialPort, O_RDONLY | O_NDELAY);
	  if (fd < 0) {
	    die_strerror("Failed to open serial port %s", serialPort);
	  }
	  if(flock(fd, LOCK_EX | LOCK_NB) == -1) {
	    close(fd);
	    die_strerror("Serial port %s in use", serialPort);
	  }
	  cfmakeraw(&newtio);
	  newtio.c_cflag = B115200 | CS8 | CLOCAL | CREAD ;
	  newtio.c_iflag = IGNPAR;
	  newtio.c_lflag = ~ICANON;
	  newtio.c_cc[VTIME] = 0; // ignore timer
	  newtio.c_cc[VMIN] = 0; // no blocking read
	  tcflush(fd, TCIFLUSH);
	  tcsetattr(fd, TCSANOW, &newtio);
  } else {
	  fd = open(serialPort, O_RDONLY);
	  if (fd < 0) {
	    die_strerror("Failed to open file %s", serialPort);
	  }
  }
  return fd;
}

/** Decode a stream string of positive and negative integers and convert into their binary equiv
 * and hand each to the decoder ring.
 */
void processSerial(char *data, int len) {
  static char buff[10];
  static int buff_cnt = 0;
  int durState;
  int framecnt = 0;
  RFdecoder *decoder;
  int i;

  for(i=0; i<len; i++) {
    // Accumulate until its not a valid digit
    if(isdigit(data[i]) || data[i] == '-') {
      buff[buff_cnt++] = data[i];
      if(buff_cnt > sizeof(buff)) {
	alert("Buffer overflow");
	buff_cnt--;
      }
      continue;
    }
    if(buff_cnt == 0) continue;
    
    buff[buff_cnt] = 0;
    buff_cnt = 0;
    
    // String to integer
    if(sscanf(buff, "%d", &durState) == EOF) {
      err("Bad String: %s", buff);
      continue;
    }
    
    // Hand to each decoder state machine
    debug("Frame %d: %d", framecnt++, durState);
    LL_FOREACH(decoderList, decoder) {
      if(URFConsumer(decoder->ctx, durState ))
	framecnt = 0;
    }
  }
} 

/** Handle serial data from a VIRTUAL serial port.
 * xAP Filter Callback
 */
void vSerialHandler(void *uData) {
  char *data = xapGetValue("serial.received","data");
  if(data == NULL) return;

  const char *magicString = "RFRX ";
  const int len = strlen(magicString);
  // Data coming from a virtual serial port will being with the magic string RFRX
  // data=RFRX 10000,-382 etc...
  if(strncmp(data, magicString, len)) return;

  processSerial(data+len, strlen(data)-len);
}

/** Handle serial data from a REAL serial port.
 * xAP Socket Listener Callback
 */
void serialInputHandler(int fd, void *uData) {
  int len;
  char serial[300];
  
  while( (len = read(fd, serial, sizeof(serial)-1)) > 0) {
    if(getLoglevel() >= LOG_INFO) {
      serial[len] = 0;
      info(serial);
    }
    processSerial(serial, len);
  }  
}

static void usage(char *prog)
{
  printf("%s: [options]\n",prog);
  printf("  -i, --interface IF\n");
  printf("  -s, --serial DEV       Default %s\n", serialPort);
  printf("  -d, --debug            0-7\n");
  printf("  -h, --help\n");
  exit(1);
}

/* Setup URF RX decoders from the ini file.
 *
 * [urfrx]
 * devices=1
 * rf1.on=01010177046504650177060127101900001500
 * rf1.off=01010177046504650177060127101900001400
 */
void setupFromINI() {
  int devices, i;
  char rf[100];
  char buff[10];

  // We can be driven via xap-serial inbound messages too
  // instead of a direct serial port connection.
  vSerial = ini_getl("urfrx", "vserial", 0, inifile);
  if(vSerial) info("Virtual serial port enabled");
  ini_gets("urfrx", "vsource", "dbzoo.livebox.serial", vSource, sizeof vSource, inifile);

  devices = ini_getl("urfrx", "devices", -1, inifile);
  for(i = 1; i <= devices; i++) {

    snprintf(buff,sizeof buff,"%d", i);
    bscEndpoint *e = bscAddEndpoint(&endpointList, "rf", buff, BSC_INPUT, BSC_BINARY, NULL, NULL);

    snprintf(buff,sizeof buff,"rf%d.on",i);
    if(ini_gets("urfrx", buff, "", rf, sizeof rf, inifile)) {
      addDecoder( initURFContext(e, rf, BSC_STATE_ON));
    }

    snprintf(buff,sizeof buff,"rf%d.off",i);
    if(ini_gets("urfrx", buff, "", rf, sizeof rf, inifile)) {
      addDecoder( initURFContext(e, rf, BSC_STATE_OFF));
    }    
  }
}

int main(int argc, char **argv) {
  int i;

  printf("Universal RF Receiver for xAP\n");
  printf("Copyright (C) DBzoo, 2013\n\n");

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

  xapInitFromINI("xap","dbzoo","urfrx","00EE",interfaceName,inifile);
  setupFromINI();

  // Register the endpoints for xapBSC.query/cmd callbacks too.
  bscAddEndpointFilterList(endpointList, INFO_INTERVAL);

  if(vSerial == 1) { // Setup for a virtual serial port (aka xap-serial)
    xAPFilter *f = NULL;
    xapAddFilter(&f, "xap-header", "class","serial.comms");
    xapAddFilter(&f, "xap-header", "source", vSource);
    xapAddFilter(&f, "serial.received", "port", serialPort);
    xapAddFilterAction(&vSerialHandler, f, NULL);

    char buff[XAP_DATA_LEN];
    snprintf(buff, sizeof(buff), "xap-header\n"
	     "{\n"
	     "v=12\n"
	     "hop=1\n"
	     "uid=%s\n"
	     "class=Serial.Comms\n"
	     "source=%s\n"
	     "target=%s\n"
	     "}\n"
	     "Serial.Setup\n"
	     "{\n"
	     "port=%s\n"
	     "baud=57600\n"
	     "stop=1\n"
	     "databits=8\n"
	     "parity=\"none\"\n"
	     "flow=\"none\"\n"
	     "}\n", xapGetUID(), xapGetSource(), vSource, serialPort);
    xapSend(buff);

    xapProcess();
  } else  if(strncmp(serialPort,"/dev/",5) == 0) {
    xapAddSocketListener(setupSerialPort(), &serialInputHandler, NULL);
    xapProcess();
  } else {
    serialInputHandler(setupSerialPort(), NULL);
  }
    
  return 0;
}
