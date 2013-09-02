/* Ethernet enable relays with a nice Web User interface
 * Uses the dbzoo EthRelay v1.0 board.
*/
#include <arduino.h>
#include <EtherCard.h>

/* CONNECTIONS:
 ETHERNET MODULE       ARDUINO BOARD
 PIN 1  (CLK OUT)      N/A
 PIN 2  (INT)          N/A
 PIN 3  (WOL)          N/A
 PIN 4  (SO)           DIO 12
 PIN 5  (SI)           DIO 11
 PIN 6  (SCK)          DIO 13
 PIN 7  (CS)           DIO 10
 PIN 8  (RES)          N/A
 PIN 9  (VCC)          +3.3V
 PIN 10 (GND)          GND
 */
#define CS_PIN  10   // By default EtherCard uses 8

byte Ethernet::buffer[1000];

// Must be PORTD pins
#define RELAY1_DIO 2
#define RELAY2_DIO 3

static uint8_t RelayPins[2] = {
  RELAY1_DIO, RELAY2_DIO};  // Pins to drive relays
uint8_t RelayState = 0;

static uint8_t mymac[6] = {
  0xde, 0xad, 0xbe, 0xef, 0x00, 0x24};  
#define STATIC 0 // set to 1 to disable DHCP (adjust myip/gwip values below)

#if STATIC
static byte myip[] = { 
  192,168,1,119 };  // ethernet interface ip address
static byte gwip[] = { 
  192,168,1,1 };    // gateway ip address
#endif

static BufferFiller bfill;  

static int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

/* Initialise the ethernet interface and DIO pins */
void setup()
{   
 // Reset relays to OFF - float high.
 for(int i=0; i<sizeof(RelayPins); i++) {
    pinMode(RelayPins[i], OUTPUT);
    digitalWrite(RelayPins[i], HIGH);
 }

  // Until we initialize - there is nothing we can do.
  // Would be nice to flash a led on error here.
  while(ether.begin(sizeof Ethernet::buffer, mymac, CS_PIN) == 0);

#if STATIC
  ether.staticSetup(myip, gwip);
#else
  // Would be nice to flash a led on error here too.
  while(!ether.dhcpSetup());
#endif
}

boolean checkUrl(const __FlashStringHelper *val, const char* data) {
  const char PROGMEM *p = (const char PROGMEM *)val;
  while (1) {
    char c = pgm_read_byte(p++);
    if (c == 0) break;
    if (*data != c) return false;
    data++;
  }
  return true;
}

void writeHeaders(BufferFiller& buf) {
  buf.print(F("HTTP/1.0 200 OK\r\nPragma: no-cache\r\n"));
}

void homePage(BufferFiller& buf) {
  writeHeaders(buf);
  buf.println(F("Content-Type: text/html\r\n"));
  buf.print(F(
  "<!DOCTYPE html><html><head>"
    "<meta name='viewport' content='width=device-width, initial-scale=1' />"
    "<title>RelayServer</title>"
    "<link rel='stylesheet' href='//code.jquery.com/mobile/1.2.0/jquery.mobile-1.2.0.min.css' />"
    "<link rel='stylesheet' href='main.css' />"
    "<script src='//code.jquery.com/jquery-1.8.2.min.js'></script>"
    "<script src='//code.jquery.com/mobile/1.2.0/jquery.mobile-1.2.0.min.js'></script>"
    "<script src='main.js'></script>"
    "</head><body><div id='main' data-role='page'>"
    "<div data-role='header' data-position='fixed'><h3>Web Relay</h3></div>"
    "<div data-role='content'>"
    "<ul id='list' data-role='listview' data-inset='true'></ul>"
    "<p id='info'></p></div></div></body></html>"));
}

void mainCss(BufferFiller& buf) {
  writeHeaders(buf);
  buf.println(F("Content-Type: text/css\r\n"));
  buf.print(F(
  ".ui-li-aside{font-weight:bold;font-size:xx-large;}"
    "#info{margin-top:10px;text-align:center;font-size:small;}"));
}

void mainJs(BufferFiller& buf) {
  writeHeaders(buf);
  buf.println(F("Content-Type: application/javascript\r\n"));
  // Make sure this JS code isn't too big it must fit in 'buf' and a TCP frame.
  buf.print(F(
  "function pad(n){ if(n<10) return '0'+n; return n}"
  "function ms2tm(ms) {"
    "var m=Math.floor;"
    "var sec = ms/1000;"
    "ms = m(ms%1000);"
    "var mins = sec/60;"
    "sec = m(sec%60);"
    "var hrs = mins/60;"
    "mins = m(mins%60);"
    "var dys = hrs/24;"
    "hrs = m(hrs%24);"
    "dys = m(dys%24);"
    "return dys+\"d \"+pad(hrs)+':'+pad(mins)+':'+pad(sec)+'.'+ms;"
  "}"
  "$(document).ready(function(){reload()});"
  "function reload(){"
    "$.getJSON('list.json',function(c){"
      "var d=[];"
      "$.each(c.list,function(a,b){d.push('<li id=\"'+b.id+'\"><a><h3>'+b.name+'</h3>"
      "<p class=\"ui-li-aside\">'+b.val+'</p></a></li>')});"
      "$('#list').html(d.join('')).trigger('create').listview('refresh');"
      "$('.ui-li').click(function(){"
        "var id=$(this).attr('id');"
        "if(typeof(id) != \"undefined\"){$.get('/relay'+id); reload();}}"
      ");" // .click
      "$('#info').html('Uptime: '+ms2tm(c.uptime)+' ('+c.free+' bytes free)')"
    "});" // getJSON
  "}" //reload
  ));    
}

void listJson(BufferFiller& buf) {
  writeHeaders(buf);
  buf.println(F("Content-Type: application/json\r\n"));
  buf.print(F("{\"list\":["));
  for(int i=0; i<sizeof(RelayPins); i++) {
    if (i) buf.write(',');
    buf.emit_p(PSTR("{\"id\":\"$D\",\"name\":\"Relay $D\",\"val\":\"$S\"}"),
    i, i+1, bitRead(RelayState,i) ? "on" : "off");
  }
  buf.emit_p(PSTR("],\"uptime\":$L,\"free\":$D}"), millis(), freeRam());
}

void changeRelayState(int relay, BufferFiller& buf) { 
  PIND = _BV(RelayPins[relay]); // Toggle the relay.   
  RelayState ^= 1 << relay;     // Toggle state too.
  writeHeaders(bfill);
}

void loop()
{
  word len = ether.packetReceive();
  word pos = ether.packetLoop(len);
  if (pos) {
    bfill = ether.tcpOffset();
    char *data = (char *)Ethernet::buffer + pos;

    if (checkUrl(F("GET / "), data))
      homePage(bfill);
    else if (checkUrl(F("GET /main.css "), data))
      mainCss(bfill);
    else if (checkUrl(F("GET /main.js "), data))
      mainJs(bfill);
    else if (checkUrl(F("GET /list.json "), data))
      listJson(bfill);  
    else if (checkUrl(F("GET /relay0 "), data))
      changeRelayState(0, bfill);
    else if (checkUrl(F("GET /relay1 "), data))
      changeRelayState(1, bfill);    
    else
      bfill.print(F(
      "HTTP/1.0 404 Not Found\r\n"
        "Content-Type: text/html\r\n"
        "\r\n"
        "<h1>404 Not Found</h1>"));  
    ether.httpServerReply(bfill.position()); // send web page data
  }
}

