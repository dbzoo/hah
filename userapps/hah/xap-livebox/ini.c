/* $Id$

   INI file processing
*/
#ifdef IDENT
#ident "@(#) $Id$"
#endif

#include <stdio.h>
#include <string.h>
#include "appdef.h"
#include "xapdef.h"
#include "ini.h"
#include "minIni.h"
#include "endpoint.h"
#include "reply.h"
#include "debug.h"

// *** DO NOT CHANGE UNLESS YOU ALIGN WITH THE FIRMWARE ***
#define MAXCHANNEL 16     

#define sizearray(a)  (sizeof(a) / sizeof((a)[0]))

static void iniError(char *section, char *err) {
	 if(g_debuglevel > 0) {
		  printf("Section %s: %s\n", section, err);
	 }
}

static int duplicateAddr(int addr) {
	 FOREACH_ENDPOINT(x) {
		  if(x->subid == addr) return 1;
	 }
	 return 0;
}

/* Register each I2C address so that it will be monitored.  This
   inserts the address into the firmware's internal array list
   periodically each i2c device will be polled for a change in state.
 */
static void setup_i2c_ppe(int addr) {
	 char arg[5];
	 snprintf(arg, sizeof(arg), "M%02X", addr);
	 serial_cmd_msg("i2c", arg);
}

/* Send an instruction to reset the number of PPE device's registered.
   We do this as the xap-adapter could be stopped and started with the
   .ini file being modified between invocations and we don't want to 
   register the same i2c address more than once.
*/
static void reset_i2c_ppe() {
	 serial_cmd_msg("i2c", "R");
}

/* Parse the .ini file and dynamically create XAP endpoints.
 */
void parseini() {
	 char section[30];
	 char s_addr[5];
	 int addr;
	 char mode[5];
	 long n;
	 int s;
	 char buff[30];

	 in("parseini");

	 reset_i2c_ppe();
	 for (s = 0; ini_getsection(s, section, sizearray(section), inifile) > 0; s++) {
		  if(g_debuglevel > 1) {
			   dumpme(section,"%s\n");
		  }

		  /* Handle section: [ppeX]: 
			We actually don't care what appears after the characters
		    ppe as long as its unique to distinguish one entry from
		    another.
		  */

		  if(strncmp("ppe", section, 3) == 0) {
			   n = ini_gets(section, "address", "", s_addr, sizearray(s_addr), inifile);
			   if (n == 0) {
					iniError(section,"Missing address=[0x40-0x47]");
					continue;
			   }
			   sscanf(s_addr,"%x", &addr);
			   if(addr < 0x40 || addr > 0x47) {
					iniError(section,"Invalid address");
					continue;
			   }
			   if(duplicateAddr(addr)) {
					iniError(section,"Duplicate address");
					continue;
			   }

			   n = ini_gets(section, "mode", "", mode, sizearray(mode), inifile);
			   if(n == 0) {
					iniError(section,"Missing mode=[byte|pin]");
					continue;
			   }
			   if(strcmp(mode,"byte") == 0) {
					snprintf(buff,sizeof buff,"i2c.%02X", addr); 
					add_endpoint(buff, addr, &xap_cmd_ppe_byte, &info_level_output);
					setup_i2c_ppe(addr);
			   } else if(strcmp(mode,"pin") == 0) {
					int pin;
					for(pin=0; pin<7; pin++) {
						 snprintf(buff,sizeof buff,"i2c.%02X.%d", addr, pin);
						 add_endpoint(buff, addr, &xap_cmd_ppe_pin, &info_binary_output);
					}
					setup_i2c_ppe(addr);
			   } 
			   else {
					iniError(section,"Invalid mode value");
					continue;
			   }
		  }  // Handle section: [1wire]
		  else if(strcmp("1wire", section) == 0) {
			   n = ini_getl(section, "devices", -1, inifile);
			   if (n > 15) n = 15;
			   while(n > 0) {
					snprintf(buff,sizeof buff,"1wire.%d", n);
					add_endpoint(buff, n, NULL, &info_level_input);					
					n--;
			   }
		  } 
		  else if(strcmp("rf",section) == 0) {
			   int i,j;
			   char rf[20];
			   char serialrf[30];
			   int devices;

			   devices = ini_getl(section, "devices", -1, inifile);
			   if (devices > MAXCHANNEL-4) n = MAXCHANNEL-4;
			   for(i = 1; i <= devices; i++) {
					add_relay("rf", i, 4);

					// Look for RF configuration for this relay.
					for(j=0; j<2; j++) {
						 snprintf(buff,sizeof buff,"rf%d.%s",i,j==0?"off":"on");
						 n = ini_gets(section, buff, "", rf, sizearray(rf), inifile);
						 if(g_debuglevel>1) { 
							  printf("%s = (%d) %s\n", buff, n, rf);
						 }
						 if(n) {
							  // Internally we start at CHANNEL 5 so offset I by 4.
							  snprintf(serialrf,sizeof serialrf,"%02X%d%s",i+4,j,rf);
							  serial_cmd_msg("rf", serialrf);
							  usleep(50*1000); // Wait 50ms to allow the eeprom to write 6 bytes.
						 }
					}
			   }
		  }
	 }
	 out("parseini");
}


void setupXAPini() {
	 char uid[5];
	 char guid[9];
	 long n;

	 // default to 00DB if not present
	 n = ini_gets("xap","uid","00DB",uid,sizearray(uid),inifile);

	 // validate that the UID can be read as HEX
	 if(n == 0 || !(isxdigit(uid[0]) && isxdigit(uid[1]) && 
					isxdigit(uid[2]) && isxdigit(uid[3]))) 
	 {
		  strlcpy(uid,"00DB",sizeof uid);
	 }
	 snprintf(guid,sizeof guid,"FF%s00", uid);
	 XAP_GUID = strdup(guid);

	 char control[30];
	 n = ini_gets("xap","instance","Controller",control,sizearray(control),inifile);
	 if(n == 0 || strlen(control) == 0) 
	 {
		  strlcpy(control,"Controller",sizeof control);
	 }
	 XAP_DEFAULT_INSTANCE = strdup(control);
}
