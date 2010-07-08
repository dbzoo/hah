/*
  || $Id$
  ||
  || Provide a xAP capabilities
  ||
  || This library is free software; you can redistribute it and/or
  || modify it under the terms of the GNU Lesser General Public
  || License as published by the Free Software Foundation; version
  || 2.1 of the License.
  || 
  || This library is distributed in the hope that it will be useful,
  || but WITHOUT ANY WARRANTY; without even the implied warranty of
  || MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  || Lesser General Public License for more details.
  || 
  || You should have received a copy of the GNU Lesser General Public
  || License along with this library; if not, write to the Free Software
  || Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
  ||
*/

#include <string.h>
#include "xAP.h"

XapClass::XapClass(char *source, char *uid) {
     SOURCE = source;
     UID = uid;
     resetHeartbeat();
}

XapClass::XapClass(void) {
     XapClass("dbzoo.arduino.demo","FFDB2000");
}

void XapClass::resetHeartbeat(void) {
     heartbeatTimeout = smillis() + XAP_HEARTBEAT;
}

int XapClass::after(long timeout)
{
     return smillis()-timeout > 0;
}

int XapClass::getType(void) {
     if (xapMsgPairs==0) return XAP_MSG_NONE;
     if (strcasecmp(xapMsg[0].section,"xap-hbeat")==0) return XAP_MSG_HBEAT;
     if (strcasecmp(xapMsg[0].section,"xap-header")==0) return XAP_MSG_ORDINARY;
     return XAP_MSG_UNKNOWN;
}

char *XapClass::getValue(char *section, char *key) {
     for(int i=0; i<xapMsgPairs; i++) {
	  if(strcasecmp(section, xapMsg[i].section) == 0 && strcasecmp(key, xapMsg[i].key) == 0)
	       return xapMsg[i].value;
     }
     return (char *)NULL;
}

int XapClass::decode_state(char *msg) {
     static const char *value[] = {"on","off","true","false","yes","no","1","0"};
     static const int state[] = {1,0,1,0,1,0,1,0};
     if (msg == NULL) return -1;
     for(int i=0; i < sizeof(value); i++) {
	  if(strcasecmp(msg, value[i]) == 0)
	       return state[i];
     }
     return -1;
}

int XapClass::getBSCstate(char *section, char *key) {
     return decode_state(getValue(section, key));
}

int XapClass::isValue(char *section, char *key, char *value) {
     char *kvalue = getValue(section, key);
     return kvalue && strcasecmp(kvalue, value) == 0;
}

void XapClass::rtrim( byte *msg,  byte *p) {
     while(*p < 32 && p > msg)
	  *p-- = '\0';
}

// buf is modified.
int XapClass::parseMsg(byte *buf) {
     enum {
	  START_SECTION_NAME, IN_SECTION_NAME, START_KEYNAME, IN_KEYNAME, START_KEYVALUE, IN_KEYVALUE  
     } state = START_SECTION_NAME;
     char *current_section = NULL;
     byte *msg = buf;
  
     for(xapMsgPairs=0; *buf; buf++) {
	  switch (state) {
	  case START_SECTION_NAME:
	       if ( (*buf>32) && (*buf<128) ) {
		    state = IN_SECTION_NAME;
		    current_section = (char *)buf;
	       }
	       break;
	  case IN_SECTION_NAME:
	       if (*buf == '{') {
		    *buf = '\0';
		    rtrim(msg, buf);
		    state = START_KEYNAME;
	       }
	       break;
	  case START_KEYNAME:
	       if (*buf == '}') {
		    state = START_SECTION_NAME;
	       } 
	       else if ((*buf>32) && (*buf<128)) {
		    xapMsg[xapMsgPairs].section = current_section;
		    xapMsg[xapMsgPairs].key = (char *)buf;
		    state = IN_KEYNAME;
	       }               
	       break;
	  case IN_KEYNAME:
	       if ((*buf < 32) || (*buf == '=')) {
		    *buf = '\0';
		    rtrim(msg, buf);
		    state = START_KEYVALUE;
	       }
	       break;
	  case START_KEYVALUE:
	       if ((*buf>32) && (*buf<128)) {
		    state = IN_KEYVALUE;
		    xapMsg[xapMsgPairs].value = (char *)buf;
	       }
	       break;
	  case IN_KEYVALUE:
	       if (*buf < 32) {
		    *buf = '\0';
		    rtrim(msg, buf);
		    state = START_KEYNAME;
		    xapMsgPairs++;
		    if (xapMsgPairs >= MAX_XAP_PAIRS) {
			 xapMsgPairs = 0;
		    }
	       }        
	       break;
	  }
     }
     return xapMsgPairs;
}
