--[[ $Id$

   Use in conjuction with the BlueNode.ino sketch.
   Copyright (c) Brett England, 2014

   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.
--]]
local DEBUG = rawget(_G,'_DEBUG')

module("bluenode", package.seeall)

require("xap.bsc")
jeenode = require("xap.jeenode")
Nodule = jeenode.Nodule
class = require("pl.class")

class.BlueNode(Nodule)

--[[
Sample configuation for RF node 18 which has a BT device attached.

   [18] = BlueNode{instance="bluetooth:presence",
		   endpoints={
		      ["1d:fe:720479"] = "brett.1",
		      ["12:37:e90912"] = "brett.2",
		      ["80ea:96:9"] = "gary",
		}
           }

This is acceptable.  Devices will be discovered dynamically.

   [18] = BlueNode{instance="bluetooth:presence"}
--]]

-- create BSC endpoints
function BlueNode:build(...)
   Nodule.build(self, ...)

   ttl = self.cfg.ttl or 35
   --  don't use the default reaper as it will expire ALL devices at the same time.
   self.cfg.ttl = nil
   
   if self.cfg.endpoints == nil then
      self.cfg.endpoints = {}
   else
      for device in pairs(self.cfg.endpoints) do
	 self:add {key=device, direction=bsc.INPUT, type=bsc.BINARY, displaytext=device, lastSeen=os.time(), isAlive=false, ttl=ttl}
      end
   end

   -- Check for non-responding Endpoints every 10 seconds
   xap.Timer(BlueNode.reaper, 10, self):start()
end

function BlueNode:process(data)
--[[
-- From the BlueNode.ino Sketch
    struct {
      unsigned int nap:16;
      unsigned int uap:8;
      unsigned int lap:24;
    } payload;
--]]
    local device = string.format("%x:%x:%x", jeenode.bitslicer(data,16,8,24))
    
    local e = self[device]
    -- If this device has no pre-configured endpoint dynamically create it.
    if e == nil then
       -- a) Tell self:add what our name will be (dynamic)
       -- b) Allow the reaper to pick up this endpoint.
       self.cfg.endpoints[device] = device:gsub(':','.')

       -- The override ttl at this point has gone - use default of 35 seconds.
       -- If you want the endpoint to persist beyond expiry set isRemovable=false
       self:add {key=device, direction=bsc.INPUT, type=bsc.BINARY, displaytext=device, lastSeen=os.time(), isAlive=true, ttl=35, isRemovable=true}
       e = self[device]
    else
       -- Update to say this device is now present. Its alive !
       e.lastSeen = os.time()
       e.isAlive = true
    end

    -- set state and send event.
    Nodule.process(self,{[device]=bsc.STATE_ON})
end

-- Custom reaper that can deal with multiple endpoints on the same base address
-- We break with the colon form as the 1st argument won't be BlueNode (self)
-- as the Timer is going to pass itself in this position.  Which is not used.
-- In this context self is the userdata passed into the callback from the timer.
function BlueNode.reaper(_, self)
    local now = os.time()

    for key,v in pairs(self.cfg.endpoints) do
         local device = self[key]	 
	 if DEBUG then
	    print(string.format("BlueNode reaper %s alive=%s ttl=%s timeout=%s", 
				key,
				utils.choose(device.isAlive == true,"true","false"),
                                device.ttl,
				now - device.lastSeen))
	    
	 end
	if device.isAlive and now - device.lastSeen > device.ttl then
	   if DEBUG then
	      print('expire '.. key)
	   end
	   device:setState(bsc.STATE_OFF)
           device:sendEvent()
           device.isAlive = false
	   -- We were dynamically created so we can be destroyed now we have expired.
	   -- This keeps our endpoint list managable.  Caveat: we burn UID's
	   if device.isRemovable then
	      -- check for destroy method, earlier releases don't have it.
	      if device.destroy then 
		 self.cfg.endpoints[key] = nil
		 device:destroy() 
	      end
	   end
        end
    end
end
