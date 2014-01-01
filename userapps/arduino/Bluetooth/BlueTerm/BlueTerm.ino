/* $Id$

Serial terminal for the HC-05 bluetooth module.
Sets up BT module and allow user interaction with the device via the Arduino Serial Monitor.

Wiring of BT HC-05 device to the Arduino
RX -> 11
TX -> 10
KEY -> 9
*/

#include <SoftwareSerial.h>

// Bluetooth to Arduino pins.
#define BTkey 9
#define BTrxPin 10
#define BTtxPin 11

enum { SYNC, ASYNC };

SoftwareSerial BTSerial(BTrxPin, BTtxPin);

// Send a command to the bluetooth device.
// mode ASYNC - don't want for the response
// mode SYNC - wait for the response
void sendBluetoothCommand(String cmd, uint8_t mode) {
  Serial.print(mode == SYNC ? "SYNC> " : "ASYNC> ");
  Serial.println(cmd);

  BTSerial.println(cmd);
  if (mode == ASYNC) return;
  
  char ch = 0;
  while (ch != '\n') {
    if (BTSerial.available()) {
      ch = BTSerial.read();
       Serial.write(ch);
    }
  }
}

void setup()
{
   Serial.begin(38400);
   
  /////////////////////////////////////////////////////////
  // SETUP BLUETOOTH  
  // define pin modes for tx, rx:
  pinMode(BTrxPin, INPUT);
  pinMode(BTtxPin, OUTPUT);
  // Switch module to AT mode
  pinMode(BTkey, OUTPUT);
  digitalWrite(BTkey, HIGH);  
  BTSerial.begin(38400);
  sendBluetoothCommand("AT", SYNC);
  sendBluetoothCommand("AT+NAME=blueNode", SYNC);
  // AT+ROLE=p1
  // p1 - 0 Slave Role, 1 Master Role, 2 Slave-Loop role.
  sendBluetoothCommand("AT+ROLE=1", SYNC);
  // AT+INQM=p1,p2,p3
  // p1 - 0 inquire_mode_standad, 1 inquire_mode_rssi
  // p2 - maximum number of bluetooth devices response
  // p3 - the maximum of limited inquiring time. 1~48 (1.28s ~ 61.44s)
  //      15*1.28 = 19sec 
  sendBluetoothCommand("AT+INQM=0,5,15", SYNC);
}

void loop()
{
  // Keep reading from HC-05 and send to Arduino Serial Monitor
  if (BTSerial.available())
    Serial.write(BTSerial.read());

  // Keep reading from Arduino Serial Monitor and send to HC-05
  if (Serial.available())
    BTSerial.write(Serial.read());
}
