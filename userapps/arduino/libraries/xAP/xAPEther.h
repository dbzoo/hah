/*
  ||
  || @file 	xAPEther.h
  || @version	1.0
  || @author	Brett England
  || @contact	brett@dbzoo.com
  ||
  || @description
  || | Provide an xAP capabilities for NuElectronic ethernet module
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
#ifndef XAPETHER_H
#define XAPETHER_H

#include "xAP.h"
#include <../EtherCard/EtherCard.h>

#define XAP_PORT 3639

class XapEther : public XapClass {
 public:
  XapEther();
  XapEther(byte *mac, byte *ip);
  XapEther(char *source, char *uid, byte *mac, byte *ip);

  void setBuffer(byte *buf, word len);
  void process(word len, void (*callback)());
  void sendHeartbeat(void);

 private:
  byte *xapbuf;
  word xapbuflen;
};

extern EtherCard eth;
extern BufferFiller bfill;
#endif
