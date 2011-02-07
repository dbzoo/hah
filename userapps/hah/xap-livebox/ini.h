/* $Id$
*/

#ifndef INI_H
#define INI_H
#include "bsc.h"

extern bscEndpoint *endpointList;
extern const char *inifile;

void addIniEndpoints();
int infoEventBinary(bscEndpoint *e, char *clazz);
void resetOneWireEndpoints();

struct tempSensor {
	time_t lastSerialEvent;
	char romid[19]; // 64bit 1wire address
};

struct unassignedROMID {
	char romid[19];
	char temperature[6];
	struct unassignedROMID *next;
};

extern struct unassignedROMID *unassignedROMIDList;

#endif
