// $Id$
// PulseCounter - for measuring electricty usage.
// Based on: http://sandeen.net/wordpress/energy/energy-monitor-proof-of-concept/
//
// Use interrupt to detect change on LDR
// So we know what's up, light up the built-in LED
//
// Wiring
// PD3 --- LDR --- GND
//
// PD7 ---/\/\/\---LED--- GND
//          330
//
// PINS wired to suit a HAH Node

// PIN DEFINITIONS - HAH Node Port
int led_pin = 7;               // PD7 - Port4 DIO
int int1_pin = 3;              // PD3 - Port1-4 IRQ

// Delays
int bounce_delay_ms = 50;     // pulse is 10ms wide
int blink_delay_ms = 50;      // LED on time; delays loop()

unsigned long counter = 0;      // Nr of pulses seen
unsigned long first_pulse_time = 0;
unsigned long last_pulse_time = 0;

// These are modified in the interrupt
volatile int pulse_seen = 0;   // Pulse was seen in interrupt

static unsigned long ms_per_hour = 3600000UL;  // ms per hour for power calcs

void setup()
{
 pinMode(led_pin, OUTPUT);

 // We want internal pullup enabled:
 pinMode(int1_pin, INPUT);
 digitalWrite(int1_pin, HIGH);  // Turns on pullup resistors

 // And set up the interrupt.
 // RISING: -pin- goes from low to high
 // FALLING: -pin- goes from high to low
 attachInterrupt(1, blink, FALLING);
 // initialize serial communication:
 Serial.begin(9600);
 Serial.println("[PulseCounter]");
}

void loop()
{
 static unsigned long avg_watts;        // average watt draw since startup
 unsigned long delta;            // Time since last interrupt
 int incoming_byte;

 // Query exists on serial port; service it.
 if (Serial.available() > 0) {
   incoming_byte = Serial.read();
   Serial.flush();
   switch (incoming_byte) {
   case 'c':
     // Just print the count since last query
     Serial.println(counter);
     counter = 0;
     break;
   case 'p':
     // print the avg watt draw since last query
     delta = last_pulse_time - first_pulse_time;
     if (delta && counter) {
       avg_watts = (counter - 1) * ms_per_hour / delta;
       counter = 0;
     }
     Serial.println(avg_watts);
     break;
   default:
     break;
   }
 }

 // IR pulse interrupt happened
 if (pulse_seen == 1) {
   counter++;

   last_pulse_time = millis();

   if (counter == 1 || first_pulse_time == 0) // we were reset
     first_pulse_time = last_pulse_time;

   // Blinky-blink
   digitalWrite(led_pin, HIGH);
   delay(blink_delay_ms);
   digitalWrite(led_pin, LOW);
   // Reset until next one.
   pulse_seen = 0;
 }
}

// Interrupt handler
// Whenever we get an IR state change our LED should change too.
//
// We get here for RISING; so when IR turns on.
//
void blink()
{
 static unsigned long last_interrupt_time = 0;
 unsigned long interrupt_time = millis();

 // If interrupts come faster than bounce_delay_ms, assume it's a bounce and ignore
 //
 // Note, millis() wraps every 50 days, be sure we cope...
 //
 if (interrupt_time - last_interrupt_time > bounce_delay_ms)
   pulse_seen = 1;  // loop() sees pulse == 1 and takes action

 last_interrupt_time = interrupt_time;
}
