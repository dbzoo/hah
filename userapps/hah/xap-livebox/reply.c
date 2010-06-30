/* $Id$
  */

#include "xapdef.h"
#include "appdef.h"
#include "debug.h"
#include "ini.h"
#include <stdio.h>
#include <string.h>

#ifdef IDENT
#ident "@(#) $Id$"
#endif

static void xap_message(endpoint_t *self, char *msgtype, char *io_type, char *body) {
	char i_xapmsg[1500];
	char endpoint[8];
	int len;

	strlcpy(endpoint, g_uid, sizeof endpoint);
	sprintf(&endpoint[6], "%s", self->id);  // create sub address
	len = snprintf(i_xapmsg, sizeof(i_xapmsg),
		       "xAP-header\n{\nv=12\nhop=1\nuid=%s\n"		\
		       "class=xAPBSC.%s\nsource=%s.%s.%s:%s\n}"	\
		       "\n%s.state\n{\n%s\n}\n",
		       endpoint, msgtype, XAP_ME, XAP_SOURCE, g_instance, self->name, io_type, body);

	// If truncation of the buffer occurs don't send the message treat as malformed.
	if( len > sizeof(i_xapmsg)) {
		// What do we do with this?
		cmd_lcd("err 01: reply.c");
	} else {
		xap_send_message(i_xapmsg);
	}
}

static void bsc_text(char *type, endpoint_t *self, char *io_type, char *text, char *displaytext) {
	in("bsc_text");
	char body[100] = "state=on\ntext=";
	strlcat(body, text, sizeof(body));
	if(displaytext && *displaytext) {
		strlcat(body, "\ndisplaytext=", sizeof(body));
		strlcat(body, displaytext, sizeof(body));
		strlcat(body, " ", sizeof(body));
		strlcat(body, text, sizeof(body));
	}
	xap_message(self, type, io_type, body);
	out("bsc_text");
}

static void bsc_1wire_text(char *type, endpoint_t *self) {
	in("bsc_1wire_text");
	char buf[20], key[30];
	snprintf(key, sizeof(key), "sensor%d.label", self->subid);
	ini_gets("1wire", key, "", buf, sizeof(buf), inifile);
	bsc_text(type, self, "input", self->state, buf);
	out("bsc_1wire_text");
}

static void bsc_state(char *type, endpoint_t *self, char *io_type) {
	in("bsc_state");
	char body[64] = "state=";
	strlcat(body, self->state, sizeof(body));
	xap_message(self, type, io_type, body);
	out("bsc_state");
}

static void bsc_level(char *type, endpoint_t *self, char *io_type) {
	in("bsc_level");
	char body[64] = "state=on\nlevel=";
	strlcat(body, self->state, sizeof(body));
	xap_message(self, type, io_type, body);
	out("bsc_level");
}

/** BSC Events **/
inline void event_1wire(endpoint_t *self) {
	bsc_1wire_text("event", self);
}

inline void event_binary_input(endpoint_t *self) {
	bsc_state("event", self, "input");
}

inline void event_binary_output(endpoint_t *self) {
	bsc_state("event", self, "output");
}

inline void event_level_input(endpoint_t *self) {
	bsc_level("event", self, "input");
}

inline void event_lcd(endpoint_t *self) {
	bsc_text("event", self, "input", g_lcd_text, NULL);
}

/** BSC Info **/
inline void info_1wire(endpoint_t *self) {
	bsc_1wire_text("info", self);
}

inline void info_level_output(endpoint_t *self) {
	bsc_level("info", self, "output");
}

inline void info_binary_input(endpoint_t *self) {
	bsc_state("info", self, "input");
}

inline void info_binary_output(endpoint_t *self) {
	bsc_state("info", self, "output");
}

inline void info_lcd(endpoint_t *self) {
	bsc_text("info", self, "input", g_lcd_text, NULL);
}
