/*
  ||
  || @file 	xAPSerial.cpp
  || @version	1.0
  || @author	Brett England
  || @contact	brett@dbzoo.com
  ||
  || @description
  || | Provide an xAP capabilities for Serial ports
  || #
  ||
  || @license
  || | This library is free software; you can redistribute it and/or
  || | modify it under the terms of the GNU Lesser General Public
  || | License as published by the Free Software Foundation; version
  || | 2.1 of the License.
  || |
  || | This library is distributed in the hope that it will be useful,
  || | but WITHOUT ANY WARRANTY; without even the implied warranty of
  || | MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  || | Lesser General Public License for more details.
  || |
  || | You should have received a copy of the GNU Lesser General Public
  || | License along with this library; if not, write to the Free Software
  || | Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
  || #
  ||
*/
#include "xAPSerial.h"
#include <HardwareSerial.h>

XapSerial::XapSerial(void) : XapClass() {
     state = ST_START;
     p = NULL;
     xapRaw = NULL;
     xapRawSize = 0;
}

XapSerial::XapSerial(char *source, char *uid) : XapClass(source,uid) {
  XapSerial();
}

void XapSerial::process(void (*func)()) {
  while(Serial.available() > 0) {
    if(unframeSerialMsg(Serial.read())) {
      parseMsg(xapRaw, p-xapRaw);
      (*func)();
    }
  }
  heartbeat();
}

void XapSerial::sendHeartbeat(void) {
  Serial.print(STX, BYTE);
  Serial.println("xap-hbeat\n{\nv=12\nhop=1");
  Serial.print("uid=");  Serial.println(UID);
  Serial.println("class=xap-hbeat.alive");
  Serial.print("source="); Serial.println(SOURCE);
  Serial.print("interval="); Serial.println(XAP_HEARTBEAT/1000);
  Serial.print("}----");
  Serial.print(ETX, BYTE);
}

void XapSerial::setBuffer(byte *p, int s) {
  xapRaw = p;
  xapRawSize = s;
}

// Unframe and calculate the checksum
byte *XapSerial::unframeSerialMsg(int ch) {
     // Check for: Unset buffer or buffer over-run.
     if (xapRaw == NULL || p >= (xapRaw + xapRawSize)) {
	  state = ST_START;
     }

     while(1) {
	  switch(state) {
	  case ST_START:
	       if(ch == STX) {
		    state = ST_COMPILE;
		    p = xapRaw;   // init Accumulation ptr.
	       }
	       return NULL;
      
	  case ST_COMPILE:
	       switch(ch) {
	       case STX: // Spurious STX.
		    state = ST_START;
		    break;
	       case ETX: // End of message
		    state = ST_END;
		    break;
	       case ESC:  // Consume next char its escaped.
		    state = ST_COMPILE_ESC;
		    return NULL;
	       default:
		    *p++ = ch;  // Accumulate char.
		    return NULL;
	       }
	       break;

	  case ST_COMPILE_ESC:
	       *p++ = ch;  // Accumulate char.
	       state = ST_COMPILE;
	       return NULL;

	  case ST_END:
	       state = ST_START;
	       *p = 0;  // NULL terminate our frame               
	       p -= 4;  // Last 4 bytes are the checksum
	       if(p < xapRaw) { // Check for buffer under-run                   
		    return NULL;
	       }
	       /*
		 int buflen = p - xapRaw;
		 if(strcmp("----", p)) {  // We have a checksum.
		 unsigned short msg_crc, calc_crc;
		 sscanf(p, "%X", &msg_crc);
		 calc_crc = CRC16_BlockChecksum(xapRaw, buflen);
		 if(msg_crc != calc_crc) {
		 return NULL;
		 }
		 }
	       */
	       *p = 0; // return a ZERO terminated string (minus CRC).
	       return xapRaw;
	  }         
     }
     return NULL; // never reached
}
