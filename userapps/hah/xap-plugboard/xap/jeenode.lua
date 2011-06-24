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

-- Binary to Decimal
function bin2dec(num,weight)
   if num==0 then
      return 0
   else
      return (num%2)*math.pow(2,weight) + bin2dec(math.floor(num/10), weight+1)
   end
end

-- Binary to Decimal
function Bin2Dec(num)
   return bin2dec(num,0)
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
   local id, msg = msg:match("OK (%d+) (.+)")
   if id then
      config[tonumber(id)]:process(msg)
   end
end

-- A factory that produces a function that will be passed to
-- each NODE so be used to send data when an incoming BSC cmd
-- is detected.
local function senderFactory(id, port)
   local msg = string.format([[
xap-header
{
class=Serial.Comms
target=dbzoo.livebox.serial
}
Serial.Send
{
port=%s
data=%%s,%s s
}]], port,id)
   return function(data)
	     xap.sendShort(string.format(msg, data))
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

--[=[
   -- RF12Demo control
   -- set quiet mode (1 = don't report bad packets)
   xap.sendShort(string.format([[
xap-header
{
class=Serial.Comms
target=dbzoo.livebox.serial
}
Serial.Send
{
   port=%s
   data=1q
}
]], t.port))
--]=]

   local f = xap.Filter()
   f:add("xap-header","class","serial.comms")
   f:add("serial.received","port",t.port)
   f:callback(serialHandler, config)

   -- Build the BSC endpoints
   for k,v in pairs(config) do
      if DEBUG then
	 print("Build node: "..k)
      end
      v:build(senderFactory(k, t.port))
   end
end

