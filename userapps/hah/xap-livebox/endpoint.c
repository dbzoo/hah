/* $Id$
*/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "appdef.h"
#include "xapdef.h"
#include "reply.h"
#include "debug.h"

#define CMD_IGNORE NULL

static int element_count = 0;
endpoint_t *endpoint_list = NULL;

/* Add an endpoint to the system.
   It will be added to the HEAD of the list.
 */
endpoint_t *add_endpoint(char *name, 
						 char subid, 
						 void (*cmd)(endpoint_t *, char *),
						 void (*info)(endpoint_t *)
	 )
{
	 endpoint_t *elem;

	 in("add_endpoint");
	 element_count++;

	 if(g_debuglevel>2) printf(WHERESTR" %s %d\n", WHEREARG, name, subid);

	 if (endpoint_list == NULL) {
		  endpoint_list = (endpoint_t *)malloc(sizeof(endpoint_t));
		  endpoint_list->next = NULL;
		  elem = endpoint_list;
	 } else {
		  elem = (endpoint_t *)malloc(sizeof(endpoint_t));
		  elem->next = endpoint_list;
	 }
	 elem->subid = subid;
	 elem->name = strdup(name);
	 elem->cmd = cmd;
	 elem->info = info;
	 elem->id = (char *)malloc(EP_ID_SIZE);
	 sprintf(elem->id,"%02X", element_count);
	 elem->state = (char *)malloc(EP_STATE_SIZE);
	 strcpy(elem->state,"?");

	 endpoint_list = elem;
	 out("add_endpoint");
	 return elem;
}

endpoint_t *find_endpoint(char *name) {
	 in("find_endpoint");
	 FOREACH_ENDPOINT(endpoint) {
		  if(strcmp(name, endpoint->name) == 0) {
			   out("find_endpoint");
			   return endpoint;
		  }
	 }
	 out("find_endpoint");
	 return NULL;
}

void add_relay(char *name, int id, int offset) {
	 char buff[20];
	 in("add_relay");
	 snprintf(buff, sizeof buff, "%s.%d", name, id);
	 endpoint_t *relay = add_endpoint(buff, id+offset, 
					  &xap_cmd_relay, &info_binary_output_labeled);
	 strcpy(relay->state,"off");
	 out("add_relay");
}

/* Load all the STATIC endpoints.
   This is hardware that is always active on the PCB.
 */
void load_endpoints() {
	 int i;
	 in("load_endpoints");
	 for(i = 1; i < 5; i++) {
		  add_relay("relay",i,0);
	 }
	 add_endpoint("lcd", 0, &xap_cmd_lcd, &info_lcd);
	 add_endpoint("input.1", 1, CMD_IGNORE, &info_binary_input_labeled);
	 add_endpoint("input.2", 2, CMD_IGNORE, &info_binary_input_labeled);
	 add_endpoint("input.3", 3, CMD_IGNORE, &info_binary_input_labeled);
	 add_endpoint("input.4", 4, CMD_IGNORE, &info_binary_input_labeled);
	 out("load_endpoints");
}
