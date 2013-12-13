#ifndef BSCRELAY_H
#define BSCRELAY_H

#include "xAPEther.h"

#define BSC_INFO_CLASS "xAPBSC.info"
#define BSC_EVENT_CLASS "xAPBSC.event"
#define BSC_CMD_CLASS "xAPBSC.cmd"
#define BSC_QUERY_CLASS "xAPBSC.query"

// Milliseconds.
#define INFO_HEARTBEAT 120000

class BscRelay
{
 public:
  BscRelay(XapEther& xap, uint8_t id, uint8_t pin);
  void process();
  void heartbeat();
  
 private:
   void cmd();
   int after(long);                    // Useful for managing TIMED events such as heartbeats and info
   void sendInfoEvent(char *clazz);
   void resetHeartbeat();
   boolean filter(char *section, char *k, char *kv);
   char UID[9];

   uint8_t pin;
   uint8_t id;
   long heartbeatTimeout;
   char source[30];
   XapEther &xap;
   byte state;
};
#endif
