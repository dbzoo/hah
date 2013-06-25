/* $Id$
* 
* This sketch makes the job of catching a RF pulse stream without using a Logic analyzer a little easier.
* It works by looking for two pulse sequences that are identical and separated by a gap of at least 10ms.
*
* brett@dbzoo.com
*/
#include "RFRX.h"

// RF receiver signal in pin
// We don't require an RSSI - just pump all the noise in and we'll deal with it
#define RECV_PIN 4

RFrecv rfrecv(RECV_PIN);
RFResults results;

void setup() {
  Serial.begin(57600);
  Serial.println("Starting...");
  rfrecv.enableRFIn();
}

void dumpRaw(RFResults *results) {
  unsigned int *buf = results->buf;
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

void loop() {
  if(rfrecv.match(&results)) {
    dumpRaw(&results);
    rfrecv.resume();
  };
}

