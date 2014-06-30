/****************************************************************************
 EthSensorXap.ino - Ethernet enabled weather sensor node; 
                    Humdity, Pressure and two independant temperature sensors.
 
 CONNECTIONS:
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
 
 http://www.sureelectronics.net/goods.php?id=904
 Temperature and Relative Humidity Sensor Module DC-SS500
 
 DC-SS500             ARDUINO BOARD
 PIN 3  (RX)           PIN 2
 PIN 4  (TX)           PIN 3
 PIN 12 (5V)           +5V
 PIN 24 (GND)          GND
 
 BMP0085 - Wired to Arduino UNO
 http://www.arduino.cc/en/Reference/Wire
 https://www.sparkfun.com/tutorials/253
 SDA   -> pin A4   (no pull up resistors)
 SCL   -> pin A5   (no pull up resistors)
 XCLR  -> N/A
 EOC   -> N/A
 GND   -> pin GND
 VCC   -> pin 3.3V / 5V
 
 LDR wiring - http://4tronix.co.uk/arduino/4-Sensor-Datalogger.php
       pin A0 -----+--- LDR --- GND
                   |
                   +--- 10k --- +5v 
 
 1WIRE
       -> pin 7
       
xAP Payload - semi compliant with this schema
http://www.xapautomation.org/index.php?title=xAP_Weather_Schema

xap-header
{
v=12
hop=1
uid=FFDBFF00
class=weather.data
source=dbzoo.nanode.weather
}
weather.report
{
TempC=11
Humidity=91
InTempC=10.4
AirPressureP=974
}

*/
#include <EtherCard.h>
#include <xAPEther.h>
#include <SoftwareSerial.h>
#include "TimedAction.h"
#include <Wire.h>
#include <BMP085.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// DEBUGGING
#define SERIAL_BAUD 9600
#define SERIAL_DEBUG 1

// ENC28J60
#define CS_PIN 10

// ethernet interface mac address
static uint8_t mymac[6] = { 
  0xde, 0xad, 0xbe, 0xef, 0x00, 0x30};  
byte Ethernet::buffer[500];
BufferFiller bfill;

// xAP Configuration
XapEther xap("dbzoo.arduino.weather","FFDBFF00");
TimedAction timedAction = TimedAction(120 * 1000, process); // 120 sec

// DC-SS500
#define rxPin 3
#define txPin 2
SoftwareSerial dcss500 =  SoftwareSerial(rxPin, txPin);
int dcssT, dcssRH;  // Temperature, Relative Humidity

// BMP085
BMP085 dps = BMP085();
int32_t bmpT = 0, bmpP = 0; // Temperature, Pressure

// LDR
#define LDRpin A0
int ldr;

// 1WIRE
#define ONE_WIRE_PIN 7
#define TEMPERATURE_PRECISION 12
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_PIN);
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);
DeviceAddress deviceAddress;
int outTemp;

static int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

void setup() {
#if SERIAL_DEBUG   
  Serial.begin(SERIAL_BAUD);
reset:  
  Serial.println("[xAP Ethernet Weather]");
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

  pinMode(txPin, OUTPUT);
  pinMode(rxPin, INPUT);
  pinMode(LDRpin, INPUT);
  dcss500.begin(9600);
  Wire.begin();
  delay(1000);
  dps.init();

  sensors.begin();
#if SERIAL_DEBUG
  Serial.print("Locating OneWire devices...");
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");
#endif
  if (sensors.getAddress(deviceAddress, 0)) {
    sensors.setResolution(deviceAddress, TEMPERATURE_PRECISION);
  } 
#if SERIAL_DEBUG
  else {
    Serial.println("Unable to find address for Device 0");
  }
#endif

  process();  // Start with a payload.
}

void xapSend() {
  // bmpT is in 0.1C units - get Integer and Decimal component.
  int bmpT1 = bmpT / 10;
  int bmpT2 = bmpT - bmpT1 * 10;
  int hPa = bmpP / 100; // Pascals to HectoPascals
  int outTemp1 = outTemp / 10;
  int outTemp2 = outTemp - outTemp1 * 10;

  bfill = ether.buffer + UDP_DATA_P;
  bfill.emit_p(PSTR("xap-header\n"
    "{\n"
    "v=12\n"
    "hop=1\n"
    "uid=$S\n"
    "class=weather.data\n"
    "source=$S\n"
    "}\n"
    "weather.report\n"
    "{\n"
    "TempC=$D\n"
    "Humidity=$D\n"    
    "InTempC=$D.$D\n"
    "AirPressureP=$D\n"
    "Light=$D\n"
    "OutTempC=$D.$D\n"    
    "}\n" 
    ), xap.UID, xap.SOURCE, dcssT, dcssRH, bmpT1, bmpT2, hPa, ldr, outTemp1, outTemp2);
  xap.broadcastUDP(bfill.position());  
}

int str_to_int(char *s){  
  // assumes the first three characters are numeric 
  int v = 0, n;
  for (n=0; n<3; n++) {
    if(isdigit(s[n])) {
      v = v * 10 + (s[n] - '0');
    }
  }
  return v;
} 

void get_str(char *s, int ms, int len){
  char ch;
  char *maxs = s + len;  

  while(ms--)  {
    while (dcss500.available() > 0) {
      ch = dcss500.read();
      if ((ch == '\r') || (ch =='\n') || (s == maxs)) { // CR or LF
        *s = 0; // null terminate the string
        return;
      } 
      else {
        *s++ = ch;
      }
    }
    delay(1);
  }
  *s = 0;
  return;
}

void process_dcss() {    
  char s[30];
  int T, RH;

  // measure and display the temperature in degrees Celcuis
  dcss500.flush();  // clear the Rx UART   
  dcss500.println("$sure temp -c");
  get_str(s, 100, sizeof(s));
  dcssT = str_to_int(s);
#if SERIAL_DEBUG     
  Serial.println(s);
  Serial.print("T_C = ");
  Serial.println(dcssT);
#endif
  // Now the same for the relative humidity,
  dcss500.flush();  // clear the Rx UART   
  dcss500.println("$sure humidity");
  get_str(s, 100, sizeof(s)); 
  dcssRH = str_to_int(s);    
#if SERIAL_DEBUG 
  Serial.println(s);
  Serial.print("RH = ");
  Serial.println(dcssRH);    
#endif
}

void process_bmp() {
  dps.getTemperature(&bmpT); // 0.1C units
  dps.getPressure(&bmpP);  // Pascals
#if SERIAL_DEBUG     
  Serial.print("BMP T = ");
  Serial.print(bmpT);
  Serial.print("  ");
  Serial.print("P = ");
  Serial.print(bmpP);
  Serial.println("   ");
#endif    
}

// Callback for timedAction()
void process() {
  process_dcss();
  process_bmp();
  
  ldr = 1023 - analogRead(LDRpin); // LDR is path to GND (inverse)
#if SERIAL_DEBUG     
  Serial.print("LDR = ");
  Serial.println(ldr);
#endif

  sensors.requestTemperatures();
  outTemp = sensors.getTempC(deviceAddress) * 10;
#if SERIAL_DEBUG     
  Serial.print("1WIRE TEMP = ");
  Serial.println(outTemp);
#endif

  xapSend();
}

void loop() {
  word len = ether.packetReceive();
  ether.packetLoop(len);  // handle ICMP  
  timedAction.check();
  // We don't process any inbound xAP packets, just to heartbeat stuff.
  xap.process(len, 0);
}






