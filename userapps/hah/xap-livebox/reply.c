/* $Id: reply.c 33 2009-11-10 09:56:39Z brett $
*/

#include "xapdef.h"
#include "appdef.h"
#include "debug.h"
#include <stdio.h>
#include <string.h>

#ifdef IDENT
#ident "@(#) $Id: reply.c 33 2009-11-10 09:56:39Z brett $"
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

static inline void event(endpoint_t *self, char *io_type, char *body) {
	 xap_message(self, "event", io_type, body);
}

static void event_binary(endpoint_t *self, char *io_type) {
	 char body[6+EP_STATE_SIZE] = "state=";
	 strlcat(body, self->state, sizeof(body));
	 event(self, io_type, body);
}

static void event_level(endpoint_t *self, char *io_type) {
	 char body[6+EP_STATE_SIZE] = "level=";
	 strlcat(body, self->state, sizeof(body));
	 event(self, io_type, body);
}

inline void event_binary_input(endpoint_t *self) {
	 event_binary(self, "input");
}

inline void event_binary_output(endpoint_t *self) {
	 event_binary(self, "output");
}

inline void event_level_input(endpoint_t *self) {
	 event_binary(self, "input");
}

static inline void info(endpoint_t *self, char *io_type, char *body) {
	 xap_message(self, "info", io_type, body);
}

static void info_binary(endpoint_t *self, char *io_type) {
	 char body[6+EP_STATE_SIZE] = "state=";
	 strlcat(body, self->state, sizeof(body));
	 info(self, io_type, body);
}

static void info_level(endpoint_t *self, char *io_type) {
	 char body[6+EP_STATE_SIZE] = "level=";
	 strlcat(body, self->state, sizeof(body));
	 info(self, io_type, body);
}

inline void info_level_input(endpoint_t *self) {
	 info_level(self,"input");
}

inline void info_level_output(endpoint_t *self) {
	 info_level(self,"output");
}

inline void info_binary_input(endpoint_t *self) {
	 info_binary(self,"input");
}

inline void info_binary_output(endpoint_t *self) {
	 info_binary(self,"output");
}

void info_stream_output(endpoint_t *self) {
	 char body[48] = "state=on\ntext=";
	 strlcat(body, g_lcd_text, sizeof(body));
	 info(self, "output", body);
}
