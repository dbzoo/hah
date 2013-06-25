// New version of the Room Node, derived from rooms.pde
// 2010-10-19 <jcw@equi4.com> http://opensource.org/licenses/mit-license.php
// $Id$

// see http://jeelabs.org/2010/10/20/new-roomnode-code/
// and http://jeelabs.org/2010/10/21/reporting-motion/

// The complexity in the code below comes from the fact that newly detected PIR
// motion needs to be reported as soon as possible, but only once, while all the
// other sensor values are being collected and averaged in a more regular cycle.
//
//  Brett - Added OneWire support (RoomNode2)
//
// ATMEL ATMEGA8 & 168 / ARDUINO
//
// ATMEGA328 / Arduino / JeeNode mapping
//
//                                   +-\/-+
//             (PCINT14,RESET) PC6  1|    |28  PC5 (ADC5,SCL,PCINT13,A0)
//            (PCINT16,RXD,D0) PD0  2|    |27  PC4 (ADC4,SDA,PCINT12,A1)
//            (PCINT17,TXD,D1) PD1  3|    |26  PC3 (ADC3,PCINT11,A2)      JP4A
// RFM12 INT (PCINT18,INT0,D2) PD2  4|    |25  PC2 (ADC2,PCINT10,A3)      JP3A
// JPint     (PCINT19,INT1,D3) PD3  5|    |24  PC1 (ADC1,PCINT9,A4)       JP2A
// JP1D           (PCINT20,D4) PD4  6|    |23  PC0 (ADC0,PCINT8,A5)       JP1A
//                             VCC  7|    |22  GND
//                             GND  8|    |21  AREF
//              (PCINT6,XTAL1) PB6  9|    |20  AVCC
//              (PCINT7,XTAL2) PB7 10|    |19  PB5 (SCK,PCINT5,D13)
// JP2D           (PCINT21,D5) PD5 11|    |18  PB4 (MISO,PCINT4,D12)      RFM12 SDO
// JP3D           (PCINT22,D6) PD6 12|    |17  PB3 (MOSI,OC2A,PCINT3,D11) RFM12 SDI
// JP4D           (PCINT23,D7) PD7 13|    |16  PB2 (SS,OSC1B,PCINT2,D10)  RFM12 SEL
//                 (PCINT0,D8) PB0 14|    |15  PB1 (OC1,PCINT1,D9)
//                                   +----+
#include <Ports.h>
#include <PortsSHT11.h>
#include <RF12.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <avr/io.h>

#include <util/atomic.h>

#define SERIAL  0   // set to 1 to also report readings on the serial port
#define DEBUG   0   // set to 1 to display each loop() run and PIR trigger

// SHT11 & ONE_WIRE are mutually exclusive.
// for One WIRE we refer to the ARDUINO (D 4) PIN 6
// For the other we use JeeNode PORT mapping.... yeah confusing.

#define ONE_WIRE_PIN  4   // PD4 - defined in a OneWire sensor is connected to JeePort 1.
#define ONE_WIRE_PIN2 5   // PD5 - defined in a OneWire sensor is connected to JeePort 2.
//#define SHT11_PORT  1   // defined if SHT11 is connected to a port
#define LDR_PORT    4   // defined if LDR is connected to a port's AIO pin
#define PIR_PORT    4   // defined if PIR is connected to a port's DIO pin
// If you change this pin adjust the ISR() vector

#define MEASURE_PERIOD  300 // how often to measure, in tenths of seconds
#define RETRY_PERIOD    10  // how soon to retry if ACK didn't come in
#define RETRY_LIMIT     5   // maximum number of times to retry
#define ACK_TIME        10  // number of milliseconds to wait for an ack
#define REPORT_EVERY    5   // report every N measurement cycles
#define SMOOTH          3   // smoothing factor used for running averages

// set the sync mode to 2 if the fuses are still the Arduino default
// mode 3 (full powerdown) can only be used with 258 CK startup fuses
#define RADIO_SYNC_MODE 2

// The scheduler makes it easy to perform various tasks at various times:

enum { 
  MEASURE, REPORT, TASK_END };

static word schedbuf[TASK_END];
Scheduler scheduler (schedbuf, TASK_END);

// Other variables used in various places in the code:

static byte reportCount;    // count up until next report, i.e. packet send
static byte myNodeID;       // node ID used for this unit

// This defines the structure of the packets which get sent out by wireless:

struct {
  byte light;     // light sensor: 0..255
byte moved :
  1;  // motion detector: 0..1
byte humi  :
  7;  // humidity: 0..100
int temp   :
  10; // temperature: -500..+500 (tenths)
byte lobat :
  1;  // supply voltage dropped under 3.1V: 0..1
int temp2   :   10; // temperature: -500..+500 (tenths)
}
payload;

// Conditional code, depending on which sensors are connected and how:

#if ONE_WIRE_PIN
#define TEMPERATURE_PRECISION 12

#include <OneWire.h>
#include <DallasTemperature.h>
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature sensors(&oneWire);
// Pass our oneWire reference to Dallas Temperature. 
DeviceAddress deviceAddress;
#endif

// A second temperature sensor on PORT 2
#if ONE_WIRE_PIN2
OneWire oneWire2(ONE_WIRE_PIN2);
DallasTemperature sensors2(&oneWire2);
DeviceAddress deviceAddress2;
#endif

#if SHT11_PORT
SHT11 sht11 (SHT11_PORT);
#endif

#if LDR_PORT
Port ldr (LDR_PORT);
#endif

#if PIR_PORT
#define PIR_HOLD_TIME   30  // hold PIR value this many seconds after change
#define PIR_PULLUP      1   // set to one to pull-up the PIR input pin
#define PIR_FLIP        1   // 0 or 1, to match PIR reporting high or low

class PIR : 
public Port {
  volatile byte value, changed;
  volatile uint32_t lastOn;
public:
  PIR (byte portnum)
: 
    Port (portnum), value (0), changed (0), lastOn (0) {
    }

  // this code is called from the pin-change interrupt handler
  void poll() {
    // see http://talk.jeelabs.net/topic/811#post-4734 for PIR_FLIP
    byte pin = digiRead() ^ PIR_FLIP;
    // if the pin just went on, then set the changed flag to report it
    if (pin) {
      if (!state())
        changed = 1;
      lastOn = millis();
    }
    value = pin;
  }

  // state is true if curr value is still on or if it was on recently
  byte state() const {
    byte f = value;
    if (lastOn > 0)
      ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        if (millis() - lastOn < 1000 * PIR_HOLD_TIME)
          f = 1;
      }
    return f;
  }

  // return true if there is new motion to report
  byte triggered() {
    byte f = changed;
    changed = 0;
    return f;
  }
};

PIR pir (PIR_PORT);

// the PIR signal comes in via a pin-change interrupt
ISR(PCINT2_vect) {
  pir.poll(); 
}
#endif

// has to be defined because we're using the watchdog for low-power waiting
ISR(WDT_vect) { 
  Sleepy::watchdogEvent(); 
}

// utility code to perform simple smoothing as a running average
static int smoothedAverage(int prev, int next, byte firstTime =0) {
  if (firstTime)
    return next;
  return ((SMOOTH - 1) * prev + next + SMOOTH / 2) / SMOOTH;
}

// spend a little time in power down mode while the SHT11 does a measurement
static void shtDelay () {
  Sleepy::loseSomeTime(32); // must wait at least 20 ms
}

// wait a few milliseconds for proper ACK to me, return true if indeed received
static byte waitForAck() {
  MilliTimer ackTimer;
  while (!ackTimer.poll(ACK_TIME)) {
    if (rf12_recvDone() && rf12_crc == 0 &&
      // see http://talk.jeelabs.net/topic/811#post-4712
    rf12_hdr == (RF12_HDR_DST | RF12_HDR_CTL | myNodeID))
      return 1;
    set_sleep_mode(SLEEP_MODE_IDLE);
    sleep_mode();
  }
  return 0;
}

// readout all the sensors and other values
static void doMeasure() {
  byte firstTime = payload.humi == 0; // special case to init running avg

  payload.lobat = rf12_lowbat();

#if SHT11_PORT
#ifndef __AVR_ATtiny84__
  sht11.measure(SHT11::HUMI, shtDelay);        
  sht11.measure(SHT11::TEMP, shtDelay);
  float h, t;
  sht11.calculate(h, t);
  int humi = h + 0.5, temp = 10 * t + 0.5;
#else
  //XXX TINY!
  int humi = 50, temp = 25;
#endif
  payload.humi = smoothedAverage(payload.humi, humi, firstTime);
  payload.temp = smoothedAverage(payload.temp, temp, firstTime);
#endif
#if LDR_PORT
  ldr.digiWrite2(1);  // enable AIO pull-up
  byte light = ~ ldr.anaRead() >> 2;
  ldr.digiWrite2(0);  // disable pull-up to reduce current draw
  payload.light = smoothedAverage(payload.light, light, firstTime);
#endif
#if PIR_PORT
  payload.moved = pir.state();
#endif

#if ONE_WIRE_PIN
  sensors.requestTemperatures();
  payload.temp = sensors.getTempC(deviceAddress) * 10;
#endif
#if ONE_WIRE_PIN2
  sensors2.requestTemperatures();
  payload.temp2 = sensors2.getTempC(deviceAddress2) * 10;
#endif
}

// periodic report, i.e. send out a packet and optionally report on serial port
static void doReport() { 
  // the RF12 runs on INT0 this interrupt is on PCINT18 the same as port PD as
  // the PIR (PCINT23) so when the RF12 is being used the entire PORT gets a PIN
  // CHANGE event PCIFR.
  // We need to capture this and mask it off so the PIR does not fire again.
  
  PCICR &= ~(1 << PCIE2);  // disable the PD pin-change interrupt.     

  rf12_sleep(RF12_WAKEUP);
  while (!rf12_canSend())
    rf12_recvDone();
  rf12_sendStart(0, &payload, sizeof payload, RADIO_SYNC_MODE);
  rf12_sleep(RF12_SLEEP);

  // When a logic change on any PCINT23:16 pin triggers an interrupt request, 
  // PCIF2 becomes set(one). If the I-bit in SREG and the PCIE2 bit in PCICR 
  // are set (one), the MCU will jump to thecorresponding Interrupt Vector. 
  // The flag is cleared when the interrupt routine is executed. Alternatively,
  // the flag can be cleared by writing a logical one to it.
  PCIFR = (1<<PCIF2);   //clear any pending interupt

  PCICR |= (1<<PCIE2);   // and re-enable the PIR interrupt

#if SERIAL
  Serial.print("ROOM ");
  Serial.print((unsigned int) payload.light);
  Serial.print(' ');
  Serial.print((unsigned int) payload.moved);
  Serial.print(' ');
  Serial.print((unsigned int) payload.humi);
  Serial.print(' ');
  Serial.print((unsigned int) payload.temp);
  Serial.print(' ');
  Serial.print((unsigned int) payload.lobat);
  Serial.print(' ');
  Serial.print((unsigned int) payload.temp2);
  Serial.println();
  delay(2); // make sure tx buf is empty before going back to sleep
#endif
}

// send packet and wait for ack when there is a motion trigger
static void doTrigger() {
#if DEBUG
  Serial.print("PIR ");
  Serial.print((int) payload.moved);
  delay(2);
#endif

  for (byte i = 0; i < RETRY_LIMIT; ++i) {
    rf12_sleep(RF12_WAKEUP);
    while (!rf12_canSend())
      rf12_recvDone();
    rf12_sendStart(RF12_HDR_ACK, &payload, sizeof payload, RADIO_SYNC_MODE);
    byte acked = waitForAck();
    rf12_sleep(RF12_SLEEP);

    if (acked) {
#if DEBUG
      Serial.print(" ack ");
      Serial.println((int) i);
      delay(2);
#endif
      // reset scheduling to start a fresh measurement cycle
      scheduler.timer(MEASURE, MEASURE_PERIOD);
      return;
    }

    Sleepy::loseSomeTime(RETRY_PERIOD * 100);
  }
  scheduler.timer(MEASURE, MEASURE_PERIOD);
#if DEBUG
  Serial.println(" no ack!");
  delay(2);
#endif
}

void setup () {
#if SERIAL || DEBUG
  Serial.begin(57600);
  Serial.print("\n[roomNode.5]");
  myNodeID = rf12_config();
#else
  myNodeID = rf12_config(0); // don't report info on the serial port
#endif

  rf12_sleep(RF12_SLEEP); // power down

#if PIR_PORT
  pir.digiWrite(PIR_PULLUP);
#ifdef PCMSK2
  bitSet(PCMSK2, PIR_PORT + 3);
  bitSet(PCICR, PCIE2);
#else
  //XXX TINY!
#endif
#endif

#if ONE_WIRE_PIN
  sensors.begin();
#if SERIAL || DEBUG
  Serial.print("Locating OneWire devices...");
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");
#endif

  if (sensors.getAddress(deviceAddress, 0)) {
    sensors.setResolution(deviceAddress, TEMPERATURE_PRECISION);
  } 
  else {
#if SERIAL || DEBUG    
    Serial.println("Unable to find address for OneWire device");
#endif
  }
#endif

#if ONE_WIRE_PIN2
  sensors2.begin();
#if SERIAL || DEBUG
  Serial.print("Locating OneWire2 devices...");
  Serial.print("Found ");
  Serial.print(sensors2.getDeviceCount(), DEC);
  Serial.println(" devices.");
#endif

  if (sensors2.getAddress(deviceAddress2, 0)) {
    sensors2.setResolution(deviceAddress2, TEMPERATURE_PRECISION);
  } 
  else {
#if SERIAL || DEBUG    
    Serial.println("Unable to find address for OneWire2 device");
#endif
  }
#endif

  reportCount = REPORT_EVERY;     // report right away for easy debugging
  scheduler.timer(MEASURE, 0);    // start the measurement loop going
}

void loop () {
#if DEBUG
  Serial.print('.');
  delay(2);
#endif

#if PIR_PORT
  if (pir.triggered()) {
    payload.moved = pir.state();
    doTrigger();
  }
#endif

  switch (scheduler.pollWaiting()) {

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


