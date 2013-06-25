// $Id $
#include <JeeLib.h>

#define SERIAL  1   // set to 1 to also report readings on the serial port
#define DEBUG   0   // set to 1 to display each loop() run

#define MEASURE_PERIOD  600 // how often to measure, in tenths of seconds
#define RETRY_PERIOD    10  // how soon to retry if ACK didn't come in
#define RETRY_LIMIT     5   // maximum number of times to retry
#define ACK_TIME        10  // number of milliseconds to wait for an ack
#define REPORT_EVERY    5   // report every N measurement cycles
#define NODEID          8   // MY NODE ID *** ADJUST PLEASE **

// The scheduler makes it easy to perform various tasks at various times:

Port out[] = {Port(1), Port(2), Port(3), Port(4)};

enum { MEASURE, REPORT, TASK_END };

static word schedbuf[TASK_END];
Scheduler scheduler (schedbuf, TASK_END);

// Other variables used in various places in the code:

static byte reportCount;    // count up until next report, i.e. packet send

// This defines the structure of the packets which get sent out by wireless:
struct {
    byte p1:1;
    byte p2:1;
    byte p3:1;
    byte p4:1;
    byte lobat:1;
} payload;

// UNUSED.
// wait a few milliseconds for proper ACK to me, return true if indeed received
static byte waitForAck() {
    MilliTimer ackTimer;
    while (!ackTimer.poll(ACK_TIME)) {
        if (rf12_recvDone() && rf12_crc == 0 &&
                // see http://talk.jeelabs.net/topic/811#post-4712
                rf12_hdr == (RF12_HDR_DST | RF12_HDR_CTL | NODEID))
            return 1;
    }
    return 0;
}

// periodic report, i.e. send out a packet and optionally report on serial port
static void doReport() {
    while (!rf12_canSend())
        rf12_recvDone();
    rf12_sendStart(0, &payload, sizeof payload);

    #if SERIAL
        Serial.print("OUTPUT ");
        Serial.print((int) payload.p1);
        Serial.print(' ');
        Serial.print((int) payload.p2);
        Serial.print(' ');
        Serial.print((int) payload.p3);
        Serial.print(' ');
        Serial.print((int) payload.p4);
        Serial.print(' ');
        Serial.print((int) payload.lobat);
        Serial.println();
    #endif
}

static void doMeasure() {
    payload.lobat = rf12_lowbat();
}

static void setPort (byte port, byte on) {
#if SERIAL || DEBUG
    Serial.print("Port ");
    Serial.print(port, DEC);
    Serial.println(on ? " set on" : " set off");
#endif
    port--; // Adjust for ZERO based index Port1 = index[0]
    out[port].mode(OUTPUT);
    out[port].digiWrite(on);
}

// Processing two ASCII bytes of RF12 data.
// XY
// X = PORT
// Y = STATE
static void doIncoming() {
       byte port = rf12_data[0];
       if(port < 1 || port > 4) {
#if SERIAL
          Serial.println("Port out of range 1-4");
#endif
          return;
       }
       byte state = rf12_data[1];
       if(state > 1) state=1;       
       setPort(port, state);
}

void setup () {
    #if SERIAL || DEBUG
        Serial.begin(57600);
        Serial.print("\n[outputNode.1] i");
        Serial.print(NODEID,DEC);
        Serial.println(" g212 @ 868Mhz");
    #endif    
    rf12_initialize(NODEID, RF12_868MHZ, 212);
    
    reportCount = REPORT_EVERY;     // report right away for easy debugging
    scheduler.timer(MEASURE, 0);    // start the measurement loop going
}

void loop () {
    #if DEBUG
        Serial.print('.');
        delay(2);
    #endif
  
      // process incoming data packet
    if (rf12_recvDone() && rf12_crc == 0 && rf12_len >0 && rf12_hdr == (RF12_HDR_DST | NODEID)) {
      #if SERIAL || DEBUG
        Serial.print("GOT");
        for (byte i = 0; i < rf12_len; ++i) {
            Serial.print(' ');
            Serial.print((int)rf12_data[i]);
        }
        Serial.println();
      #endif
        
        doIncoming();
    }
    
    switch (scheduler.poll()) {
        case MEASURE:
            // reschedule these measurements periodically
            scheduler.timer(MEASURE, MEASURE_PERIOD);
    
            doMeasure();

            // every so often, a report needs to be sent out
            if (++reportCount >= REPORT_EVERY) {
                reportCount = 0;
                scheduler.timer(REPORT, 0);
            }
            break;
            
        case REPORT:
            doReport();
            break;                
    }
}
