// this sketch is for tweaking soft i2c signals

#include <SoftI2cMaster.h>

#define SCL_PIN 7
#define SDA_PIN 8

SoftI2cMaster i2c;

// also test base class idea
TwoWireBase *bus;

uint8_t mode;

void setup(void)
{
  Serial.begin(9600);
  i2c.init(SCL_PIN, SDA_PIN);
  
  // convert softI2cMaster to TwoWireBase to test base class idea
  bus = &i2c;
  
  Serial.println("enter 0 for write, any other for read");
  while (!Serial.available());
  mode = Serial.read();
  if (mode == '0') {
    Serial.println("Write Mode");
  }
  else {
    Serial.println("Read Mode");
  }
}

void loop(void)
{
  if (mode == '0') {
    bus->write(0X55);
  }
  else {
    bus->read(0);
  }
  delay(1);
}
