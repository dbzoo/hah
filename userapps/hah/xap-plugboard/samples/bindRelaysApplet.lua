--[[
   Demonstrate how to bind relays together so that they will stay in sync.
   This example can be adapted to bind any group of things together.
--]]

module(...,package.seeall)

require("xap")

info={
   version="1.0", description="Binding relays 2,3,4"
}

local ignoreNextEvent = false

local function contains(t, e)
  for i = 1,#t do
    if t[i] == e then return true end
  end
  return false
end

-- Timer callback
local function oneShot(self)
  self:stop()
  ignoreNextEvent = false
end

-- Filter Callback
local function relayBind(relays)
   -- When this function send a cmds to toggle the other relay
   -- we will fire when it generates an Event.  TRAP.
    if ignoreNextEvent then
     return
   end

   -- Strip the Relay ID from the source.
   local source = xap.getValue("xap-header","source")
   local id = tonumber(source:sub(-1))

   -- Event for a relay we aren't binding
   if not contains(relays, id) then return end
   
   source = source:sub(1,-2) -- Strip relay ID from source.
   local state = xap.getValue("output.state","state")

   -- From the set of relays to keep in sync remove our trigger
   for i,j in ipairs(relays)
   do
     if j == id then
        table.remove(relays,i) -- take this index out.
     end
   end

   -- As the inbound event won't be received until we send
   -- We'll block for 1 second all incoming relay events
   -- so we don't process our own messages.
   ignoreNextEvent = true
   xap.Timer(oneShot, 1):start()

   for _,i in pairs(relays)
   do
     xap.send(xap.fillShort(string.format("xap-header\
{\
target=%s%s\
class=xAPBSC.cmd\
}\
output.state.1\
{\
id=*\
state=%s\
}", source, i, state)))

   end
end

function init()
  f = xap.Filter()
  f:add("xap-header","source","dbzoo.livebox.Controller:relay.*")
  f:add("xap-header","class","xapbsc.event")
  f:callback(function() relayBind{2,3,4} end)
end
