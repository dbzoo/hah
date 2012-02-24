--[[
   Demonstrate how to bind relays together so that they will stay in sync.
   This example can be adapted to bind any group of things together.

   As we block events for 1 second after the action, any CMD event seen 
   in that period will not trip the other the other relays
--]]

module(...,package.seeall)

require("xap")
require("xap.bsc")
require("pl.list")

info={
   version="1.0", description="Binding relays 2,3,4"
}

local ignoreNextEvent = false

-- Timer callback
local function oneShot(self)
  self:delete()
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
   if not relays:contains(id) then return end

   -- From the set of relays to keep in sync remove our trigger
   relays:remove_value(id)
   
   source = source:sub(1,-2) -- Strip relay ID from source.
   local state = xap.getValue("output.state","state")

   -- As the inbound event won't be received until we send
   -- We'll block for 1 second all incoming relay events
   -- so we don't process our own messages.
   ignoreNextEvent = true
   xap.Timer(oneShot, 1):start()

   for i in List.iter(relays) do
     bsc.sendState(source .. tostring(i), state)
   end
end

function init()
  f = xap.Filter()
  f:add("xap-header","source","dbzoo.livebox.Controller:relay.*")
  f:add("xap-header","class","xapbsc.event")
  f:callback(function() relayBind(List{2,3,4}) end)
end
