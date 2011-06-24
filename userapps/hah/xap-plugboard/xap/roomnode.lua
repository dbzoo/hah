--[[ $Id$

   Use in conjuction with the RoomNode JeeNode sketch.
   Copyright (c) Brett England, 2011

   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.
--]]
local _G = _G
local DEBUG = rawget(_G,'_DEBUG')

module("xap.roomnode", package.seeall)

require("xap.bsc")
bitslicer = require("xap.jeenode").bitslicer
class = require("pl.class").class
require("pl")

class.RoomNode()

function RoomNode:_init(base)
   self.base = base
end

-- create BSC endpoints
-- sender (unused): function to send xAP data, used with the cmdCB handler
function RoomNode:build(sender)
   self.light = bsc.Endpoint {source=self.base..".light", direction=bsc.INPUT, type=bsc.STREAM}
   self.temp = bsc.Endpoint {source=self.base..".temp", direction=bsc.INPUT, type=bsc.STREAM}
   self.lobat = bsc.Endpoint {source=self.base..".lobat", direction=bsc.INPUT, type=bsc.BINARY}
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
} payload;
--]]
   light, moved, humi, temp, lobat = bitslicer(data,8,1,7,-10,1)
   temp = temp  / 10
   
   if DEBUG then
      print("ROOM NODE")
      print(string.format([[
light: %s
moved: %s
humi: %s
temp: %s
lobat: %s
			  ]], light, moved, humi, temp, lobat))
   end

   if self.temp.text ~= temp then
      self.temp:setText(temp)
      self.temp:sendEvent()
   end

   if self.light.text ~= light then
      self.light:setText(light)
      self.light:sendEvent()
   end
   
   lobat = bsc.decodeState(lobat)
   if self.lobat.state ~= lobat then
      self.lobat:setState(lobat)
      self.lobat:sendEvent()
   end
end