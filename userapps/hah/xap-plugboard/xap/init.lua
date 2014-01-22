--[[ $Id$
   xAP library
   Copyright (c) Brett England, 2011

   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.
--]]
local _G = _G
local DEBUG = rawget(_G,'_DEBUG')

module("xap", package.seeall)

require("socket")
local class = require("pl.class")
local List = require("pl.List")
local config = require("pl.config")

local gframe
defaultKeys={}

MSG_ORDINARY = "xap-header"
MSG_HBEAT = "xap-hbeat"
MSG_UNKNOWN = "unknown"
FILTER_ANY = "xap_filter_any"
FILTER_ABSENT = "xap_filter_absent"

local function getRxPort()
   rxport=3639
   local udp = socket.udp()
   udp:setoption('broadcast',true)
   if (udp:setsockname('*',3639) == nil) then
      if DEBUG then
	 print("Broadcast port 3639 in use")
	 print("Assuming hub is active")
      end
      for i=3640,4639,1 do
	 udp = socket.udp() --On bind failure the descriptor is closed (bug?)
	 if (udp:setsockname('127.0.0.1',i) ~= nil) then
	    if DEBUG then print("Discovered port "..i) end
	    rxport=i
	    break
	 else
	    if DEBUG then print ("Port "..i.." in use") end
	 end
      end
   else
      if DEBUG then print("Acquired broadcast port 3639") end
   end
   return udp
end

local function getTxPort()
   local udp = socket.udp()
   udp:setoption('broadcast',true)
   udp:setpeername('255.255.255.255', 3639)
   return udp
end

function send(msg)
   tx:send(msg)
end

function expandShortMsg(msg, section)
   section = section or "xap-header"
   local f = Frame(msg)
   for k,v in pairs(defaultKeys) do
      if not f[section][k] then
	 f[section][k] = v
      end
   end
   return tostring(f)
end

function sendShort(msg)   
   tx:send(expandShortMsg(msg))
end

local heartBeat=function(tm)
		   send(string.format([[
xap-hbeat
{
v=12
hop=1
uid=%s
class=xap-hbeat.alive
source=%s
interval=%s
port=%s
}]], defaultKeys.uid, defaultKeys.source, tm.interval, rxport))
		end

local handlePacket = function(udp, data)
			local dgram, err = udp:receive()
			if dgram and #filterList > 0 then
			   frame = Frame(dgram)
			   filterList:foreach(function(f)
						 f:dispatch(frame)
					      end
					   )
			end
		     end

function init(t, uid)
   if type(t) == "table" then
      assert(t.uid, "Missing UID")
      defaultKeys = {source=buildXapAddress(t), uid=t.uid, v=12, hop=1}
   else
      defaultKeys = {source=t, uid=uid, v=12, hop=1}
   end
   verbose = true
   tx = getTxPort()

   Select(handlePacket, getRxPort())
   Timer(heartBeat, 60):start()
end

function process()
   while true do
      local readable,_,error = socket.select(readList, nil, 1)
      -- we must use ipairs because readable contains double keyed items
      for _, input in ipairs(readable) do
	 threadList[input]:dispatch()
      end

      -- timeout dispatcher
      local now = os.time()
      timerList:foreach(function(t)
			   if t and t.ttl <= now and t.isalive then
			      t:dispatch()
			    end
			 end
		     )
   end
end

-- Timer class --

class.Timer()

timerList=List()

function Timer:_init(func, interval, userdata)
   self.callback = func
   self.interval = interval
   self.ttl = 0
   self.isalive = false
   self.interval = interval
   self.userdata = userdata
   timerList:append(self)
   return self
end

-- If the callback should be fired on timer startup T=0
-- then pass true, otherwise leave blank or pass false
function Timer:start(initial)
   self.isalive = true
   if initial == true then
      self.ttl = 0
   else
     self:reset()
   end
   return self
end

function Timer:stop()
   self.isalive = false
   return self
end

function Timer:reset()
   self.ttl = os.time() + self.interval
   return self
end

function Timer:dispatch()
   self:reset()
   self.callback(self, self.userdata)
end

function Timer:delete()
   timerList:remove_value(self)
end

-- Select class --

class.Select()

threadList={}
readList={}

function Select:_init(func, socket, userdata)
   self.socket = socket
   self.callback = func
   self.userdata = userdata
   threadList[socket] = self
   table.insert(readList, socket)
   return self
end

function Select:delete()
   threadList[self.socket] = nil
   for i,v in pairs(a) do 
      if v==self then readList[i]=nil end
   end
end

function Select:dispatch()
   self.callback(self.socket, self.userdata)
end

-- Frame class --

class.Frame()

function Frame:_init(msg)
   local line

   local co = coroutine.create(function()
				  for i in msg:gfind("%s*(.-)%s*\n") do
				     coroutine.yield(i)
				  end
			       end
			    )

   local function nextLine()
      local status, ret = coroutine.resume(co)
      if status then return ret end
      return nil
   end

   local function readBody()
     local _,k,v,t
     line = nextLine()
     t={}
     while line ~= nil and line ~= "}" do
       _,_,k,v = line:find("(.-)%s*=%s*(.*)")
       if k then t[k:lower()] = v end
       line = nextLine()
     end
     return t
   end

   local function readXap()
     local t,section
     t={}
     line = nextLine()
     while line ~= nil do
       section = line
       nextLine() -- Skip the {
       t[section:lower()] = readBody()
       line = nextLine()
     end
     return t
   end
   
   return readXap()
end

function Frame:getValue(section, key)
  section = section:lower()
  if self[section] then
    if key == nil then
      return FILTER_ANY
    end
    return self[section][key:lower()]
  else
    return nil
  end
end

-- Backward compatibilty with previous BETA api
function getValue(section, key)
   return gframe:getValue(section, key)
end

function Frame:isValue(section, key, value)
  return self:getValue(section, key) == value
end

function Frame:getType()
  if self["xap-header"] ~= nil then return MSG_ORDINARY end
  if self["xap-hbeat"] ~= nil then return MSG_HBEAT end
  return MSG_UNKNOWN
end

function Frame:__tostring()

  local function encodeSection(s)
    local t,v
    t = s..'\n{\n'
    for k,v in pairs(self[s]) do
      t = t..k.."="..v.."\n"
    end
    t = t..'}\n'
    return t
  end

  local out = ""
  local header = nil
  -- sequence the header sections first
  for _,v in pairs{'xap-header','xap-hbeat'} do
     if self[v] then
	header = v
	out = encodeSection(header)
	break
     end
  end

  for k,_ in pairs(self) do
    if k ~= header then
      out = out .. encodeSection(k)
    end
  end

  return out
end


-- Filter class --

class.FilterKey()
function FilterKey:_init(section,key,value)
   self.section = section
   self.key = key
   self.value = value
end

function FilterKey:equals(section, key, value)
   return self.section == section and self.key == key and self.value == value
end

function FilterKey:__tostring()
   return string.format("(%s,%s,%s)", self.section, self.key, self.value)
end

class.Filter()

filterList=List()
function Filter:_init(filter)
   self.filterChain = {}
   self.callback = nil
   filterList:append(self)

   if type(filter) == "table" then
      for section, keys in pairs(filter) do
	 for k,v in pairs(keys) do
	    self:add(section, k, v)
	 end
      end
   end
end

function Filter:destroy()
   for i = 1,#self.filterChain do
      self.filterChain[i] = nil
   end
   self.filterChain = nil
   self.callback = nil
   self.userdata = nil
   filterList:remove_value(self)
end

function Filter:delete(section, key, value)
   for k,v in pairs(self.filterChain) do
      if v:equals(section, key, value) then
	 table.remove(self.filterChain, k)
      end
   end
end

function Filter:add(section, key, value)
   table.insert(self.filterChain, FilterKey(section,key,value:lower()))
end

function Filter:callback(func, userdata)
   self.callback = func
   self.userdata = userdata
end

function Filter:ismatch(frame)
   local value, match = nil, true -- No filter chain is match

   function filterAddrSubaddress(filterAddr, addr)
      if filterAddr == nil then
	 -- empty filterAddr matches all
	 match = true
      elseif filterAddr:find('*') == nil and filterAddr:find('>') == nil then
	 -- no wildcards - simple compare
	 match = filterAddr == addr
      else
	 -- Match using wildcard logic
	 filterAddr = filterAddr:gsub('%.','%%.') -- escape the .
	 filterAddr = filterAddr:gsub('\*','[%%w%%-_]+')
	 filterAddr = filterAddr:gsub('>','.*')
	 local start,finish = addr:find(filterAddr)
	 match = start == 1 and finish == #addr
      end
   end

   for _,fk in pairs(self.filterChain) do
      value = frame:getValue( fk.section, fk.key )
      if fk.value == FILTER_ABSENT then
	 match = value == nil
      elseif fk.value == FILTER_ANY then
	 match = value ~= nil
      elseif value == nil then
	 match = false
      else
	 value = value:lower()
	 if fk.section == "xap-header" and (fk.key == "target" or
					    fk.key == "source" or
					    fk.key == "class") then
	    if fk.key == "target" then
	       filterAddrSubaddress(value, fk.value)
	    else -- source /class
	       filterAddrSubaddress(fk.value, value)
	    end
	 else
	    match = value == fk.value
	 end
      end
      if not match then break end
   end

   return match
end

function Filter:dispatch(frame)
   if self.callback and self:ismatch(frame) then
      gframe = frame
      local status,err = pcall(self.callback, frame, self.userdata)
      if status == false then
         error(err..'\nxAP Message being processed.\n'..tostring(frame))
      end
   end
end

function getDeviceID()
   local xapini = config.read("/etc/xap.d/system.ini")
   if xapini and xapini['xap'] then
      deviceid = xapini.xap['instance']
      if deviceid and #deviceid == 0 then
	 deviceid = nil
      end
   end
   if deviceid == nil then 
      deviceid = stringx.split(socket.dns.gethostname(),'.')[1] 
   end
   return deviceid
end

function buildXapAddress(t)
   assert(t.instance,"missing instance")
   if t.vendorid == nil then
      t.vendorid = "dbzoo"
   end
   if t.deviceid == nil then
      t.deviceid = getDeviceID()
   end
   return t.vendorid.."."..t.deviceid.."."..t.instance
end
