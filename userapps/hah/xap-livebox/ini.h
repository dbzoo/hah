#ifndef INI_H
#define INI_H
#include "bsc.h"

extern bscEndpoint *endpointList;
extern const char *inifile;

void addIniEndpoints();
int infoEventBinary(bscEndpoint *e, char *clazz);
void resetOneWireEndpoints();
void resetOneWireEndpoints();
char *getSerialPort();

struct tempSensor {
	time_t lastSerialEvent;
	char romid[19]; // 64bit 1wire address
        float hysteresis;
        float offset;
        char *label;
        char prevValue[6]; // -55.0 to 127.0
};

struct unassignedROMID {
	char romid[19];
	char temperature[6];
	struct unassignedROMID *next;
};

struct ppeEndpoint {
	unsigned char i2cAddr;
	unsigned char section;
};

extern struct unassignedROMID *unassignedROMIDList;

#endif
