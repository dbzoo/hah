// $Id$

#include <JeeLib.h>
#include <util/crc16.h>
#include <util/parity.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>

// Do we want this HAHCentral to also be a 433 Mhz RF receiver.
//#define CENTRAL_RF

#ifdef CENTRAL_RF
#include <RFRX.h>

// RF receiver signal in pin
// We don't require an RSSI - just pump all the noise in and we'll deal with it
// PIN4 is the Digital pin of PORT 1
#define RECV_PIN 4

RFrecv rfrecv(RECV_PIN);
RFResults results;
#endif

// Hardcode our node ID so it won't ever change once flashed.
#define NODE_ID 1
#define GROUP 212

// Uncomment to also make the Central a ROOMNODE !
//#define DHT11PIN 4      // Arduino PD4 - is port 1
//s#define LDR_PORT    4   // defined if LDR is connected to a port's AIO pin

#if LDR_PORT
Port ldr (LDR_PORT);
#endif
#if DHT11PIN
DHTxx dht(DHT11PIN);
#endif

#define smillis() ((long)millis())
#define TIMEOUT 120000 // how often to report in milliseconds (2mins)
long waitUntil = smillis();

struct roomnode {
  byte light;     // light sensor: 0..255
byte moved :
  1;  // motion detector: 0..1
byte humi  :
  7;  // humidity: 0..100
int temp   :
  10; // temperature: -500..+500 (tenths)
byte lobat :
  1;  // supply voltage dropped under 3.1V: 0..1
};

union {
  byte b[sizeof(struct roomnode)];
  struct roomnode n;
}
payload;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// RF12 configuration setup code

static char cmd;
static byte value, stack[RF12_MAXDATA], top, sendLen, dest, quiet=1;
static byte testbuf[RF12_MAXDATA], testCounter;

static void addCh (char* msg, char c) {
  byte n = strlen(msg);
  msg[n] = c;
}

static void addInt (char* msg, word v) {
  if (v >= 10)
    addInt(msg, v / 10);
  addCh(msg, '0' + v % 10);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

char helpText1[] PROGMEM = 
"\n"
"Available commands:" "\n"
"  ...,<nn> a - send data packet to node <nn>, with ack" "\n"
"  ...,<nn> s - send data packet to node <nn>, no ack" "\n"
"  <n> q      - set quiet mode (1 = don't report bad packets)" "\n"
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
    case 'a': // send packet to node ID N, request an ack
    case 's': // send packet to node ID N, no ack
      cmd = c;
      sendLen = top;
      dest = value;
      memcpy(testbuf, stack, top);
      break;
    case 'q': // turn quiet mode on or off (don't report bad packets)
      quiet = value;
      break;
    }
    value = top = 0;
    memset(stack, 0, sizeof stack);   
  } 
  else if (c > ' ')
    showHelp();
}
// readout all the sensors and other values
static void doMeasure() {
  memset(&payload, 0, sizeof payload);
#if LDR_PORT
  ldr.digiWrite2(1);  // enable AIO pull-up
  payload.n.light = ~ ldr.anaRead() >> 2;
  ldr.digiWrite2(0);  // disable pull-up to reduce current draw
#endif
#if DHT11PIN
  int t,h;
  if(dht.reading(t, h)) {    
    payload.n.temp = t;
    // Convert from tenth's back to 0-100 range.
    payload.n.humi = h/10;  
  }
#endif
}

// periodic report, i.e. send out a packet and optionally report on serial port
static void doReport() { 
  Serial.print("OK");
  Serial.print(' ');
  Serial.print(NODE_ID, DEC);    
  for (byte i = 0; i < sizeof payload; i++) {
    Serial.print(' ');
    Serial.print(payload.b[i], DEC);
  }
  Serial.println();
}

#ifdef CENTRAL_RF
void dumpRaw(RFResults *results) {
  unsigned int *buf = results->buf;
  Serial.print("RFRX ");
  for (int i = 0; i < results->len; i++) {
    // yes I know it was unsigned int and I could loose precision.
    // but a real pulse won't be using numbers > 32767
    int val = (*buf)*USECPERTICK;
    Serial.print(i%2 == 0 ? -val : val, DEC);
    if(i < results->len-1) Serial.print(",");
    buf++;
  }
  Serial.println("");
}
#endif

void setup() {
  Serial.begin(57600);
  Serial.print("\n[HAHCentral]");
  Serial.print(NODE_ID,DEC);
  Serial.print(" g");
  Serial.print(GROUP,DEC);
  Serial.print(" @ 868Mhz ");

  // RF12_868MHZ, RF12_915MHZ, RF12_433MHZ
  rf12_initialize(NODE_ID, RF12_868MHZ, GROUP);

#ifdef CENTRAL_RF
  Serial.print("RFRX ");
  rfrecv.enableRFIn();
#endif
#if DHT11PIN
  Serial.print("DHT11 ");
#endif
#if LDR_PORT
  Serial.print("LDR ");
#endif
  Serial.println();
}

void loop() {
   if (Serial.available())
    handleInput(Serial.read());

  if (rf12_recvDone()) {
    byte n = rf12_len;
    if (rf12_crc == 0) {
      Serial.print("OK");
    } 
    else {
      if (quiet)
        return;
      Serial.print(" ?");
      if (n > 20) // print at most 20 bytes if crc is wrong
        n = 20;
    }
    if (GROUP == 0) {
      Serial.print("G ");
      Serial.print((int) rf12_grp);
    }
    Serial.print(' ');
    Serial.print((int) rf12_hdr);
    for (byte i = 0; i < n; ++i) {
      Serial.print(' ');
      Serial.print((int) rf12_data[i]);
    }
    Serial.println();

    if (rf12_crc == 0) {
      if (RF12_WANTS_ACK) {
        Serial.println(" -> ack");
        rf12_sendStart(RF12_ACK_REPLY, 0, 0);
      }
    }
  }

  if (cmd && rf12_canSend()) {
    Serial.print(" -> ");
    Serial.print((int) sendLen);
    Serial.println(" b");
    byte header = cmd == 'a' ? RF12_HDR_ACK : 0;
    if (dest)
      header |= RF12_HDR_DST | dest;
    rf12_sendStart(header, testbuf, sendLen);
    cmd = 0;
  }  
#ifdef CENTRAL_RF
  if(rfrecv.match(&results)) {
    dumpRaw(&results);
    rfrecv.resume();
  };
#endif

#if DHT11PIN || LDR_PORT
  if (smillis() - waitUntil > 0) {
     waitUntil = smillis() + TIMEOUT;
     doMeasure();
     doReport();
  }
#endif
}



