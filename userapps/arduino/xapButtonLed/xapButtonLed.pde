/* $Id$
 This example shows how to detect when a button or button changes from off to on
 and on to off.  An xAP event will be sent when a state change is detected.
 	
 The circuit:
 * pushbutton attached to pin 2 from +5V
 * 10K resistor attached to pin 2 from ground
 * LED attached from pin 13 to ground (or use the built-in LED on
   most Arduino boards)
*/
#include <xAPSerial.h>

// xAP Identification
#define UID "FFDB5500"
#define SOURCE "dbzoo.arduino.button"

const int buttonPin = 2;
const int ledPin = 13;
int buttonState = 0;  // current state of the button
int lastButtonState = 0;
XapSerial xap(SOURCE, UID);

void setup() {
  Serial.begin(38400);
  pinMode(buttonPin, INPUT);
  pinMode(ledPin, OUTPUT);
  xap.sendHeartbeat();
}

void loop() {
   // read the pushbutton input pin:
  buttonState = digitalRead(buttonPin);

  // compare the buttonState to its previous state
  if (buttonState != lastButtonState) {
    digitalWrite(ledPin,  buttonState);
    sendXapEvent();
    // save the current state as the last state, 
    //for next time through the loop
    lastButtonState = buttonState;
  }
  xap.heartbeat();
}

void sendXapEvent() {
     Serial.print(STX, BYTE);
     Serial.println("xap-header");
     Serial.println("{");
     Serial.println("v=12");
     Serial.println("hop=1");
     Serial.print("uid=");  Serial.println(UID);
     Serial.println("class=xAPBSC.event");
     Serial.print("source="); Serial.println(SOURCE);
     Serial.println("}");
     Serial.println("input.state");
     Serial.println("{");  
     Serial.print("state="); Serial.println(buttonState ? "on" : "off");
     Serial.print("}----"); // no CRC
     Serial.print(ETX, BYTE);
}
