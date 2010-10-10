/* $Id$
   Copyright (c) Brett England, 2010

   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.
*/

#include <stdio.h>
#include <string.h>
#include "xap.h"

/// Return the type of xAP packet
int xapGetType() {
	if (gXAP->parsedMsgCount==0) return XAP_MSG_NONE;
	if (strcasecmp(gXAP->parsedMsg[0].section,"xap-hbeat")==0) return XAP_MSG_HBEAT;
	if (strcasecmp(gXAP->parsedMsg[0].section,"xap-header")==0) return XAP_MSG_ORDINARY;
	// Patrick had these in his xaplib are these valid?
	if (strcasecmp(gXAP->parsedMsg[0].section,"xap-config-request")==0) return XAP_MSG_CONFIG_REQUEST;
	if (strcasecmp(gXAP->parsedMsg[0].section,"xap-cache-request")==0) return XAP_MSG_CACHE_REQUEST;
	if (strcasecmp(gXAP->parsedMsg[0].section,"xap-cache-reply")==0) return XAP_MSG_CACHE_REPLY;
	if (strcasecmp(gXAP->parsedMsg[0].section,"xap-config-reply")==0) return XAP_MSG_CONFIG_REPLY;
	return XAP_MSG_UNKNOWN;
}

/** Fetch the value for a section/key combination.
* NULL will be returned if the section/key is not found
* otherwise a pointer to the value.
*/
char *xapGetValue(char *section, char *key) {
	int i;
	debug("section=%s key=%s", section, key);
	for(i=0; i < gXAP->parsedMsgCount; i++) {
		if(strcasecmp(section, gXAP->parsedMsg[i].section) == 0 &&
		   strcasecmp(key, gXAP->parsedMsg[i].key) == 0) {
			   char *val = gXAP->parsedMsg[i].value;
			debug("found value=%s", val);
			return val;
		   }
	}
	return (char *)NULL;
}

/** Test if a section/key is equal to a value.
* Returns 1 is a match is found otherwise 0
*/
int xapIsValue(char *section, char *key, char *value) {
	char *kvalue = xapGetValue(section, key);
	return kvalue && strcasecmp(kvalue, value) == 0;
}

/// Right trim control chars and whitespace
static void rtrim(unsigned char *msg,  unsigned char *p) {
	while(*p < 32 && p > msg)
		*p-- = '\0';
}

/** Populates the struct parsedMsg with pointers into dataPacket for the parsing rules.
* dataPacket is modified.
*/
int parseMsg() {
	enum {
		START_SECTION_NAME, IN_SECTION_NAME, START_KEYNAME, IN_KEYNAME, START_KEYVALUE, IN_KEYVALUE  
	} state = START_SECTION_NAME;
	char *current_section = NULL;
	unsigned char *buf;
	gXAP->parsedMsgCount = 0;
  
	for(buf = gXAP->dataPacket; buf < gXAP->dataPacket+XAP_DATA_LEN; buf++) {
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
				rtrim(gXAP->dataPacket, buf);
				state = START_KEYNAME;
			}
			break;
		case START_KEYNAME:
			if (*buf == '}') {
				state = START_SECTION_NAME;
			} 
			else if ((*buf>32) && (*buf<128)) {
				gXAP->parsedMsg[gXAP->parsedMsgCount].section = current_section;
				gXAP->parsedMsg[gXAP->parsedMsgCount].key = (char *)buf;
				state = IN_KEYNAME;
			}
			break;
		case IN_KEYNAME:
			if ((*buf < 32) || (*buf == '=')) {
				*buf = '\0';
				rtrim(gXAP->dataPacket, buf);
				state = START_KEYVALUE;
			}
			break;
		case START_KEYVALUE:
			if ((*buf>32) && (*buf<128)) {
				state = IN_KEYVALUE;
				gXAP->parsedMsg[gXAP->parsedMsgCount].value = (char *)buf;
			}
			break;
		case IN_KEYVALUE:
			if (*buf < 32) {
				*buf = '\0';
				rtrim(gXAP->dataPacket, buf);
				state = START_KEYNAME;
				gXAP->parsedMsgCount++;
				if (gXAP->parsedMsgCount > XAP_MSG_ELEMENTS) {
					gXAP->parsedMsgCount = 0;
				}
			}
			break;
		}
	}
	return gXAP->parsedMsgCount;
}

/** Reconstruct an XAP packet from the parsed components.
*/
int parsedMsgToRawWithoutSection(char *msg, int size, char *section) {
	char *currentSection = NULL;
	int i;
	int len = 0;
	msg[len] = '\0';
	for(i=0; i < gXAP->parsedMsgCount; i++) {
		// Skip a body section if required.
		if(section && strcasecmp(section, gXAP->parsedMsg[i].section) == 0) continue;
		
		if (currentSection == NULL || currentSection != gXAP->parsedMsg[i].section) {
			if(currentSection != NULL) {
				len += snprintf(&msg[len], size, "}\n");
			}
			len += snprintf(&msg[len],size,"%s\n{\n", gXAP->parsedMsg[i].section);
			currentSection = gXAP->parsedMsg[i].section;
		}
		len += snprintf(&msg[len],size,"%s=%s\n", gXAP->parsedMsg[i].key, gXAP->parsedMsg[i].value);
	}
	len += snprintf(&msg[len],size,"}\n");
	return len;
}

int parsedMsgToRaw(char *msg, int size) {
	return parsedMsgToRawWithoutSection(msg, size, NULL);
}
