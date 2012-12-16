--[[
   XAP library testing
--]]

module(...,package.seeall)

require("xap")
require("pl.test")
require("pl.pretty")
require("pl.stringx").import()

info = {version="0.01", description="Validate xap library Timers"}

local count={t1=0, t2=0}
local t1 = nil

function test10_Timer()
   t1 = xap.Timer(cb_t1, 2):start()
end

function cb_t1(self)
   print("....test10_Timer callback")
   count.t1 = count.t1 + 1
   if count.t1 > 1 then
      self:stop()
   end
end

function test20_Timer()
   xap.Timer(cb_t2, 2):start()
end

function cb_t2(self)
   print("....test20_Timer callback")
   count.t2 = count.t2 + 1
   if count.t2 > 1 then
      self:delete()
      count.t1 = 0
      t1:start()
   end
end

function test30_IdleTimer()
   -- Check a timer isn't started on construction.
   xap.Timer(function() assert(false,"timer started!!") end ,1)
end

local st
function test40_EdgeTimer()
   st = os.time()
   xap.Timer(function(t)
		print("....test40_EdgeTimer callback "..os.time())
		assert(st, os.time()-t.interval)
		st = os.time()
	     end ,5):start()
end

function pairsByKeys (t, f)
   local a = {}
   for n in pairs(t) do table.insert(a, n) end
   table.sort(a, f)
   local i = 0
   local iter = function ()
		   i = i + 1
		   if a[i] == nil then return nil
		   else return a[i], t[a[i]]
		   end
      end
   return iter
end

function init()
   for k,v in pairsByKeys(_M) do
      if k:find("^test") then
	 io.write("...."..string.ljust(k,40).."    ")
	 v()
	 print("[ok]")
      end
   end
end

