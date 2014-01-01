/* $Id$
Bluetooth Proximity detector for the HS-05 device.
This sketch is designed to be flashed on to a JeeNode/HAHnode.
The decoding and interpretting of the "payload" occurs on the HAH system.

HC-05  Arduino
KEY -> 9
TX -> 10
RX -> 11
*/

#include <SoftwareSerial.h>
#include <JeeLib.h>

// RF12 - our node ID
#define NODE_ID 18

// Bluetooth to Arduino pins.
#define BTkey 9
#define BTrxPin 10
#define BTtxPin 11

// Production setting: SERIAL commented, WITHRF uncommented.
#define SERIAL  1        // uncomment to debug to serial port.
//#define WITHRF 1         // Comment out when running on hardware with no RF12B hardware.
#define BTPOLL_PERIOD  300 // how often to poll for bluetooth devices, in tenths of seconds

enum { BTPOLL, TASK_END };
enum { SYNC, ASYNC };

// RF Transmission payload.
struct {
  union {
    struct {
      unsigned int nap:16;
      unsigned int uap:8;
      unsigned int lap:24;
    };
    uint8_t device[6];
  };
} payload;

// RF Configuration struct.
struct {
  byte nodeId;
  byte group;
} config;

SoftwareSerial BTSerial(BTrxPin, BTtxPin);
static word schedbuf[TASK_END];
Scheduler scheduler (schedbuf, TASK_END);

// Serial buffer management
const uint8_t inBufferLen = 64;
char inBuffer[inBufferLen+1];
uint8_t inPtr = 0;

/*************************************************************/

/* Process a response to an AT command, or asychronous messages, from the bluebooth unit.
*/
void processBluetoothResponse() {
#if SERIAL
    Serial.println(inBuffer);
#endif
    if(strncmp("+INQ:",inBuffer,5) == 0) {
    // response:  +INQ:<p1>,<p2>,<p3> ...
    // p1 - Bluetooth address
    // p2 - device type
    // p3 - RSSI signal intensity
    // Multiple +INQ: lines may be in the response for each BT device.
    // ex.  +INQ:1D:FE:720479,7A020C,7FFF    

      // Carve the responses p1,p2,p3 into arg[0..2]
      // Destroys inBuffer.     
      char *arg[3];
      char *p = inBuffer + 5;
      uint8_t i = 0;
      while(i < 3) {
        arg[i++] = p;
        while(*p && *p != ',') p++;
        if(! *p) break;
        *p++ = '\0';
      }      
      
#if SERIAL      
      Serial.print("device=");
      Serial.print(arg[0]);
      Serial.print(" type=");
      Serial.print(arg[1]);
      Serial.print(" rssi=");
      Serial.println(arg[2]);
#endif

#if WITHRF     
// A bluetooth device address breaks down like this: NAP:UAP:LAP
// Each Bluetooth device is assigned a 48-bit address consisting of three parts: 
// a 24-bit lower address part (LAP), an 8-bit upper address part (UAP), 
// and a 16-bit so-called non-significant address part (NAP). 

      uint8_t uap;
      uint16_t nap;
      uint32_t lap;
      sscanf(arg[0],"%hx:%hhx:%x", &nap, &uap, &lap);
	
      payload.nap = nap;
      payload.uap = uap;
      payload.lap = (lap & 0xffffff); // 24 bits.
	      
      rf12_sleep(RF12_WAKEUP);
      while (!rf12_canSend())
        rf12_recvDone();
      rf12_sendStart(0, &payload, sizeof payload);
      rf12_sleep(RF12_SLEEP);
#endif      
    }
}

// Return true when a line has been successful processed
boolean accumulateSerialChars() {
  uint8_t inByte = BTSerial.read();
  switch(inByte) {
  case '\r':  break;
  case '\n': 
    if(inPtr > 0) {
       processBluetoothResponse();
       inPtr = 0;
       return true;
    }
    break;
  default:
    if(inPtr < inBufferLen) {
      inBuffer[inPtr] = inByte;
      inPtr++;
      inBuffer[inPtr] = '\0';
    }
  }
  return false;
}

// Send a command to the bluetooth device.
// mode ASYNC - don't want for the response
// mode SYNC - wait for the response
void sendBluetoothCommand(String cmd, uint8_t mode) {
#if SERIAL
  Serial.print(mode == SYNC ? "SYNC> " : "ASYNC> ");
  Serial.println(cmd);
#endif
  BTSerial.println(cmd);
  if (mode == ASYNC) return;
  
  boolean done = false;
  while(! done) {
    if (BTSerial.available()) {
      done = accumulateSerialChars();
    }
  }
}

void setup()
{
   Serial.begin(38400);

#if WITHRF
  ////////////////////////////////////////////////////////
  // SETUP RF sub-system
  config.nodeId = NODE_ID;
  config.group = 212;
#if SERIAL
  Serial.print("\n[BlueNode.1]");
  Serial.print(config.nodeId,DEC);
  Serial.print(" g");
  Serial.print(config.group,DEC);
  Serial.println(" @ 868Mhz");
#endif

  // RF12_868MHZ, RF12_915MHZ, RF12_433MHZ
  rf12_initialize(config.nodeId, RF12_868MHZ, config.group);
  rf12_sleep(RF12_SLEEP); // power down
#else
#if SERIAL
  Serial.println("\n[BlueNode.1] - RF disabled");
#endif
#endif

  /////////////////////////////////////////////////////////
  // SETUP BLUETOOTH  
  // define pin modes for tx, rx:
  pinMode(BTrxPin, INPUT);
  pinMode(BTtxPin, OUTPUT);
  // Switch module to AT mode
  pinMode(BTkey, OUTPUT);
  digitalWrite(BTkey, HIGH);  
  BTSerial.begin(38400);
  
  sendBluetoothCommand("AT", SYNC);
  sendBluetoothCommand("AT+NAME=blueNode", SYNC);
  // AT+ROLE=p1
  // p1 - 0 Slave Role, 1 Master Role, 2 Slave-Loop role.
  sendBluetoothCommand("AT+ROLE=1", SYNC);
  // AT+INQM=p1,p2,p3
  // p1 - 0 inquire_mode_standad, 1 inquire_mode_rssi
  // p2 - maximum number of bluetooth devices response
  // p3 - the maximum of limited inquiring time. 1~48 (1.28s ~ 61.44s)
  //      15*1.28 = 19sec 
  sendBluetoothCommand("AT+INQM=0,5,15", SYNC);
  
  // start bluetooth polling
  scheduler.timer(BTPOLL, 0);    
}

void loop()
{
  switch (scheduler.poll()) {
  case BTPOLL:
    // reschedule the polling for the next period and ask the BT device what devices it can see.
    scheduler.timer(BTPOLL, BTPOLL_PERIOD);    
    sendBluetoothCommand("AT+INQ", ASYNC);
  }
  
  // Handle async responses
  while(BTSerial.available()) {
    accumulateSerialChars();
  } 
}
