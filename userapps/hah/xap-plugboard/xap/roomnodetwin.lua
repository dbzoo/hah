--[[ $Id$

   Use in conjuction with the RoomNodeTwin JeeNode sketch.
   Allow an additional 1-Wire temperature sensor on JeePort2
   Copyright (c) Brett England, 2011

   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.
--]]
module("xap.roomnodetwin", package.seeall)

require("xap.bsc")
jeenode = require("xap.jeenode")
Nodule = jeenode.Nodule
class = require("pl.class").class

class.RoomNode(Nodule)

-- create BSC endpoints
function RoomNode:build(...)
   Nodule.build(self, ...)

   -- Creates the endpoints: self[key] as in self.light, self.moved etc.. 
   -- If the user has configured that they want them that is.
   self:add {key="light", direction=bsc.INPUT, type=bsc.STREAM}
   self:add {key="moved", direction=bsc.INPUT, type=bsc.BINARY}
   self:add {key="humi", direction=bsc.INPUT, type=bsc.STREAM}
   self:add {key="temp",  direction=bsc.INPUT, type=bsc.STREAM}
   self:add {key="lobat", direction=bsc.INPUT, type=bsc.BINARY}
   self:add {key="temp2",  direction=bsc.INPUT, type=bsc.STREAM}
end

function RoomNode:process(data)
--[[
-- From the RoomNode.pde Sketch
struct {
    byte light;     // light sensor: 0..255
    byte moved :1;  // motion detector: 0..1
    byte humi  :7;  // humidity: 0..100
    int temp   :10; // temperature: -500..+500 (tenths)
    byte lobat :1;  // supply voltage dropped under 3.1V: 0..1
    int temp2   :10; // temperature: -500..+500 (tenths)
} payload;
--]]
   local li, mo, hu, te, lo, te2 = jeenode.bitslicer(data,8,1,7,-10,1,-10)
   te = te  / 10
   te2 = te2  / 10

-- Add the temperature offset if one has been supplied
   if self.cfg.toff then
      te = te + self.cfg.toff
   end
   if self.cfg.toff2 then
      te2 = te2 + self.cfg.toff2
   end

   -- The keys here must match the key values from the self:add{key=x}
   Nodule.process(self,{light=li,moved=mo,humi=hu,temp=te,lobat=lo,temp2=te2})
end
