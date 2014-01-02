module("bluenode", package.seeall)

require("xap.bsc")
jeenode = require("xap.jeenode")
Nodule = jeenode.Nodule
require("pl.class").BlueNode(Nodule)

--[[
Sample configuation for RF node 18 which has a BT device attached.

local nodes = {
   [18] = BlueNode{instance="bluetooth:presence",
		   endpoints={
		      ["1d:fe:720479"] = "brett.1",
		      ["12:37:e90912"] = "brett.2"
		      ["80ea:96:9"] = "gary",
		}},
}
--]]

-- create BSC endpoints
function BlueNode:build(...)
   Nodule.build(self, ...)

   -- Force reapable.
   -- Seconds, must be > sketch polling time.
   self.cfg.ttl = 35

   for device in pairs(self.cfg.endpoints) do
      self:add {key=device, direction=bsc.INPUT, type=bsc.BINARY, displaytext=device}
   end
end

function BlueNode:process(data)
--[[
-- From the bluenode.pde Sketch
    struct {
      unsigned int nap:16;
      unsigned int uap:8;
      unsigned int lap:24;
    } payload;
--]]
    local device = string.format("%x:%x:%x", jeenode.bitslicer(data,16,8,24))
    -- Update to say this device is now present.
    Nodule.process(self,{[device]=bsc.STATE_ON})
end

-- Override default expiry mechanism.
-- We don't want to go unknown, on/off please.
function BlueNode:expire()
   if DEBUG then
      print('expire '.. tostring(self))
   end
   self.isalive = false
   for name in pairs(self.cfg.endpoints) do
      local e = self[name]
      e.state = bsc.STATE_OFF
      e:sendEvent()
   end
end