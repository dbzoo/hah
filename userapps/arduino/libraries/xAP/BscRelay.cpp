#include "BscRelay.h"

static BufferFiller bfill;

BscRelay::BscRelay(XapEther& _xap, uint8_t _id, uint8_t _pin) : 
xap(_xap) {
  pin = _pin;  
  pinMode(pin, OUTPUT);
  digitalWrite(pin, HIGH); // off - path to GND (LOW) on
  state = 0; // relay off

  // xap.UID will be something like FF123400
  // We set the last 2 digits based on our id to get
  // a unique UID for this endpoint.
  strcpy(UID, xap.UID);
  _id %= 99; // 3 digits will buffer overflow us (wrap).
  sprintf(UID+6, "%02d", _id);

  heartbeatTimeout = smillis();
  snprintf(source, sizeof(source), "%s:relay.%d", xap.SOURCE, _id);
  id = _id;
}

int BscRelay::after(long timeout)
{
  return smillis()-timeout > 0;
}

void BscRelay::resetHeartbeat() {
  heartbeatTimeout = smillis() + INFO_HEARTBEAT;
}

void BscRelay::heartbeat() {
  if (after(heartbeatTimeout)) {
    sendInfoEvent(BSC_INFO_CLASS);
  }
}

void BscRelay::sendInfoEvent(char *clazz) {
  bfill = ether.buffer + UDP_DATA_P;
  bfill.emit_p(PSTR("xap-header\n"
    "{\n"
    "v=12\n"
    "hop=1\n"
    "uid=$S\n"
    "class=$S\n"
    "source=$S\n"
    "}\n"
    "output.state\n"
    "{\n"
    "state=$S\n"
    "}\n"
    ), UID, clazz, source, state ? "on" : "off");
  xap.broadcastUDP(bfill.position());  
  resetHeartbeat();
}
/// Decode allowable values for a "state=value" key pair.
int bscDecodeState(char *msg)
{
  int i;
  static char *value[] = {
    "on","off","true","false","yes","no","1","0","toggle"  };
  static int state[] = {
    1,0,1,0,1,0,1,0,2  };
  if (msg == NULL)
    return -1;
  for(i=0; i < sizeof(value)/sizeof(value[0]); i++) {
    if(strcasecmp(msg, value[i]) == 0)
      return state[i];
  }
  return -1;
}

boolean BscRelay::filter(char *section, char *k, char *kv) {
  char *value = xap.getValue(section, k);
  if (value == NULL) { // no section key match must fail.
    return 0;
  } 
  return strcasecmp(value, kv) == 0 ? 1 : 0;  
}

void BscRelay::cmd() {
  char *cmdState = xap.getValue("output.state.1","state");
  if(cmdState == NULL) return; 
  byte istate = bscDecodeState(cmdState);  
  if(istate == -1) return; // Error: bad status= value

  char *sid = xap.getValue("output.state.1","id");
  if(sid == NULL) return;
  if(*sid == '*' || atoi(sid) == id) {
    if(istate == 2) istate = (state ^ 0x1); // toggle

    // If not in this state already
    if(state != istate) {
      state = istate;
      PIND = _BV(pin); // Toggle the relay.
      sendInfoEvent(BSC_EVENT_CLASS);
    }
  }
}

/** Process an xAP message 
 * Is this for us?
 * If so there are only two we can handle.
 */
void BscRelay::process() {
  if(filter("xap-header","class", BSC_CMD_CLASS) &&
    filter("xap-header","target", source)) 
  {
    cmd();
  } 
  else if(filter("xap-header","class", BSC_QUERY_CLASS) &&
    filter("xap-header","target", source)) 
  {
    sendInfoEvent(BSC_INFO_CLASS);
  }
}






