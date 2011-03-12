/* $Id$
 
INI file processing
*/
#ifdef IDENT
#ident "@(#) $Id$"
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include "bsc.h"
#include "ini.h"
#include "minIni.h"
#include "serial.h"

// *** DO NOT CHANGE UNLESS YOU ALIGN WITH THE AVR FIRMWARE ***
// Applicable for Firmware Version 1 only.
#define MAXCHANNEL 16

const char *inifile = "/etc/xap-livebox.ini";

struct unassignedROMID *unassignedROMIDList = NULL;

/** Generate an event/info and lookup the displaytext
*
* @param e Endpoint to generate the xap message for
* @param clazz either xapBSC.info or xapBSC.event
* @param prefix [INI] file key processing prefix.
* @param value String to include in the display texts
*/
static int infoEventLabeled(bscEndpoint *e, char *clazz, char *prefix, char *value)
{
        if(e->displayText == NULL)
                e->displayText = (char *)malloc(64);

        // Configure the displayText optional argument.
        char key[30], buf[64];
        snprintf(key, sizeof(key), "%s%s.label", prefix, e->subaddr);
        if(ini_gets(e->name, key, "", buf, sizeof(buf), inifile) > 0) {
                snprintf(e->displayText, 64, "%s %s", buf, value);
        } else {
                *e->displayText = '\0';
        }
        // do the default.
        return 1;
}
/** Augment default xapBSC.info & xapBSC.event handler.
* Adds displaytext for Binary endpoints.
*
* @param e Endpoint to generate the xap message for
* @param clazz either the string "xapBSC.info" or "xapBSC.event"
*/
int infoEventBinary(bscEndpoint *e, char *clazz)
{
        // Locate an INI label of the form, where X is the endpoint name.
        // [X]
        // X1.label=
        return infoEventLabeled(e, clazz, e->name, bscStateToString(e));
}

/** Augment default xapBSC.info & xapBSC.event handler.
*
* @param e Endpoint to generate the xap message for
* @param clazz either the string "xapBSC.info" or "xapBSC.event"
*/
static int infoEvent1wire (bscEndpoint *e, char *clazz)
{
        // [1wire]
        // sensor1.label=
        infoEventLabeled(e, clazz, "sensor", e->text);
}

/** Handle xapBSC.cmd for the RF endpoints (AVR rev=1)
*
* @param e BSC endpoint requiring processing
*/
static void cmdRF(bscEndpoint *e)
{
        char buf[30];
        int i = atoi(e->subaddr); // RF 1 - is channel 5 on the firmware.
        snprintf(buf, sizeof(buf), "%s %d", bscStateToString(e), i+4);
        serialSend(buf);
}

/** Handle xapBSC.cmd for the RF endpoints (AVR rev>1)
 *  Universal RF.
 *
 * @param e BSC endpoint requiring processing
 */
static void cmdURF(bscEndpoint *e)
{
        char relay_key[10];
        char rf[256];
	char serial_cmd[256];
	long n;

	// From the key RF1.ON find the [rf] section control sequence.
	snprintf(relay_key,sizeof(relay_key),"rf%s.%s", e->subaddr, bscStateToString(e));
	n = ini_gets("rf", relay_key, "", rf, sizeof(rf), inifile);
	if(n) {	
		info("%s = %s", relay_key, rf);
		strcpy(serial_cmd,"urf ");
		strlcat(serial_cmd, rf, sizeof(serial_cmd));
		
		serialSend(serial_cmd);
	} else {
		notice("section [rf] key %s not found", relay_key);
                bscSetState(e, BSC_STATE_UNKNOWN);		
	}
}

/** Check each 1wire device has transmitted from the AVR within the timeout period.
 * If timed out mark in an UNKOWN state and adjust its current value to be also unknown.
 * Timeout registered with xapAddTimeoutAction()
 * 
 * @param interval Frequency of the 1wire check
 * @param data Unused.
 */
void timeoutCheck1wire(int interval, void *data)
{
        bscEndpoint *e = (bscEndpoint *)data;
	struct tempSensor *userData = (struct tempSensor *)e->userData;
        time_t now = time(NULL);
        if(now > userData->lastSerialEvent + interval) {
                bscSetState(e, BSC_STATE_UNKNOWN);
                bscSetText(e, "?");
        }
}

static void timeoutReport1wire(int interval, void *data)
{
	// Ask AVR firmware to report current endpoints states	
	if(firmwareMajor() == 1 && firmwareMinor() == 0)
		serialSend("report");
	else
		serialSend("report 1wire");
}

/** Drive an I2C PPE in PIN mode for a targeted xAPBSC.cmd
*
* output.state.1
* {
* state=on
* }
*/
static void cmdPPEpin(bscEndpoint *e)
{
        char serialCmd[32];
        // Subaddr is of the form: section.pin
        char *pin = strrchr(e->subaddr,'.')+1;
	int addr = ((struct ppeEndpoint *)e->userData)->i2cAddr;

        // We invert the STATE.  Logical ON is a LOW PPE state 0.
        snprintf(serialCmd, sizeof(serialCmd),"i2c P%02X%s%d", addr, pin, e->state == BSC_STATE_ON ? BSC_STATE_OFF : BSC_STATE_ON);
        serialSend(serialCmd);
}

/** Drive an I2C PPE in BYTE mode for a targeted xAPBSC.cmd
*
* output.state.1
* {
* text=4F
* }
*/
static void cmdPPEbyte(bscEndpoint *e)
{
        char serialCmd[32];
        if(isxdigit(e->text[0]) && isxdigit(e->text[1]) && strlen(e->text) == 2) {
		int addr = ((struct ppeEndpoint *)e->userData)->i2cAddr;
                snprintf(serialCmd, sizeof(serialCmd),"i2c B%02X%s", addr, e->text);
                serialSend(serialCmd);
        } else {
                warning("CMD must be 2 hex digits: supplied %s", e->text);
        }
}

/** Register each I2C address so that it will be monitored.  This
*   inserts the address into the firmware's internal array list
*   periodically each i2c device will be polled for a change in state.
*/
static void setup_i2c_ppe(int addr)
{
        char arg[10];
        snprintf(arg, sizeof(arg), "i2c M%02X", addr);
        serialSend(arg);
}

/** Send an instruction to reset the number of PPE device's registered.
*   We do this as the xap-adapter could be stopped and started with the
*  .ini file being modified between invocations and we don't want to
*   register the same i2c address more than once.
*/
static void reset_i2c_ppe()
{
        serialSend("i2c R");
}

#define IMMEDIATE 1
#define DELAYED 0
static int addOneWireIniEndpoints(int mode) 
{
	long n;
	int i;
        char buff[17];
	const int maxDevices = firmwareMajor() > 1 ? 31 : 15;
	char romid[19];

	n = ini_getl("1wire", "devices", -1, inifile);
	if (n > maxDevices) n = maxDevices;

	bscSetEndpointUID(128);
	long t = ini_getl("1wire", "timeout", -1, inifile);
	for(i=1; i<=n; i++) {
		info("Adding 1wire.%d", i);

		if(firmwareMajor() > 1) { // Need a ROMID
			snprintf(buff,sizeof(buff),"sensor%d.romid", i);
			info("Looking for %s", buff);
			if(ini_gets("1wire", buff, "", romid, sizeof(romid), inifile) != 18) {
				warning("sensor%d.romid : Missing or invalid ROMID", i);
				continue;
			}
		}

		info("Creating endpoint");
		snprintf(buff,sizeof(buff),"%d", i);
		bscEndpoint *e = bscAddEndpoint(&endpointList, "1wire", buff, BSC_INPUT, BSC_STREAM, NULL, &infoEvent1wire);

		struct tempSensor *sensor = (struct tempSensor *)malloc(sizeof(struct tempSensor));
		sensor->lastSerialEvent = 0;
		if(firmwareMajor() > 1) strcpy(sensor->romid, romid);
		e->userData = sensor;

		if(t>0) xapAddTimeoutAction(&timeoutCheck1wire, t*60, (void *)e);
		if(mode == IMMEDIATE) {
			bscAddEndpointFilter(e, 120); // info interval
		}
	}
	// If a 1wire device has a constant reading for the timeout period
	// the timeoutCheck1wire will trigger it to '?' as the timeout code
	// was to detect the 1wire device not on the bus we will force an
	// AVR report of all devices 10 sec before the timeout period.
	if(t>0) xapAddTimeoutAction(&timeoutReport1wire, t*60-10, NULL);

	return n;
}

// Delete endpoints and reload the .INI file 
void resetOneWireEndpoints() 
{
	// Clear all the assigned ROMID's
	if(firmwareMajor() > 1) {
		struct unassignedROMID *e, *tmp;
		LL_FOREACH_SAFE(unassignedROMIDList, e, tmp) {
			LL_DELETE(unassignedROMIDList, e);
		}
	}

	// Reset the one wire bus to discover new/deleted 1-wire devices.
	serialSend("1wirereset");

	// Delete all existing 1wire endpoints.
        bscEndpoint *e, *tmp;
	LL_FOREACH_SAFE(endpointList, e, tmp) {
                if(strcmp(e->name, "1wire") == 0) {
			LL_DELETE(endpointList, e);

			xAPTimeoutCallback *cb = xapFindTimeoutByUserData(e);
			if(cb) {
				info("Deleting timeout action");
				xapDelTimeoutAction(cb);
			}

			struct tempSensor *sensor = (struct tempSensor *)bscDelEndpoint(e);
			free(sensor);
                }
        }

	info("Remove the 1wire timeout check (if found)");
	xapDelTimeoutActionByFunc(&timeoutReport1wire);

	info("Setup new 1-wire endpoints based on the .INI File");
	if(addOneWireIniEndpoints(IMMEDIATE) > 0) {
		// Make the ALL 1wire devices report in now.
		timeoutReport1wire(0, NULL);
	}
}

/* Add I2C/PPE endpoints for the INI file [ppeX] section.
 */
static void addPPEendpoints(char *section) {
        char s_addr[5];
        int addr;
        char mode[5];
	long n;
        char buff[30];

	int section_number = atoi(section+3); // The digit is its endpoint: ppe.1
	if(section_number < 1 || section_number > 8) {
		err("%s: Section out of range [1-8]", section);
		return;
	}
	n = ini_gets(section, "address", "", s_addr, sizeof(s_addr), inifile);
	if (n == 0) {
		err("%s: Missing address=[0x40-0x7E]", section);
		return;
	}
	// PCF8574  - 0 1 0 0 A2 A1 A0 0 - 0x40 to 0x4E
	// PCF8574A - 0 1 1 1 A2 A1 A0 0 - 0x70 to 0x7E
	sscanf(s_addr,"%x", &addr);
	// Range check and only EVEN addresses should be used.
	if(addr < 0x40 || addr > 0x7e || addr % 2 == 1) {
		err("%s: Invalid address %s", section, s_addr);
		return;
	}
	n = ini_gets(section, "mode", "", mode, sizeof(mode), inifile);
	if(n == 0) {
		err("%s: Missing mode=[byte|pin]", section);
		return;
	}			
	bscSetEndpointUID((section_number-1)*8+32); // UID range 32-95
	
	struct ppeEndpoint *ud = (struct ppeEndpoint *)malloc(sizeof(struct ppeEndpoint));
	ud->section = section_number;
	ud->i2cAddr = addr;

	if(strcasecmp(mode,"byte") == 0) {
		snprintf(buff,sizeof buff,"%d", section_number);
		bscAddEndpoint(&endpointList, "ppe", buff, BSC_OUTPUT, BSC_STREAM, &cmdPPEbyte, NULL)->userData = ud;
		setup_i2c_ppe(addr);
	} else if(strcasecmp(mode,"pin") == 0) {
		int pin;
		for(pin=0; pin<8; pin++) {
			snprintf(buff,sizeof buff,"%d.%d", section_number, pin);
			bscAddEndpoint(&endpointList, "ppe", buff, BSC_OUTPUT, BSC_BINARY, &cmdPPEpin, NULL)->userData = ud;
		}
		setup_i2c_ppe(addr);
	} else {
		free(ud);
		err("%s: Invalid mode: %s", section, mode);
		return;
	}		
}

/** Parse the .ini file and dynamically create XAP endpoints.
 */
void addIniEndpoints()
{
        char section[30];
        long n;
        int s, i;
        char buff[30];

        reset_i2c_ppe();
        for (s = 0; ini_getsection(s, section, sizeof(section), inifile) > 0; s++) {
                info("section: %s", section);

                /* Handle section: [ppeX]:
                   We actually don't care what appears after the characters
                   ppe as long as its unique to distinguish one entry from
                   another.
                */
                if(strncmp("ppe", section, 3) == 0) {
			addPPEendpoints(section);
                }  // Handle section: [1wire]
                else if(strcmp("1wire", section) == 0) {
			addOneWireIniEndpoints(DELAYED);
                } else if(firmwareMajor() == 1 && strcmp("rf",section) == 0) {
			// Single RF transmitter
                        int j;
                        char rf[20];
                        char serialrf[40];
                        int devices;
                        int waitms; // eeprom write delay (milliseconds)

                        devices = ini_getl(section, "devices", -1, inifile);
                        waitms = ini_getl(section, "eedelay", 100, inifile);
                        if (devices > MAXCHANNEL-4)
                                n = MAXCHANNEL-4;
                        bscSetEndpointUID(160);
                        for(i = 1; i <= devices; i++) {
                                snprintf(buff,sizeof buff,"%d", i);
                                bscAddEndpoint(&endpointList, "rf", buff, BSC_OUTPUT, BSC_BINARY, &cmdRF, &infoEventBinary);

                                // Look for RF configuration for this relay.
                                for(j=0; j<2; j++) {
                                        snprintf(buff,sizeof buff,"rf%d.%s",i,j==0?"off":"on");
                                        n = ini_gets(section, buff, "", rf, sizeof(rf), inifile);
                                        info("%s = (%d) %s", buff, n, rf);

                                        if(n) {
                                                // Internally we start at CHANNEL 5 so offset I by 4.
                                                snprintf(serialrf,sizeof serialrf,"rf %02X%d%s",i+4,j,rf);
                                                serialSend(serialrf);
                                                usleep(waitms*1000); // Wait to allow the eeprom to write 6 bytes.
                                        }
                                }
                        }
                } else if(firmwareMajor() > 1 && strcmp("rf",section) == 0) {
			// Universal RF transmitter
                        char buff[3];
                        int devices;

                        devices = ini_getl(section, "devices", -1, inifile);
                        if (devices > 96) {
				n = 96; // See livebox.c for range of UID mapping.
				notice("Maximum RF devices reached");
			}
                        bscSetEndpointUID(160);
                        for(i = 1; i <= devices; i++) {
                                snprintf(buff,sizeof buff,"%d", i);
                                bscAddEndpoint(&endpointList, "rf", buff, BSC_OUTPUT, BSC_BINARY, &cmdURF, &infoEventBinary);
                        }
                }                

        }
}
