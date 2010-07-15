/*
||
|| @file 	xAP.h
|| @version	1.1
|| @author	Brett England
|| @contact	brett@dbzoo.com
||
|| @description
|| | Provide an xAP capabilities
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
#ifndef XAP_H
#define XAP_H

#include "WProgram.h"

#define XAP_MSG_NONE 0
#define XAP_MSG_HBEAT  1
#define XAP_MSG_ORDINARY 2
#define XAP_MSG_UNKNOWN 0

#define MAX_XAP_PAIRS 20

// Milliseconds.
#define XAP_HEARTBEAT 60000
#define smillis() ((long)millis())

struct xapmsg_buffer {
  char *section;
  char *key;
  char *value;
};

class XapClass
{
 public:
  XapClass(void);
  XapClass(char *source, char *uid);
  char *getValue(char *section, char *key);    // Get the contents of a parsed xAP message by section/key.
  int getState(char *section, char *key);   // Get a BSC state value
  int isValue(char *section, char *key, char *value);   // does section/key have value ?
  int getType(void);               // The (TYPE) of the xAP message
  int parseMsg(byte *buf, int size);  // Parse a raw XAP message; exposed for Ethernet shield users.
  int after(long);                    // Useful for managing TIMED events such as heartbeats and info
  void dumpParsedMsg();
  void heartbeat(void);
  virtual void sendHeartbeat() {};

  const char *SOURCE;
  const char *UID;

 private:
  struct xapmsg_buffer xapMsg[MAX_XAP_PAIRS];
  byte xapMsgPairs;

  long heartbeatTimeout;

  int decode_state(char *msg);
  void rtrim(byte *msg, byte *p);
  void resetHeartbeat();
};
#endif
