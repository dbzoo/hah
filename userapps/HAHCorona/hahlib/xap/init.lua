--[[ $Id$
   xAP library for Corona
   Copyright (c) Brett England, 2013

   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.
--]]
local class = require "hahlib.xap.class"
local socket = require("socket")
local M = {}

MSG_ORDINARY = "xap-header"
MSG_HBEAT = "xap-hbeat"
MSG_UNKNOWN = "unknown"
FILTER_ANY = "xap_filter_any"
FILTER_ABSENT = "xap_filter_absent"

----------------------------------------
---------- Server class (base class)
----------------------------------------

Server = class()

function Server:expandShortMsg(msg, section)
   section = section or "xap-header"
   local f = Frame(msg)
   for k,v in pairs(self.conf) do
      if not f[section][k] then
	     f[section][k] = v
      end
   end
   return tostring(f)
end

function Server:sendShort(msg)   
   self:send(self:expandShortMsg(msg))
end
 
function Server:addFilter(f)
   self.filterList[#self.filterList + 1] = f
end

function Server:_init(param)
	local source = "dbzoo.corona.app"
	local uid = "FF123400"
	if type(param) == "table" then
		source = param.source or source
		uid = param.uid or uid
	end

	self.conf={source=source, uid=uid, v=12, hop=1}
	self.threads={}
	self.filterList={}
end

function Server:setSource(source)
  self.conf['source'] = source
end 

function Server:setUID(uid)
  self.conf['uid'] = uid
end 

--- Start the XAP engine
-- @return true/false if the engine could be started.
function Server:start()
   local handlePacket = function()
			function processFrame(msg)
			        local frame = Frame(msg)		   
  			        for _, f in ipairs(self.filterList) do
			            f:dispatch(frame)
					end
			end
   
			local msg = self:receive() -- A generator function or a string.
			if msg and #self.filterList > 0 then
			   if type(msg) == "function" then
			       for i in msg do
						processFrame(i)
					end
				else -- string
			       processFrame(msg)
				end
			end
    end

	local heartBeat = function()
		   self:send(string.format([[
xap-hbeat
{
v=12
hop=1
uid=%s
class=xap-hbeat.alive
source=%s
interval=60
port=%s
}]], self.conf.uid, self.conf.source, self.rxPort))
	end
		
	if not self:isConnected() then
	   self:connect()
  	   if not self:isConnected() then
	      return false
	   end
	end
		
   self.threads.receive = timer.performWithDelay(1, handlePacket, 0)
   self.threads.heartbeat = timer.performWithDelay(60000, heartBeat, 0)
   heartBeat()
   
   return true
end

function Server:stop()
	self:close()
	for _,t in pairs(self.threads) do
		timer.cancel(t)
		t = nil
	end
   self.txSocket = nil
end

function Server:isConnected()
  return self.txSocket ~= nil
end

function Server:getErrorMessage()
  return self.error
end

function Server:suspend()
  self:stop()
end

function Server:resume()
  self:_init()
end

----------------------------------------
---------- UDPServer class
----------------------------------------

UDPServer = class(Server)
M.UDPServer = UDPServer

function UDPServer:connect()	
    self.txSocket, err = socket.udp()
    if self.txSocket == nil then
      self.error = err.."\nFailed to create UDP socket"
      return
    end	
	self.txSocket:setoption('broadcast',true)
	self.txSocket:setpeername('255.255.255.255', 3639)

	self.rxPort = 3639
	self.rxSocket = socket.udp()
	self.rxSocket:setoption('broadcast',true)
	self.rxSocket:settimeout(0)
	if (self.rxSocket:setsockname('*',3639) == nil) then
		for i=3640,4639,1 do
			self.rxSocket = socket.udp() --On bind failure the descriptor is closed (bug?)
			if (self.rxSocket:setsockname('127.0.0.1',i) ~= nil) then
				self.rxPort=i
				break
			end
		end
	end	
end

function UDPServer:send(msg)
   if self:isConnected() then
      self.txSocket:send(msg)
   end
end

function UDPServer:receive()
  local dgram, err = self.rxSocket:receive()
  if err == "timeout" then 
	return nil
  end
  return dgram
end

function UDPServer:close()
    self.txSocket:close()
    self.rxSocket:close()
end

----------------------------------------
---------- TCPServer class
----------------------------------------

TCPServer = class(Server)
M.TCPServer = TCPServer

function TCPServer:_init(param, port, password)
	self:super(param)
  
	self.host = param		
	self.timeout = 3
	self.port = port or 9996
	self.password = password or ""
	
	-- Gracefully degrade positional parameters in favour of a table.
	if type(param) == "table" then
		self.host = param.host or "undefined host"
		self.port = param.port or self.port
		self.password = param.password or self.password
		-- how long to wait for a TCP connect() in seconds.
		self.timeout = param.timeout or self.timeout	
	end
end

function TCPServer:connect()
	function receiveAll()
		local msg, _, partial = self.txSocket:receive(1500)
		return msg or partial
	end

	self.txSocket, err = socket.tcp()
	if self.txSocket == nil then
		self.error = err.."\nFailed to create TCP socket"
		return
	end
	self.txSocket:settimeout(self.timeout) -- blocking in second
	local _, err = self.txSocket:connect(self.host, self.port)
	if err then
		self.error = err.."\nconnecting to "..self.host..":"..self.port
		self:close()
		return
	end
  
	if receiveAll() == "<ACL></ACL>" then
		self:send("<cmd>log<code>"..self.password.."</cmd>")
		receiveAll() -- <ACL>Joggler</ACL>
	else
		self.error = "Failed ACL handshake\nconnecting to "..self.host..":"..self.port
		self:close()
		return
	end
	_, self.rxPort = self.txSocket:getsockname()
	self.txSocket:settimeout(0) -- non-blocking

	-- If we are re-establishing a connection
	-- tell the iServer about our existing filters.  
	for _, f in ipairs(self.filterList) do
		self:registerFilter(f:getFilterType())
	end
end  

function TCPServer:send(msg)
   if self:isConnected() then
     self.txSocket:send("<xap>"..msg.."</xap>")
   end
end

function TCPServer:receive()
	local msg, err, partial = self.txSocket:receive(1500)
	msg = msg or partial
	if #msg == 0 then
	  return nil, err
	end	
    -- return an iterator function of frames
    return string.gmatch(msg,"<%a->(.-)</%a->")
end

function TCPServer:close()
  if self.txSocket then
    self.txSocket:close()
 	self.txSocket = nil
  end
end

function TCPServer:registerFilter(ftype, key)
   self.txSocket:send("<cmd><"..ftype.."+>"..key.."</cmd>")
end

function TCPServer:addFilter(f)
   local ftype, key = f:getFilterType()
   if ftype then
      self:registerFilter(ftype, key)
      self.filterList[#self.filterList + 1] = f
   else
      print("SOURCE or CLASS filter not found")
   end   
end

----------------------------------------
---------- Frame class
----------------------------------------

Frame = class()
M.Frame = Frame
function Frame:_init(msg)
   local line

   local co = coroutine.create(function()
					for i in msg:gfind("%s*(.-)%s*\n") do
						coroutine.yield(i)
					end
			    end)

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

  local header = 'xap-header'
  if self[header] == nil then header = 'xap-hbeat' end    
  local out = encodeSection(header)

  for k,_ in pairs(self) do
    if k ~= header then
      out = out .. encodeSection(k)
    end
  end

  return out
end

----------------------------------------
---------- Filter class
----------------------------------------

Filter = class()
M.Filter = Filter
function Filter:_init(filter)
   self.filterChain = {}
   self.callback = nil

   if type(filter) == "table" then
      for section, keys in pairs(filter) do
	 for k,v in pairs(keys) do
	    self:add(section, k, v)
	 end
      end
   end
end

function Filter:add(section, key, value)
   table.insert(self.filterChain, {section=section,key=key,value=value:lower()})
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
      local status,err = pcall(self.callback, frame, self.userdata)
      if status == false then
         error(err..'\nxAP Message being processed.\n'..tostring(frame))
      end
   end
end

function Filter:__tostring()
    local t = "[\n"
	for _,fk in pairs(self.filterChain) do
		t = t..string.format("section:%s, key:%s, value:%s\n", fk.section, fk.key, fk.value)
	end
	t=t.."]\n"
	return t
end

function Filter:getFilterType()
	for _,fk in pairs(self.filterChain) do
		if fk.section == "xap-header" then
			if fk.key == "source" then
				return "flt", fk.value
			end
            if fk.key == "class" then
				return "cflt", fk.value
			end
		end
	end
	return nil
end

return M