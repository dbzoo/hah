// This is a demo of an Xap receiver node with a WEB Server
// $Id$

#include <EtherCard.h>
#include <xAPEther.h>

// ethernet interface mac address
static byte mymac[6] = { 
  0x54,0x55,0x58,0x10,0x00,0x26 };

// ethernet interface ip address
static byte myip[4] = { 
  192,168,1,15 };

// listen port for tcp/www:
#define HTTP_PORT 80

static byte buf[500];
static BufferFiller bfill;

// XAP Identification
#define UID "FFDB5500"
#define SOURCE "dbzoo.arduino.demo"
#define ENDPOINT SOURCE":print"

EtherCard eth;
XapEther xap(SOURCE, UID);

void setup () {
  eth.spiInit();
  eth.initialize(mymac);
  eth.initIp(mymac, myip, HTTP_PORT);
  Serial.begin(115200);
}

static void homePage() {
  long t = millis() / 1000;
  word h = t / 3600;
  byte m = (t / 60) % 60;
  byte s = t % 60;
  bfill.emit_p(PSTR(
  "HTTP/1.0 200 OK\r\n"
    "Content-Type: text/html\r\n"
    "Pragma: no-cache\r\n"
    "\r\n"
    "<meta http-equiv='refresh' content='1'/>"
    "<title>XAP node</title>" 
    "<h1>Uptime: $D$D:$D$D:$D$D</h1>"), h/10, h%10, m/10, m%10, s/10, s%10);
}

void processXap() {
  if(xap.getType() != XAP_MSG_ORDINARY) return;
  if(!xap.isValue("xap-header","target", ENDPOINT)) return;
  Serial.println("Got xAP packet");
  //xap.dumpParsedMsg();
}

void loop () {
  word len = eth.packetReceive(buf, sizeof buf);  
  // ENC28J60 loop runner: handle ping and wait for a tcp packet
  word pos = eth.packetLoop(buf,len);  
  if (pos) {  // check if valid www data is received
    bfill = eth.tcpOffset(buf);
    homePage();
    eth.httpServerReply(buf,bfill.position()); // send web page data
  }
  xap.processPacket(buf, len, processXap);
}

