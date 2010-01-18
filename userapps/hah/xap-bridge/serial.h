/* $Id$
 */
#ifndef _SERIAL_H
#define _SERIAL_H

#include "config.h"

void enableSerialPorts();
char *frameSerialXAPpacket(const char* xap);
int sendSerialMsg(portConf *pDevice, char *msg);
int openSerialPort(portConf *pDevice);

#endif
