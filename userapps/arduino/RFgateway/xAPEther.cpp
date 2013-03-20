/*
  ||
  || @file 	xAPEther.cpp
  || @version	1.0
  || @author	Brett England
  || @contact	brett@dbzoo.com
  ||
  || @description
  || | Provide an xAP capabilities for EtherCard enabled devices
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

BufferFiller bfill;

XapEther::XapEther(char *source, char *uid) : XapClass(source,uid) {
    ether.copyIp(lanBroadcast, ether.mymask);
    for(byte i=0; i<4; ++i) {
      lanBroadcast[i] = ~lanBroadcast[i] | ether.myip[i];
    }    
}

void XapEther::process(word len, void (*xapCallback)()) {
  if(len && xapCallback &&
     ether.buffer[IP_PROTO_P] == IP_PROTO_UDP_V && 
     ether.buffer[UDP_DST_PORT_H_P] == 0x0e && 
     ether.buffer[UDP_DST_PORT_L_P] == 0x37) 
    {
      word udp_data_len = (((word)ether.buffer[UDP_LEN_H_P]) << 8 | ether.buffer[UDP_LEN_L_P]) - UDP_HEADER_LEN;
      if(parseMsg(ether.buffer + UDP_DATA_P, udp_data_len)) {
	(*xapCallback)();
      }
    }

  heartbeat();
}

// Assume UDP data payload already loaded in "Ethernet::buffer"
void XapEther::broadcastUDP (word len) {
  ether.udpPrepare(XAP_PORT, lanBroadcast, XAP_PORT);
  // Override MAC header for broadcast (EtherCard has issues)
  for(byte i=0; i<6; i++) { 
         ether.buffer[ETH_DST_MAC + i] = 0xff;
  }
  ether.udpTransmit(len);
}

void XapEther::sendHeartbeat(void) {
  // What's going on here?
  // We push the UDP data directly into the buffer
  // then create the PACKET datagram around it and transmit.
  bfill = ether.buffer + UDP_DATA_P;
  bfill.emit_p(PSTR("xap-hbeat\n"
		    "{\n"
		    "v=12\n"
		    "hop=1\n"
		    "uid=$S\n"
		    "class=xap-hbeat.alive\n"
		    "source=$S\n"
		    "interval=$D\n"
		    "port=$D\n"
		    "}"), UID, SOURCE, XAP_HEARTBEAT/1000, XAP_PORT);
  broadcastUDP(bfill.position());
}
