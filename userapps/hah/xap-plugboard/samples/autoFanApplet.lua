--[[
  Control a fan based on a temperature sensor
  
  When the temperature gets about 33c turn on relay4 which controls an exhaust fan
  run the fan until the temperature drop to 26c or the fan has run for 5 minutes.
--]]
module(...,package.seeall)
require("xap")
info = {version="0.03", description="Auto Fan"}

local sensor = "dbzoo.livebox.Controller:1wire.1"
local fan = "dbzoo.livebox.Controller:relay.4"
local autoShut = nil
local prevTemp = 0

function fanOff()
  bsc.sendState(fan, "off")
  if autoShut ~= nil then
    autoShut:delete()
    autoShut = nil
  end
end

function tempEvent(frame)
  temp = tonumber(frame:getValue("input.state","text"))
  --print(string.format("Temp %s", temp))
  if temp < prevTemp then
     if temp < 26 and autoShut then
        fanOff()
     end
  elseif temp > prevTemp then
    if temp > 33 and autoShut == nil then
        bsc.sendState(fan, "on")
        -- Don't run the system longer than 5 minutes
        -- Could be we just can't cool down to the lower limit
        autoShut = xap.Timer(fanOff, 60*5):start()
    end
  end
  prevTemp = temp
end

function init()
  f = xap.Filter()
  f:add("xap-header","source", sensor)
  f:add("xap-header","class","xAPBSC.event")
  f:callback(tempEvent)
end
