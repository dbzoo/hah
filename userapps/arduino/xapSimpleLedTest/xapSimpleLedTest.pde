/* $Id$ 
 * Test harness to send an framed XAP event down the serial port
 * to an Arduino that will unframe and interpret the XAP payload.
 *
 * http://processing.org/
 */
import processing.serial.*;

Serial myPort;

int STX = 2;
int ETX = 3;

boolean led = false;
color off = color(255,0,0);
color on = color(0,255,0);
boolean circleOver=false;
color currentColor=off;
int circleX, circleY;  // Position of circle button
int circleSize = 50;   // Diameter of circle

void setup() {
  size(200,200);
  smooth();
  String portName = Serial.list()[0];
  println("Writing to port: "+portName);
  myPort = new Serial(this, portName, 38400);
  circleX = width/2;
  circleY = height/2;
  ellipseMode(CENTER);
}

void xapLed() {
  String out = "x"+char(STX)+"xAP-header\n"+
    "{\n"+
    "v=12\n"+
    "hop=1\n"+
    "uid=FF00DC00\n"+
    "class=xAPBSC.cmd\n"+
    "target=dbzoo.arduino.demo:led\n"+
    "source=dbzoo.test.demo\n"+
    "}\n"+
    "output.state.1\n"+
    "{\n"+
    "state="+(led ? "on" : "off")+"\n"+
    "}----"+char(ETX);
  myPort.write(out);
  print(out);
}

void draw() {
  update(mouseX, mouseY);
  background(0);
  fill(255);
  text("XAP Arduino test harness",10,10);
  fill(currentColor);
  stroke(0);
  ellipse(circleX, circleY, circleSize, circleSize);
}

void update(int x, int y)
{
  circleOver = overCircle(circleX, circleY, circleSize);
}

void mousePressed()
{
  if(circleOver) {
    led = !led;
    currentColor = led ? on : off;
    xapLed();
  }
}

boolean overCircle(int x, int y, int diameter) 
{
  float disX = x - mouseX;
  float disY = y - mouseY;
  if(sqrt(sq(disX) + sq(disY)) < diameter/2 ) {
    return true;
  } else {
    return false;
  }
}
