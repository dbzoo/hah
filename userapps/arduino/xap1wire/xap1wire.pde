/* $Id$ 
  xAP 1wire monitoring with nuElectronics ethernet shield

 Arduino
  GND   ------1+--------\
  Pin 9 ------2+ ds1820 | 
  +5v   ------3+--------/
   
  If the pull resister isn't on the HAH board you need a 4k7 between PIN 2 AND 3.
  
*/
#include <EtherCard.h>
#include <xAP.h>
#include <xAPEther.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <stdlib.h>

// ethernet interface mac address
static byte mymac[6] = { 
  0x54,0x55,0x58,0x10,0x00,0x26 };
static byte myip[4] = { 
  192,168,1,15 };
static byte buf[500];

// XAP Identification
#define UID "FFDB5500"
#define SOURCE "dbzoo.arduino.temp"

// PIN 9
#define ONE_WIRE_BUS 9
#define TEMPERATURE_PRECISION 12
#define MAXSENSORS 20

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

int numberOfDevices; // Number of temperature devices found
char tempS[10];
char tempFS[10];

struct _sensor {     
  DeviceAddress deviceAddress;
  float temp;
} 
sensor[MAXSENSORS];

XapEther xap(SOURCE, UID, mymac, myip);

void setup () {
  Serial.begin(9600);
  Serial.println("Arduino temperature monitor");
  sensors.begin();
  numberOfDevices = sensors.getDeviceCount();
  if(numberOfDevices > MAXSENSORS) numberOfDevices = MAXSENSORS;
  Serial.print("Found ");
  Serial.print(numberOfDevices, DEC);
  Serial.println(" Sensors");

  for(int i=0;i<numberOfDevices; i++) {
    if(sensors.getAddress(sensor[i].deviceAddress, i)) {
      sensors.setResolution(sensor[i].deviceAddress, TEMPERATURE_PRECISION);
    } 
  }

  xap.setBuffer(buf, sizeof buf);
  delay(800);
    for(int i=0;i<numberOfDevices; i++) {
      sendXapState("xAPBSC.info", &sensor[i]);
    }    
}

static void homePage() {
  // Web server only reports the 1st sensor
  bfill = eth.tcpOffset(buf);  
  bfill.emit_p(PSTR(
  "HTTP/1.0 200 OK\r\n"
    "Content-Type: text/html\r\n"
    "Pragma: no-cache\r\n"
    "\r\n"
    "<title>xAP Temperature</title>"
    "Temperature $S&deg;C / $S&deg;F<p>"), 
  dtostrf(sensor[0].temp,3,1,tempS), dtostrf(DallasTemperature::toFahrenheit(sensor[0].temp), 3,1, tempFS));  
  eth.httpServerReply(buf,bfill.position()); // send web page data
}

void sendXapState(char *clazz, struct _sensor *sensor) {
  bfill = eth.udpOffset(buf);    
  bfill.emit_p(PSTR("xap-header\n"
    "{\n"
    "v=12\n"
    "hop=1\n"
    "uid=$S\n"
    "class=$S\n"
    "source=$S\n"
    "}\n"
    "input.state\n"
    "{\n"
    "state=on\n"
    "text=$S\n"
    "}"), UID, clazz, SOURCE, dtostrf(sensor->temp,3,1,tempS));
  eth.sendUdpBroadcast(buf, bfill.position(), XAP_PORT, XAP_PORT);    
}

void processXapMsg() {
  if(xap.getType() != XAP_MSG_ORDINARY) return;
  if(!xap.isValue("xap-header","target", SOURCE)) return;
  if(xap.isValue("xap-header","class","xAPBSC.query")) {

    for(int i=0;i<numberOfDevices; i++) {
      sendXapState("xAPBSC.info", &sensor[i]);
    }    
  }
}

void updateTemperature(struct _sensor *sensor) {
  float tempC = sensors.getTempC(sensor->deviceAddress);
  if(sensor->temp != tempC) {
    sensor->temp = tempC;
    sendXapState("xAPBSC.event", sensor);
  }
}

void loop () {
  sensors.requestTemperatures(); // Send the command to get temperatures

  word len = eth.packetReceive(buf, sizeof(buf));
  // ENC28J60 loop runner: handle ping and wait for a tcp packet
  word pos = eth.packetLoop(buf, len);  
  if(pos) {  // Check if valid www data is received.
    homePage();        
  } else { // Handle xAP UDP
    xap.process(len, processXapMsg);
    for(int i=0;i<numberOfDevices; i++) {
      updateTemperature(&sensor[i]);
    }
  }  
}
