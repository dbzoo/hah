--[[
  xAP protocol control for the Airytec PC switchoff utility
  http://www.airytec.com/

xap-header
{
v=12
hop=1
uid=FF00AA00
source=dbzoo.acme.test
target=dbzoo.lua.switchoff
class=device.off
}
host
{
ip=192.168.1.1
}
]]--

module(...,package.seeall)

http = require("socket.http")
require("xap")

info={
   version="1.0", description="Airytec shutdown service"
}

function shutdown()
  local ip = xap.getValue("host","ip")
  --print("Shutting down "..ip)
  http.request("http://User:password@"..ip..":8000/?action=System.Shutdown")
end

function init()
  f = xap.Filter()
  f:add("xap-header","target", "dbzoo.lua.switchoff")
  f:add("xap-header","class","device.off")
  f:add("host","ip",xap.FILTER_ANY)
  f:callback(shutdown)
end
