/*
Ethernet enabled IR Unit that emits an xAP packet when an IR signal is received.
Allows you to place an Ethernet enabled Arduino on your network as an xAP compliant
device that will convert IR remote signal into xAP packets to control other devices.

The xAP packet is modelled loosely around
http://www.xapautomation.org/index.php?title=IR_Schema

xap-header
{
v=12
hop=1
uid=FFDEEF00
class=ir.receive
source=dbzoo.ethurf.unit
}
ir.signal
{
type=[unknown|nec|sony|rc5|rc6|panasonic|jvc]
value=0x[HEX]
}
*/
#include <EtherCard.h>
#include <IRremote.h>
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
 
 IR Reciever           DIO 7
 */

#define CS_PIN 10
#define SERIAL_BAUD 9600
#define SERIAL_DEBUG 0

// ethernet interface mac address
static uint8_t mymac[6] = { 
  0xde, 0xad, 0xbe, 0xef, 0x00, 0x30};  
byte Ethernet::buffer[500];
BufferFiller bfill;

XapEther xap("dbzoo.ethir.unit","FFDEEF00");
int RECV_PIN = 7;
IRrecv irrecv(RECV_PIN);
decode_results results;

static int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

void setup() {
#if SERIAL_DEBUG   
  Serial.begin(SERIAL_BAUD);
reset:  
  Serial.println("[xAP Ethernet IR]");
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
  irrecv.enableIRIn(); // Start the receiver
}

boolean filter(char *section, char *k, char *kv) {
  char *value = xap.getValue(section, k);
  if (value == NULL) { // no section key match must fail.
    return 0;
  } 
  return strcasecmp(value, kv) == 0 ? 1 : 0;  
}

const char *getDecodeType(decode_results *results) {
  if (results->decode_type == UNKNOWN) {
    return "UNKNOWN";
  } 
  else if (results->decode_type == NEC) {
    return "NEC";
  } 
  else if (results->decode_type == SONY) {
    return "SONY";
  } 
  else if (results->decode_type == RC5) {
    return "RC5";
  } 
  else if (results->decode_type == RC6) {
    return "RC6";
  }
  else if (results->decode_type == PANASONIC) {
    return "PANASONIC";
  }
  else if (results->decode_type == JVC) {
    return "JVC";
  } 
  return "?";
}

char * decToHex(unsigned int decValue) {
  static char buf[sizeof(int)+1];
  itoa(decValue, buf, 16);
  char *p = &buf[0];
  while(*p) {
    *p = toupper(*p);
    p++;
  }
  return buf;
}

void xapSend(decode_results *results) {
  //String asHex = String(results->value, HEX);
  bfill = ether.buffer + UDP_DATA_P;
  bfill.emit_p(PSTR("xap-header\n"
    "{\n"
    "v=12\n"
    "hop=1\n"
    "uid=$S\n"
    "class=ir.receive\n"
    "source=$S\n"
    "}\n"
    "rf.signal\n"
    "{\n"
    "type=$S\n"
    "value=0x$S\n"    
    "}\n"
    ), xap.UID, xap.SOURCE, getDecodeType(results), decToHex(results->value));
  xap.broadcastUDP(bfill.position());  
}

unsigned long last = millis();
void loop() {
  word len = ether.packetReceive();
  ether.packetLoop(len);  // handle ICMP  
  
  if (irrecv.decode(&results)) {
    // Defeat repeating signals.
    // If it's been at least 1/4 second since the last IR received.
    if (millis() - last > 250) {
      xapSend(&results);
    }
    last = millis();      
    irrecv.resume(); // Receive the next value
  }

  // We don't process any inbound xAP packets, just to heartbeat stuff.
  xap.process(len, 0);
}



