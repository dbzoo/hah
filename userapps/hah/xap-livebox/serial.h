/* $Id$

Serial interfacing to the external AVR hardware
*/
#ifndef _SERIAL_H
#define _SERIAL_H

extern int gSerialfd;
extern int firmware_revision;

int setupSerialPort(char *serialport, int baud);
void serialSend(char *buf);
void serialInputHandler(int fd, void *data);
int firmwareMajor();
int firmwareMinor();

#endif
