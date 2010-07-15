/* $Id$
 * XAP compliant receiver
 * Control the Arduino in-built LED with an xAPBSC.cmd message.
 * 
 * Copyright (c) Brett England, 2010
 *
 * No commercial use.
 * No redistribution at profit.
 * All derivative work must retain this message and
 * acknowledge the work of the original author.
 *
 *
 * Configure your HAH /etc/xap-livebox.ini file like this
 * [bridge]
 * enable=1
 * s1=/dev/ttyUSB0:B38400 CLOCAL IGNBRK CS8 CREAD:RX TX
 */
#include <xAPSerial.h>

// XAP Identification
#define UID "FFDB5500"
#define SOURCE "dbzoo.arduino.demo"
#define ENDPOINT SOURCE":led"

const int ledPin = 13;    // LED connected to digital pin 13
byte xapBuff[256];        // xAP message accumulation buffer.

XapSerial Xap(SOURCE, UID);  // Register our SOURCE and UID

void setup()
{
     pinMode(ledPin, OUTPUT);  // Mark the LED PIN for OUTPUT
     digitalWrite(ledPin, HIGH); // Light LED to show we are initializing.
 
     Xap.setBuffer(xapBuff, sizeof(xapBuff));
 
     Serial.begin(38400);      // Setup the serial port 38400 baud.     
     Xap.sendHeartbeat();      // Send an initial heartbeat to show we are alive.
     delay(2000);              // Wait 2 secs
     sendXapInfo();            // To be xAPBSC compliant we need to send this on start-up
     digitalWrite(ledPin, LOW); // Turn it off - we are now READY.
}

void loop() {
     // Receive framed xap message on the serial port, 
     // de-frame, parse and call inboundXapMsg() when done.
     Xap.process(inboundXapMsg);
}

// Now we do some work with an inbound XAP message.
void inboundXapMsg() {
     // Ignore heartbeats
     if(Xap.getType() != XAP_MSG_ORDINARY) return;

     // Is this message for us?
     if(! Xap.isValue("xap-header","target", ENDPOINT)) return;  

     char *clazz = Xap.getValue("xap-header", "class");

     if(strcasecmp(clazz, "xAPBSC.cmd") == 0) {
        digitalWrite(ledPin, Xap.getState("output.state.1","state"));         
     } else if(strcasecmp(clazz,"xAPBSC.query") == 0) {
         sendXapInfo();
     }
}

void sendXapInfo() {
     Serial.print(STX, BYTE);
     Serial.println("xap-header");
     Serial.println("{");
     Serial.println("v=12");
     Serial.println("hop=1");
     Serial.print("uid=");  Serial.println(UID);
     Serial.println("class=xAPBSC.info");
     Serial.print("source="); Serial.println(SOURCE);
     Serial.println("}");
     Serial.println("input.state");
     Serial.println("{");  
     Serial.print("state="); Serial.println(digitalRead(ledPin));
     Serial.print("}----"); // no CRC
     Serial.print(ETX, BYTE);
}
