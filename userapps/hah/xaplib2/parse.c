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
int xapGetType(xAP *this) {
	if (this->parsedMsgCount==0) return XAP_MSG_NONE;
	if (strcasecmp(this->parsedMsg[0].section,"xap-hbeat")==0) return XAP_MSG_HBEAT;
	if (strcasecmp(this->parsedMsg[0].section,"xap-header")==0) return XAP_MSG_ORDINARY;
	return XAP_MSG_UNKNOWN;
}

/** Fetch the value for a section/key combination.
* NULL will be returned if the section/key is not found
* otherwise a pointer to the value.
*/
char *xapGetValue(xAP *this, char *section, char *key) {
	int i;
	debug("section=%s key=%s", section, key);
	for(i=0; i < this->parsedMsgCount; i++) {
		if(strcasecmp(section, this->parsedMsg[i].section) == 0 && 
		   strcasecmp(key, this->parsedMsg[i].key) == 0) {
			char *val = this->parsedMsg[i].value;
			debug("found value=%s", val);
			return val;
		   }
	}
	return (char *)NULL;
}

/** Test if a section/key is equal to a value.
* Returns 1 is a match is found otherwise 0
*/
int xapIsValue(xAP *this, char *section, char *key, char *value) {
	char *kvalue = xapGetValue(this, section, key);
	return kvalue && strcasecmp(kvalue, value) == 0;
}

/// Right trim control chars and whitespace
static void rtrim(unsigned char *msg,  unsigned char *p) {
	while(*p < 32 && p > msg)
		*p-- = '\0';
}

/** Populates the struct parsedMsgElement with pointers into MSG for the parsing rules.
* MSG is modified.
*/
int parseMsg(struct parsedMsgElement parsedMsg[], int maxParsedMsgCount, unsigned char *msg) {
	enum {
		START_SECTION_NAME, IN_SECTION_NAME, START_KEYNAME, IN_KEYNAME, START_KEYVALUE, IN_KEYVALUE  
	} state = START_SECTION_NAME;
	char *current_section = NULL;
	unsigned char *buf;
	int parsedMsgCount = 0;
  
	for(buf = msg; buf < msg+XAP_DATA_LEN; buf++) {
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
				parsedMsg[parsedMsgCount].section = current_section;
				parsedMsg[parsedMsgCount].key = (char *)buf;
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
				parsedMsg[parsedMsgCount].value = (char *)buf;
			}
			break;
		case IN_KEYVALUE:
			if (*buf < 32) {
				*buf = '\0';
				rtrim(msg, buf);
				state = START_KEYNAME;
				parsedMsgCount++;
				if (parsedMsgCount >= maxParsedMsgCount) {
					parsedMsgCount = 0;
				}
			}
			break;
		}
	}
	return parsedMsgCount;
}

/** Reconstruct an XAP packet from the parsed components.
*/
int parsedMsgToRaw(struct parsedMsgElement parsedMsg[], int parsedMsgCount, char *msg, int size) {
	char *currentSection = NULL;
	int i;
	int len = 0;
	msg[len] = '\0';
	for(i=0; i < parsedMsgCount; i++) {
		if (currentSection == NULL || currentSection != parsedMsg[i].section) {
			if(currentSection != NULL) {
				len += snprintf(&msg[len], size, "}\n");
			}
			len += snprintf(&msg[len],size,"%s\n{\n", parsedMsg[i].section);
			currentSection = parsedMsg[i].section;
		}
		len += snprintf(&msg[len],size,"%s=%s\n", parsedMsg[i].key, parsedMsg[i].value);
	}
	len += snprintf(&msg[len],size,"}\n");
	return len;
}

