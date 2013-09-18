--[[
   Turns on a relay based on the sunrise/sunset
   requires the sunForecastApplet.lua to be running.
--]]

module(...,package.seeall)
require("xap")
require("xap.bsc")

info = {version="0.03", description="Automatic Lights"}
relay = "dbzoo.livebox.Controller:relay.1"
latch=nil

function checkLightState(frame)
   -- We don't know the relays current state (wait)
   if latch == nil then return end

   local daylight = frame:getValue("forecast","daylight")

   if latch then
      -- relay is ON -- keep it on whilst there is no daylight
      state = daylight == "no"
   else  
      -- relay is OFF -- keep it off whilst there is daylight
      state = daylight == "yes"
   end

   if state ~= latch then
      bsc.sendState(relay, state and "on" or "off")
   end
end

function latchRelay(frame)
  latch = frame:isValue("output.state","state","on")
end

function init()
   -- When we get a sun forecast check the relay state
   xap.Filter{["xap-header"]={class="sun.forecast"}}:callback(checkLightState)

   -- Make sure we always know the relays current state
   -- Captures both xAPBSC.info and xAPBSC.event class methods
   xap.Filter{["xap-header"]={source=relay}}:callback(latchRelay)
end
