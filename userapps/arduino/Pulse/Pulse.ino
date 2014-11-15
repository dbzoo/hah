// Useful for testing the PulseCounter Sketch.
int ledPin = 5;                 // LED connected to digital pin 13

void setup()
{
  pinMode(ledPin, OUTPUT);      // sets the digital pin as output
}

void loop()
{
  digitalWrite(ledPin, HIGH);   // sets the LED on
  delay(10);                  // waits for 10ms
  digitalWrite(ledPin, LOW);    // sets the LED off
  delay(1000);                  // waits for 1000ms
}
