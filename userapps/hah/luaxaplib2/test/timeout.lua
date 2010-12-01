-- Demonstrate Timer functionality.
require("xap")

elapsed = 0
t1time = 0
t2time = 0

function t1tick(self)
   t1time = t1time + self.interval
   elapsed = elapsed + self.interval
   print("T1 tick "..t1time.." Elapsed "..elapsed .. " seconds")
   if t1time >= 6 then
     print("Stopping T1, Starting T2")
     self:stop()  
     t2time = 0
     t2:start()  
   end
end

function t2tick(self)
   t2time = t2time + self.interval
   elapsed = elapsed + self.interval
   print("T2 tick "..t2time.." Elapsed "..elapsed .. " seconds")
   if t2time >= 5 then
     print("Stopping T2, Starting T1")
     self:stop()  
     t1time = 0
     t1:start()  
   end
end

xap.setLoglevel(7)
xap.init("dbzoo.lua.test","FF00CC00","eth0")
t1 = xap.Timer(t1tick, 2):start()
t2 = xap.Timer(t2tick, 1)
-- Create an anonymous timer
xap.Timer(function(interval) print("TIMER FIRED") end, 5):start()
xap.process()
