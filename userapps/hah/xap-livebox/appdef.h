/* $Id: appdef.h 33 2009-11-10 09:56:39Z brett $
*/
#ifndef APPDEF_H
#define APPDEF_H

#include "endpoint.h"

extern char g_lcd_text[];

void xap_cmd_relay(endpoint_t *self, char *section);
void xap_cmd_lcd(endpoint_t *self, char *section);
void xap_cmd_ppe_pin(endpoint_t *self, char *section);
void xap_cmd_ppe_byte(endpoint_t *self, char *section);
int serial_cmd_msg(const char *cmd, const char *arg);

#endif
