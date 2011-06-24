--[[
  Monitor xAP heartbeats when a process stops send an event
--]]

module(...,package.seeall)

require("xap")

info={
   version="1.0", description="Process Watchdog"
}

hbeats={}

function reaper(t)
  for k,v in pairs(hbeats) do
    v.ttl = v.ttl - t.interval
    if v.ttl <= 0 and v.isalive then
      v.isalive = false
      msg = string.format([[xap-header
{
class=hbeat.stopped
}
body
{
source=%s
}]], k)
      xap.sendShort(msg)
    end
  end
end

function addOrUpdateHbeatEntry(frame)
  local source = frame:getValue("xap-hbeat","source")
  local interval = frame:getValue("xap-hbeat","interval")
  hbeats[source] = {ttl = interval + 2, isalive = true}
end

function init()
  local f = xap.Filter()
  f:add("xap-hbeat","source", xap.FILTER_ANY)
  f:callback(addOrUpdateHbeatEntry)
  
  xap.Timer(reaper, 10):start()
end
