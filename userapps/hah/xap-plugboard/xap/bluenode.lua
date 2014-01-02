local DEBUG = rawget(_G,'_DEBUG')

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
		      ["12:37:e90912"] = "brett.2",
		      ["80ea:96:9"] = "gary",
		}
           }
}
--]]

-- create BSC endpoints
function BlueNode:build(...)
   Nodule.build(self, ...)

   ttl = self.cfg.ttl or 35
   
   for device in pairs(self.cfg.endpoints) do
      self:add {key=device, direction=bsc.INPUT, type=bsc.BINARY, displaytext=device, lastSeen=os.time(), isAlive=false, ttl=ttl}
   end

   --  use default reaper as it would expire ALL devices at the same time.
   self.cfg.ttl = nil

   -- Check for non-responding Endpoints every 10 seconds
   xap.Timer(BlueNode.reaper, 10, self):start()
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

    -- Update to say this device is now present, its alive !
    self[device].lastSeen = os.time()
    self[device].isAlive = true
    Nodule.process(self,{[device]=bsc.STATE_ON})
end

-- Custom reaper that can deal with multiple endpoints on the same base target
-- Don't fear the reaper - Bluetooth oyster cult. :)

function BlueNode:reaper(self)
    local now = os.time()
    for key in pairs(self.cfg.endpoints) do
         local device = self[key]
	 if DEBUG then
	    print(string.format("BlueNode reaper %s alive=%s ttl=%s timeout=%s", 
				key,
				utils.choose(device.isAlive == true,"true","false"),
                                device.ttl,
				now-device.lastSeen))
	    
	 end
	if device.isAlive and now - device.lastSeen > device.ttl then
	   if DEBUG then
	      print('expire '.. key)
	   end
	   device:setState(bsc.STATE_OFF)
           device:sendEvent()
        end
    end
end
