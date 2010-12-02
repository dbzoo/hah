--[[ 
Wake-on-LAN service

To wake up a server send an xAP message like

xap-header
{
v=12
hop=1
uid=FF00AA00
source=dbzoo.acme.test
target=dbzoo.lua.wol
class=device.wol
}
wake
{
mac=00:0a:e4:86:6b:83
}
--]]

module(...,package.seeall)

require("wol")
require("xap")

info={
   version="1.0", description="Wake on LAN service"
}

function init()
  f = xap.Filter()
  f:add("xap-header","target", "dbzoo.lua.wol")
  f:add("xap-header","class","device.wol")
  f:add("wake","mac",xap.FILTER_ANY)
  f:callback(function() wakeOnLan(xap.getValue("wake","mac")) end)
end
