/* $Id$
   Copyright (c) Brett England, 2010

   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.  
 */
#include <stdio.h>
#include <string.h>
#include "log.h"
#include "xively.h"
#include "pachulib.h"
#include "mem.h"

static const char *eeml_head = "<eeml xmlns=\"http://www.eeml.org/xsd/005\""
        "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\""
        "xsi:schemaLocation=\"http://www.eeml.org/xsd/005 http://www.eeml.org/xsd/005/005.xsd\" version=\"5\">"
        "<environment>";
static const char *eeml_foot ="</environment></eeml>";

/* Send a pre-formated XML datastream set to pachube */
int pach_updateDatastreamXml(int feed, char *api, char *xml) {
     if(xml == NULL) return;

     info("feed %d", feed);

     char *eeml = mem_malloc(strlen(eeml_head) + strlen(eeml_foot) + strlen(xml) + 1, M_NONE);
     strcpy(eeml, eeml_head);
     strcat(eeml, xml);
     strcat(eeml, eeml_foot);

     int ret = update_environment(feed, api, eeml, XML);
     info("return %d", ret);
     mem_free(eeml);

     return ret;

}

