/* $Id$

Serial interfacing to the external AVR hardware
*/
#ifndef _SERIAL_H
#define _SERIAL_H

extern int gSerialfd;

int setupSerialPort(char *serialport, int baud);
void serialSend(char *buf);
void serialInputHandler(int fd, void *data);

#endif
