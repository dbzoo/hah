--[[ $Id$
   xAP library for implementing BSC
   Copyright (c) Brett England, 2011

   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.
--]]
local utils = require('pl.utils')
local class = require("pl.class")
local rjust = require("pl.stringx").rjust

module("bsc", package.seeall)

require "xap"

local id = 0

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
   utils.assert_arg(1,endpoint,'table')

   -- Source is a fully formed xAP address
   if endpoint.source == nil then
      -- Perhaps just a endpoint onto our running xap source
      if endpoint.name then
	 endpoint.source = xap.defaultKeys.source .. ":".. endpoint.name
      else
	 -- dropping back to vendorid,deviceid,instance keys
	 endpoint.source = xap.buildXapAddress(endpoint)
      end
   end

   assert(endpoint.direction, "direction is mandatory")
   assert(endpoint.type,"type is mandatory")

   if endpoint.direction == INPUT then
      endpoint.state = STATE_UNKNOWN
   else -- endpoint.direction == OUTPUT
      endpoint.state = utils.choose(endpoint.type == BINARY,STATE_OFF, STATE_ON)
   end

   endpoint.timeout = endpoint.timeout or 120
   endpoint.infoEventCB = endpoint.infoEventCB or function() return true end

   if endpoint.type ~= BINARY then
      endpoint.text = "?"
   end

   if endpoint.uid == nil then
      if endpoint.id == nil then
	 id = id + 1
	 endpoint.id = id
      else
	 id = endpoint.id -- reset the internal counter to the last set UID
      end
      endpoint.uid = xap.defaultKeys.uid:sub(1,-3) .. rjust(tostring(endpoint.id),2,'0')
   end
   
   -- Record destructors to call on endpoint point destroy
   endpoint._dtors = {}

   -- create xap.Filters for this endpoint
   if endpoint.direction == OUTPUT then
      f = xap.Filter()
      f:add("xap-header","class", CMD_CLASS)
      f:add("xap-header","target", endpoint.source)
      f:callback(incomingCmd, endpoint)
      table.insert(endpoint._dtors, function() f:destroy() end)
   end
   
   f = xap.Filter()
   f:add("xap-header","class", QUERY_CLASS)
   f:add("xap-header","target", endpoint.source)
   f:callback(function(frame, e)
		 e:sendInfo()
	      end, 
	      endpoint)
   table.insert(endpoint._dtors, function() f:destroy() end)

   t = xap.Timer(function(t, e)
		e:sendInfo()
	     end, 
	     endpoint.timeout, 
	     endpoint)
   t:start()
   table.insert(endpoint._dtors, function() t:delete() end)

   -- Send initial INFO event to show we exist
   sendInfoEvent(endpoint, INFO_CLASS)

   return endpoint
end

function Endpoint:destroy()
   for _,dtor in pairs(self._dtors) do
      dtor()
   end
   for k in pairs(self) do
      self[k] = nil
   end
end

function Endpoint:sendEvent()
   sendInfoEvent(self, EVENT_CLASS)
end

function Endpoint:sendInfo()
   sendInfoEvent(self, INFO_CLASS)
end

function Endpoint:setText(text)
   self.state = STATE_ON
   self.text = text or ""
end

function Endpoint:setDisplayText(text)
   self.state = STATE_ON
   self.displaytext = text
end

stateMap={["on"]=STATE_ON,["off"]=STATE_OFF,
	  ["true"]=STATE_ON,["false"]=STATE_OFF,
	  ["yes"]=STATE_ON,["no"]=STATE_OFF,
	  ["1"]=STATE_ON,["0"]=STATE_OFF,
	  [1]=STATE_ON,[0]=STATE_OFF,
	  ["toggle"]=STATE_TOGGLE}

function decodeState(state)
   if stateMap[state] then
      return stateMap[state]
   end
   return STATE_UNKNOWN
end

function Endpoint:setState(state)
   if state == STATE_TOGGLE then
      self.state = utils.choose(self.state == STATE_ON,STATE_OFF, STATE_ON)
   else
      self.state = state
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
	 e:setState(decodeState(state))
	 
	 if e.type == LEVEL then
	    e.text = frame:getValue(sectionKey, "level")
	 elseif e.type == STREAM then
	    e.text = frame:getValue(sectionKey, "text")
	 end
	 
	 if e.cmdCB then e.cmdCB(e) end
	 e:sendEvent()
      end
   end
end

function send(body)
   assert(type(body) == "table", "Parameter must be a table")
   assert(body.target, "Missing target")

   local msg = string.format([[
xap-header
{
target=%s
class=xAPBSC.cmd
}
output.state.1
{
id=*
]], body.target)

  body.target=nil
  body.state = decodeState(body.state)

  for k,v in pairs(body) do
    msg = msg .. k.."="..v.."\n"
  end
  msg = msg .. "}"

  xap.sendShort(msg)
end

function sendText(target, text, state)
  state = state or STATE_ON
  text = text or ""
  send{target=target, text=text, state=state}
end

function sendState(target, state)
  send{target=target, state=state}
end

function sendLevel(target, level, state)
  state = state or STATE_ON
  level = level or ""
  send{target=target, level=level, state=state}
end
