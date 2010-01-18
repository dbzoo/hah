/* $Id$

   Copyright (c) Brett England, 2010
   Portions Copyright (C) 1996  Joseph Croft <joe@croftj.net>  
   
   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.  
*/

#include "bridge.h"
#include "crc16.h"
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>

void buildDeviceSettingStr(int mfd, portConf *pDevice)
{
   struct termios tios;
   speed_t mspeed;
   char buf[256];
   int x;

   strcpy(buf, "");
   tcgetattr(mfd, &tios);
   switch (tios.c_cflag & CSIZE)
   {
      case CS8:
         strcat(buf, "8");
         break;
      case CS7:
         strcat(buf, "7");
         break;
      case CS6:
         strcat(buf, "6");
         break;
      case CS5:
         strcat(buf, "5");
         break;
      default:
         strcat(buf, "8");
         break;
   }
   switch (tios.c_cflag & (PARENB | PARODD))
   {
      case PARENB | PARODD:
         strcat(buf, "O");
         break;
      case PARENB:
         strcat(buf, "E");
         break;
      default:
         strcat(buf, "N");
   }

   if ((x = tios.c_cflag) & CRTSCTS)
      strcat(buf, "C1");
   else
      strcat(buf, "C0");

   if (tios.c_iflag & IXON)
      strcat(buf, "S1");
   else
      strcat(buf, "S0");

   setPortStr(pDevice, buf);

   mspeed = cfgetospeed(&tios);
   switch (mspeed)
   {
      case B300:
         strcpy(buf, "300");
         break;

      case B600:
         strcpy(buf, "600");
         break;

      case B1200:
         strcpy(buf, "1200");
         break;

      case B2400:
         strcpy(buf, "2400");
         break;

      case B4800:
         strcpy(buf, "4800");
         break;

      case B9600:
         strcpy(buf, "9600");
         break;

      case B19200:
         strcpy(buf, "19200");
         break;

      case B38400:
         strcpy(buf, "38400");
         break;

      case B57600:
         strcpy(buf, "57600");
         break;

      case B115200:
         strcpy(buf, "115200");
         break;

      case B230400:
         strcpy(buf, "230400");
         break;

      default:
         strcpy(buf, "38400");
         break;
   }
   setBaudStr(pDevice, buf);
}

/*
** Open the serial ports, if a port does not open mark its as disabled.
** returns the File descriptor for conveniance.
*/
int openSerialPort(portConf *pDevice) {
     int mflgs, fd = -1;

     debug(LOG_DEBUG,"openSerial: Opening device |%s|", pDevice->devc);
     if ((fd = open(pDevice->devc, O_RDWR | O_NDELAY)) < 0)
     {
	  debug(LOG_ERR, "Error opening Device %s:%m", pDevice->devc);
	  fd = -1;
     }
     else
     {
	  if (((mflgs = fcntl(fd, F_GETFL, 0)) == -1) ||
	      (fcntl(fd, F_SETFL, mflgs & ~O_NDELAY) == -1))
	  {
	       debug(LOG_ERR, "Error Resetting O_NODELAY on device %s:%m",
		     pDevice->devc);
	       close(fd);
	       fd = -1;
	  }
	  else
	  {
	       /*
	       ** Get the current termios structure for the device
	       ** and set up the initial defaults. Then configure
	       ** it as was specified in the configuration file.
	       */
	       struct termios tios;
	       tcgetattr(fd, &tios);
	       tios.c_cflag = CREAD | CLOCAL | CS8 | B38400 | CRTSCTS;;
	       tios.c_lflag = 0;
	       tios.c_oflag = 0;
	       tios.c_iflag |= IGNBRK;
	       tios.c_cc[VMIN] = 0;
	       tios.c_cc[VTIME] = 0;
	       tcsetattr(fd, TCSANOW, &tios);
	       cfsetispeed(&pDevice->tios, pDevice->speed);
	       cfsetospeed(&pDevice->tios, pDevice->speed);
	       tcsetattr(fd, TCSANOW, &pDevice->tios);
	       buildDeviceSettingStr(fd, pDevice);
	  }
     }

     pDevice->fd = fd;
     pDevice->enabled = fd != -1;

     return(fd);
}

/* Frame an xAP packet for Serial Transport.
**
** http://www.xapautomation.org/index.php?title=Protocol_definition#Transport_Wrapper
*/
char *frameSerialXAPpacket(const char* xap)
{
     static char buff[1500];
     char *p = buff;
     unsigned short crc;

     // The xAP message is prefixed with the ASCII control character
     // <STX> (ASCII character decimal 2)
     *p++ = 2;

     // Any instances of the <STX> character and <ETX> character
     // (ASCII characters decimal 2 and decimal 3 respectively) are
     // escaped by prefixing the character with <ESC> (decimal 27)
     // This mechanism is defined here for completeness; in practice
     // it would be very unusual to transmit non-printable characters
     // as part of a xAP message.
     CRC16_InitChecksum(&crc);
     while(*xap) {
	  switch(*xap) {
	  case 2:
	  case 3:
	  case 27:
	       *p = 27;
	       CRC16_Update(&crc, *p++);
	  }
	  *p = *xap++;
	  CRC16_Update(&crc, *p++);
     }
     CRC16_FinishChecksum(&crc);

     //At the end of the xAP message a 16-bit CRC checksum is appended
     //as four ASCII-hex digits (ie. human readable, not binary). The
     //checksum is applied to all data within the message envelope:
     //the <STX>, checksum itself, and <ETX> character are not
     //included in the CRC calculation. Hex digits A-F are represented
     //in upper case.
     sprintf(p,"%04X", crc);
     p += 4;

     // The checksum is immediately followed by <ETX> (ASCII character decimal 3) 
     *p++ = 3;
     *p = 0;

     return buff;
}

int sendSerialMsg(portConf *pDevice, char *msg) {
     int rv = write(pDevice->fd, msg, strlen(msg));
     if(rv == -1) {
	  debug(LOG_DEBUG,"Failed serial write %s:%m", pDevice->devc);
     }
     return rv;
}

// Recieve a serial xap frame package and unframe it returning
// a pointer to the unframe payload.
char *unframeSerialMsg(char *buf, int size) {
     static char xap[1500];
     char *p;
     int state, i;
     unsigned short msg_crc, calc_crc;
     int haveChecksum = 0;
     enum {ST_START, ST_COMPILE, ST_END};

     // Unframe and calculate the checksum
     state = ST_START;
     for(i=0; i < size;) {
	  switch(state) {
	  case ST_START:
	       if(buf[i] == 2) {
		    state = ST_COMPILE;
		    // Setup output ptr and CRC
		    p = xap;
	       }
	       i++;
	       break;
	  case ST_COMPILE:
	       switch(buf[i]) {
	       case 2: // Spurious STX.
		    state = ST_START;
		    break;
	       case 3:
		    state = ST_END;
		    *p = 0;
		    break;
	       case 27:
		    i++;
	       default:
		    *p++ = buf[i++];
	       }
	       break;
	  case ST_END:
	       // Last 4 bytes are the checksum
	       p = xap + strlen(xap) - 4;
	       debug(LOG_DEBUG,"readSerialMsg(): Serial CRC %s", p);
	       if(strcmp("----",p)) {
		    haveChecksum = 1;
		    sscanf(p, "%X", &msg_crc);
	       }
	       *p = 0; // remove CRC from xAP msg.
	       i = size;
	  }
     }
     debug(LOG_DEBUG,"readSerialMsg(): unframed %s", xap);

     // If the xAP has a Checksum...
     if(haveChecksum) {
	  calc_crc = CRC16_BlockChecksum(xap, strlen(xap));
    	  if(msg_crc != calc_crc) {
	       debug(LOG_WARNING, "Checksums to not match: msg %h != calc %h", msg_crc, calc_crc);
	       return NULL;
	  }
     }

     return xap;
}
