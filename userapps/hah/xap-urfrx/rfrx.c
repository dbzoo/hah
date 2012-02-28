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
#include <fcntl.h>
#include <stdio.h>
#include <termios.h>
#include <stdlib.h>
#include <sys/select.h>
#include "log.h"
#include "rfrx.h"
#include "urfdecoder.h"
#include "xap.h"

static char *interfaceName = "br0";
static char *serialPort = "/dev/ttyUSB0";
const char *inifile = "/etc/xap-livebox.ini";
static RFdecoder *decoderList;

RFdecoder *addDecoder(void *ctx, int (*signalConsumer)(void *ctx, int durState)) {
  if(ctx == NULL) return NULL;

  RFdecoder *d = (RFdecoder *)malloc(sizeof(RFdecoder));
  d->ctx = ctx;
  d->signalConsumer = signalConsumer;

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

void serialInputHandler(int fd, void *data) {
  int len;
  int durState;
  RFdecoder *decoder;
  int framecnt = 1;
  
  while((len = read(fd, &durState, sizeof(int))) > 0) {
    debug("Frame %d [%d,%d] ", framecnt++, durState <0 , abs(durState));
    LL_FOREACH(decoderList, decoder) {
      (*decoder->signalConsumer)( decoder->ctx, durState );
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

int main(int argc, char **argv) {
  int i;

  printf("Universal RF Receiver for xAP\n");
  printf("Copyright (C) DBzoo, 2012\n\n");

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

  xapInitFromINI("xap","dbzoo.livebox","URFRX","00EE",interfaceName,inifile);

  // Hardcode patterns for now - BBSB A1 on/off.
  addDecoder( initURFContext("01010177046504650177060127101900001400","dbzoo.livebox.controller:rf.1","off"), URFConsumer);
  addDecoder( initURFContext("01010177046504650177060127101900001500","dbzoo.livebox.controller:rf.1","on"), URFConsumer);

  if(strncmp(serialPort,"/dev/",5) == 0) {
    xapAddSocketListener(setupSerialPort(), &serialInputHandler, NULL);
    xapProcess();
  } else {
    serialInputHandler(setupSerialPort(), NULL);
  }

  return 0;
}
