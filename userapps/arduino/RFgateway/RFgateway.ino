// Slurp RF packets out of the air and xmit them as a pseudo xap-serial device onto the ethernet.
#include <JeeLib.h>
#include <util/crc16.h>
#include <util/parity.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <EtherCard.h>
#include "XapEther.h"

#define SERIAL_BAUD 57600

// ethernet interface mac address
static byte mymac[] = { 
  0x74,0x69,0x69,0x2D,0x30,0x31 };
byte Ethernet::buffer[700];
static long timer;
static BufferFiller bfill;  // used as cursor while filling the buffer

XapEther xap("dbzoo.nanode.gateway","FFABCD00");

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// RF12 configuration setup code

typedef struct {
  byte nodeId;
  byte group;
  char msg[RF12_EEPROM_SIZE-4];
  word crc;
} 
RF12Config;

static RF12Config config;

static char cmd;
static byte value, stack[RF12_MAXDATA], top, sendLen, dest;

static void addCh (char* msg, char c) {
  byte n = strlen(msg);
  msg[n] = c;
}

static void addInt (char* msg, word v) {
  if (v >= 10)
    addInt(msg, v / 10);
  addCh(msg, '0' + v % 10);
}

static void saveConfig () {
  // set up a nice config string to be shown on startup
  memset(config.msg, 0, sizeof config.msg);
  strcpy(config.msg, " ");

  byte id = config.nodeId & 0x1F;
  addCh(config.msg, '@' + id);
  strcat(config.msg, " i");
  addInt(config.msg, id);    
  strcat(config.msg, " g");
  addInt(config.msg, config.group);

  strcat(config.msg, " @ ");
  static word bands[4] = { 
    315, 433, 868, 915   };
  word band = config.nodeId >> 6;
  addInt(config.msg, bands[band]);
  strcat(config.msg, " MHz ");

  config.crc = ~0;
  for (byte i = 0; i < sizeof config - 2; ++i)
    config.crc = _crc16_update(config.crc, ((byte*) &config)[i]);

  // save to EEPROM
  for (byte i = 0; i < sizeof config; ++i) {
    byte b = ((byte*) &config)[i];
    eeprom_write_byte(RF12_EEPROM_ADDR + i, b);
  }

  if (!rf12_config())
    Serial.println("config save failed");
}

const char helpText1[] PROGMEM = 
"\n"
"Available commands:" "\n"
"  <nn> i     - set node ID (standard node ids are 1..26)" "\n"
"               (or enter an uppercase 'A'..'Z' to set id)" "\n"
"  <n> b      - set MHz band (4 = 433, 8 = 868, 9 = 915)" "\n"
"  <nnn> g    - set network group (RFM12 only allows 212, 0 = any)" "\n"
;

static void showString (PGM_P s) {
  for (;;) {
    char c = pgm_read_byte(s++);
    if (c == 0)
      break;
    if (c == '\n')
      Serial.print('\r');
    Serial.print(c);
  }
}

static void showHelp () {
  showString(helpText1);
  Serial.println("Current configuration:");
  rf12_config();
}

static void handleInput (char c) {
  if ('0' <= c && c <= '9')
    value = 10 * value + c - '0';
  else if (c == ',') {
    if (top < sizeof stack)
      stack[top++] = value;
    value = 0;
  } 
  else if ('a' <= c && c <='z') {
    Serial.print("> ");
    Serial.print((int) value);
    Serial.println(c);
    switch (c) {
    default:
      showHelp();
      break;
    case 'i': // set node id
      config.nodeId = (config.nodeId & 0xE0) + (value & 0x1F);
      saveConfig();
      break;
    case 'b': // set band: 4 = 433, 8 = 868, 9 = 915
      value = value == 8 ? RF12_868MHZ :
      value == 9 ? RF12_915MHZ : RF12_433MHZ;
      config.nodeId = (value << 6) + (config.nodeId & 0x3F);
      saveConfig();
      break;
    case 'g': // set network group
      config.group = value;
      saveConfig();
      break;
    }
    value = top = 0;
    memset(stack, 0, sizeof stack);
  } 
  else if ('A' <= c && c <= 'Z') {
    config.nodeId = (config.nodeId & 0xE0) + (c & 0x1F);
    saveConfig();
  } 
  else if (c > ' ')
    showHelp();
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  Serial.println("\n[RF12gw.1]");
  if (rf12_config(0)) {
    config.nodeId = eeprom_read_byte(RF12_EEPROM_ADDR);
    config.group = eeprom_read_byte(RF12_EEPROM_ADDR + 1);
  } 
  else {    
    config.nodeId = 0x98; // 866 MHz, node 24
    config.group = 0xD4;  // default group 212
    saveConfig();
  }
  if (ether.begin(sizeof Ethernet::buffer, mymac) == 0) 
    Serial.println( "Failed to access Ethernet controller");

  if (!ether.dhcpSetup())
    Serial.println("DHCP failed");

  ether.printIp("My IP: ", ether.myip);
  ether.printIp("Netmask: ", ether.mymask);
  ether.printIp("GW IP: ", ether.gwip);
  ether.printIp("DNS IP: ", ether.dnsip);

  showHelp();
  ether.enableBroadcast();
  xap.heartbeat();
}

// When a xAP frame has been decoded.
void xapCallback() {
  xap.dumpParsedMsg();
}

// Relay the RF message as a Class=Serial.Comms message (fake being a xap-serial gateway).
void xapSerial() {
  bfill = ether.buffer + UDP_DATA_P;
  bfill.emit_p(PSTR("xap-header\n"
    "{\n"
    "v=12\n"
    "hop=1\n"
    "uid=$S\n"
    "class=Serial.Comms\n"
    "source=$S\n"
    "}\n"
    "Serial.Received\n"
    "{\n"
    "port=/dev/ttyUSB0\n"
    "data=OK $D"
    ), xap.UID, xap.SOURCE, rf12_hdr);
  for (byte i = 0; i < rf12_len; ++i) {
    bfill.emit_p(PSTR(" $D"), (int) rf12_data[i]);      
  }
  bfill.emit_p(PSTR("\n}"));

  xap.broadcastUDP(bfill.position());
}

void loop() {
  /*
  if (Serial.available())
    handleInput(Serial.read());
  */

  if (rf12_recvDone()) {
    byte n = rf12_len;
    if (rf12_crc == 0) {
      Serial.print("OK ");
      Serial.print((int) rf12_hdr);
      for (byte i = 0; i < n; ++i) {
        Serial.print(' ');
        Serial.print((int) rf12_data[i]);
      }
      Serial.println();
      
      if (RF12_WANTS_ACK) {
         Serial.println(" -> ack");
         rf12_sendStart(RF12_ACK_REPLY, 0, 0);
      }

      xapSerial();        
    }
  }

  word len = ether.packetReceive();
  ether.packetLoop(len);  // handle ICMP
  xap.process(len, NULL /*xapCallback*/ );
}

