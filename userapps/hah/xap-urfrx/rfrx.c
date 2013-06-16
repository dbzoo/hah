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
#include "log.h"
#include "rfrx.h"
#include "urfdecoder.h"
#include "xap.h"
#include "minIni.h"

#define INFO_INTERVAL 120
static char *interfaceName = "br0";
static char *serialPort = "/dev/ttyUSB0";
const char *inifile = "/etc/xap-livebox.ini";
static RFdecoder *decoderList;
static bscEndpoint *endpointList;

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

/* Decode a stream string of positive and negative integers and convert into their binary equiv
 * and hand each to the decoder ring.
 */
void serialInputHandler(int fd, void *data) {
  static char buff[10];
  static int buff_cnt = 0;
  int durState;
  int framecnt = 0;
  RFdecoder *decoder;
  char serial[300];
  int len, i;

  while( (len = read(fd, serial, sizeof(serial)-1)) > 0) {
    if(getLoglevel() >= LOG_INFO) {
      serial[len] = 0;
      info(serial);
    }
      for(i=0; i<len; i++) {
	// Accumulate until its not a valid digit
	if(isdigit(serial[i]) || serial[i] == '-') {
	  buff[buff_cnt++] = serial[i];
	  if(buff_cnt > sizeof(buff)) {
	    alert("Buffer overflow");
	    buff_cnt--;
	  }
	  continue;
	}
	
	if(serial[i] == '\n') {
	  info("Reset RF decoders");
	  LL_FOREACH(decoderList, decoder) {
	    URFReset(decoder->ctx);
	  }
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
} 


static void usage(char *prog)
{
  printf("%s: [options]\n",prog);
  printf("  -i, --interface IF     Default %s\n", interfaceName);
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

  xapInitFromINI("xap","dbzoo.livebox","urfrx","00EE",interfaceName,inifile);
  setupFromINI();

  // Register the endpoints for xapBSC.query/cmd callbacks too.
  bscAddEndpointFilterList(endpointList, INFO_INTERVAL);

  if(strncmp(serialPort,"/dev/",5) == 0) {
    xapAddSocketListener(setupSerialPort(), &serialInputHandler, NULL);
    xapProcess();
  } else {
    serialInputHandler(setupSerialPort(), NULL);
  }

  return 0;
}
