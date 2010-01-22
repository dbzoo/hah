/* $Id$
 */
#ifndef _SERIAL_H
#define _SERIAL_H

struct _portConf;

enum _state {ST_START, ST_COMPILE, ST_COMPILE_ESC, ST_END};

struct _serialState {
     char xap[1500];
     enum _state state;
     char *p;  // Current position in XAP for accumulation.
};

typedef struct _serialState serialState;

void enableSerialPorts();
char *frameSerialXAPpacket(const char* xap);
int sendSerialMsg(struct _portConf *pDevice, char *msg);
int openSerialPort(struct _portConf *pDevice);
char *unframeSerialMsg(serialState *ss, char *buf, int size);

#endif
