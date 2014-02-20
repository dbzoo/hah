--[[ $Id$

   Use in conjuction with the OutputNode JeeNode sketch.
   Copyright (c) Brett England, 2011

   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.
--]]
module("xap.outputnode", package.seeall)

require("xap.bsc")
jeenode = require("xap.jeenode")
Nodule = jeenode.Nodule
class = require("pl.class")
require("pl")

class.OutputNode(Nodule)

-- create BSC endpoints
function OutputNode:build(...)
   Nodule.build(self, ...)

   for i=1,4 do
      self:add {key="p"..i, direction=bsc.OUTPUT, type=bsc.BINARY, cmdCB=function(e) self:portCmd(e) end}
   end

-- In the Sketch the node constantly checks for an inbound RF message
-- so its doubtful you would want to run this on batteries for any
-- length of time.
   self:add {key="lobat", direction=bsc.INPUT, type=bsc.BINARY}
end

function OutputNode:portCmd(e)
   -- The output port is the last character of the key (a little kludgy)
   local port = e.key:sub(-1)
   local state = utils.choose(e.state=="off",0,1)
   self:sender(port ..",".. state)
end

function OutputNode:process(data)
--[[
-- From the OutputNode.pde Sketch
struct {
    byte p1 :1;
    byte p2 :1;
    byte p3 :1;
    byte p4 :1;
    byte lobat :1;  // supply voltage dropped under 3.1V: 0..1
} payload;
--]]
   local p1, p2, p3, p4, lobat = jeenode.bitslicer(data,1,1,1,1,1)
   Nodule.process(self,{p1=p1,p2=p2,p3=p3,p4=p4,lobat=lobat})
end

return OutputNode
