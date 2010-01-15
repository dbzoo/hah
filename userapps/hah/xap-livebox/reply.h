/* $Id: reply.h 33 2009-11-10 09:56:39Z brett $
*/
#include "appdef.h"

void info_binary_input(endpoint_t *self);
void info_binary_output(endpoint_t *self);
void info_level_input(endpoint_t *self);
void info_level_output(endpoint_t *self);
void info_stream_output(endpoint_t *self);

void event_binary_input(endpoint_t *self);
void event_level_input(endpoint_t *self);
