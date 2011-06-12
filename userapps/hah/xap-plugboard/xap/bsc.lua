--[[ $Id$
   xAP library for implementing BSC
   Copyright (c) Brett England, 2011

   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.
--]]
local _G = _G
local DEBUG = rawget(_G,'_DEBUG')

module("bsc", package.seeall)

pretty = require("pl.pretty")
ljust = require "pl.stringx".ljust
require "xap"

local class = require("pl.class").class
local List = require("pl.list").List

uid = 1

INPUT="input"
OUTPUT="output"

BINARY=0
LEVEL=1
STREAM=2

STATE_OFF="off"
STATE_ON="on"
STATE_TOGGLE="toggle" -- transient
STATE_UNKNOWN="?"

INFO_CLASS="xAPBSC.info"
EVENT_CLASS="xAPBSC.event"
CMD_CLASS="xAPBSC.cmd"
QUERY_CLASS="xAPBSC.query"

-- Endpoint class --
class.Endpoint()

function Endpoint:_init(endpoint)
   assert(type(endpoint) == "table","Did not supply a table argument")
   assert(endpoint.name,"name is mandatory")
   assert(endpoint.direction, "direction is mandatory")
   assert(endpoint.type,"type is mandatory")

   if endpoint.direction == INPUT then
      endpoint.state = STATE_UNKNOWN
   else -- endpoint.direction == OUTPUT
      endpoint.state = endpoint.type == BINARY and STATE_OFF or STATE_ON
   end

   endpoint.timeout = endpoint.timeout or 120
   endpoint.infoEventCB = endpoint.infoEventCB or function() return true end

   if endpoint.type ~= BINARY then
      endpoint.text = "?"
   end
   
   if endpoint.uid == nil then
      uid = uid + 1
      endpoint.uid = uid
   else
      uid = endpoint.uid -- reset the internal counter to the last set UID
   end

   endpoint.source = xap.defaultKeys.source .. ":" .. endpoint.name
   endpoint.uid = xap.defaultKeys.uid:sub(1,-3) .. ljust(tostring(endpoint.uid),2,'0')
   
   -- create xap.Filters for this endpoint
   if endpoint.direction == OUTPUT then
      f = xap.Filter()
      f:add("xap-header","class", CMD_CLASS)
      f:add("xap-header","target", endpoint.source)
      f:callback(incomingCmd, endpoint)
   end
   
   f = xap.Filter()
   f:add("xap-header","class", QUERY_CLASS)
   f:add("xap-header","target", endpoint.source)
   f:callback(function(frame, e)
		    sendInfoEvent(e, INFO_CLASS)
	      end, 
	      endpoint)

   xap.Timer(function(t, e)
		sendInfoEvent(e, INFO_CLASS)
	     end, 
	     endpoint.timeout, 
	     endpoint):start()

   -- Send initial INFO event to show we exist
   sendInfoEvent(endpoint, INFO_CLASS)
   return self
end

stateMap={["on"]=STATE_ON,["off"]=STATE_OFF,
	  ["true"]=STATE_ON,["false"]=STATE_OFF,
	  ["yes"]=STATE_ON,["no"]=STATE_OFF,
	  ["1"]=STATE_ON,["0"]=STATE_OFF,
	  ["toggle"]=STATE_TOGGLE}

function decodeState(state)
   if stateMap[state] then
      return stateMap[state]
   end
   return STATE_UNKNOWN
end

function setState(e, state)
   if state == STATE_TOGGLE then
      e.state = e.state == STATE_ON and STATE_OFF or STATE_ON
   else
      e.state = state
   end
end

function infoEventInternal(e, clazz)
   local msg = string.format([[
xap-header
{
v=12
hop=1
uid=%s
class=%s
source=%s
}
%s.state
{
state=%s
]], e.uid, clazz, e.source, e.direction, e.state)
  if e.type == LEVEL then
     msg = msg .. "level=" .. e.text .. "\n"
  elseif e.type == STREAM then
     msg = msg .. "text=" .. e.text .. "\n"
  end
  if e.displaytext then
     msg = msg .. "displaytext=" .. e.displaytext .. "\n"
  end
  msg = msg .. "}"
  xap.send(msg)
end

function sendInfoEvent(e, clazz)
   if e.infoEventCB(e, clazz) then
      infoEventInternal(e, clazz)
   end
end

function incomingCmd(frame, e)
   if e.direction == INPUT then
      return -- Can't control an INPUT only device.
   end
   
   for i=1,9 do
      local sectionKey = "output.state."..i
      local state = frame:getValue(sectionKey,"state")
      local id = frame:getValue(sectionKey, "id")
      if state == nil or id == nil then
	 break
      end
      
      if id == "*" or id == e.uid:sub(-2) then
	 setState(e, decodeState(state))
	 
	 if e.type == LEVEL then
	    e.text = frame:getValue(sectionKey, "level")
	 elseif e.type == STREAM then
	    e.text = frame:getValue(sectionKey, "text")
	 end
	 
	 if e.cmdCB then e.cmdCB(e) end
	 sendInfoEvent(e, EVENT_CLASS)
      end
   end
end

