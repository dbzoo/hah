-- This program is a Gateway between a xAP client that subscribe via a
-- mqtt broker and those that publish their messages via the iServer
-- or directly as UDP packets.  It operates by subscribing to the mqtt
-- broker and listening for all topics being published.  As each
-- message via the broker is an xAP packet it picks up the
-- xap-hbeat/xap-header and extracts the source= tag.  From this it
-- can create an xAP FILTER that allows it to redirect xAP messages to
-- the broker.
module(...,package.seeall)
require("xap")
MQTT = require("mqtt_library")

info={version="1.0", description="xAP/MQTT gateway"}
-- nil port means use the default
broker={host="192.168.1.8",port=nil} --mqtt broker location
subscribedTopics={}

function xAPAddrToTopic(addr)
   -- convert a.b.c:d.e -> a/b/c/d/e
   return addr:gsub("[%.:]","/")
end

function bumpHop(f, section)
   if f[section] == nil then
      return
   end
   if f[section]['hop'] then
      f[section].hop = f[section].hop + 1
   else
      f[section].hop = 2
   end
end

function toMqtt(frame)
   -- rebadge the message as coming from the GW.
   -- we use this fact later to ignore messages from ourself.
   frame:setValue("xap-header", "source", xap.defaultKeys.source)
   bumpHop(frame)

   local topic = xAPAddrToTopic(frame:getValue('xap-header','target'))
   broker.c:publish(topic ,tostring(frame))
end

function fromMqtt(topic, payload)
   local f = xap.Frame(payload)
   -- This happens as we subscribe to # so we get our own publish.
   if f:getValue("xap-header","source") == xap.defaultKeys.source then
      return -- Message was from us to the MQTT
   end

   if subscribedTopics[topic] == nil then
      local source = f:getValue("xap-header","source")
      if source  == nil then 
	 source = f:getValue("xap-hbeat","source")
      end      
      local flt = xap.Filter()
      flt:add("xap-header","target",source)
      flt:callback(toMqtt)
      subscribedTopics[topic]=true
   end
 

   if f['xap-header'] then
      -- sendShort only for xap-header messages
      bumpHop(f,'xap-header')
      xap.sendShort(tostring(f))
   else
      -- expect the xap-hbeat to be fully formed
      bumpHop(f,'xap-hbeat')
      xap.send(tostring(f))
   end
end

function init()
   broker.c = MQTT.client.create(broker.host, broker.port, fromMqtt)
   broker.c:connect(xap.defaultKeys.source)
   -- As long as we don't have a lot of topics and high volumes we
   -- should be able to keep up with filtering and forwarding.
   -- I know subscribing to # is not recommended.  :(
   broker.c:subscribe({"#"})
   -- pump the mqtt client every second
   xap.Timer(function() broker.c:handler() end, 1):start()
end

--[[
-- For standalone testing/opertion.
xap.init{instance="gateway",uid="FF00DD00"}
init()
xap.process()
--]]
