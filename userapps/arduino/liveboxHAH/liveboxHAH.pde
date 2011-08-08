/* $Id$
 Copyright (c) Brett England, 2011
 HAH AVR firmware
 
 No commercial use.
 No redistribution at profit.
 All derivative work must retain this message and
 acknowledge the work of the original author.
 */
#include <LiquidCrystal.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <TimedAction.h> 
#include <UniversalRF.h>
#include <SoftI2cMaster.h>
#include <avr/wdt.h>
#include <avr/pgmspace.h>

//#define PRODUCTION

#define SCL_PIN 6 // PD.6
#define SDA_PIN 7 // PD.7

SoftI2cMaster i2c;
#define MAXPPE 8   // Maximum number of devices
byte ppeCount = 0; // Current number of PPE devices
struct {
  byte addr;
  byte val;
} 
ppe[MAXPPE];

boolean debug = false;
TimedAction reportChanges = TimedAction(1000, pollDevices);
UniversalRF RF = UniversalRF(13); // Transmitter on PB.5
//http://www.arduino.cc/en/Hacking/PinMapping168

static byte i; // Used for all loops

const int firmwareMajor = 3;
const int firmwareMinor = 2;
const byte rport[] = { 
  8,9,10,11}; // to PIN - PB.0-3
const byte inputs[] = { 
  2,3,4,5}; // to PIN - PD.2-5

/****** LCD *******/
/* Hitachi HD44780 driver
 ' PORT C
 '   0 : RS  (LCD)  analogue input 0
 '   1 : E   (LCD)  analogue input 1
 '   2 : DB4 (LCD)  analogue input 2
 '   3 : DB5 (LCD)  analogue input 3
 '   4 : DB6 (LCD)  analogue input 4
 '   5 : DB7 (LCD)  analogue input 5
 '   6 : n/c
 '   7 : n/c
 */
LiquidCrystal lcd(A0,A1,A2,A3,A4,A5); //RS,E,DB4,DB5,DB6,DB7

/*****  COMMAND PROCESSOR *****/
const int inBufferLen = 255;
uint8_t inPtr;
char inBuffer[inBufferLen+1];

union {
  byte all;
  struct {
unsigned input:
    1; 
unsigned onewire:
    1; 
unsigned ppe:
    1; 
  } 
  bit;
} 
report;

/****** 1-WIRE SYSTEM ***********/
boolean oneWireReady = false;
#define TEMPERATURE_PRECISION 12 // bit
#define ONE_WIRE_BUS 12 // PB.4 - digital PIN 12
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress tempDeviceAddress;
int oneWireCount; // Number of devices on the bus.

/****** End variable configuration ***********/

int freeRam () {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

void StreamPrint_progmem(Print &out,PGM_P format,...)
{
  // program memory version of printf - copy of format string and result share a buffer
  // so as to avoid too much memory use
  char formatString[128], *ptr;
  strncpy_P( formatString, format, sizeof(formatString) ); // copy in from program mem
  // null terminate - leave last char since we might need it in worst case for result's \0
  formatString[ sizeof(formatString)-2 ]='\0';
  ptr=&formatString[ strlen(formatString)+1 ]; // our result buffer...
  va_list args;
  va_start (args,format);
  vsnprintf(ptr, sizeof(formatString)-1-strlen(formatString), formatString, args );
  va_end (args);
  formatString[ sizeof(formatString)-1 ]='\0';
  out.print(ptr);
}

#define Serialprint(format, ...) StreamPrint_progmem(Serial,PSTR(format),##__VA_ARGS__)
#define Streamprint(stream,format, ...) StreamPrint_progmem(stream,PSTR(format),##__VA_ARGS__)
#define debugprint(format, ...) if (debug) StreamPrint_progmem(Serial,PSTR(format),##__VA_ARGS__)

//void (*doReboot)() = 0;

void doReboot() {
  wdt_enable(WDTO_15MS);
  for(;;) {
  }
}

void relayOn(char *data) {
  byte relay = atoi(data);
  if(relay < 1 || relay > 4) {
    debugprint("Invalid range must be 1-4\n");
    return;
  }
  digitalWrite(rport[relay-1], HIGH);    
}

void relayOff(char *data) {
  byte relay = atoi(data);
  if(relay < 1 || relay > 4) {
    debugprint("Invalid range must be 1-4\n");
    return;
  }
  digitalWrite(rport[relay-1], LOW);    
}

void doVersion() {
  Serialprint("rev %d.%d\n", firmwareMajor, firmwareMinor);
}

void doHelp() {
  Serialprint("$Revision: 191 $  Available Commands:\n");
  Serialprint("<relay> = 1-4\n");
  Serialprint("  DEBUG\n");
  Serialprint("  VERSION\n");
  Serialprint("  HELP\n");
  Serialprint("  REPORT [1WIRE|PPE|INPUT]\n");
  Serialprint("  1WIRERESET\n");
  Serialprint("  STATUS [1WIRE|RELAY|INPUT]\n");
  Serialprint("  REBOOT\n");
  Serialprint("  ON <relay>\n");
  Serialprint("  OFF <relay>\n");
  Serialprint("  LCD <message>\n");
  Serialprint("  I2C R\n");
  Serialprint("      Maa   - addr\n");
  Serialprint("      Baavv - addr/value\n");
  Serialprint("      Paapv - addr/port/value\n");
  Serialprint("  URF hex\n");
}

void doDebug() {
  debug = !debug;
  RF.setDebug(debug);
}

void doRelayStatus() {
  for(i=0; i<sizeof(rport); i++) {
    Serialprint("Relay %d: %d\n", i+1, rport[i]);    
  }
}

void doInputStatus() { 
  for(i=0; i<sizeof(inputs); i++) {
    Serialprint("Input %d: %d\n", i+1, digitalRead(inputs[i]));
  }
}

void doLCD(char *msg) {
  lcd.clear();
  lcd.setCursor(0,0);  // We also seem to need this, as the above doesn't reposition to 0,0
  lcd.print(msg);
}

byte hexDigit(byte c)
{
  if (c >= '0' && c <= '9') {
    return c - '0';
  } 
  else if (c >= 'a' && c <= 'f') {
    return c - 'a' + 10;
  } 
  else if (c >= 'A' && c <= 'F') {
    return c - 'A' + 10;
  } 
  else {
    debugprint("Invalid HEX character : %c", c);
    return -1;   // getting here is bad: it means the character was invalid
  }
}

void doI2C(char *arg) {
  char cmd = *arg++;  

  // "I2C R" is a Reset
  if (cmd == 'R' || cmd == 'r') { 
    ppeCount = 0;
    debugprint("unregistered all PPE devices\n");
    return;
  }

  // All other commands supply an Address as two ASCII hex digits.
  byte addr = (hexDigit(*arg) << 4) | hexDigit(*(arg+1));
  arg += 2;

  debugprint("i2c cmd: %s\n i2c addr: 0x%x\n", cmd, addr);

  // "I2C Maa" - Add a new I2C device at an address aa
  if (cmd == 'M' || cmd == 'm') { 
    if (ppeCount < MAXPPE) {
      ppe[ppeCount].addr = addr;
      ppe[ppeCount].val = 0;
      ppeCount++;
      debugprint("PPE Registered\n");
    }
    return;
  }

  // Operating on a existing PPE chip
  // See if its registered
  byte ppeIdx = 255;
  for(i=0; i<ppeCount; i++) {
    if(ppe[i].addr == addr) {
      ppeIdx = i;
      break;
    }
  }  
  if (ppeIdx == 255) {
    debugprint("Unregistered PPE address\n");
    return;    
  }

  switch(cmd) {
    //P 	Pin mode selector
    //XX 	Hexadecimal address of the PPE chip
    //Y 	Pin in the Range 0-7
    //Z 	State: 1 or 0, on/off
  case 'P':
  case 'p': 
    { // PXXYZ
      byte pin = *arg - '0';
      if (pin > 7) {
        debugprint("Invalid PIN: range must be 0-7\n");
        return;
      }
      arg++;
      // Set the BIT (pin) according to its STATE
      if(*arg == '1') {
        ppe[ppeIdx].val |= (1<<pin); 
      } 
      else {
        ppe[ppeIdx].val &= ~(1<<pin);
      }      
    }
    break;
    //B 	Byte mode selector
    //XX 	Hexadecimal address of the PPE chip
    //YY 	Hexadecimal value to write to the chip    
  case 'B': // BXXYY
  case 'b':
    ppe[ppeIdx].val = (hexDigit(*arg) << 4) | hexDigit(*(arg+1));
    break;
  default:
    debugprint("Unknown I2C cmd\n");
    return;
  }

  debugprint("i2c send addr 0x%x val 0x%x\n", addr, ppe[ppeIdx].val);

  if(i2c.start(addr | I2C_WRITE)) {
    i2c.write(ppe[ppeIdx].val);
    i2c.stop();
  } 
  else debugprint("Write failure\n");
}

// Scan the I2C bus and see what addresses respond
// Debugging Helper

void i2cScan() {
 uint8_t add = 0;
 
 // try read
 do {
 if (i2c.start(add | I2C_READ)) {
   Serialprint("Addr read: 0x%x\n", add | I2C_READ);
   i2c.read(true);
 }
 i2c.stop();
 add += 2;
 } 
 while (add);
 
 // try write
 add = 0;
 do {
 if (i2c.start(add | I2C_WRITE)) {
   Serialprint("Addr write: 0x%x\n", add | I2C_WRITE);
 }
 i2c.stop();
 add += 2;
 } 
 while (add);
 Serialprint("Done");  
}
 
void doCommand() {
  if(strcasecmp(inBuffer,"report") == 0) {
    report.all = 0xFF;
  } 
  else if(strcasecmp(inBuffer,"version") == 0) {
    doVersion();
  } 
  else if(debug && strcmp(inBuffer,"help") == 0) {
    doHelp();
  } 
  else if(strcasecmp(inBuffer,"reboot") == 0) {
    doReboot();
  } 
  else if(strcasecmp(inBuffer,"report") == 0) {
    report.all = 0xff;
  } 
  else if(strcasecmp(inBuffer,"report 1wire") == 0) {
    report.bit.onewire = 1;
  } 
  else if(strcasecmp(inBuffer,"report ppe") == 0) {
    report.bit.ppe = 1;
  } 
  else if(strcasecmp(inBuffer,"report input") == 0) {
    report.bit.input = 1;
  } 
  else if(strcasecmp(inBuffer,"debug") == 0) {
    doDebug();
  } 
   else if(strcasecmp(inBuffer,"i2c scan") == 0) {
   i2cScan();
   } 
  else if(debug && strcasecmp(inBuffer,"status relay") == 0) {
    doRelayStatus();
  } 
  else if(debug && strcasecmp(inBuffer,"status input") == 0) {
    doInputStatus();
  } 
  else if(strcasecmp(inBuffer,"1wirereset") == 0 || strcasecmp(inBuffer,"status 1wire") == 0) {   
    setupOneWire();
  } 
  else {
    char *arg = strchr(inBuffer,' ');
    if(arg) {
      *arg = '\0';
      arg++;
    } 
    else {
      debugprint("Missing argument\n");
      return;
    }
    if(strcasecmp(inBuffer,"urf") == 0) {
      RF.transmit(arg);
    } 
    else if(strcasecmp(inBuffer,"on") == 0) {
      relayOn(arg);
    } 
    else if(strcasecmp(inBuffer,"off") == 0) {
      relayOff(arg);
    } 
    else if(strcasecmp(inBuffer,"lcd") == 0) {
      doLCD(arg);
    }
    else if (strcasecmp(inBuffer,"i2c") == 0) {
      doI2C(arg);
    }
    else {
      debugprint("Command not found");
    }
  }   
}

void resetSerialBuffer() {
  inPtr = 0;
  inBuffer[0] = '\0';
  if(debug) Serial.print(">");
}

void readSerial() {
  if(Serial.available() == 0) return;

  int inByte = Serial.read();
  if(inByte == '\r') { // CR to LF
    inByte = '\n';
  } 
  else if(inByte == 127) { // DEL to BS
    inByte = '\b';
  }

  switch(inByte) {
  case '\n':
    debugprint("\n");
    if(inPtr > 0) {
      doCommand();
    }
    resetSerialBuffer();
    break;
  case '\b':
    if(debug && inPtr > 0) {
      Serial.print(8, BYTE);
      Serial.print(' ');
      Serial.print(8, BYTE);
      inPtr--;
      inBuffer[inPtr] = '\0';
    }
    break;
  default:
    if(inPtr < inBufferLen) {
      inBuffer[inPtr] = inByte;
      inPtr++;
      inBuffer[inPtr] = '\0';
      if(debug) {
        Serial.print(inByte, BYTE);
      }
    }
  }
}

// Loop through each device, print out temperature data
void reportOneWire() {
  for(i=0;i<oneWireCount; i++)
  {
    // Search the wire for address
    if(sensors.getAddress(tempDeviceAddress, i))
    {
      // Output the device ID
      Serialprint("1wire ");
      printAddress(tempDeviceAddress);
      Serialprint(" ");
      Serial.println(sensors.getTempC(tempDeviceAddress));
    } 
  }
}

void reportI2C() {
  byte result;
  for(i=0; i<ppeCount; i++) {
    i2c.start(ppe[i].addr | I2C_READ); // Read Addr = WRITE Addr + 1
    result = i2c.read(true); // and send NAK to terminate read
    i2c.stop(); 

    if(result != ppe[i].val || report.bit.ppe ) {
      Serialprint("i2c-ppe %d %d %d\n", ppe[i].addr, ppe[i].val, result);
      ppe[i].val = result;
    }
  }
  report.bit.ppe = 0;
}

void reportInputs() {
  static byte prev_value = 0;
  byte portd = PIND & B00111100; // Mask off other pins.  
  if(prev_value != portd || report.bit.input) {
    report.bit.input = 0;
    Serialprint("input %d %d\n", portd, prev_value);
    prev_value = portd;    
    delay(25); // (ms) debounce
  }
}

// Every other second we ASK the 1wire devices to get a temperate ready for reading
// 12bits precision takes 750ms
// Every Second we report on the I2C bus status.
void pollDevices() {
  //reportI2C();
  if(oneWireReady) {
    reportOneWire();
    oneWireReady = false;
  } 
  else {
    sensors.requestTemperatures(); 
    oneWireReady = true;
  }
}

// print 1wire ROM address
void printAddress(DeviceAddress deviceAddress)
{
  for (i=0; i<8; i++)
  {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

void setupOneWire() {
  sensors.begin();  
  oneWireCount = sensors.getDeviceCount();
  debugprint("Found %d 1wire sensors\n", oneWireCount);
  for(i=0; i<oneWireCount; i++) {
    if(sensors.getAddress(tempDeviceAddress, i))
    {
      if(debug) {
        Serialprint("ID: %d ROM: ", i);
        printAddress(tempDeviceAddress);
        Serialprint(" Model: ");
        switch(tempDeviceAddress[0]) {
        case DS18S20MODEL: 
          Serialprint("DS18S20"); 
          break;
        case DS18B20MODEL: 
          Serialprint("DS18B20"); 
          break;
        case DS1822MODEL: 
          Serialprint("DS1822"); 
          break;
        default:
          Serialprint("Unknown"); 
          break;          
        }
        Serialprint("Setting resolution to %d bits\n", TEMPERATURE_PRECISION);
      }

      sensors.setResolution(tempDeviceAddress, TEMPERATURE_PRECISION);

      if(debug) {
        Serialprint(", actual resolution %d bits", sensors.getResolution(tempDeviceAddress));
      }
    }
    else debugprint("ID: %d ghost device. Could not detect address. Check power and cabling");
  }
}

// Wait for +++ before entering command processor
void bootWait() {
  i = 0;
  int inByte;

  doLCD("Booting...");
  do {  
    if(Serial.available()) {
      inByte = Serial.read();
      if(inByte == '+') i++; 
      else i = 0;   
    }
  } 
  while(i < 3);
}

void setup() {
#ifdef PRODUCTION  
  Serial.begin(115200);
#else
  Serial.begin(9600);
  doDebug(); // always enabled - remove for production and testing.
  Serialprint(" memory available: %d bytes\n",freeRam());
  doVersion();
#endif  
  resetSerialBuffer();
  setupOneWire();

  // Reset relays
  for(i=0; i<sizeof(rport); i++) {
    pinMode(rport[i], OUTPUT);
    digitalWrite(rport[i], LOW);    
  }

  // Enable INPUTS and pull-ups
  for(i=0; i<sizeof(inputs); i++) {
    pinMode(inputs[i], INPUT);
    digitalWrite(inputs[i], HIGH);    
  }
  // LCD - analogue pins into OUTPUT mode (digital)
  pinMode(A0, OUTPUT);
  pinMode(A1, OUTPUT);
  pinMode(A2, OUTPUT);
  pinMode(A3, OUTPUT);
  pinMode(A4, OUTPUT);
  pinMode(A5, OUTPUT);
  lcd.begin(8,2); // Columns x Rows

  // I2C
  i2c.init(SCL_PIN, SDA_PIN);

#ifdef PRODUCTION
  bootWait();
#endif
}

void loop() {
  readSerial();
  reportChanges.check();
  reportInputs();  
}

