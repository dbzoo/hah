/* $Id$
   Copyright (c) Brett England, 2010
 
   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "xap.h"
#include "bsc.h"

static char *io_str[] = {"input","output"};
static char *state_str[] = {"off","on","?","?"};

/// Decode allowable values for a "state=value" key pair.
int bscDecodeState(char *msg)
{
        int i;
        static char *value[] = {
                                       "on","off","true","false","yes","no","1","0","toggle"
                               };
        static int state[] = {
                                     1,0,1,0,1,0,1,0,2
                             };
        if (msg == NULL)
                return -1;
        for(i=0; i < sizeof(value); i++) {
                if(strcasecmp(msg, value[i]) == 0)
                        return state[i];
        }
        return -1;
}

/// Fully qualified endpoint name.
static char *bscFQEN(char *target, bscEndpoint *e)
{
        char addr[512];
        strlcpy(addr, target, sizeof addr);
        strlcat(addr, ":", sizeof addr);
        strlcat(addr, e->name, sizeof addr);
        if(e->subaddr) {
                strlcat(addr, ".", sizeof addr);
                strlcat(addr, e->subaddr, sizeof addr);
        }
        return strdup(addr);
}

/// Set the value of a BSC STREAM device type
void bscSetText(bscEndpoint *e, char *text)
{
        if(e->text)
                free(e->text);
        e->text = strdup(text);
	info("text=%s", e->text);
}

/// Set the value of a BSC LEVEL device type.
inline void bscSetLevel(bscEndpoint *e, char *level)
{
        bscSetText(e, level);
}

void bscSendCmdEvent(bscEndpoint *e) {
	if(*e->cmd) (*e->cmd)(e);
	if(*e->infoEvent) (*e->infoEvent)(e, "xapBSC.event");
}

/// Set the value of a BSC BINARY device type.
inline void bscSetState(bscEndpoint *e, int state)
{
        e->state = state & 0x3;
	info("state=%d", e->state);
}

/// Return a string representation of the state
inline char *bscStateToString(bscEndpoint *e)
{
        return state_str[e->state];
}

/// Return a string representation of the IO type
inline char *bscIOToString(bscEndpoint *e)
{
        return io_str[e->io];
}

/// Locate an Endpoint given its name and optional sub-address.
bscEndpoint *bscFindEndpoint(bscEndpoint *head, char *name, char *subaddr)
{
	debug("name %s subaddr %s", subaddr);
        bscEndpoint *e;
	LL_FOREACH(head, e) {
                if(strcmp(e->name, name) == 0 &&
                                ((e->subaddr == NULL && subaddr == NULL) ||
                                 (e->subaddr && subaddr && strcmp(e->subaddr, subaddr) == 0))) {
	                debug("Found ", e->source);
                        return e;
                }
        }
        return NULL;
}

/** Parse BSC LEVEL device type value.
 *
 * An XAP BSC level message may be of the form.
 *     level = <value>/<range>  i.e: 256/512 = 50
 */
int bscParseLevel(char *str)
{
        int level;
        int range;
        if(index(str,'/') == 0) {
	        warning("level=%s", str);
                return 0;
        }
        sscanf(str,"%d/%d", &level, &range);
        return range / level;
}

/** Handle an incoming xapBSC.cmd class control message
 * directed at an endpoint or a set of endpoints.
 */
static void bscIncomingCmd(void *data)
{
        bscEndpoint *e = (bscEndpoint *)data;
	info("xapBSC.cmd detected for %s", e->source);
	
        // Can't control an INPUT device!
	if(e->io == BSC_INPUT) {
		warning("INPUT devices can't be controlled");
                return;
	}
        // Walk each multi block section updating the ENDPOINT if the section ID allows it.
        char section[16];
        int i;
        for(i=1; i<99; i++) {
                sprintf(section, "output.state.%d", i);
                // State/ID are mandatory items for ALL BSC commands
                char *state = xapGetValue(section, "state");
                char *id = xapGetValue(section, "id");
                if(state == NULL || id == NULL)
                        break;  // malformed section.

                // Match the ENDPOINT ID (UID sub-address) and process.
                if(*id == '*' || strcmp(e->id, id) == 0) {
                        int istate = bscDecodeState(state);
                        if(istate == -1)
                                continue; // invalid state
                        if(istate == 2) // toggle
                                istate = e->state == BSC_STATE_ON ? BSC_STATE_OFF : BSC_STATE_ON;
                        e->state = istate;

                        if(e->type == BSC_LEVEL) {
                                bscSetLevel(e, xapGetValue(section, "level"));
                        } else if(e->type == BSC_STREAM) {
                                bscSetText(e, xapGetValue(section, "text"));
                        }

                        // Perform Endpoint action.
	                bscSendCmdEvent(e);
                }
        }
}

/// Send an xapBSC.info message for this endpoint.
static void bscIncomingQuery(void *data)
{
        bscEndpoint *e = (bscEndpoint *)data;
	info("xapBSC.query detected for %s", e->source);	
	if(e->infoEvent) {
                e->last_report = time(NULL);
                (*e->infoEvent)(e, "xapBSC.info");
        }
}

/** Send XAP INFO message for this endpoint (TIMEOUT).
*
* To reduce traffic if the device is sending EVENTS then we postpone
* the regular INFO message until the INTERVAL time has elasped and no
* xapBSC.query or xapBSC.event has been seen.
*/
static void bscInfoTimeout(int interval, void *data)
{
        bscEndpoint *e = (bscEndpoint *)data;
        time_t now = time(NULL);
        if(e->infoEvent && (e->last_report = 0 || e->last_report + interval < now )) {
	        info("Timeout for %s", e->source);
	        e->last_report = now;
                (*e->infoEvent)(e, "xapBSC.info");
        }
}

//////////////////////////////////////////////////////////////////////////////////////////////

void bscInfoEvent(bscEndpoint *e, char *clazz)
{
        int len;

        char buff[XAP_DATA_LEN];
        len = snprintf(buff, XAP_DATA_LEN, "xap-header\n"
                       "{\n"
                       "v=12\n"
                       "hop=1\n"
                       "uid=%s\n"
                       "class=%s\n"
                       "source=%s\n"
                       "}\n"
                       "%s.state\n"
                       "{\n"
                       "state=%s\n",
                       e->uid, clazz, e->source, bscIOToString(e), bscStateToString(e));

        if(e->type == BSC_LEVEL) {
                len += snprintf(&buff[len], XAP_DATA_LEN-len, "level=%s\n", e->level);
        } else if(e->type == BSC_STREAM) {
                len += snprintf(&buff[len], XAP_DATA_LEN-len, "text=%s\n", e->level);
        }
        if(e->displayText && *e->displayText) {
                len += snprintf(&buff[len], XAP_DATA_LEN-len, "displaytext=%s\n", e->displayText);
        }
        len += snprintf(&buff[len], XAP_DATA_LEN-len, "}\n");

        if(len > XAP_DATA_LEN)
                err("xAP message buffer truncated - not sending\n");
        else
                xapSend(buff);
}

/// Create a BSC endpoint these are use for automation control.
bscEndpoint *bscAddEndpoint(bscEndpoint **head, char *name, char *subaddr, unsigned int dir, unsigned int typ,
                    void (*cmd)(struct _bscEndpoint *self),
                    void (*infoEvent)(struct _bscEndpoint *self, char *clazz)
                   )
{
	static int id = 1;
	char s_id[3];
	die_if(name == NULL, "Name is mandatory");
        bscEndpoint *e = (bscEndpoint *)calloc(1, sizeof(bscEndpoint));

	e->name = strdup(name);
        if(subaddr)
                e->subaddr = strdup(subaddr);

	// Each endpoint added gets unique a UID id
	sprintf(s_id,"%02X", ++id);
        e->id = strdup(s_id);

        e->io = dir;
        e->type = typ;
	// Is this logic too restrictive for a library?
        if(e->io == BSC_INPUT)
                e->state = BSC_STATE_UNKNOWN;
	else // BSC_OUTPUT
		e->state = typ == BSC_BINARY ? BSC_STATE_OFF : BSC_STATE_ON;
        e->cmd = cmd;
        // If the user hasn't supplied handlers use the defaults.
        e->infoEvent = infoEvent == NULL ? &bscInfoEvent : infoEvent;

	// LEVEL & STREAM initialize to unknown
	if(e->type != BSC_BINARY) { 
		e->text = strdup("?");
	}

	LL_PREPEND(*head, e);
	return e;
}

/// Add a linked-list of BSC endpoints
void bscAddEndpointFilterList(bscEndpoint *head, int info_interval) {
	notice_if(head == NULL, "No endpoints to add!?");
	while(head) {
		bscAddEndpointFilter(head, info_interval);
		head = head->next;
	}
}

/// Add CMD and QUERY filter callbacks for a BSC endpoint
void bscAddEndpointFilter(bscEndpoint *head, int info_interval)
{
	xAPFilter *filter;

	// UID of the endpoint is the xAP UID with the latest 2 digits replaced with the endpoint ID.
	head->uid = strdup(gXAP->uid);
	strncpy(&head->uid[6], head->id, 2);
	
	// Compute ENDPOINTS source name.
	head->source = bscFQEN(gXAP->source, head);
	info("source=%s", head->source);
	
	if(head->io == BSC_OUTPUT) {
		filter = NULL;
		xapAddFilter(&filter, "xap-header","class","xapBSC.cmd");
		xapAddFilter(&filter, "xap-header","target", head->source);
		xapAddFilterAction(&bscIncomingCmd, filter, head);
	}
	
	filter = NULL;
	xapAddFilter(&filter, "xap-header","class","xapBSC.query");
	xapAddFilter(&filter, "xap-header","target", head->source);
	xapAddFilterAction(&bscIncomingQuery, filter, head);
	
	if(info_interval > 0) {
		xapAddTimeoutAction(&bscInfoTimeout, info_interval, head);
	}
}
