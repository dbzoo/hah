/*
  ||
  || @file 	xAPEther.cpp
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
#include "xAPEther.h"
#include <net.h>

XapEther::XapEther(void) : XapClass() {
}

XapEther::XapEther(char *source, char *uid) : XapClass(source,uid) {
  XapEther();
}

void XapEther::processPacket(byte *buf, word len, void (*func)()) {
  // UDP port 3639
  if(len && 
     buf[IP_PROTO_P] == IP_PROTO_UDP_V && 
     buf[UDP_DST_PORT_H_P] == 0x0e && 
     buf[UDP_DST_PORT_L_P] == 0x37) 
    {
      word udp_data_len = (((word)buf[UDP_LEN_H_P]) << 8 | buf[UDP_LEN_L_P]) - UDP_HEADER_LEN;
      if(parseMsg(&buf[UDP_LEN_H_P], udp_data_len)) {
	(*func)();
      }
    }
  
  heartbeat();
}

void XapEther::heartbeat(void) {
     if (after(heartbeatTimeout)) {      
       sendHeartbeat();
     }
}

void XapEther::sendHeartbeat(void) {
}
