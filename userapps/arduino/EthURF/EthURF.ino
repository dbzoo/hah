/*
Ethernet enabled Universal RF using xAP
Allows you to place an Ethernet enabled Arduino on your network as an xAP compliant
device that will accept the rf.xmit URF schema.
 
Sample: http://www.dbzoo.com/livebox/universalrf#lidl_night_light
 
xap-header
{
v=12
hop=1
uid=FF00EE00
class=rf.xmit
source=dbzoo.fake.source
target=dbzoo.ethurf.unit
}
rf
{
data=0101024407C6024401CC140116F80D3638
} 
*/
#include <EtherCard.h>
#include "UniversalRF.h"
#include "xAPEther.h"

/* CONNECTIONS:
 ETHERNET MODULE       ARDUINO BOARD
 PIN 1  (CLK OUT)      N/A
 PIN 2  (INT)          N/A
 PIN 3  (WOL)          N/A
 PIN 4  (MISO)         DIO 12
 PIN 5  (MOSI)         DIO 11
 PIN 6  (SCK)          DIO 13
 PIN 7  (CS)           DIO 10
 PIN 8  (RES)          N/A
 PIN 9  (VCC)          +3.3V
 PIN 10 (GND)          GND
 
 RF MODULE
 TX                    PIN 7
 VCC                   +5v
 GND                   GND
 */

#define CS_PIN 10
#define RFTX_PIN  7
#define SERIAL_BAUD 57600
#define SERIAL_DEBUG 1

// ethernet interface mac address
static uint8_t mymac[6] = { 
  0xde, 0xad, 0xbe, 0xef, 0x00, 0x25};  
byte Ethernet::buffer[700];
BufferFiller bfill;

XapEther xap("dbzoo.ethurf.unit","FFCEEF00");
UniversalRF RF(RFTX_PIN);

static int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

void setup() {
#if SERIAL_DEBUG   
  Serial.begin(SERIAL_BAUD);
reset:  
  Serial.println("[xAP Ethernet URF]");
  if (ether.begin(sizeof Ethernet::buffer, mymac, CS_PIN) == 0) {
    Serial.println( "Failed to access Ethernet controller");
    goto reset;
  }
  if (!ether.dhcpSetup()) {
    Serial.println("DHCP failed");
    goto reset;
  }
  ether.printIp("My IP: ", ether.myip);
  ether.printIp("Netmask: ", ether.mymask);
  ether.printIp("GW IP: ", ether.gwip);
  ether.printIp("DNS IP: ", ether.dnsip);
  Serial.print("Free RAM: ");
  Serial.println(freeRam());
  RF.setDebug(true);
#else
  // Until we initialize - there is nothing we can do.
  // Would be nice to flash a led on error here.
  while(ether.begin(sizeof Ethernet::buffer, mymac, CS_PIN) == 0);
  while(!ether.dhcpSetup());
#endif

  ether.enableBroadcast();
  xap.heartbeat(); // We are alive.
}

boolean filter(char *section, char *k, char *kv) {
  char *value = xap.getValue(section, k);
  if (value == NULL) { // no section key match must fail.
    return 0;
  } 
  return strcasecmp(value, kv) == 0 ? 1 : 0;  
}

void sendOK() {
  bfill = ether.buffer + UDP_DATA_P;
  bfill.emit_p(PSTR("xap-header\n"
    "{\n"
    "v=12\n"
    "hop=1\n"
    "uid=$S\n"
    "class=rf.sent\n"
    "source=$S\n"
    "}\n"
    "empty\n"
    "{\n"
    "}\n"
    ), xap.UID, xap.SOURCE);
  xap.broadcastUDP(bfill.position());  
}

// When a xAP frame has been decoded.
void xapCallback() {
  if(filter("xap-header","target", (char *)xap.SOURCE) == 0) return;
  if(filter("xap-header","class", "rf.xmit") == 0) return;
  char *data = xap.getValue("rf","data");
  if(data == NULL) return;
  RF.transmit(data);
  sendOK();  
}

void loop() {
  word len = ether.packetReceive();
  ether.packetLoop(len);  // handle ICMP  
  // Send heartbeat every XAP_HEARTBEAT and call us back if there is a xap packet to process.
  xap.process(len, xapCallback );
}



