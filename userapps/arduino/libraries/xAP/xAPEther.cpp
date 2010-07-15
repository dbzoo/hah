/*
  ||
  || @file 	xAPEther.cpp
  || @version	1.0
  || @author	Brett England
  || @contact	brett@dbzoo.com
  ||
  || @description
  || | Provide an xAP capabilities for Nuelectronics ethernet module
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

 // Create a couple of instances for usage
EtherCard eth;
BufferFiller bfill;

// Defaults if using an empty constructor.
static byte mymac[6] = { 0x54,0x55,0x58,0x10,0x00,0x26 };
static byte myip[4] = { 192,168,1,15 };

XapEther::XapEther() : XapClass() {
  XapEther(mymac, myip);
}

XapEther::XapEther(byte *mac, byte *ip) : XapClass() {
  eth.spiInit();
  eth.initialize(mac);
  eth.initIp(mac, ip, 80);  // & webserver port.
}

XapEther::XapEther(char *source, char *uid, byte *mac, byte *ip) : XapClass(source,uid) {
  XapEther(mac, ip);
}

void XapEther::setBuffer(byte *buf, word len) {
  xapbuf = buf;
  xapbuflen = len;
}

void XapEther::process(word len, void (*xapCallback)()) {
  // UDP port 3639
  if(len && xapCallback &&
     xapbuf[IP_PROTO_P] == IP_PROTO_UDP_V && 
     xapbuf[UDP_DST_PORT_H_P] == 0x0e && 
     xapbuf[UDP_DST_PORT_L_P] == 0x37) 
    {
      word udp_data_len = (((word)xapbuf[UDP_LEN_H_P]) << 8 | xapbuf[UDP_LEN_L_P]) - UDP_HEADER_LEN;
      if(parseMsg(eth.udpOffset(xapbuf), udp_data_len)) {
	(*xapCallback)();
      }
    }

  heartbeat();
}

void XapEther::sendHeartbeat(void) {
  // What's going on here?
  // We push the UDP data directly into the buffer
  // then create the PACKET datagram around it and transmit.
  bfill = eth.udpOffset(xapbuf);
  bfill.emit_p(PSTR("xap-hbeat\n"
		    "{\n"
		    "v=12\n"
		    "hop=1\n"
		    "uid=$S\n"
		    "class=xap-hbeat.alive\n"
		    "source=$S\n"
		    "interval=$D\n"
		    "port=3639\n"
		    "}"), UID, SOURCE, XAP_HEARTBEAT/1000);
  eth.sendUdpBroadcast(xapbuf, bfill.position(), 3639, 3639);
}
