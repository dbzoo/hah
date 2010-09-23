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
#include "bsc.h"
#include "ini.h"
#include "minIni.h"
#include "serial.h"

// *** DO NOT CHANGE UNLESS YOU ALIGN WITH THE AVR FIRMWARE ***
#define MAXCHANNEL 16

const char inifile[] = "/etc/xap-livebox.ini";

/** Generate an event/info and lookup the displaytext
*
* @param e Endpoint to generate the xap message for
* @param clazz either xapBSC.info or xapBSC.event
* @param prefix [INI] file key processing prefix.
*/
static void infoEventLabeled(bscEndpoint *e, char *clazz, char *prefix)
{
        if(e->displayText == NULL)
                e->displayText = (char *)malloc(64);

        // Configure the displayText optional argument.
        char key[30], buf[64];
        snprintf(key, sizeof(key), "%s%s.label", prefix, e->subaddr);
        if(ini_gets(e->name, key, "", buf, sizeof(buf), inifile) > 0) {
                snprintf(e->displayText, 64, "%s %s", buf, e->text);
        } else {
                *e->displayText = '\0';
        }
        // do the default.
        bscInfoEvent(e, clazz);
}
/** Augment default xapBSC.info & xapBSC.event handler.
* Adds displaytext for RF and Relay endpoints.
*
* @param e Endpoint to generate the xap message for
* @param clazz either the string "xapBSC.info" or "xapBSC.event"
*/
void infoEventRFnRelay(bscEndpoint *e, char *clazz)
{
        // Locate a label of the form.
        // [rf]
        // rf1.label=
        // [relay]
        // relay2.label=
        infoEventLabeled(e, clazz, e->name);
}

/** Augment default xapBSC.info & xapBSC.event handler.
*
* @param e Endpoint to generate the xap message for
* @param clazz either the string "xapBSC.info" or "xapBSC.event"
*/
static void infoEvent1wire (bscEndpoint *e, char *clazz)
{
        // [1wire]
        // sensor1.label=
        infoEventLabeled(e, clazz, "sensor");
}

/** Handle xapBSC.cmd for the RF endpoints.
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


/** Check each 1wire device has transmitted from the AVR within the timeout period.
 * If timed out mark in an UNKOWN state and adjust its current value to be also unknown.
 * Timeout registered with xapAddTimeoutAction()
 * 
 * @param interval Frequency of the 1wire check
 * @param data Unused.
 */
void timeoutCheck1wire(int interval, void *data)
{
        long t = ini_getl("1wire", "timeout", -1, inifile);
        if (t < 1)
                return;
        t *= 60; // minutes to seconds

        time_t now = time(NULL);
        bscEndpoint *e;
        LL_FOREACH(endpointList, e) {
                if(strcmp("1wire", e->name) == 0) {
                        if(now > *(time_t *)e->userData + t) {
                                setbscState(e, STATE_UNKNOWN);
                                setbscText(e, "?");
                        }
                }
        }

}

/** Parse the .ini file and dynamically create XAP endpoints.
 */
void addIniEndpoints()
{
        char section[30];
        long n;
        int s;
        char buff[30];

        for (s = 0; ini_getsection(s, section, sizeof(section), inifile) > 0; s++) {
                info("section: %s");

                /* Handle section: [ppeX]:
                   We actually don't care what appears after the characters
                   ppe as long as its unique to distinguish one entry from
                   another.
                */

                if(strcmp("1wire", section) == 0) {
                        n = ini_getl(section, "devices", -1, inifile);
                        if (n > 15)
                                n = 15;
                        if(n) {
                                // Every minute check the 1wire bus for timeouts.
                                xapAddTimeoutAction(&timeoutCheck1wire, 60, NULL);
                        }
                        while(n > 0) {
                                snprintf(buff,sizeof buff,"%d", (int)n);
                                bscEndpoint *e = bscAddEndpoint(&endpointList, "1wire", buff, BSC_INPUT, BSC_STREAM, NULL, &infoEvent1wire);
                                // extra data to hold last 1wire event serial time.
                                e->userData = (void *)malloc(sizeof(time_t));
                                n--;
                        }
                } else if(strcmp("rf",section) == 0) {
                        int i,j;
                        char rf[20];
                        char serialrf[40];
                        int devices;
                        int waitms; // eeprom write delay (milliseconds)

                        devices = ini_getl(section, "devices", -1, inifile);
                        waitms = ini_getl(section, "eedelay", 100, inifile);
                        if (devices > MAXCHANNEL-4)
                                n = MAXCHANNEL-4;
                        for(i = 1; i <= devices; i++) {
                                snprintf(buff,sizeof buff,"%d", i);
                                bscAddEndpoint(&endpointList, "rf", buff, BSC_OUTPUT, BSC_BINARY, &cmdRF, &infoEventRFnRelay);

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
                }
        }
}
