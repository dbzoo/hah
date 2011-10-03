/* $Id$
 * IRNode: detect an IR signal and relay via RF
 *         decoded by the IRNode LUA HAH backend.
 * An IR detector/demodulator must be connected to the input RECV_PIN.
 * Version 0.1 October, 2011
 * Copyright 2011 Brett England
 * http://www.homeautomationhub.com/
 */
#include <Ports.h>
#include <RF12.h>
#include <IRremote.h>

int RECV_PIN = 5; // Jee Port 2 DIO - Signal PD5 chip pin 11.

IRrecv irrecv(RECV_PIN);
decode_results results;

#define SERIAL  1   // set to 1 to also report readings on the serial port
static byte myNodeID;       // node ID used for this unit

// This defines the structure of the packets which get sent out by wireless:
struct {
  int type; // NEC, SONY, RC5, UNKNOWN
  int bits; // Number of bits in decoded value
  unsigned long value;  // decoded value
} 
payload;

// periodic report, i.e. send out a packet and optionally report on serial port
static void doReport(decode_results *results) {

#if SERIAL
  if (results->decode_type == UNKNOWN) {
    Serial.println("Could not decode message");
  }  
  else {
    if (results->decode_type == NEC) {
      Serial.print("Decoded NEC: ");
    } 
    else if (results->decode_type == SONY) {
      Serial.print("Decoded SONY: ");
    } 
    else if (results->decode_type == RC5) {
      Serial.print("Decoded RC5: ");
    } 
    else if (results->decode_type == RC6) {
      Serial.print("Decoded RC6: ");
    }
    Serial.print(results->value, HEX);
    Serial.print(" (");
    Serial.print(results->bits, DEC);
    Serial.println(" bits)");
  }
#endif    

  payload.type = results->decode_type;
  payload.bits = results->bits;
  payload.value = results->value;

  while (!rf12_canSend())
    rf12_recvDone();
  rf12_sendStart(0, &payload, sizeof payload);
}


void setup () {
#if SERIAL
  Serial.begin(57600);
  Serial.print("\n[IRNode.1]");
  myNodeID = rf12_config();
#else
  myNodeID = rf12_config(0); // don't report info on the serial port
#endif    
  irrecv.enableIRIn(); // Start the receiver
}

void loop () {
  // This is not power friendly so don't run this on batteries!
  if (irrecv.decode(&results)) {
    doReport(&results);
    irrecv.resume(); // Receive the next value
  }
}


