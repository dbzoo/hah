--[[ $Id$

   Use in conjuction with the OutputNode JeeNode sketch.
   Copyright (c) Brett England, 2011

   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.
--]]
local _G = _G
local DEBUG = rawget(_G,'_DEBUG')

module("xap.outputnode", package.seeall)

require("xap.bsc")
bitslicer = require("xap.jeenode").bitslicer
class = require("pl.class").class
require("pl")

class.OutputNode()

function OutputNode:_init(base)
   self.base = base
end

-- create BSC endpoints
-- sender (unused): function to send xAP data, used with the cmdCB handler
function OutputNode:build(sender)
   self.t={}
   for i=1,4 do
      self.t[i] = bsc.Endpoint {id=i, 
				source=self.base..".p"..i,  
				direction=bsc.OUTPUT, 
				type=bsc.BINARY, 
				cmdCB=function(e)
					 portCmd(e, sender)
				      end
			     }
   end
end

function portCmd(e, sender)
   if DEBUG then
      print("Port "..e.id.." to "..e.state);
   end
   local cmd = string.byte(e.id) ..","..string.byte( utils.choose(e.state=="off",0,1))
   sender(cmd)
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
   p1, p2, p3, p4, lobat = bitslicer(data,1,1,1,1,1)
   
   p1 = bsc.decodeState(p1)
   p2 = bsc.decodeState(p2)
   p3 = bsc.decodeState(p3)
   p4 = bsc.decodeState(p4)

   if DEBUG then
      print("OUTPUT NODE")
      print(string.format([[
p1: %s
p2: %s
p3: %s
p4: %s
lobat: %s
			  ]], p1, p2, p3, p4, lobat))
   end

   local v={p1,p2,p3,p4}
   for i=1,4 do
      if self.t[i].state ~= v[i] then
	 self.t[i]:setState(v[i])
	 self.t[i]:sendEvent()
      end
   end
end