/* Compliant Interface for Livebox AVR board

   $Author: brett $ : $RCSfile: endpoint.h,v $
   $Source: /home/brett/xap/xap-livebox/RCS/endpoint.h,v $
   $Revision: 1.2 $, $Date: 2009/02/03 16:35:19 $
*/
#ifndef ENDPOINT_H
#define ENDPOINT_H

#include <time.h>

#define EP_STATE_SIZE 10
#define EP_ID_SIZE    3

typedef struct _endpoint {
	char *id;
	char *name;
	char subid;
	char *state; // 5 bytes of something.
	time_t event_time;
	void (*cmd )(struct _endpoint *self, char *section);
	void (*info)(struct _endpoint *self);
	void (*event)(struct _endpoint *self);
	struct _endpoint *next;
} endpoint_t;

extern endpoint_t *endpoint_list;        // Head of endpoint list

endpoint_t *find_endpoint(char *name);
endpoint_t *add_endpoint(char *name, 
			 char subid, 
			 void (*cmd)(endpoint_t *, char *),
			 void (*info)(endpoint_t *),
			 void (*event)(endpoint_t *));
void load_endpoints();
void add_relay(char *name, int id, int offset);

#define FOREACH_ENDPOINT(x) endpoint_t *x; for(x=endpoint_list;x; x=x->next)

#endif

