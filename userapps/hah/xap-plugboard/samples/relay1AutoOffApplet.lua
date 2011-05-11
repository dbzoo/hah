--[[
   Turn relay 1 off automatically after 30sec of it being switched on.
   Its a bit nieve in that the timer does not reset if the relay is
   switched on and then off again within those 30secs.
--]]

module(...,package.seeall)

require("xap")

info={
   version="1.0", description="Relay 1 Auto off"
}

function relayOff(timer)
  print("Relay 1 auto off")
  xap.sendShort([[xap-header
{
target=dbzoo.livebox.Controller:relay.1
class=xAPBSC.cmd
}
output.state.1
{
id=*
state=off
}]])
  timer:delete()
end

function relayOn()
  print("Relay 1 auto off in 30 secs")
  xap.Timer(relayOff, 30):start()
end

function init()
  local f = xap.Filter()
  f:add("xap-header","source","dbzoo.livebox.Controller:relay.1")
  f:add("xap-header","class","xAPBSC.event")
  f:add("output.state","state","on")
  f:callback(relayOn)
end
