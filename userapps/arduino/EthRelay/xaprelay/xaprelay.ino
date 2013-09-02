/* Ethernet enable relays using xAP
 * Uses the dbzoo EthRelay v1.0 board.
*/
#include <EtherCard.h>
#include "XapEther.h"
#include "BscRelay.h"

/* CONNECTIONS:
 ETHERNET MODULE       ARDUINO BOARD
 PIN 1  (CLK OUT)      N/A
 PIN 2  (INT)          N/A
 PIN 3  (WOL)          N/A
 PIN 4  (SO)           DIO 12
 PIN 5  (SI)           DIO 11
 PIN 6  (SCK)          DIO 13
 PIN 7  (CS)           DIO 10
 PIN 8  (RES)          N/A
 PIN 9  (VCC)          +3.3V
 PIN 10 (GND)          GND

 RELAYS:
  1                    DIO 2              
  2                    DIO 3
 */
#define CS_PIN  10   // By default EtherCard uses 8
#define SERIAL_BAUD 57600
#define SERIAL_DEBUG 0

// ethernet interface mac address
static uint8_t mymac[6] = { 0xde, 0xad, 0xbe, 0xef, 0x00, 0x24};  
byte Ethernet::buffer[500];
BufferFiller bfill;

// Control pins for the relays
#define RELAY1_DIO 2
#define RELAY2_DIO 3

XapEther xap("dbzoo.ethrelay.unit","FFBEEF00");
BscRelay relay1(xap, 1, RELAY1_DIO);
BscRelay relay2(xap, 2, RELAY2_DIO);

static int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

void setup() {
#if SERIAL_DEBUG   
  Serial.begin(SERIAL_BAUD);
reset:  
  Serial.println("[xAP Ethernet relay]");
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
#else
  // Until we initialize - there is nothing we can do.
  // Would be nice to flash a led on error here.
  while(ether.begin(sizeof Ethernet::buffer, mymac, CS_PIN) == 0);
  while(!ether.dhcpSetup());
#endif

  ether.enableBroadcast();
  xap.heartbeat(); // We are alive.
}

// When a xAP frame has been decoded.
void xapCallback() {
  relay1.process();
  relay2.process();
}

void loop() {
  word len = ether.packetReceive();
  ether.packetLoop(len);  // handle ICMP  
  // Send heartbeat every XAP_HEARTBEAT and call us back if there is a xap packet to process.
  xap.process(len, xapCallback );
  // Send a xapBSC.info message every INFO_HEARTBEAT if no xapBSC.event has been sent.
  relay1.heartbeat();
  relay2.heartbeat();
}

