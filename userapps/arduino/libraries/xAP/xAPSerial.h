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
#ifndef XAPSERIAL_H
#define XAPSERIAL_H

#include "xAP.h"

#define STX 2
#define ETX 3
#define ESC 27

enum unframe_state {
    ST_START, ST_COMPILE, ST_COMPILE_ESC, ST_END
};

class XapSerial : public XapClass {
 public:
  XapSerial(char *source, char *uid);

  // When a framed xAP message is received, parse and callback (func)
  void process(void (*func)());
  // Send a xap-hbeat
  void sendHeartbeat(void);
 // unframeSerialMsg() requires an accumulation buffer.
  void setBuffer(byte *buf, int size);

 private:
  // State machine to accumulate an XAP message from the serial port.
  byte *unframeSerialMsg(int ch); 

  byte *xapRaw;
  int xapRawSize;

  enum unframe_state state;
  byte *p; // Position in xapRaw between unframeSerialMsg calls.
};

#endif
