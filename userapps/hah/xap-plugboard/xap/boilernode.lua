--[[  boilernode.lua

   Use in conjuction with the RoomNode JeeNode sketch.
   Copyright (c) Brett England, 2011

   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.

   Has a temperature range of 0 to 102.4C
--]]
module("boilernode", package.seeall)

require("xap.bsc")
jeenode = require("xap.jeenode")
Nodule = jeenode.Nodule
class = require("pl.class")

class.BoilerNode(Nodule)

-- create BSC endpoints
function BoilerNode:build(...)
   Nodule.build(self, ...)

   -- Creates the endpoints: self[key] as in self.light, self.moved etc.. 
   -- If the user has configured that they want them that is.
   self:add {key="light", direction=bsc.INPUT, type=bsc.STREAM}
   self:add {key="moved", direction=bsc.INPUT, type=bsc.BINARY}
   self:add {key="humi", direction=bsc.INPUT, type=bsc.STREAM}
   self:add {key="temp",  direction=bsc.INPUT, type=bsc.STREAM}
   self:add {key="lobat", direction=bsc.INPUT, type=bsc.BINARY}
end

function BoilerNode:process(data)
--[[
-- From the RoomNode.pde Sketch
struct {
    byte light;     // light sensor: 0..255
    byte moved :1;  // motion detector: 0..1
    byte humi  :7;  // humidity: 0..100
    int temp   :10; // temperature: -500..+500 (tenths)
    byte lobat :1;  // supply voltage dropped under 3.1V: 0..1
} payload;
--]]
   local li, mo, hu, te, lo = jeenode.bitslicer(data,8,1,7,10,1)
   te = te  / 10
   -- The keys here must match the key values from the self:add{key=x}
   Nodule.process(self,{light=li,moved=mo,humi=hu,temp=te,lobat=lo})
end
