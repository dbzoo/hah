--[[
   Turn relay 1 off automatically after 30sec of it being switched on.
--]]

module(...,package.seeall)

require("xap")

info={
   version="1.0", description="Relay 1 Auto off"
}

function relayOff(self)
  print("Relay 1 auto off")
  xap.send(xap.fillShort("xap-header\
{\
target=dbzoo.livebox.Controller:relay.1\
class=xAPBSC.cmd\
}\
output.state.1\
{\
id=*\
state=off\
}\
"))
  self:stop()
end

function relayOn()
  print("Relay 1 auto off in 30 secs")
  xap.Timer(relayOff, 30):start()
end

function init()
  f = xap.Filter()
  f:add("xap-header","source","dbzoo.livebox.Controller:relay.1")
  f:add("xap-header","class","xAPBSC.event")
  f:add("output.state","state","on")
  f:callback(relayOn)
end
