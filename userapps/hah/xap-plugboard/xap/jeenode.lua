--[[ $Id$

   JeeNode xAP mapping
   Copyright (c) Brett England, 2011

   No commercial use.
   No redistribution at profit.
   All derivative work must retain this message and
   acknowledge the work of the original author.
--]]
local _G = _G
local DEBUG = rawget(_G,'_DEBUG')

module("xap.jeenode", package.seeall)

require("xap")
require("xap.bsc")
require("pl")

-- decode Sketch startup string
-- [RF12demo.7] A i1 g212 @ 915 MHz
-- <ID> = '@' + (nodeId & 0x1F) ie... A B C D E F...
-- [<name>.<version>] <ID> i<id> g<group> @ <band> Mhz
function decodeSketchId(str)
   return str:match("%[(%w+)%.(%d+)%] (%a) i(%d+) g(%d+) @ (%d+)")
end

-- Convert a decimal number into its binary representation
function dec2bin(dec)
   if dec>0 then
      return tostring(dec2bin(math.floor(dec/2)))..tostring(dec%2)
   else
      return ""
   end
end

-- Convert Decimal to Binary and pad for alignment
-- dec: Decimal number
-- num: binary length padding (8,16,32 etc..)
-- return: a string of binary for the number
function Dec2Bin(dec,num)
   return stringx.ljust(dec2bin(dec),num,'0')
end

function Bin2Dec(num)
   return tonumber(num,2) -- convert num of base 2 to decimal
end

-- Convert a string of space separated decimal numbers into a binary stream
function stream2bin(stream)
   return table.concat(tablex.map(function(x) return Dec2Bin(x,8):reverse() end, 
				  tablex.map(tonumber, utils.split(stream))))
end

-- Flip all bits of a binary string (1's Complement)
function onesComplement(b)
   return b:gsub("([01])",function(x) return utils.choose(x=="0","1","0") end)
end

-- Take a binary string and slice them into integers on bit boundaries.
-- raw: string - sequence of space separated digits ie "0 0 3 1"
-- ...: list bits to extract for next int value (negative to sign-extend)
-- Returns the list of extracted integer values.
function bitslicer(raw, ...)
   local binary = stream2bin(raw)
   local pos = 1
   local t={}
   local n,b
   for k,v in ipairs{...} do
      n = math.abs(v)
      b = binary:sub(pos, pos+n-1):reverse()
      -- Handle -ve bit slices with sign-extension.
      if v < 0 and b:sub(1,1) == "1" then
	 -- 2's Complement and flip sign.
	 t[k] = -(Bin2Dec(onesComplement(b))+1)
      else
	 t[k] = Bin2Dec(b)
      end
      pos = pos + n
   end
   return unpack(t)
end

-- Incoming Serial data (filter callback)
function serialHandler(frame, config)
   if DEBUG then
      pretty.dump(frame["serial.received"])
   end
   local msg = frame["serial.received"].data
   -- Sometimes the O gets dropped. So make this optional.
   local id, msg = msg:match("O?K (%d+) (.+)")
   -- Any node that reports with a ACK gets 32 added to the ID
   -- see http://talk.jeelabs.net/topic/811#post-4734
   local idx = tonumber(id)
   if idx and config[idx%32] then
      config[idx%32]:process(msg)
   end
end

-- The main entry point
-- t - a serial configuration table
-- config - a table keyed by NODEID of processing Nodules
function monitor(t, config)
   local msg = [[
xap-header
{
class=Serial.Comms
target=dbzoo.livebox.serial
}
Serial.Setup
{
]]
   for k,v in pairs(t) do
      msg = msg..k.."="..v.."\n"
   end
   msg = msg .. "}"
   xap.sendShort(msg)

   local f = xap.Filter()
   f:add("xap-header","class","serial.comms")
   f:add("serial.received","port",t.port)
   f:callback(serialHandler, config)

   -- Build all the Nodules
   for k,v in pairs(config) do
      if v.build then
	 v:build(k, t.port)
      end
   end

   -- Check for non-responding JeeNodes every 10 seconds
   xap.Timer(grimreaper, 10, config):start()
end

-- If a Nodule does not report in after a TTL period of time
-- we mark its INPUT endpoints as UNKNOWN and make this Endpoint DEAD.
-- TTL is optional for an endpoint and can configured by the subclasses
-- however it should be allowed to overriden by the user configuration
function grimreaper(_, config)
   local now = os.time()
   for _,v in pairs(config) do
      if v.cfg and v.cfg.ttl then -- is it even reapable?
	 if DEBUG then
	    print(string.format("Reaping %s alive=%s ttl=%s timeout=%s", 
				tostring(v),
				utils.choose(v.isalive == true,"true","false"),
				v.cfg.ttl, 
				now-v.lastProcessed))
	    
	 end
	 -- Does this endpoint have an expiry timeout setting?
	 -- Is is alive?  (don't reap the dead!)
	 -- Has it expired?
	 if v.isalive == true and now - v.lastProcessed > v.cfg.ttl then
	    v:expire()
	 end
      end
   end
end

-- The base class for JeeNode communication nodules
class.Nodule()

-- id: the nodes UNIQUE ID as defined in the SKETCH
-- config: a table of configuration paramters
function Nodule:_init(config)
   self.cfg = config
end

function Nodule:build(id, port)
   self.cfg.id = id
   self.cfg.port = port
   self.lastProcessed = os.time()
   self.isalive = true
   if DEBUG then
      print('build '.. tostring(self))
   end
end

-- Mark the endpoint as non responsive and make its INPUT states unknown.
function Nodule:expire()
   if DEBUG then
      print('expire '.. tostring(self))
   end
   self.isalive = false
   for name in pairs(self.cfg.endpoints) do
      local e = self[name]
      -- Catch the nil (e) where by the user has configured
      -- an invalid endpoint as part of the node defn
      assert(e, string.format("%s: has no '%s' configurable endpoint", tostring(self), name))
      if e.direction == bsc.INPUT then
	 e.state = bsc.STATE_UNKNOWN
	 if e.type ~= bsc.BINARY then
	    e.text = "?"
	 end
	 e:sendEvent()
      end
   end
end

-- Implement a basic sender
-- This sends a comma delimited data payload to the Control Sketch
-- which will forward this to node <ID>
-- <payload>,<id> s
-- data(string): RF control data
function Nodule:sender(data)
   xap.sendShort(string.format([[
xap-header
{
class=Serial.Comms
target=dbzoo.livebox.serial
}
Serial.Send
{
port=%s
data=%s,%s s
}]], self.cfg.port, data, self.cfg.id))
end

-- Add a BSC endpoint to this Nodule
-- e(table): a BSC specification
function Nodule:add(e)
   local epvalue = self.cfg.endpoints[e.key]

   -- not a present endpoint configuration.
   if epvalue == nil then
      return
   end

   -- User configured to not want it.
   if epvalue == 0 then
      self.cfg.endpoints[e.key] = nil
      return
   end

   -- do we want the key value as the epvalue name?
   if epvalue == 1 then -- NO
      e.source = self.cfg.base.."."..e.key
   else
      -- The user has supplied their own name
      e.source = self.cfg.base.."."..epvalue
   end
   self[e.key] = bsc.Endpoint(e)
end

-- Update BSC endpoints with new values and send an event.
-- m(table): key= Endpoint names, value=their new values.
function Nodule:process(m)
   if DEBUG then
      print('process '.. tostring(self))
   end

   -- We got some data we must be alive
   self.lastProcessed = os.time()
   self.isalive = true

   for endpointname, v in pairs(m) do
      if DEBUG then
	 print("\t"..endpointname.."="..v)
      end
      local e = self[endpointname]
      if e then
	 if e.type == bsc.STREAM then
	    -- did its value just change?
	    if e.text ~= v then
	       e:setText(v)
	       e:sendEvent()
	    end
	 elseif e.type == bsc.BINARY then
	    state = bsc.decodeState(v)
	    if e.state ~= state then
	       e:setState(state)
	       e:sendEvent()
	    end
	 end
      end
   end
end

function Nodule:__tostring()
   return self._name..'['..self.cfg.id..']'
end
