/* $Id$
  Copyright (c) Brett England, 2009

  No commercial use.
  No redistribution at profit.
  All derivative work must retain this message and
  acknowledge the work of the original author.
 */
#ifdef IDENT
#ident "@(#) $Id$"
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <libxml/parser.h>
#include "xap.h"
#include "bsc.h"
#include "utlist.h"

// Seconds between xapBSC.info messages.
#define INFO_INTERVAL 120

const char *inifile = "/etc/xap.d/xap-currentcost.ini";
char serialPort[20];
int hysteresis; // Whole of house
char *interfaceName = "eth0";

enum {CC128, CLASSIC, ORIGINAL, EDF} model;

enum {ST_NONE, ST_SENSOR, ST_CH1, ST_CH2, ST_CH3, ST_TEMPR, ST_TEMPRF, ST_IMP, ST_TYPE, ST_DATA};
unsigned char state = ST_NONE;

bscEndpoint *currentTag = NULL;
bscEndpoint *endpointList = NULL;

static int infoEventChannel(bscEndpoint *, char *);
static int infoEventTemp(bscEndpoint *, char *);

struct _msg {
  char *tempr;
  char *temprf;
  int sensor;
  int type;
  char *ch1, *ch2, *ch3;
  char *imp;
} msg;

// Hold this against each BSC endpoint as userdata.
typedef struct {
	char type;  // Analog, Digital, Impulse (ADI)
	char *previousValue;
	char *currentValue; // Used for impulse
	int hysteresis;
	char *unit;
} UserData;

/// BSC callback - Only emit info/event for Channels that adjust outside of the hysteresis amount.
static int infoEventChannel(bscEndpoint *e, char *clazz)
{
        info("%s Sensor 0 %s.%s = %s", clazz, e->name, e->subaddr, e->text);
        int old = 0;
	UserData *u = (UserData *)e->userData;
        if(u->previousValue)
		old = atoi(u->previousValue);
        int new = atoi(e->text);
        // Alway report INFO events so we can repond to xAPBSC.query + Timeouts.
        // xapBSC.event are only emitted based on the hystersis
        if(strcmp(clazz, BSC_INFO_CLASS) == 0 || new > old + hysteresis || new < old - hysteresis) {
                if(e->displayText == NULL)
                        e->displayText = (char *)malloc(15);
                snprintf(e->displayText, 15, "%d Watts", new);
                return 1;
        }
	return 0;
}

/// BSC callback - Emit info/event for Temperature endpoints.
static int infoEventTemp(bscEndpoint *e, char *clazz)
{
        info("%s %s = %s", clazz, e->name, e->text);
	UserData *u = (UserData *)e->userData;
        if(strcmp(clazz, BSC_INFO_CLASS) == 0 || u->previousValue == NULL || strcmp(e->text, u->previousValue)) {
                if(e->displayText == NULL)
                        e->displayText = (char *)malloc(15);
                char unit = *(e->name + strlen(e->name) -1) == 'F' ? 'F' : 'C'; // tmpr/tmprF
                snprintf(e->displayText, 15, "Temp %s%c", e->text, unit);

                return 1;
        }
	return 0;
}

/** Load a dynamic key for the [currentcost] section from the INI file.
*
* @param key Dynamic key name
* @param sensor CurrentCost sensor we need to load data for. 1 based index.
* @param location Pointer to area for INI value
* @param size Width of the memory pointer.
*/
static long loadSensorINI(char *key, int sensor, char *location, int size)
{
        char buff[10];
        sprintf(buff,"%s%d",key,sensor);
        long n = ini_gets("currentcost", buff, "", location, size, inifile);
        info("%s=%s", buff, location);
        return n;
}

/// BSC callback - For Sensors (IAMS)
static int sensorInfoEvent(bscEndpoint *e, char *clazz)
{
        info("%s %s.%s", clazz, e->name, e->subaddr);
        int old = 0;
	UserData *u = e->userData;
        if(u->previousValue)
		old = atoi(u->previousValue);
        int new = atoi(e->text);

        // Alway report INFO events so we can repond to xAPBSC.query + Timeouts.
        // xapBSC.event are only emitted based on the hystersis or if they have never been reported.
        if(strcmp(clazz, BSC_INFO_CLASS) == 0 || new > old + u->hysteresis || new < old - u->hysteresis ||
          	e->last_report == 0) {
                if(u->unit) {
                        if(e->displayText == NULL) { // Lazy malloc
                                e->displayText = (char *)malloc(30);
                        }
                        if(e->type == BSC_BINARY) {
                                snprintf(e->displayText, 30, "%s %s", u->unit, bscStateToString(e));
                        } else {
                                snprintf(e->displayText, 30, "%s %s", e->text, u->unit);
                        }
                }
                return 1;
        }
	return 0;
}

// Binary and Analog are straight forward
// we keep the previous endpoint->text value in the userData->previousValue
// For impulse meters we need to keep the DELTA in the endpoint->text
// So the the current MeterReading and the previous MeterReading
// are held in userData->previousValue and userData->impulseValue respectively.
void updateEndpointValue(bscEndpoint *e, char *value) {
	UserData *u = (UserData *)e->userData;

	if(u->type == 'I') { // Impulse (Delta)
		if(u->currentValue == NULL) { // Our first reading.
			u->currentValue = strdup(value);
		} else
		  if(e->text == NULL // Never reported before
		     || strcmp(value, u->currentValue) // or meter value change
		     || (u->previousValue && strcmp(u->currentValue, u->previousValue))) // or 1st ZERO delta
		    {
			if(u->previousValue) free(u->previousValue);
			u->previousValue = u->currentValue;
			u->currentValue = strdup(value); // Save new current value.

			int newDelta = atoi(u->currentValue) - atoi(u->previousValue);
			char delta[16];
			snprintf(delta, sizeof(delta),"%d", newDelta);
			if(e->text) free(e->text); // discard current endpoint value.
			e->text = strdup(delta); // save new delta value.
			bscSetState(e, BSC_STATE_ON);
			bscSendEvent(e);
		}
	} else { // Analogue and Digital
		// If its different report it.
		if(e->text == NULL || strcmp(value, e->text)) {
			// Rotate the text data value through the user data field and free it on update.
			if(u->previousValue) {
				free(u->previousValue);
			}
			u->previousValue = (void *)e->text;
			e->text = strdup(value);

			debug("endpoint type %s", (e->type == BSC_BINARY ? "binary" : "analog"));
			if(e->type == BSC_BINARY) {
				// 0 is off, 500 is ON.  We'll use any value != 0 as ON.
				bscSetState(e, atoi(e->text) ? BSC_STATE_ON : BSC_STATE_OFF);
			} else {
				bscSetState(e, BSC_STATE_ON);
			}
			bscSendEvent(e);
		}
	}
}

// Augment the bscAddEndpoint function so we always create our userdata struct.
bscEndpoint *newBscEndpoint(char *name, char *subaddr, unsigned int type, int (*infoEvent)(bscEndpoint *self, char *clazz), char sensorType) {
	bscEndpoint *e;
	e = bscAddEndpoint(&endpointList, name, subaddr, BSC_INPUT, type, NULL, infoEvent);
	UserData *u = (UserData *)calloc(1,sizeof(UserData));
	e->userData = u;
	u->type = sensorType;

	if(subaddr && isdigit(*subaddr)) {
		int sensorId = atoi(subaddr);
		char i_hysteresis[4];
		long n;
		n = loadSensorINI("hysteresis", sensorId, i_hysteresis, sizeof(i_hysteresis));
		if(n > 0) u->hysteresis = atoi(i_hysteresis);

		char unit[10];
		n = loadSensorINI("unit", sensorId, unit, sizeof(unit));
		if(n > 0) u->unit = strdup(unit);
	}

	bscAddEndpointFilter(e, INFO_INTERVAL);
	return e;
}

/*
 Creation of a sensor should follow this pattern

 sensor.n        Phase 1 - IAM's sensor
 sensor.n.2      Phase 2
 sensor.n.3      Phase 3

 We use the word phase here when talking about ch1/ch2/ch3
*/
static void updateSensor(int sensorId, char *eFormat, char *value, int (*infoEvent)(bscEndpoint *, char *), int uidOffset)
{
	bscEndpoint *e;
        char subaddr[20];
	snprintf(subaddr, sizeof(subaddr), eFormat, sensorId);
	info(subaddr);

        e = bscFindEndpoint(endpointList, "sensor", subaddr);
        if(e == NULL) {
		char stype[2]= {0,0};
		unsigned char uid = (sensorId-1)*4+uidOffset+10;
		info("Adding new sensor uid=%02x",uid);
		// Add to the list we want to search and manage
		bscSetEndpointUID(uid);
		loadSensorINI("type", sensorId, stype, sizeof(stype));
		int bscType = stype[0] == 'D' ? BSC_BINARY : BSC_STREAM; // Analog/Impulse are both BSC_STREAM
		e = newBscEndpoint("sensor", subaddr, bscType, infoEvent, stype[0]);

        }
	updateEndpointValue(e, value);
}

static void freePtr(char **p) {
	if(*p) {
		free(*p);
		*p = NULL;
	}
}

// Duplicate a string removing any leading ZERO's
char *dupZero(char *s) {
	if(s == NULL || *s == '\0') // No string?
		return NULL;

	while(*s && *s == '0')
		s++;

	// End of String?  Must have been ALL zeros
	if(*s == '\0') {
		s--;     // One is ok then.
	}
	return strdup(s);
}

/// SAX element (TAG) callback - EDF currentcost
static void startElementEDFCB(void *ctx, const xmlChar *name, const xmlChar **atts)
{
        debug("<%s>", name);

        if(strncmp("ch",name, 2) == 0) {
          char *ename, *subaddr;
          int (*infoEvent)(bscEndpoint *, char *);

          if(strncmp("chH",name, 3) == 0) { // <chH> is special
            ename = "ch";
            subaddr = "H";
            bscSetEndpointUID(1);
            infoEvent = &infoEventChannel;
          } else {
            ename = "sensor";
            subaddr = (char *)name+2;
            if(*subaddr == '0') subaddr++;
            // Sensors start at UID 0x0A.  <ch01> is first sensor
            bscSetEndpointUID(9 + atoi(subaddr));
            infoEvent = &sensorInfoEvent;
          }

          state = ST_DATA;
          currentTag = bscFindEndpoint(endpointList, ename, subaddr);
          if(currentTag == NULL) {
		  currentTag = newBscEndpoint(ename, subaddr, BSC_STREAM, infoEvent,'A');
          }
        }
}

/// SAX cdata callback - EDF currentcost
static void cdataBlockEDFCB(void *ctx, const xmlChar *ch, int len)
{
        char output[40];
        int i;

        if(state == ST_NONE)
                return;

        // Value to STRZ
        for (i = 0; (i<len) && (i < sizeof(output)-1); i++)
                output[i] = ch[i];
        output[i] = 0;

        debug("%s", output);
        switch(state) {
        case ST_DATA:
                // If its different report it.
                if(currentTag->text == NULL || strcmp(output, currentTag->text)) {
			UserData *u = (UserData *)currentTag->userData;
                        // Rotate the text data value through the user data field and free it on update.
                        if(u->previousValue)
                                free(u->previousValue);
                        u->previousValue = (void *)currentTag->text;
                        currentTag->text = dupZero(output);

                        debug("endpoint type %d", currentTag->type);
			// Q. Can EDF units support binary IAMS ?
                        if(currentTag->type == BSC_BINARY) {
                                // 0 is off, 500 is ON.  We'll use any value != 0 as ON.
                                bscSetState(currentTag, atoi(currentTag->text) ? BSC_STATE_ON : BSC_STATE_OFF);
                        } else {
                                bscSetState(currentTag, BSC_STATE_ON);
                        }
                        bscSendEvent(currentTag);
                }
                break;
        }
        state = ST_NONE;
}

static void endElementEDFCB(void *ctx, const xmlChar *name)
{
       debug("</%s>", name);
}

/// SAX element (TAG) callback
static void startElementCB(void *ctx, const xmlChar *name, const xmlChar **atts)
{
        debug("<%s>", name);
        if(strcmp("sensor", name) == 0) {
                state = ST_SENSOR;
        } else if(strcmp(name,"ch1") == 0) {
                state = ST_CH1;
        } else if(strcmp(name,"ch2") == 0) {
                state = ST_CH2;
        } else if(strcmp(name,"ch3") == 0) {
                state = ST_CH3;
        } else if(strcmp(name,"imp") == 0) {
                state = ST_IMP;
        } else if(strcmp(name,"tmpr") == 0) {
                state = ST_TEMPR;
        } else if(strcmp(name,"tmprF") == 0) {
                state = ST_TEMPRF;
        } else if(strcmp(name,"type") == 0) {
                state = ST_TYPE;
        } else if(strcmp(name,"msg") == 0) {
		freePtr(&msg.ch1);
		freePtr(&msg.ch2);
		freePtr(&msg.ch3);
		freePtr(&msg.tempr);
		freePtr(&msg.temprf);
		freePtr(&msg.imp);
		// No <sensor> tag for these models default to Whole of House.
		if(model == ORIGINAL || model == CLASSIC)
		  msg.sensor = 0;
		else
		  msg.sensor = -1;
	}
}

/// SAX cdata callback - Currentcost
static void cdataBlockCB(void *ctx, const xmlChar *ch, int len)
{
        char output[40];
        int i;

        if(state == ST_NONE)
                return;

        // Value to STRZ
        for (i = 0; (i<len) && (i < sizeof(output)-1); i++)
                output[i] = ch[i];
        output[i] = 0;

        debug("%s", output);

        switch(state) {
        case ST_SENSOR: msg.sensor = atoi(output); break;
        case ST_TYPE: msg.type = atoi(output); break;
        case ST_CH1: msg.ch1 = dupZero(output); break;
        case ST_CH2: msg.ch2 = dupZero(output); break;
        case ST_CH3: msg.ch3 = dupZero(output); break;
        case ST_IMP: msg.imp = dupZero(output); break;
        case ST_TEMPR: msg.tempr = strdup(output); break;
        case ST_TEMPRF: msg.temprf = strdup(output); break;
        }
        state = ST_NONE;
}

static void updateWholeOfHouseEndpoint(char *addr, char *subaddr, char *value, int (*infoEvent)(bscEndpoint *, char *), int uidOffset) {
	bscEndpoint *e;
	static bscEndpoint *ccTotal = NULL; // Endpoint to sum the Wattage over all Channels

	e = bscFindEndpoint(endpointList, addr, subaddr);
	if(e == NULL) {
		// Add to the list we want to search and manage
		bscSetEndpointUID(uidOffset);
		e = newBscEndpoint(addr, subaddr, BSC_STREAM, infoEvent,'A');
	}
	updateEndpointValue(e, value);

	// Was this a new channel value?  Update the PSEDUO channel: ch.total
	if(strcmp("ch", addr) == 0) {
		// Sum all channels for ch.total
		int total = 0;
		int channelCount = 0;
		bscEndpoint *cce;
		LL_FOREACH(endpointList, cce) {
			if(strcmp("ch", cce->name) == 0 && strcmp("total", cce->subaddr)) {
				debug("ch.total adding %s.%s value %s", cce->name, cce->subaddr, cce->text);
				total += atoi(cce->text);
				channelCount ++;
			}
		}

		// Defer construction of this endpoint until we no there are more then 3 phases.
		if(channelCount > 1) {

			if(ccTotal == NULL) {
				// ch.total = ch.1 + ch.2 + ch.3
				bscSetEndpointUID(5);
				ccTotal = newBscEndpoint("ch", "total", BSC_STREAM, &infoEventChannel,'A');
				bscSetState(ccTotal, BSC_STATE_ON);
			}

			// roll previous total into userData for hystersis calculations.
			UserData *u = (UserData *)ccTotal->userData;
			if(u->previousValue)
				free(u->previousValue);
			u->previousValue = (void *)ccTotal->text;

			char totalText[10];
			snprintf(totalText, sizeof(totalText),"%d", total);
			ccTotal->text = strdup(totalText);

			bscSendEvent(ccTotal);
		}
	}
}

static void endElementCB(void *ctx, const xmlChar *name)
{
	debug("</%s>", name);
	if(strcmp("msg", name)) return;

	if(msg.sensor == 0) { // Whole of house
		if(msg.ch1) updateWholeOfHouseEndpoint("ch","1", msg.ch1, infoEventChannel, 1);
		if(msg.ch2) updateWholeOfHouseEndpoint("ch","2", msg.ch2, infoEventChannel, 2);
		if(msg.ch3) updateWholeOfHouseEndpoint("ch","3", msg.ch3, infoEventChannel, 3);
		if(msg.tempr) updateWholeOfHouseEndpoint("temp", NULL, msg.tempr, infoEventTemp, 4);
		if(msg.temprf) updateWholeOfHouseEndpoint("tempF", NULL, msg.temprf, infoEventTemp, 4);
	} else if(msg.sensor > 0) {
		if(msg.ch1) updateSensor(msg.sensor, "%d", msg.ch1, sensorInfoEvent, 1);
		if(msg.ch2) updateSensor(msg.sensor, "%d.2", msg.ch2, sensorInfoEvent, 2);
		if(msg.ch3) updateSensor(msg.sensor, "%d.3", msg.ch3, sensorInfoEvent, 3);

		// Impulse meters and IAMS are mutally exclusive.
		// That is [ ch1/ch2/ch3 and imp ] so we can reuse the endpoint of ch1.
		if(msg.imp) updateSensor(msg.sensor, "%d", msg.imp, NULL, 1);

		// Sensors don't have separate temperature readings
		// but the CC unit will include the WHOLE OF HOUSE temp
		// reading as part of a sensor payload.
		if(msg.tempr) updateWholeOfHouseEndpoint("temp", NULL, msg.tempr, infoEventTemp, 4);
		if(msg.temprf) updateWholeOfHouseEndpoint("tempF", NULL, msg.temprf, infoEventTemp, 4);
	}
}

/// Parse an Currentcost XML message.
void parseXml(char *data, int size)
{
        xmlSAXHandler handler;

        info("%s", data);

        memset(&handler, 0, sizeof(handler));
        handler.initialized = XML_SAX2_MAGIC;
	if(model == EDF) {
	  handler.startElement = startElementEDFCB;
	  handler.characters = cdataBlockEDFCB;
	  handler.endElement = endElementEDFCB;
	} else {
	  handler.startElement = startElementCB;
	  handler.characters = cdataBlockCB;
	  handler.endElement = endElementCB;
	}

        state = ST_NONE;

        if(xmlSAXUserParseMemory(&handler, NULL, data, size) < 0) {
                err("Document not parsed successfully.");
                return;
        }
}

///. Read the XML stream from the currentcost unit.
void serialInputHandler(int fd, void *data)
{
        char serial_buff[256];
        static char serial_xml[4096]= {0};
        static int serial_cursor = 0;
        int i;
        int len;
	char *ch;
	static int bstate = 0, estate = 0;

        debug("(fd:%d)(len:%d)(xml:%s)", fd, serial_cursor, serial_xml);
        while((len = read(fd, serial_buff, sizeof(serial_buff)-1)) > 0) {
		if(getLoglevel() == LOG_DEBUG) {
			for(i=0;i<len;i++)
				putchar(serial_buff[i]);
			putchar('\n');
		}
                for(ch = &serial_buff[0], i=0; i < len; i++, ch++) {
                        if(isspace(*ch))
                                continue;
                        // Prevent buffer overruns.
                        if(serial_cursor == sizeof(serial_xml)-1) {
                                serial_cursor = 0;
                        }
                        serial_xml[serial_cursor++] = *ch;
                        serial_xml[serial_cursor] = 0;

			if (bstate == 0 && *ch == '<') bstate = 1;
			else if (bstate == 1 && *ch == 'm') bstate = 2;
			else if (bstate == 2 && *ch == 's') bstate = 3;
			else if (bstate == 3 && *ch == 'g') bstate = 4;
			else if (bstate == 4 && *ch == '>' && serial_cursor > 5) {
			  strcpy(serial_xml,"<msg>");
			  serial_cursor = 5;
			  bstate = 0;
			}
			else
			  bstate = 0;

			if (estate == 0 && *ch == '<') estate = 1;
			else if (estate == 1 && *ch == '/') estate = 2;
			else if (estate == 2 && *ch == 'm') estate = 3;
			else if (estate == 3 && *ch == 's') estate = 4;
			else if (estate == 4 && *ch == 'g') estate = 5;
			else if (estate == 5 && *ch == '>') {
			  parseXml(serial_xml, serial_cursor);
			  serial_cursor = 0;
			  serial_xml[0] = 0;
			  estate = 0;
                        } else
			  estate = 0;
		}
        }
}

/// Setup the serial port.
int setupSerialPort()
{
  int fd;
  if(strncmp(serialPort,"/dev/",5) == 0) {
          struct termios newtio;
	  fd = open(serialPort, O_RDONLY | O_NDELAY);
	  if (fd < 0) {
	    die_strerror("Failed to open serial port %s", serialPort);
	  }
	  if(flock(fd, LOCK_EX | LOCK_NB) == -1) {
	    close(fd);
	    die_strerror("Serial port %s in use", serialPort);
	  }
	  cfmakeraw(&newtio);
	  switch(model) {
	  case CC128:
	  case EDF:
	    newtio.c_cflag = B57600 | CS8 | CLOCAL | CREAD ;
	    break;
	  case ORIGINAL:
	    newtio.c_cflag = B2400 | CS8 | CLOCAL | CREAD ;
	    break;
	  default:
                newtio.c_cflag = B9600 | CS8 | CLOCAL | CREAD ;
	  }
	  newtio.c_iflag = IGNPAR;
	  newtio.c_lflag = ~ICANON;
	  newtio.c_cc[VTIME] = 0; // ignore timer
	  newtio.c_cc[VMIN] = 0; // no blocking read
	  tcflush(fd, TCIFLUSH);
	  tcsetattr(fd, TCSANOW, &newtio);
  } else {
	  fd = open(serialPort, O_RDONLY);
	  if (fd < 0) {
	    die_strerror("Failed to open file %s", serialPort);
	  }
  }
  return fd;
}

/// Display usage information and exit.
static void usage(char *prog)
{
        printf("%s: [options]\n",prog);
        printf("  -i, --interface IF     Default %s\n", interfaceName);
        printf("  -s, --serial DEV       Default %s\n", serialPort);
        printf("  -m, --model [CLASSIC|CC128|ORIGINAL|EDF]  Default CLASSIC\n");
        printf("  -d, --debug            0-7\n");
        printf("  -h, --help\n");
        exit(1);
}

/// Process the INI file for xAP control data and setup XAP.
void setupXap()
{
        char defaultPort[20];
        strcpy(defaultPort, serialPort);

        xapInitFromINI("currentcost","dbzoo","CurrentCost","00DC",interfaceName,inifile);

        hysteresis = ini_getl("currentcost", "hysteresis", 10, inifile);
        ini_gets("currentcost","usbserial",defaultPort, serialPort, sizeof(serialPort), inifile);

        char model_s[20];
        model = CLASSIC;
        ini_gets("currentcost","model","classic", model_s, sizeof(model_s), inifile);
        if(strcasecmp(model_s,"CC128") == 0) {
                info("Selecting CC128 model");
                model = CC128;
        } else if(strcasecmp(model_s,"ORIGINAL") == 0) {
                info("Selecting ORIGINAL model");
                model = ORIGINAL;
	} else if(strcasecmp(model_s,"CC128") == 0) {
                info("Selecting CC128 model");
                model = CC128;
        } else if(strcasecmp(model_s,"EDF") == 0) {
                info("Selecting EDF model");
                model = EDF;
        } else {
                info("Selecting CLASSIC model");
        }
}

/// Setup Endpoints, the Serial port, a callback for the serial port and process xAP messages.
int main(int argc, char *argv[])
{
        int i;
        printf("\nCurrent Cost Connector for xAP v12\n");
        printf("Copyright (C) DBzoo, 2009-2012\n\n");
        strcpy(serialPort,"/dev/ttyUSB0");

        for(i=0; i<argc; i++) {
                if(strcmp("-i", argv[i]) == 0 || strcmp("--interface",argv[i]) == 0) {
                        interfaceName = argv[++i];
                } else if(strcmp("-s", argv[i]) == 0 || strcmp("--serial", argv[i]) == 0) {
                        strlcpy(serialPort, argv[++i], sizeof(serialPort));
                } else if(strcmp("-d", argv[i]) == 0 || strcmp("--debug", argv[i]) == 0) {
                        setLoglevel(atoi(argv[++i]));
                } else if(strcmp("-h", argv[i]) == 0 || strcmp("--help", argv[i]) == 0) {
                        usage(argv[0]);
                }
        }

        setupXap();

        // The model switch if provided overrides the INI setting.
        for(i=0; i<argc; i++) {
                if(strcmp("-m", argv[i]) == 0 || strcmp("--model",argv[i]) == 0) {
		        i++;
                        if(strcasecmp("CC128", argv[i]) == 0) {
                                info("Command line override selecting CC128 model");
                                model = CC128;
			} else if(strcasecmp("EDF", argv[i]) == 0) {
                                info("Command line override selecting EDF model");
                                model = EDF;
                        } else if(strcasecmp("ORIGINAL", argv[i]) == 0) {
	                        info("Command line override selecting ORIGINAL model");
	                        model = ORIGINAL;
                        } else if(strcasecmp("CLASSIC", argv[i]) == 0) {
	                        info("Command line override selecting CLASSIC model");
	                        model = CLASSIC;
                        }
                }
        }

	if(strncmp(serialPort,"/dev/",5) == 0) {
	  xapAddSocketListener(setupSerialPort(), &serialInputHandler, endpointList);
	  xapProcess();
	} else {
	  serialInputHandler(setupSerialPort(), NULL);
	}
	return 0;
}
